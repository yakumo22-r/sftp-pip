#include "config_manager.h"
#include <fstream>
#include <iomanip>
#include <sstream>

namespace lua_sftp
{
std::string parse_config(lua_State* L, SFTPConfig& c, int tbl, std::string_view name)
{
    lua_getfield(L, tbl, "login");
    if (lua_istable(L, -1))
    {
        int login = lua_gettop(L);
        lua_getfield(L, login, "hostname");
        const char* str = lua_tostring(L, -1);
        if (str) { c.login.hostname = str; }
        else { return fmt::format("err [{}]: login must contain [hostname]", name); }

        lua_pop(L, 1);

        lua_getfield(L, login, "username");
        if (lua_isstring(L, -1)) c.login.username = std::string(lua_tostring(L, -1));
        lua_pop(L, 1);

        lua_getfield(L, login, "password");
        if (lua_isstring(L, -1)) c.login.password = std::string(lua_tostring(L, -1));
        lua_pop(L, 1);

        lua_getfield(L, login, "port");
        if (lua_isnumber(L, -1)) c.login.port = int(lua_tointeger(L, -1));
        lua_pop(L, 1);
    }
    else { return fmt::format("err [{}]: config must contain [login]", name); }
    lua_pop(L, 1);

    lua_getfield(L, tbl, "ignores");
    if (lua_istable(L, -1))
    {
        int arr = lua_gettop(L);
        lua_pushnil(L);
        while (lua_next(L, arr))
        {
            if (lua_type(L, -2) == LUA_TNUMBER)
            {
                if (lua_type(L, -1) == LUA_TSTRING) c._ignores.emplace_back(lua_tostring(L, -1));
            }
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);

    lua_getfield(L, tbl, "local_root");
    auto str = lua_tostring(L, -1);
    if (str)
        c.local_root = str;
    else
        return fmt::format("err [{}]: config must contain [local_root]", name);
    lua_pop(L, 1);

    lua_getfield(L, tbl, "remote_root");
    str = lua_tostring(L, -1);
    if (str)
        c.remote_root = str;
    else
        return fmt::format("err [{}]: config must contain [remote_root]", name);
    lua_pop(L, 1);

    return "";
}

int ConfigManager::check_load_config(lua_State* L)
{
    const char * filename = luaL_checkstring(L, 1);

    std::ifstream file(filename, std::ios::binary);
    MD5_CTX md5Context;
    MD5_Init(&md5Context);
    std::vector<unsigned char> buffer(4096);
    while (file.good())
    {
        file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
        MD5_Update(&md5Context, buffer.data(), file.gcount());
    }
    unsigned char digest[MD5_DIGEST_LENGTH];
    MD5_Final(digest, &md5Context);

    std::ostringstream oss;
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) { oss << std::hex << std::setw(2) << std::setfill('0') << (int)digest[i]; }

    bool update = false;
    if (curr_config_md5 != oss.str())
    {
        if (luaL_dofile(L, filename))
        {
            lua_pushboolean(L, false);
            lua_pushnil(L);
            lua_pushstring(L, lua_tostring(L, -3));
            curr_config_md5 = "";
            return 3;
        }

        update = true;
    }

    if (!lua_istable(L, -1))
    {
        lua_pushboolean(L, true);
        lua_pushnil(L);
        lua_pushstring(L, "Returned value is not a table");
        return 3;
    }

    if (update)
    {
        curr_config_md5 = oss.str();

        int index = lua_gettop(L);
        lua_pushnil(L);
        while (lua_next(L, index))
        {
            // TAG: parse config(lua table)
            const char* key = "";
            if (lua_type(L, -2) == LUA_TSTRING) { key = lua_tostring(L, -2); }
            int tbl = lua_gettop(L);
            if (lua_type(L, tbl) == LUA_TTABLE)
            {
                auto& c = configs[key];

                auto err = parse_config(L, c, tbl, key);
                fmt::println("hostname: {}", c.login.hostname);
                if (err != "")
                {
                    curr_config_md5 = "";
                    lua_pushboolean(L, true);
                    lua_pushnil(L);
                    lua_pushstring(L, err.c_str());
                    return 3;
                }
            }

            lua_pop(L, 1);
        }

        fmt::println("update config md5: {} ", curr_config_md5);
    }

    lua_pushboolean(L, true);
    lua_pushvalue(L, -2);
    return 2;
}
int ConfigManager::azure_config(int& version, std::string_view name, SFTPConfig& cfg) const
{
    const auto& c = configs.at(name.data());
    if (version != this->version)
    {
        // fmt::println("azure_config, {}, {}", cfg.login.hostname, c.login.hostname);
        version = this->version;
        if (cfg.login != c.login)
        {
            cfg = c;
            return CfgLoginRefresh;
        }
    }

    cfg = c;
    return CfgLoginKeep;
}
} // namespace lua_sftp
