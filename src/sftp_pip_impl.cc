#include "sftp_pip_impl.h"
#include "libssh/libssh.h"

#include <fstream>
#include <sstream>
#include <stack>
#include <string>
#include <unordered_map>
#include <fcntl.h>
#include <thread>

#include <libssh/sftp.h>
#include <fmt/format.h>

struct SFTPSession
{
    ssh_session ssh;
    sftp_session sftp;
    std::string hostname;
    unsigned int port;
    std::string uname;
    std::string password;
    bool is_login;
};

std::unordered_map<int, SFTPSession> sftp_sessions;
int session_count = 0;

void clear_login(SFTPSession& session)
{
    if (session.ssh)
    {
        if (session.is_login) ssh_disconnect(session.ssh);
        ssh_free(session.ssh);
        session.ssh = nullptr;
    }
    session.is_login = false;
}

bool session_init(SFTPSession& session, Responser response, int cmd, int id)
{

    clear_login(session);
    session.ssh = ssh_new();

    ssh_options_parse_config(session.ssh, nullptr);

    ssh_options_set(session.ssh, SSH_OPTIONS_HOST, session.hostname.c_str());
    ssh_options_set(session.ssh, SSH_OPTIONS_USER, session.uname.c_str());

    auto ERRSTATUS = RES_ERROR;

    int timeout = 10;
    ssh_options_set(session.ssh, SSH_OPTIONS_TIMEOUT, &timeout);

    if (ssh_connect(session.ssh) != SSH_OK)
    {
        response(cmd, id, ERRSTATUS, //
                 fmt::format("SSH connect failed. host({}) username({}), {}", session.hostname, session.uname, ssh_get_error(session.ssh)));
        clear_login(session);
        return false;
    }


    if(session.password.empty()){
        if (session.uname.empty()){
            char* uname_cstr = nullptr;
            ssh_options_get(session.ssh, SSH_OPTIONS_USER, &uname_cstr);
            session.uname = uname_cstr;
        }else{
            ssh_options_set(session.ssh, SSH_OPTIONS_USER, session.uname.c_str());
        }

        if (session.port){
            ssh_options_set(session.ssh, SSH_OPTIONS_PORT, &session.port);
        }else{
            ssh_options_get_port(session.ssh, &session.port);
        }

        if (ssh_userauth_publickey_auto(session.ssh, nullptr, nullptr) != SSH_AUTH_SUCCESS)
        {
            response(
                cmd, id, ERRSTATUS, //
                fmt::format("SSH authentication failed. host({}:{}) username({}), {}", session.hostname, session.port, session.uname, ssh_get_error(session.ssh)));
            clear_login(session);
            return false;
        }
    }else{
        if (session.port){
            ssh_options_set(session.ssh, SSH_OPTIONS_PORT, &session.port);
        }
        if(ssh_userauth_password(session.ssh, session.uname.c_str(), session.password.c_str()) != SSH_AUTH_SUCCESS)
        {
            response(cmd, id, ERRSTATUS, //
                     fmt::format("SSH authentication failed. host({}:{}) username({}), {}", session.hostname, session.port, session.uname, ssh_get_error(session.ssh)));
            clear_login(session);
            return false;
        }

        response(CMD_READY, id, RES_INFO, fmt::format("{} \n debug", "password"));
    }


    auto str = fmt::format("SSH login success. host({}:{}) username({})", session.hostname, session.port, session.uname);
    response(cmd, id, RES_INFO, str);


    response(cmd, id, RES_INFO, fmt::format("SSH authentication success. host({}:{}) username({})", session.hostname, session.port, session.uname));

    // Create SFTP session
    session.sftp = sftp_new(session.ssh);

    if (session.sftp == nullptr)
    {
        response(cmd, id, ERRSTATUS,
                 fmt::format("SFTP create failed. host({}:{}) username({}), {}", session.hostname, session.port, session.uname, ssh_get_error(session.ssh)));
        clear_login(session);
        return false;
    }

    if (sftp_init(session.sftp) != SSH_OK)
    {
        auto msg =
            fmt::format("SFTP init failed. host({}:{}) username({}), err: {}", session.hostname, session.port, session.uname, sftp_get_error(session.sftp));
        response(cmd, id, ERRSTATUS, msg);
        sftp_free(session.sftp);
        clear_login(session);
        return false;
    }

    return true;
}

void get_req_head(std::string_view msg, ReqHead& head)
{
    std::istringstream iss(msg.data());
    iss >> head.cmd >> head.id >> head.sessionId;
    if (sftp_sessions.count(head.sessionId)){
        head.session = &sftp_sessions[head.sessionId];
    }else {
        head.session = nullptr;
    }
    iss.eof();
}

