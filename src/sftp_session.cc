#include "sftp_session.h"

#include <fstream>
#include <filesystem>
#include <stack>

#include "config_manager.h"
#include "fmt/base.h"
#include "fmt/format.h"
#include "libssh/libssh.h"
#include "libssh/sftp.h"
#include "log_mgr.hpp"

#ifndef _WIN32
#include <sys/stat.h>
#else
#define S_IRWXU 0700
#endif

namespace fs = std::filesystem;

namespace lua_sftp
{

SFTPSession::SFTPSession(std::string_view target_name) : target_name(target_name), cfg_version(-1), sftp(nullptr), working(false), login_id(0), login() {}

bool SFTPSession::_connect()
{
    if (sftp) { sftp_free(sftp); }

    sftp = sftp_new(login->get_ssh());

    std::string_view hostname = login->get_hostname();
    std::string_view uname = login->get_username();
    int port = login->get_port();

    if (sftp == nullptr)
    {
        tick_log.error("SFTP create failed. host({}:{}) username({}), {}", hostname, port, uname, ssh_get_error(sftp));
        return false;
    }

    if (sftp_init(sftp) != SSH_OK)
    {
        tick_log.error("SFTP init failed. host({}:{}) username({}), err: {}", hostname, port, uname, sftp_get_error(sftp));
        sftp_free(sftp);
        sftp = nullptr;
        return false;
    }

    tick_log.info("SFTP init success. host({}:{}) username({})", hostname, port, uname).apply();
    return true;
}

SFTPSession::~SFTPSession()
{
    if (sftp) sftp_free(sftp);
}

void SFTPSession::send_cmd(int CMD, std::vector<std::string>&& args)
{
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        Task task;
        if (CMD == close)
        {
            while (!tasks.empty()) { tasks.pop(); }
        }

        task.CMD = CMD;
        task.args = std::move(args);
        tasks.push(std::move(task));

        if (!working)
        {
            working = true;
            SFTPTaskManager::Ins().submit(*this);
        }
    }
}

bool SFTPSession::tick_task()
{
    Task task;
    {
        std::unique_lock<std::mutex> lock(queue_mutex);

        if (!working || tasks.empty())
        {
            working = false;
            return false;
        }
        task = std::move(tasks.front());
        tasks.pop();
    }

    if (ConfigManager::Ins().azure_config(cfg_version, target_name, cfg) == ConfigManager::CfgLoginRefresh)
    {
        login = SSHSession::Get(cfg.login);
        login_id = -1;
    }


    int login_id = login->check_login();
    if (this->login_id != login_id)
    {
        this->login_id = login_id;
        if (!_connect()) { return false; }
    }

    // TODO: error & message
    switch (task.CMD)
    {
    case upload_file:
        return login->is_login() && _upload_files({task.args[0]});
    case download_file:
        tick_log.debug("download \"{}\" \"{}\"", task.args[0], task.args[1]);
        break;
    case upload_batch:
        break;
    case download_batch:
        break;
    case close:
        tick_log.debug("recycle");
        delete this;
        return false;
    }
    return true;
}

enum
{
    ERR_LOCAL = -1,
    SUCCUSS = 0,
    ERR_CONNECT = 1,
    ERR_OTHER = 2,
};
inline std::string sftp_error_str(int code, int* r = nullptr)
{
    if (r) *r = ERR_OTHER;
    switch (code)
    {
    case SSH_FX_NO_SUCH_FILE:
        return "No such file";
    case SSH_FX_PERMISSION_DENIED:
        return "Permission denied";
    case SSH_FX_NO_CONNECTION:
        if (r) *r = ERR_CONNECT;
        return "No connection";
    case SSH_FX_CONNECTION_LOST:
        if (r) *r = ERR_CONNECT;
        return "Connection lost";
    case SSH_FX_WRITE_PROTECT:
        return "Write protected";
    default:
        return fmt::format("Unknow error {}", code);
    }
}

