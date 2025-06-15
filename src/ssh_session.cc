#include "ssh_session.h"
#include <functional>
#include <unordered_map>

#include <libssh/libssh.h>
#include "log_mgr.hpp"

namespace lua_sftp
{

inline size_t do_hash(const SSHLoginConfig& cfg)
{
    std::hash<std::string> hasher;
    size_t r = hasher(cfg.hostname);
    r += cfg.port.get_int();
    r += hasher(cfg.username.get_string().data());
    return r;
}

static std::unordered_map<size_t, SSHSession*> ssh_maps;

SSHSessionShared SSHSession::Get(const SSHLoginConfig& cfg)
{
    size_t hash_code = do_hash(cfg);

    if (ssh_maps.count(hash_code) == 0)
    {
        auto s = new SSHSession();
        s->cfg = cfg;
        s->hash = hash_code;
        ssh_maps[hash_code] = s;
    }

    return SSHSessionShared(ssh_maps[hash_code]);
};

SSHSession::SSHSession() : ssh(nullptr), _is_login(false), login_id(0), mutex(), condition(), shared_count(0) {}

SSHSession::~SSHSession()
{
    clear_login();
    ssh_maps.erase(hash);
}

void SSHSession::clear_login()
{
    std::unique_lock<std::mutex> lock(mutex);

    if (ssh)
    {
        if (_is_login) ssh_disconnect(ssh);
        ssh_free(ssh);
        ssh = nullptr;
    }

    _is_login = false;
    login_id++;
}

int SSHSession::check_login()
{
    std::unique_lock<std::mutex> lock(mutex);

    if (_is_login) return login_id;

    Log::Buf log;

    ssh = ssh_new();
    ssh_options_parse_config(ssh, nullptr);

    std::string_view hostname = cfg.hostname;
    std::string_view username = cfg.username.get_string();
    ssh_options_set(ssh, SSH_OPTIONS_HOST, hostname.data());
    ssh_options_set(ssh, SSH_OPTIONS_USER, username.data());

    int timeout = 10;
    ssh_options_set(ssh, SSH_OPTIONS_TIMEOUT, &timeout);
    if (ssh_connect(ssh) != SSH_OK)
    {
        log.error("SSH connect failed. host({}) username({}), {}", hostname, username, ssh_get_error(ssh)).apply();
        clear_login();
        return -1;
    }

    char* uname_cstr = nullptr;
    ssh_options_get(ssh, SSH_OPTIONS_USER, &uname_cstr);
    uname = uname_cstr;

    ssh_options_get_port(ssh, &port);

    log.info("SSH connect success. host({}:{}) username({})", hostname, port, uname);

    // auth
    if (ssh_userauth_publickey_auto(ssh, nullptr, nullptr) != SSH_AUTH_SUCCESS)
    {
        log.error("SSH authentication failed. host({}:{}) username({}) {}", hostname, port, uname, ssh_get_error(ssh)).apply();
        clear_login();
        return -1;
    }

    log.info("SSH auhentication success. host({}:{}) username({})", hostname, port, uname).apply();

    _is_login = true;
    login_id++;
    return login_id;
}

void SSHSession::return_one()
{
    std::unique_lock<std::mutex> lock(mutex);
    shared_count--;
    if (shared_count <= 0) { delete this; }
}

SSHSession* SSHSession::share_one()
{
    std::unique_lock<std::mutex> lock(mutex);
    shared_count++;
    return this;
}

} // namespace lua_sftp
