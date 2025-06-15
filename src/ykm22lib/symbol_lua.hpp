#ifndef YKM22_SYMBOL_LUA_HPP
#define YKM22_SYMBOL_LUA_HPP

#include <cassert>
#include <stddef.h>
#include <limits.h>
#include "symbol_loader.hpp"

/*
** The following structures and type definitions are derived from the Lua 5.4 source code.
** They are provided here for reference and to facilitate dynamic symbol loading without
** directly including the Lua headers.
**
** Copyright © 1994–2024 Lua.org, PUC-Rio.
** Permission is hereby granted, free of charge, to any person obtaining a copy of this
** software and associated documentation files (the "Software"), to deal in the Software
** without restriction, including without limitation the rights to use, copy, modify, merge,
** publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
** to whom the Software is furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all copies or
** substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
** INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
** PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
** FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
** OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
** DEALINGS IN THE SOFTWARE.
*/

typedef struct lua_State lua_State;
typedef int (*lua_CFunction)(lua_State* L);

typedef struct luaL_Reg
{
    const char* name;
    lua_CFunction func;
} luaL_Reg;

typedef double lua_Number; // Typically defined as double

typedef long long lua_Integer; // Typically defined as long long

typedef ptrdiff_t lua_KContext; // Typically defined as ptrdiff_t

typedef int (*lua_KFunction)(lua_State* L, int status, lua_KContext ctx);

typedef const char* (*lua_Reader)(lua_State* L, void* data, size_t* size);

typedef int (*lua_Writer)(lua_State* L, const void* p, size_t sz, void* ud);

typedef void* (*lua_Alloc)(void* ud, void* ptr, size_t osize, size_t nsize);

const static int LUA_OK = 0;

#if defined(_WIN32) || defined(_WIN64)
inline static ykm22::libh_t auto_get_lualib(){
    static ykm22::libh_t lib = NULL;
    if(lib) return lib;

    const char* findlist[] = {
        "lua5.1.dll",
        "lua5.2.dll",
        "lua5.3.dll",
        "lua5.4.dll",
        "lua51.dll",
    };

    for (size_t i = 0; i < sizeof(findlist)/sizeof(findlist[0]); i++) {
        ykm22::libh_t t = GetModuleHandle(findlist[i]);
        if (t){
            lib = t;
            return lib;
        }
    }

    assert(lib && "the process not load lua library {lua5.1.dll ,lua5.2.dll, lua5.3.dll, lua5.4.dll, lua51.dll(luajit)}");

    return NULL;
}

inline ykm22::libh_t test(){
    return auto_get_lualib();
}

#define YKM22_LUA_SYMBOL(name,r,p,i) YKM22_AUTOLOAD_SYMBOL_LIB(auto_get_lualib(), name,r,p,i)
inline void* load_lua_symbol(const char* symbolName)
{
    void* symbol = ykm22::get_symbol_addr(auto_get_lualib(), symbolName);
    return symbol;
}
#else
#define YKM22_LUA_SYMBOL(name,r,p,i) YKM22_AUTOLOAD_SYMBOL(name,r,p,i)
inline void* load_lua_symbol(const char* symbolName)
{
    return ykm22::load_symbol_proc(symbolName, true);
}
#endif

// TAG: wrap lua api
// clang-format off

YKM22_LUA_SYMBOL(
luaL_setfuncs,
void,
(lua_State *L, const luaL_Reg *l, int nup),
(L,l,nup))

YKM22_LUA_SYMBOL(
lua_createtable,
void,
(lua_State *L, int narr, int nrec),
(L,narr,nrec))

YKM22_LUA_SYMBOL(
luaL_checkinteger,
int, 
(lua_State*L, int i),
(L,i));

YKM22_LUA_SYMBOL(
lua_pushinteger,
void, 
(lua_State*L, int i),
(L,i));

// Load lua_gettop function
YKM22_LUA_SYMBOL(
lua_gettop,
int,
(lua_State *L),
(L))

// Load lua_settop function
YKM22_LUA_SYMBOL(
lua_settop,
void,
(lua_State *L, int idx),
(L, idx))

// Load lua_pushvalue function
YKM22_LUA_SYMBOL(
lua_pushvalue,
void,
(lua_State *L, int idx),
(L, idx))

// Load lua_remove function
YKM22_LUA_SYMBOL(
lua_remove,
void,
(lua_State *L, int idx),
(L, idx))

