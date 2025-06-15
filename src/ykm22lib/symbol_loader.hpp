#ifndef YKM22_SYMBOL_LOADER_HPP
#define YKM22_SYMBOL_LOADER_HPP

#if defined(_WIN32) || defined(_WIN64)
#include <cassert>
#include <windows.h>
namespace ykm22
{
typedef HMODULE libh_t;
inline libh_t load_lib(const char* libname) { return LoadLibraryA(libname); }
inline void* get_symbol_addr(libh_t libhandle, const char* symbol) { return GetProcAddress(libhandle, symbol); }
inline void unload_lib(libh_t libhandle) { FreeLibrary(libhandle); }
inline libh_t get_proc_libh(bool mute = true){
    libh_t hModule = GetModuleHandle(NULL);
    if(!mute){
        assert(hModule!=NULL && "fatal error,hModule is null");
    }
    return hModule;
}
} // namespace ykm22

#else // TAG: unix/linux

#include <dlfcn.h>
namespace ykm22
{
typedef void* libh_t;
inline libh_t load_lib(const char* libname) { return dlopen(libname, RTLD_LAZY); }
inline void* get_symbol_addr(libh_t libhandle, const char* symbol) { return dlsym(libhandle, symbol); }
inline void unload_lib(libh_t libhandle) { dlclose(libhandle); }
inline libh_t get_proc_libh() { return RTLD_DEFAULT; }
} // namespace ykm22
#endif

#include <cstdio>

namespace ykm22
{
// Function to load a symbol from a shared library
inline void* load_symbol_lib(const char* symbolName, libh_t libHandle = nullptr)
{
    void* symbol = get_symbol_addr(libHandle, symbolName);
    if (!symbol)
    {
#if defined(_WIN32) || defined(_WIN64)
        printf("Error: %lu\n", GetLastError());
#else
        printf("Error: %s\n", dlerror());
#endif
    }
    return symbol;
}

inline void* load_symbol_proc(const char* symbolName,bool mute=false)
{
    static libh_t h = get_proc_libh();
    void* symbol = get_symbol_addr(h, symbolName);
    if (!symbol && !mute)
    {
#if defined(_WIN32) || defined(_WIN64)
        printf("Error: %lu\n", GetLastError());
#else
        printf("Error: %s\n", dlerror());
#endif
    }
    return symbol;
}

} // namespace ykm22

// Macro to define a function from process`s symbol
#define YKM22_AUTOLOAD_SYMBOL(name, r, p, i)              \
    inline r name p                                       \
    {                                                     \
        typedef r(*t) p;                                  \
        static t f = (t)::ykm22::load_symbol_proc(#name); \
        return f i;                                       \
    }

// Macro to define a function from dylib`s symbol
#define YKM22_AUTOLOAD_SYMBOL_LIB(lib, name, r, p, i)         \
    inline r name p                                           \
    {                                                         \
        typedef r(*t) p;                                      \
        static t f = (t)::ykm22::load_symbol_lib(#name, lib); \
        return f i;                                           \
    }

#endif // YKM22_SYMBOL_LOADER_H 
