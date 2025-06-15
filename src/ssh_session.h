#ifndef YKM22_LUA_SFTP_SSH_SESSION_HPP
#define YKM22_LUA_SFTP_SSH_SESSION_HPP

#include <cstddef>
#include <mutex>
#include <libssh/sftp.h>
#include "config_manager.h"
namespace lua_sftp
{

class SSHSessionShared;

class SSHSession
{
  public:
    static SSHSessionShared Get(const SSHLoginConfig& cfg);

    bool is_login() const { return _is_login; }
    void clear_login();

    ssh_session get_ssh() { return ssh; }
    std::string_view get_hostname() const {return cfg.hostname;}
    std::string_view get_username() const { return uname; }
    int get_port()const {return port;}

    /**
     * auto login
     * @return login_id
     */
    int check_login();

  private:
    SSHSession();
    ~SSHSession();

    friend class SSHSessionShared;
    void return_one();
    SSHSession* share_one();

    ssh_session ssh;
    SSHLoginConfig cfg;
    std::mutex mutex;
    std::condition_variable condition;
    size_t hash;

    unsigned int port;
    std::string uname;

    int login_id;
    int shared_count;
    bool _is_login;
};

class SSHSessionShared
{
  public:
    SSHSessionShared() : s(nullptr) {}
    SSHSessionShared(SSHSession* s) : s(s ? s->share_one() : s) {}
    SSHSessionShared(const SSHSessionShared& r) : s(r.s ? r.s->share_one() : r.s) {}
    SSHSessionShared(SSHSessionShared&& r) : s(r.s) { r.s = nullptr; }

    SSHSessionShared& operator=(SSHSession* s)
    {
        auto p = this->s;
        this->s = s ? s->share_one() : s;
        if (p) p->return_one();
        return *this;
    }
    SSHSessionShared& operator=(const SSHSessionShared& r)
    {
        auto p = this->s;
        this->s = r.s ? r.s->share_one() : r.s;
        if (p) p->return_one();
        return *this;
    }
    SSHSessionShared& operator=(SSHSessionShared&& r)
    {
        auto p = this->s;
        this->s = r.s;
        r.s = nullptr;
        if (p) p->return_one();
        return *this;
    }

    operator bool() { return s; }
    SSHSession* operator*() { return s; }
    SSHSession* operator->() { return s; }
    SSHSession* get() { return s; }
    const SSHSession* get() const { return s; }

    void reset()
    {
        if (s) s->return_one();
    }
    ~SSHSessionShared() { this->reset(); }

  private:
    mutable SSHSession* s;
};

} // namespace lua_sftp

#endif