// Load lua_insert function
YKM22_LUA_SYMBOL(
lua_insert,
void,
(lua_State *L, int idx),
(L, idx))

// Load lua_replace function
YKM22_LUA_SYMBOL(
lua_replace,
void,
(lua_State *L, int idx),
(L, idx))

// Load lua_checkstack function
YKM22_LUA_SYMBOL(
lua_checkstack,
int,
(lua_State *L, int n),
(L, n))

// Load lua_isnumber function
YKM22_LUA_SYMBOL(
lua_isnumber,
int,
(lua_State *L, int idx),
(L, idx))

// Load lua_isstring function
YKM22_LUA_SYMBOL(
lua_isstring,
int,
(lua_State *L, int idx),
(L, idx))

// Load lua_iscfunction function
YKM22_LUA_SYMBOL(
lua_iscfunction,
int,
(lua_State *L, int idx),
(L, idx))

// Load lua_isuserdata function
YKM22_LUA_SYMBOL(
lua_isuserdata,
int,
(lua_State *L, int idx),
(L, idx))

// Load lua_type function
YKM22_LUA_SYMBOL(
lua_type,
int,
(lua_State *L, int idx),
(L, idx))

// Load lua_typename function
YKM22_LUA_SYMBOL(
lua_typename,
const char *,
(lua_State *L, int tp),
(L, tp))

// Load lua_tonumberx function
YKM22_LUA_SYMBOL(
lua_tonumberx,
lua_Number,
(lua_State *L, int idx, int *isnum),
(L, idx, isnum))

// Load lua_tointegerx function
YKM22_LUA_SYMBOL(
lua_tointegerx,
lua_Integer,
(lua_State *L, int idx, int *isnum),
(L, idx, isnum))

// Load lua_toboolean function
YKM22_LUA_SYMBOL(
lua_toboolean,
int,
(lua_State *L, int idx),
(L, idx))

// Load lua_tolstring function
YKM22_LUA_SYMBOL(
lua_tolstring,
const char *,
(lua_State *L, int idx, size_t *len),
(L, idx, len))

// Load lua_rawlen function
YKM22_LUA_SYMBOL(
lua_rawlen,
size_t,
(lua_State *L, int idx),
(L, idx))

// Load lua_tocfunction function
YKM22_LUA_SYMBOL(
lua_tocfunction,
lua_CFunction,
(lua_State *L, int idx),
(L, idx))

// Load lua_touserdata function
YKM22_LUA_SYMBOL(
lua_touserdata,
void *,
(lua_State *L, int idx),
(L, idx))

// Load lua_tothread function
YKM22_LUA_SYMBOL(
lua_tothread,
lua_State *,
(lua_State *L, int idx),
(L, idx))

// Load lua_topointer function
YKM22_LUA_SYMBOL(
lua_topointer,
const void *,
(lua_State *L, int idx),
(L, idx))

// Load lua_arith function
YKM22_LUA_SYMBOL(
lua_arith,
void,
(lua_State *L, int op),
(L, op))

// Load lua_rawequal function
YKM22_LUA_SYMBOL(
lua_rawequal,
int,
(lua_State *L, int idx1, int idx2),
(L, idx1, idx2))

// Load lua_compare function
YKM22_LUA_SYMBOL(
lua_compare,
int,
(lua_State *L, int idx1, int idx2, int op),
(L, idx1, idx2, op))

// Load lua_pushnil function
YKM22_LUA_SYMBOL(
lua_pushnil,
void,
(lua_State *L),
(L))

// Load lua_pushnumber function
YKM22_LUA_SYMBOL(
lua_pushnumber,
void,
(lua_State *L, lua_Number n),
(L, n))

// Load lua_pushlstring function
YKM22_LUA_SYMBOL(
lua_pushlstring,
void,
(lua_State *L, const char *s, size_t len),
(L, s, len))

// Load lua_pushstring function
YKM22_LUA_SYMBOL(
lua_pushstring,
void,
(lua_State *L, const char *s),
(L, s))

// Load lua_pushcclosure function
YKM22_LUA_SYMBOL(
lua_pushcclosure,
void,
(lua_State *L, lua_CFunction fn, int n),
(L, fn, n))

// Load lua_pushboolean function
YKM22_LUA_SYMBOL(
lua_pushboolean,
void,
(lua_State *L, int b),
(L, b))