void new_session(const ReqHead& head, std::vector<std::string>& msgs, Responser response)
{
    auto id = head.id;

    SFTPSession session;
    session.ssh = nullptr;
    session.sftp = nullptr;
    session.is_login = false;


    session.hostname = msgs[1];
    session.uname = msgs[2];
    session.password = msgs[3];

    // response(CMD_READY, id, RES_INFO, fmt::format("{} \n debug", msgs[4]));
    // return;
    if (msgs[4] == "") session.port = 0;
    else session.port = std::stoi(msgs[4]);

    if (!session_init(session, response, CMD_NEW_SESSION, id)) {
        response(CMD_NEW_SESSION, id, RES_ERROR_DONE, "Failed to initialize SFTP session"); }
    else
    {
        sftp_sessions[session_count] = session;
        response(CMD_NEW_SESSION, id, RES_DONE, fmt::format("{}\ncreate new session successfully", session_count));
        session_count++;
    }
}

#include <filesystem>
namespace fs = std::filesystem;
#ifndef _WIN32
#include <sys/stat.h>
#else
#define S_IRWXU 0700
#endif

std::string ensure_remote_dir(sftp_session sftp, const std::string& remote_path);
std::string sftp_error_str(int code);

int upload_one_file(SFTPSession& session, int id, const std::string& localRoot, const std::string& remoteRoot, std::string_view path, Responser response)
{
    std::string abs_local = fs::absolute(fs::path(localRoot) / path).string();
    std::string abs_remote = (fs::path(remoteRoot) / path).generic_string();

    std::ifstream file(abs_local, std::ios::binary);

    if (!file)
    {
        response(                       //
            CMD_UPLOADS, id, RES_ERROR, //
            fmt::format("Local file open failed | not found: {}", abs_local));
        return -1;
    }

    // local file exists
    sftp_file remote_file = sftp_open(session.sftp, abs_remote.data(), O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);

    if (!remote_file)
    {
        // remote file not exists or error
        int errcode = sftp_get_error(session.sftp);
        if (errcode == SSH_FX_NO_SUCH_FILE)
        {
            auto mkdir_r = ensure_remote_dir(session.sftp, abs_remote);
            if (mkdir_r == "") { remote_file = sftp_open(session.sftp, abs_remote.data(), O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU); }
        }
    }


    if (!remote_file)
    {
        int errcode = sftp_get_error(session.sftp);
        response(                       //
            CMD_UPLOADS, id, RES_ERROR, //
            fmt::format("Remote file open failed: {}, err ({}): {}", abs_remote, errcode, sftp_error_str(errcode)));
        return errcode;
    }

    char buffer[4096];
    auto uploadOk = true;
    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0)
    {
        if (sftp_write(remote_file, buffer, file.gcount()) != file.gcount())
        {
            int errcode = sftp_get_error(session.sftp);
            uploadOk = false;
            response(                       //
                CMD_UPLOADS, id, RES_ERROR, //
                fmt::format("File upload error , remote: {}, err ({}) {}", abs_remote, errcode, sftp_error_str(errcode)));
            sftp_close(remote_file);
            return errcode;
        }
    }

    if (uploadOk)
    {
        response(                      //
            CMD_UPLOADS, id, RES_INFO, //
            fmt::format("File uploaded successfully {} -> {}", path, abs_remote));
    }
    return 0;
}

