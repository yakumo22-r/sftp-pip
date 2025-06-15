#pragma once
#ifndef YKM22_LUA_SFTP_CONFIG_MANAGER_H
#define YKM22_LUA_SFTP_CONFIG_MANAGER_H

#include <string>
#include <vector>
#include <string_view>
#include <unordered_map>
// #include <fstream>
// #include <sstream>

#include <openssl/md5.h>
#include <fmt/format.h>
#include <variant>

// #include "fmt/base.h"
#include "ykm22lib/symbol_lua.hpp"

namespace lua_sftp
{

struct ConfigValue
{
    ConfigValue() : var(), b(false) {}
    void operator=(int value)
    {
        this->b = true;
        this->var = value;
    }

    void operator=(double value)
    {
        this->b = true;
        this->var = value;
    }

    void operator=(std::string_view value)
    {
        this->b = true;
        this->var = std::string(value);
    }

    void operator=(std::string&& value)
    {
        this->b = true;
        this->var = std::move(value);
    }

    bool operator==(const ConfigValue& r) const { return (!b && !r.b) || ((b && r.b) && (var == r.var)); }

    operator bool() const { return this->b; }

    void set_null() { this->b = false; }
    void set_true() { this->b = true; }
    bool is_null() const { return !this->b; }

    int get_int(int _default = 0) const { return b ? std::get<int>(this->var) : _default; };
    double get_double(double _default = 0) const { return b ? std::get<double>(this->var) : _default; };
    std::string_view get_string(std::string_view _default = "") const { return b ? std::get<std::string>(this->var) : _default; };

  private:
    std::variant<int, double, std::string> var;
    bool b;
};

struct SSHLoginConfig
{
    std::string hostname;
    ConfigValue username;
    ConfigValue password;
    ConfigValue port;
    bool operator==(const SSHLoginConfig& r) const
    {
        return this->hostname == r.hostname && //
               this->username == r.username && //
               this->password == r.password && //
               this->port == r.port;
    }
    bool operator!=(const SSHLoginConfig& r) const { return !this->operator==(r); }
};

struct SFTPConfig
{
    std::string local_root;
    std::string remote_root;
    std::vector<std::string> _ignores;
    SSHLoginConfig login;
};

class ConfigManager
{
  public:
    enum{
        CfgLoginKeep=0,
        CfgLoginRefresh=1,
    };
    inline static ConfigManager& Ins()
    {
        static ConfigManager ins;
        return ins;
    }

    int check_load_config(lua_State* L);
    bool has_config(std::string_view name) const { return configs.count(std::string(name))>0; }
    SFTPConfig get_config(std::string_view name) const { return configs.at(std::string(name)); }

    // return: is login config change
    int azure_config(int& version, std::string_view name, SFTPConfig& cfg) const;

  private:
    ConfigManager() : version(0) {}
    std::unordered_map<std::string, SFTPConfig> configs;
    std::string curr_config_md5;
    int version;
};

} // namespace lua_sftp

#endif