// Load lua_pushlightuserdata function
YKM22_LUA_SYMBOL(
lua_pushlightuserdata,
void,
(lua_State *L, void *p),
(L, p))

// Load lua_pushthread function
YKM22_LUA_SYMBOL(
lua_pushthread,
int,
(lua_State *L),
(L))

// Load lua_getglobal function
YKM22_LUA_SYMBOL(
lua_getglobal,
int,
(lua_State *L, const char *name),
(L, name))

// Load lua_setglobal function
YKM22_LUA_SYMBOL(
lua_setglobal,
void,
(lua_State *L, const char *name),
(L, name))

// Load lua_gettable function
YKM22_LUA_SYMBOL(
lua_gettable,
void,
(lua_State *L, int idx),
(L, idx))

// Load lua_settable function
YKM22_LUA_SYMBOL(
lua_settable,
void,
(lua_State *L, int idx),
(L, idx))

// Load lua_rawget function
YKM22_LUA_SYMBOL(
lua_rawget,
void,
(lua_State *L, int idx),
(L, idx))

// Load lua_rawset function
YKM22_LUA_SYMBOL(
lua_rawset,
void,
(lua_State *L, int idx),
(L, idx))

// Load lua_rawgeti function
YKM22_LUA_SYMBOL(
lua_rawgeti,
void,
(lua_State *L, int idx, lua_Integer n),
(L, idx, n))

// Load lua_rawseti function
YKM22_LUA_SYMBOL(
lua_rawseti,
void,
(lua_State *L, int idx, lua_Integer n),
(L, idx, n))

// Load lua_rawgetp function
YKM22_LUA_SYMBOL(
lua_rawgetp,
void,
(lua_State *L, int idx, const void *p),
(L, idx, p))

// Load lua_rawsetp function
YKM22_LUA_SYMBOL(
lua_rawsetp,
void,
(lua_State *L, int idx, const void *p),
(L, idx, p))

// Load lua_newuserdata function
YKM22_LUA_SYMBOL(
lua_newuserdata,
void *,
(lua_State *L, size_t size),
(L, size))

// Load lua_getmetatable function
YKM22_LUA_SYMBOL(
lua_getmetatable,
int,
(lua_State *L, int objindex),
(L, objindex))

YKM22_LUA_SYMBOL(
luaL_getmetatable,
int,
(lua_State *L, const char *tname),
(L, tname))

// Load lua_setmetatable function
YKM22_LUA_SYMBOL(
lua_setmetatable,
int,
(lua_State *L, int objindex),
(L, objindex))

// Load lua_getuservalue function
YKM22_LUA_SYMBOL(
lua_getuservalue,
void,
(lua_State *L, int idx),
(L, idx))

// Load lua_setuservalue function
YKM22_LUA_SYMBOL(
lua_setuservalue,
void,
(lua_State *L, int idx),
(L, idx))

// Load lua_callk function
YKM22_LUA_SYMBOL(
lua_callk,
void,
(lua_State *L, int nargs, int nresults, lua_KContext ctx, lua_KFunction k),
(L, nargs, nresults, ctx, k))

// Load lua_pcallk function
YKM22_LUA_SYMBOL(
lua_pcallk,
int,
(lua_State *L, int nargs, int nresults, int errfunc, lua_KContext ctx, lua_KFunction k),
(L, nargs, nresults, errfunc, ctx, k))

// Load lua_load function
YKM22_LUA_SYMBOL(
lua_load,
int,
(lua_State *L, lua_Reader reader, void *dt, const char *chunkname, const char *mode),
(L, reader, dt, chunkname, mode))

// Load lua_dump function
YKM22_LUA_SYMBOL(
lua_dump,
int,
(lua_State *L, lua_Writer writer, void *data, int strip),
(L, writer, data, strip))

// Load lua_yieldk function
YKM22_LUA_SYMBOL(
lua_yieldk,
int,
(lua_State *L, int nresults, lua_KContext ctx, lua_KFunction k),
(L, nresults, ctx, k))

// Load lua_resume function
YKM22_LUA_SYMBOL(
lua_resume,
int,
(lua_State *L, lua_State *from, int narg, int *nres),
(L, from, narg, nres))

// Load lua_status function
YKM22_LUA_SYMBOL(
lua_status,
int,
(lua_State *L),
(L))