void uploads(const ReqHead& head, std::vector<std::string>& msgs, Responser response)
{

    auto id = head.id;
    auto sessionId = head.sessionId;
    auto localRoot = msgs[1];
    auto remoteRoot = msgs[2];

    if (!sftp_sessions.count(sessionId))
    {
        response(                            //
            CMD_UPLOADS, id, RES_ERROR_DONE, //
            fmt::format("Session ID ({}) not found", sessionId));
        return;
    }

    auto& session = sftp_sessions[sessionId];


    for (size_t i = 3; i < msgs.size(); ++i)
    {
        auto& path = msgs[i];
        auto err = -1;
        err = upload_one_file(session, id, localRoot, remoteRoot, path, response);
        response(CMD_UPLOADS, head.id, RES_DONE, std::to_string(msgs.size()));
        return;
        if (err != 0){
            if (err == SSH_FX_NO_CONNECTION || err == SSH_FX_CONNECTION_LOST){
                auto retry = 0;
                while (retry < 3 && !session_init(session, response, CMD_UPLOADS, id))
                {
                    retry++;
                    if(retry==3){
                        response(CMD_UPLOADS, id, RES_ERROR_DONE, "Failed to reinitialize SFTP session");
                        return;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                }
                upload_one_file(session, id, localRoot, remoteRoot, path, response);
            }
        }
    }

    
    response(CMD_UPLOADS, id, RES_DONE, fmt::format("{}\n Upload Done", sessionId));
}

int download_one_file(SFTPSession& session, int id, const std::string& localRoot, const std::string remoteRoot, std::string_view path, Responser response)
{
    std::string abs_local = fs::absolute(fs::path(localRoot) / path).string();
    std::string abs_remote = fs::absolute(fs::path(remoteRoot) / path).generic_string();

    sftp_file remote_file = sftp_open(session.sftp, abs_remote.data(), O_RDONLY, 0);
    if (!remote_file)
    {
        int errcode = sftp_get_error(session.sftp);
        response(                       //
            CMD_DOWNLOADS, id, RES_ERROR, //
            fmt::format("Remote file open failed: {}, err ({}): {}", abs_remote, errcode, sftp_error_str(errcode)));
        return errcode;
    }

    std::ofstream localFile(abs_local, std::ios::binary);
    if (!localFile.is_open())
    {
        response(                       //
            CMD_DOWNLOADS, id, RES_ERROR, //
            fmt::format("Local file open failed: {}", abs_local));
        sftp_close(remote_file);
        return -1;
    }

    char buffer[4096];
    int bytesRead;
    while ((bytesRead = sftp_read(remote_file, buffer, sizeof(buffer))) > 0)
    {
        localFile.write(buffer, bytesRead);
    }

    sftp_close(remote_file);
    response(                      //
        CMD_DOWNLOADS, id, RES_INFO, //
        fmt::format("File downloaded successfully {} -> {}", abs_remote, path));
    return 0;
}

void downloads(const ReqHead& head, std::vector<std::string>& msgs, Responser response)
{
    auto id = head.id;
    auto sessionId = head.sessionId;
    auto localRoot = msgs[1];
    auto remoteRoot = msgs[2];

    if (!sftp_sessions.count(sessionId))
    {
        response(                            //
            CMD_DOWNLOADS, id, RES_ERROR_DONE, //
            fmt::format("Session ID ({}) not found", sessionId));
        return;
    }
    
    auto& session = sftp_sessions[sessionId];

    for (size_t i = 5; i < msgs.size(); ++i)
    {
        auto& path = msgs[i];
        auto err = -1;
        err = download_one_file(session, id, localRoot, remoteRoot, path, response);
        if (err != 0){
            if (err == SSH_FX_NO_CONNECTION || err == SSH_FX_CONNECTION_LOST){
                auto retry = 0;
                while (retry < 3 && !session_init(session, response, CMD_DOWNLOADS, id))
                {
                    retry++;
                    if(retry==3){
                        response(CMD_DOWNLOADS, id, RES_ERROR_DONE, "Failed to reinitialize SFTP session");
                        return;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                }
              download_one_file(session, id, localRoot, remoteRoot, path, response);
            }
        }
    }

    response(CMD_UPLOADS, id, RES_DONE, fmt::format("{}\n Sync Done", sessionId));
}

void close_session(const ReqHead& head, std::vector<std::string>& msgs, Responser response)
{
    auto id = head.id;
    auto sessionId = head.sessionId;

    if (!sftp_sessions.count(sessionId))
    {
        response(                            //
            CMD_DOWNLOADS, id, RES_ERROR_DONE, //
            fmt::format("Session ID ({}) not found", sessionId));
        return;
    }
    
    auto& session = sftp_sessions[sessionId];

    clear_login(session);
    sftp_sessions.erase(sessionId);
    response(CMD_DOWNLOADS, id, RES_DONE, std::to_string(sessionId));
}

std::string sftp_error_str(int code)
{
    switch (code)
    {
    case SSH_FX_NO_SUCH_FILE:
        return "No such file";
    case SSH_FX_PERMISSION_DENIED:
        return "Permission denied";
    case SSH_FX_NO_CONNECTION:
        return "No connection";
    case SSH_FX_CONNECTION_LOST:
        return "Connection lost";
    case SSH_FX_WRITE_PROTECT:
        return "Write protected";
    default:
        return fmt::format("Unknow error {}", code);
    }
}

std::string ensure_remote_dir(sftp_session sftp, const std::string& remote_path)
{
    size_t pos;
    std::stack<std::string> mkdir_commands;
    std::string subdir = remote_path.substr(0, remote_path.find_last_of('/'));
    
    while (!sftp_opendir(sftp, subdir.c_str()))
    {
        if (sftp_opendir(sftp, subdir.c_str())) { break; }
        mkdir_commands.push(subdir);
        subdir.erase(subdir.find_last_of('/'));
    }

    while (!mkdir_commands.empty())
    {
        auto& path = mkdir_commands.top();
        if (sftp_mkdir(sftp, path.c_str(), S_IRWXU) != SSH_OK)
        {
            int err_code = sftp_get_error(sftp);
            if (err_code != SSH_FX_FILE_ALREADY_EXISTS) //
            {
                return fmt::format("Failed to create remote directory: {}, code: {}", path, err_code);
            }
        }
        mkdir_commands.pop();
    }
    return "";
}
