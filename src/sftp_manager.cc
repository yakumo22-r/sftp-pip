#include "sftp_session.h"
#include "config_manager.h"
#include "fmt/base.h"

#include "log_mgr.hpp"
#include <thread>

namespace lua_sftp
{

// TAG: Impl SFTPManager
SFTPManager& SFTPManager::Ins()
{
    static SFTPManager instance;
    return instance;
}

SFTPSession* SFTPManager::getSFTPSession(std::string_view target_name)
{
    if (!ConfigManager::Ins().has_config(target_name)){
        return nullptr;
    }
    SFTPConfig cfg = ConfigManager::Ins().get_config(target_name);

    SFTPSession* sftpSession;

    if (sftpSessions.find(target_name.data()) != sftpSessions.end())
    {
        sftpSession = sftpSessions[target_name.data()]; 
    }else
    {
        sftpSession = new SFTPSession(target_name);
        sftpSessions[target_name.data()] = sftpSession;
    }

    return sftpSession;
}

void SFTPManager::releaseSFTPSession(std::string_view target_name)
{
    if (sftpSessions.find(target_name.data()) != sftpSessions.end())
    {
        sftpSessions[target_name.data()]->send_cmd(SFTPSession::close, {});
        sftpSessions.erase(target_name.data());
    }
}

// TAG: Impl SFTPTaskManager
SFTPTaskManager::SFTPTaskManager(size_t thread_count) : stop(false) {}

SFTPTaskManager::~SFTPTaskManager()
{
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for (std::thread& worker : workers) { worker.join(); }
}

void SFTPTaskManager::work()
{
    Log::Buf log;
    static int _id = 0;
    int id = _id++;
    log.debug("start work id({})", id).apply();

    while (true)
    {
        SFTPSession* s = nullptr;
        {

            std::unique_lock<std::mutex> lock(queue_mutex);
            if(sessions.empty()){
                log.debug("task wait id: {}", id).apply();
            }
            condition.wait(lock, [this] { return stop || !sessions.empty(); });
            if(stop){
                break;
            }

            s = sessions.front();
            sessions.pop();
        }

        while (s->one_tick()) {
        }
    }
    log.debug("stop work id:{}", id).apply();
}

void SFTPTaskManager::submit(SFTPSession& session)
{
    int need_thread = 0;

    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        sessions.push(&session);
        need_thread = sessions.size() - workers.size();
    }

    for (size_t i = 0; i < need_thread; i++)
    {
        if (workers.size() >= 4) { break; }
        workers.emplace_back(&SFTPTaskManager::work, this);
    }

    condition.notify_one();
}
} // namespace lua_sftp