// Load lua_isyieldable function
YKM22_LUA_SYMBOL(
lua_isyieldable,
int,
(lua_State *L),
(L))

// Load lua_gc function
YKM22_LUA_SYMBOL(
lua_gc,
int,
(lua_State *L, int what, int data),
(L, what, data))

// Load lua_error function
YKM22_LUA_SYMBOL(
lua_error,
int,
(lua_State *L),
(L))

// Load lua_next function
YKM22_LUA_SYMBOL(
lua_next,
int,
(lua_State *L, int idx),
(L, idx))

// Load lua_concat function
YKM22_LUA_SYMBOL(
lua_concat,
void,
(lua_State *L, int n),
(L, n))

// Load lua_len function
YKM22_LUA_SYMBOL(
lua_len,
void,
(lua_State *L, int idx),
(L, idx))

// Load lua_stringtonumber function
YKM22_LUA_SYMBOL(
lua_stringtonumber,
size_t,
(lua_State *L, const char *s),
(L, s))

// Load lua_getallocf function
YKM22_LUA_SYMBOL(
lua_getallocf,
lua_Alloc,
(lua_State *L, void **ud),
(L, ud))

// Load lua_setallocf function
YKM22_LUA_SYMBOL(
lua_setallocf,
void,
(lua_State *L, lua_Alloc a, void *ud),
(L, a, ud))

// Load lua_toclose function
YKM22_LUA_SYMBOL(
lua_toclose,
void,
(lua_State *L, int idx),
(L, idx))

// Load lua_closeslot function
YKM22_LUA_SYMBOL(
lua_closeslot,
void,
(lua_State *L, int idx),
(L, idx))

YKM22_LUA_SYMBOL(
luaL_newstate,
lua_State*,
(void),
())

YKM22_LUA_SYMBOL(
luaL_openlibs,
void,
(lua_State *L),
(L))

YKM22_LUA_SYMBOL(
luaL_dofile,
int,
(lua_State *L, const char *filename),
(L, filename))

YKM22_LUA_SYMBOL(
lua_close,
void,
(lua_State *L),
(L))

YKM22_LUA_SYMBOL(
luaL_checkudata,
void*,
(lua_State *L, int arg, const char *tname),
(L, arg, tname))

YKM22_LUA_SYMBOL(
luaL_checklstring,
const char*,
(lua_State *L, int arg, size_t *len),
(L, arg, len))

YKM22_LUA_SYMBOL(
luaL_optlstring,
const char*,
(lua_State *L, int idx, const char *def,size_t *len),
(L, idx, def,len))

YKM22_LUA_SYMBOL(
luaL_newmetatable,
int,
(lua_State *L, const char *tname),
(L, tname))

YKM22_LUA_SYMBOL(
lua_setfield,
void,
(lua_State *L, int index, const char *k),
(L, index, k))

YKM22_LUA_SYMBOL(
luaL_loadfilex,
int,
(lua_State *L, const char *filename, const char *mode),
(L, filename,mode))

YKM22_LUA_SYMBOL(
luaL_loadstring,
int,
(lua_State *L, const char *s),
(L, s))

YKM22_LUA_SYMBOL(
luaL_error,
int,
(lua_State *L, const char *fmt),
(L, fmt))

// clang-format on

// TAG: same lua & luajit
#define LUA_TNONE (-1)       // Type for 'no value'
#define LUA_TNIL 0           // Type for 'nil'
#define LUA_TBOOLEAN 1       // Type for booleans
#define LUA_TLIGHTUSERDATA 2 // Type for light userdata
#define LUA_TNUMBER 3        // Type for numbers
#define LUA_TSTRING 4        // Type for strings
#define LUA_TTABLE 5         // Type for tables
#define LUA_TFUNCTION 6      // Type for functions
#define LUA_TUSERDATA 7      // Type for userdata
#define LUA_TTHREAD 8        // Type for threads

#define LUA_MULTRET (-1)
#define LUA_IDSIZE 60

// TAG: lua defined
#if 1000000 < (INT_MAX / 2)
#define LUAI_MAXSTACK 1000000
#else
#define LUAI_MAXSTACK (INT_MAX / 2u)
#endif
#define LUA_REGISTRYINDEX (-LUAI_MAXSTACK - 1000)

// TAG: luajit defined
#define LUAJIT_REGISTRYINDEX (-10000)

