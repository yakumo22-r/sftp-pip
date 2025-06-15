#ifndef YKM22_LUA_SFTP_HPP
#define YKM22_LUA_SFTP_HPP

#include "ykm22lib/symbol_lua.hpp"
#include <string>
#include <vector>

namespace lua_sftp
{

using LuaRegs = std::vector<luaL_Reg>;

class LuaBindings
{
  public:
    void bind(lua_State* L);
    void add_regs(const LuaRegs& funcs);
    void set_state(lua_State* L);
    void log_err(const std::string& message);

  private:
    lua_State* luaState;
    std::vector<luaL_Reg> funcs = {};
};

static int lib_info(lua_State* L)
{
    lua_pushstring(L, "lua_sftp version - dev@0.0.1");
    return 1;
}

inline void LuaBindings::bind(lua_State* L)
{
    add_regs({//
              {"lib_info", lib_info},
              {NULL, NULL}});

    lua_createtable(L, 0, funcs.size() - 1);
    luaL_setfuncs(L, funcs.data(), 0);
}

inline void LuaBindings::add_regs(const LuaRegs& funcs)
{
    for (const auto& it : funcs) { this->funcs.push_back(it); }
}

inline void LuaBindings::set_state(lua_State* L){
    luaState = L;
}

inline void LuaBindings::log_err(const std::string& message){
    if (!luaState) return;

    lua_getglobal(luaState, "lua_sftp_err");      
    if (lua_isfunction(luaState, -1)) {
        lua_pushstring(luaState, message.c_str());
        lua_pcall(luaState, 1, 0, 0);
    } else {
        lua_pop(luaState, 1);
    }
}

} // namespace lua_sftp

#endif
