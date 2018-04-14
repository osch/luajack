#include <stdio.h>
#include <lua.h>

#include "main.h"
#include "util.h"
#include "client.h"
#include "port.h"
#include "rbuf.h"
#include "process.h"
#include "thread.h"
#include "buffer.h"
#include "async_util.h"

static AtomicCounter initFlag = 0;

static int verbose(lua_State* L)
{
    setVerbose(checkonoff(L, 1));
    return 0;
}

static const struct luaL_Reg ModuleFunctions[] = 
{
    { "verbose", verbose },
    { NULL, NULL } /* sentinel */
};



LUAJACK_EXPORT int luaopen_luajack(lua_State* L)
{
    if (atomic_set_if_equal(&initFlag, 0, 1)) {
        jack_set_error_function(verbosePrint);
        jack_set_info_function(verbosePrint);  
    }
    
    int n = lua_gettop(L);
    
    int module     = ++n; lua_newtable(L);

    int clientMeta = ++n; luaL_newmetatable(L, CLIENT_TYPE_NAME);
    int clientClass= ++n; lua_newtable(L);

    int portMeta = ++n; luaL_newmetatable(L, PORT_TYPE_NAME);
    int portClass= ++n; lua_newtable(L);

    int rbufMeta = ++n; luaL_newmetatable(L, RBUF_TYPE_NAME);
    int rbufClass= ++n; lua_newtable(L);

    int threadMeta = ++n; luaL_newmetatable(L, THREAD_TYPE_NAME);
    int threadClass= ++n; lua_newtable(L);

    lua_pushvalue(L, module);
        luaL_setfuncs(L, ModuleFunctions, 0);
    
        lua_pushvalue(L, clientClass);
        lua_setfield (L, clientMeta, "__index");

        lua_pushvalue(L, portClass);
        lua_setfield (L, portMeta, "__index");

        lua_pushvalue(L, rbufClass);
        lua_setfield (L, rbufMeta, "__index");

        lua_pushvalue(L, threadClass);
        lua_setfield (L, threadMeta, "__index");

    lua_pop(L, 1);
    
    lua_checkstack(L, LUA_MINSTACK);

    luajack_open_client (L, module, clientMeta, clientClass);

    luajack_open_port   (L, module, clientMeta, clientClass,
                                       portMeta,   portClass);
    
    luajack_open_rbuf   (L, module, clientMeta, clientClass,
                                       rbufMeta,   rbufClass);
    
    luajack_open_process(L, module, clientMeta, clientClass);

    luajack_open_thread (L, module, clientMeta, clientClass,
                                     threadMeta, threadClass);

    luajack_open_buffer (L, module, clientMeta, clientClass,
                                      portMeta,   portClass);
    
    lua_settop(L, module);
    return 1;
}