// TAG: wrap function
#define lua_tonumber(L, i) lua_tonumberx(L, (i), NULL)
#define lua_tointeger(L, i) lua_tointegerx(L, (i), NULL)

#define lua_pop(L, n) lua_settop(L, -(n) - 1)
#define lua_newtable(L) lua_createtable(L, 0, 0)
#define lua_register(L, n, f) (lua_pushcfunction(L, (f)), lua_setglobal(L, (n)))
#define lua_pushcfunction(L, f) lua_pushcclosure(L, (f), 0)

#define lua_isfunction(L, n) (lua_type(L, (n)) == LUA_TFUNCTION)
#define lua_istable(L, n) (lua_type(L, (n)) == LUA_TTABLE)
#define lua_islightuserdata(L, n) (lua_type(L, (n)) == LUA_TLIGHTUSERDATA)
#define lua_isnil(L, n) (lua_type(L, (n)) == LUA_TNIL)
#define lua_isboolean(L, n) (lua_type(L, (n)) == LUA_TBOOLEAN)
#define lua_isthread(L, n) (lua_type(L, (n)) == LUA_TTHREAD)
#define lua_isnone(L, n) (lua_type(L, (n)) == LUA_TNONE)
#define lua_isnoneornil(L, n) (lua_type(L, (n)) <= 0)

#define lua_tostring(L, i) lua_tolstring(L, (i), NULL)

#define luaL_newlibtable(L, l) lua_createtable(L, 0, sizeof(l) / sizeof((l)[0]) - 1)
#define luaL_newlib(L, l) (luaL_checkversion(L), luaL_newlibtable(L, l), luaL_setfuncs(L, l, 0))
#define luaL_argcheck(L, cond, arg, extramsg) ((void)(luai_likely(cond) || luaL_argerror(L, (arg), (extramsg))))

#define luaL_checkstring(L, n) (luaL_checklstring(L, (n), NULL))
#define luaL_optstring(L, n, d) (luaL_optlstring(L, (n), (d), NULL))
#define luaL_typename(L, i) lua_typename(L, lua_type(L, (i)))
#define luaL_dofile(L, fn) (luaL_loadfilex(L, fn, NULL) || lua_pcall(L, 0, LUA_MULTRET, 0))
#define luaL_dostring(L, s) (luaL_loadstring(L, s) || lua_pcall(L, 0, LUA_MULTRET, 0))
#define luaL_getmetatable(L, n) (lua_getfield(L, lua_registryindex(), (n)))
#define luaL_opt(L, f, n, d) (lua_isnoneornil(L, (n)) ? (d) : f(L, (n)))

// TODO: macro in lua
// #define lua_insert(L, idx) lua_rotate(L, (idx), 1)
// #define lua_remove(L, idx) (lua_rotate(L, (idx), -1), lua_pop(L, 1))
// #define lua_replace(L, idx) (lua_copy(L, -1, (idx)), lua_pop(L, 1))
// #define luaL_loadbuffer(L, s, sz, n) luaL_loadbufferx(L, s, sz, n, NULL)

inline bool check_is_lua_jit()
{
    static void* lua_rotate = load_lua_symbol("lua_rotate");
    return !lua_rotate;
}

inline int lua_getfield(lua_State* L, int idx, const char* k)
{
    static void* f = load_lua_symbol("lua_getfield");
    if (check_is_lua_jit()) // in luajit
    {
        using F = void (*)(lua_State* L, int idx, const char* k);
        ((F)f)(L, idx, k);
        return 0;
    }
    else
    {
        using F = int (*)(lua_State* L, int idx, const char* k);
        return ((F)f)(L, idx, k);
    }
}

inline int lua_registryindex() { return check_is_lua_jit() ? LUAJIT_REGISTRYINDEX : LUA_REGISTRYINDEX; }

inline int lua_pcall(lua_State* L, int nargs, int nresults, int errfunc)
{
    if (check_is_lua_jit()) // in luajit
    {
        using pcall_f = int (*)(lua_State* L, int nargs, int nresults, int errfunc);

        static pcall_f pcall = (pcall_f)load_lua_symbol("lua_pcall");
        return pcall(L, nargs, nresults, errfunc);
    }
    else { return lua_pcallk(L, nargs, nresults, errfunc, 0, NULL); } // in normal lua
}

#endif
