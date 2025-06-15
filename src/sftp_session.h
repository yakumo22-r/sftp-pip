#ifndef YKM22_LUA_SFTP_SFTP_SESSION_HPP
#define YKM22_LUA_SFTP_SFTP_SESSION_HPP

#include <cassert>
#include <string>
#include <string_view>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <fcntl.h>

#include <fmt/format.h>

#include "ssh_session.h"
#include "log_mgr.hpp"

namespace lua_sftp
{

class SFTPSession
{
  public:
    enum
    {
        upload_file,
        download_file,
        upload_batch,
        download_batch,
        close,
    };

    SFTPSession(std::string_view target_name);
    ~SFTPSession();

    void send_cmd(int CMD, std::vector<std::string>&& args);
    // return continue
    bool tick_task();

    void test() { Log::Buf().debug("obj test").apply(); };

    bool one_tick()
    {
        auto r = tick_task();
        tick_log.apply();
        return r;
    }

    std::string_view get_target_name() const { return target_name; }

  private:
    struct Task
    {
        int CMD;
        std::vector<std::string> args;
    };

    using FileOps = std::vector<std::string_view>;

    bool _connect();

    // return 0: ok;
    int _upload_file(std::string_view path);
    bool _upload_files(const FileOps& ops);

    bool downloadFile(std::string_view remotePath, std::string_view localPath);
    bool uploadBatch(const std::vector<std::string>& files, std::string_view remotePath);


    std::string target_name;
    SFTPConfig cfg;
    int cfg_version;

    int login_id;
    SSHSessionShared login;
    sftp_session sftp;

    Log::Buf tick_log;

    std::mutex queue_mutex;
    std::queue<Task> tasks;

    bool working;
    bool initializeSFTP();
};

class SFTPTaskManager
{
  public:
    void submit(SFTPSession& session);

    inline static void Init(size_t thread_count)
    {
        static SFTPTaskManager i = SFTPTaskManager(thread_count);
        ins = &i;
    }

    inline static SFTPTaskManager& Ins()
    {
        if (!ins)
        {
            // TODO: log warn
            Init(3);
        };
        return *ins;
    }

  private:
    inline static SFTPTaskManager* ins = nullptr;

    SFTPTaskManager(size_t thread_count);
    ~SFTPTaskManager();
    void work();
    std::vector<std::thread> workers;
    std::queue<SFTPSession*> sessions;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};

class SFTPManager
{
  public:
    static SFTPManager& Ins();

    SFTPSession* getSFTPSession(std::string_view target_name);
    void releaseSFTPSession(std::string_view hostname);
  private:
    SFTPManager() = default;
    ~SFTPManager() = default;

    std::unordered_map<std::string, SFTPSession*> sftpSessions;
};

} // namespace lua_sftp

#endif