inline std::string ensure_remote_dir(sftp_session sftp, const std::string& remote_path)
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

int SFTPSession::_upload_file(std::string_view path)
{
    std::string abs_local = fs::absolute(fs::path(cfg.local_root) / path).string();
    std::string abs_remote = (fs::path(cfg.remote_root) / path).generic_string();


    std::ifstream file(abs_local, std::ios::binary);
    if (!file)
    {
        tick_log.warn("failed to open local file: {}", abs_local);
        return ERR_LOCAL;
    }

    bool try_mkdir = false;

    sftp_file remote_file = sftp_open(sftp, abs_remote.data(), O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
    while (!remote_file)
    {
        // tick_log.debug("some error").apply();
        int error_code = sftp_get_error(sftp);
        if (!try_mkdir && error_code == SSH_FX_NO_SUCH_FILE)
        {
            // tick_log.debug("try mkdir").apply();
            auto mkdir_r = ensure_remote_dir(sftp, abs_remote);
            if (mkdir_r != "")
            {
                tick_log.error("{}", mkdir_r);
                return ERR_OTHER;
            }
            remote_file = sftp_open(sftp, abs_remote.data(), O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
            try_mkdir = true;
            continue;
        }

        int r;
        tick_log.warn("failed to open remote files({}), path({})", sftp_error_str(error_code, &r), abs_remote);
        return r;
    }

    char buffer[4096];
    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0)
    {
        if (sftp_write(remote_file, buffer, file.gcount()) != file.gcount())
        {
            int code = sftp_get_error(sftp);
            tick_log.error("SFTP write error, code({}), path({})", code, path);
            sftp_close(remote_file);
            return code;
        }
    }

    sftp_close(remote_file);

    tick_log.info("upload success. local({}), remote({})", path, abs_remote);
    return 0;
}

bool SFTPSession::_upload_files(const FileOps& ops)
{
    for (auto path : ops)
    {
        // reconnect & retry after error
        int attempt = 0;
        int attempt_num = 1;

        do {
            int r = _upload_file(path);
            if (                        //
                (!login->is_login()) || //
                r == ERR_CONNECT)
            {
                attempt++;
                if (attempt > attempt_num)
                {
                    tick_log.error("upload failed, SSH connection lost. TASK STOP");
                    return false;
                }
                else
                {
                    tick_log.warn("upload failed, SSH connection lost. attempt({} of {})", attempt, attempt_num).apply();
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    login->clear_login();
                    login_id = login->check_login();
                    _connect();
                }
            }
            else if (r == ERR_OTHER) { return false; }
            else { break; }
        } while (attempt <= attempt_num);
    }
    return true;
}

bool SFTPSession::downloadFile(std::string_view remotePath, std::string_view localPath)
{
    if (!sftp) return false;

    sftp_file remoteFile = sftp_open(sftp, remotePath.data(), O_RDONLY, 0);
    if (!remoteFile)
    {
        // std::cerr << "Failed to open remote file: " << remotePath << std::endl;
        return false;
    }

    std::ofstream localFile(localPath.data(), std::ios::binary);
    if (!localFile.is_open())
    {
        // std::cerr << "Failed to open local file: " << localPath << std::endl;
        return false;
    }

    char buffer[4096];
    int bytesRead;
    while ((bytesRead = sftp_read(remoteFile, buffer, sizeof(buffer))) > 0) { localFile.write(buffer, bytesRead); }

    sftp_close(remoteFile);
    return true;
}

bool SFTPSession::uploadBatch(const std::vector<std::string>& files, std::string_view remotePath)
{
    // for (const auto& file : files)
    // {
    //     std::string remoteFilePath = std::string(remotePath) + "/" + file;
    //     if (!uploadFile(file, remoteFilePath))
    //     {
    //         // std::cerr << "Failed to upload file: " << file << std::endl;
    //         return false;
    //     }
    // }
    return true;
}
} // namespace lua_sftp
