#ifndef LUAJACK_MAIN_H
#define LUAJACK_MAIN_H

#include <lua.h>

#if !defined(LUAJACK_EXPORT)
#   if defined(_WIN32)
#       define LUAJACK_EXPORT __declspec(dllexport)
#   else
#       define LUAJACK_EXPORT
#   endif
#endif

LUAJACK_EXPORT int luaopen_luajack(lua_State* L);


#endif // LUAJACK_MAIN_H
