#include "util.h"
#include "process.h"
#include "process_util.h"

// TODO: load file
static int process_load(lua_State* L)
{
    int arg     = 1;
    int lastArg = lua_gettop(L);
    
    JackClient* client = getCheckedClient(L, arg++);
    
    if (!client->isMaster) {
        return luaL_error(L, "method can only be called on master client object");
    }
    
    if (client->shared->processContext) {
        return luaL_error(L, "process chunk already loaded"); // TODO: reload for deactivated clients
    }
    if (lua_type(L, arg) != LUA_TSTRING)
        luaL_error(L, "missing process chunk");
    
    /* create the process_state (unrelated to the client state) */
    lua_State* P = luaL_newstate();
    
    if (P == NULL)
        return luaL_error(L, "cannot create Lua state");
    
    luaL_openlibs(P);
    
    int chunkName = arg; // for file
    const char* script = lua_tostring(L, arg++); // TODO load file
    int rc = luaL_loadstring(P, script);
    if (rc != LUA_OK) {
        if (lua_isstring(P, -1)) 
            lua_pushstring(L, lua_tostring(P, -1));
        else
            lua_pushfstring(L, "cannot load string (luaL_loadstring() error %d)", rc);
        lua_close(P);
        return lua_error(L);
    }
    if (true /* isFromScript*/) {
        lua_Debug dbg;
        lua_getstack(L, 1, &dbg);
        lua_getinfo(L, "Sl", &dbg);
        lua_pushfstring(L, "%s:%d", dbg.short_src, dbg.currentline);
        chunkName = lua_gettop(L);
        lua_pushvalue(L, -2); // the chunk function
    }
    int isErr = luajack_xmove(client->shared, P, L, chunkName, arg, lastArg);
    if (isErr) {
        lua_close(P);
        return lua_error(L);
    }

    int nargs = lastArg - arg + 1;
    /* execute the script (note that we still are in the main thread) */

    if (lua_pcall(P, nargs, 0 , 0) != LUA_OK) {
        if (lua_isstring(P, -1)) 
            lua_pushstring(L, lua_tostring(P, -1));
        else
            lua_pushfstring(L, "cannot execute chunk (lua_pcall() error %d)", rc);
        lua_close(P);
        return lua_error(L);
    }

    /* since we still are in a non-rt thread, do a complete garbage collection,
     * then stop; garbage collection in the rt-thread will be made at indivisible
     * steps, one step at the end of each callback */
    lua_gc(P, LUA_GCCOLLECT, 0);
    lua_gc(P, LUA_GCSTOP, 0);

    client->shared->processContext = P;
    return 0;
}

static int jack_process_callback(jack_nframes_t nframes, void* arg)
{
    JackClientShared* client = arg;
    lua_State* L = client->processContext;
    
    client->currentProcessNframes = nframes;
    
    if (L && client->processCallbackRef != LUA_NOREF) {
        int oldTop = lua_gettop(L);
        lua_rawgeti(L, LUA_REGISTRYINDEX, client->processCallbackRef);
        lua_pushinteger(L, nframes);
        int rc = lua_pcall(L, 1, 0, 0);
        if (rc != LUA_OK) {
             // TODO handle error in lua
            printf("Error in process callback: %s\n", lua_tostring(L, -1));
        }
        lua_settop(L, oldTop);
    }
    client->currentProcessNframes = 0;
    return 0;
}


static int process_callback(lua_State* L)
{
    JackClient* client = getCheckedClient(L, 1);

    if (!client->isInProcessContext) {
        return luaL_argerror(L, 1, "method can only be called from process context");
    }
    
    if (!lua_isfunction(L, 2)) {
        return luaL_argerror(L, 2, "function expected");
    }
    if (client->shared->processCallbackRef != LUA_NOREF) {
        luaL_unref(L, LUA_REGISTRYINDEX, client->shared->processCallbackRef);
    }
    lua_pushvalue(L, 2);
    client->shared->processCallbackRef = luaL_ref(L, LUA_REGISTRYINDEX);
    
    jack_set_process_callback(client->ptr, jack_process_callback, client->shared);
    return 0;
}

static const struct luaL_Reg ClientMethods[] = 
{
    { "process_load",        process_load },
    { "process_callback",    process_callback },
    { NULL, NULL } /* sentinel */
};

static const struct luaL_Reg ModuleFunctions[] = 
{
    { "process_load",      process_load },
    { "process_callback",  process_callback },
    { NULL, NULL } /* sentinel */
};

bool luajack_open_process(lua_State* L, int module, int clientMeta, int clientClass)
{
    lua_pushvalue(L, module);
        luaL_setfuncs(L, ModuleFunctions, 0);

        lua_pushvalue(L, clientClass);
            luaL_setfuncs(L, ClientMethods, 0);
    
    lua_pop(L, 2);
    
    return true;
}


