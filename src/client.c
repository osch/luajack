#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <jack/session.h>

#include "util.h"
#include "client.h"
#include "client_util.h"

static int client_ptr(lua_State* L)
{
    JackClient* client = getCheckedClient(L, 1);
    lua_pushlightuserdata(L, client->ptr);
    return 1;
}

static int client_toString(lua_State* L)
{
    JackClient* client = getCheckedClient(L, 1);
    lua_pushfstring(L, "%s: \"%s\" (%p)", CLIENT_TYPE_NAME, 
                                          jack_get_client_name(client->ptr),
                                          client->ptr);
    return 1;
}

static int client_open(lua_State* L)
{
    const char* name = lua_tostring(L, 1);
    if (!name)
        return luaL_argerror(L, 1, "string expected");

    /* create the client */
    JackOptionParameters options = getJackOptionParameters(L, 2);
    
    lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_MAINTHREAD);
    lua_State* thisContext = lua_tothread(L, -1);

    JackClient* client = pushNew(L, JackClient);

    client->shared->processCallbackRef = LUA_NOREF;
    async_mutex_init(&client->shared->mutex);
    
    jack_status_t status;
    {
        if ((options.serverName != NULL) && (options.sessionId != NULL)) {
            client->ptr = jack_client_open(name, options.flags, &status, options.serverName, options.sessionId);
        }            
        else if (options.serverName != NULL) {
            client->ptr = jack_client_open(name, options.flags, &status, options.serverName);
        }
        else if (options.sessionId != NULL) {
            client->ptr = jack_client_open(name, options.flags, &status, options.sessionId);
        }
        else {
            client->ptr = jack_client_open(name, options.flags, &status);
        }
    }
    if (client->ptr) {
        client->isMaster             = true;
        client->shared->mainContext  = thisContext;
        client->shared->ptr          = client->ptr;
        verbosePrintf("created client '%s'\n", jack_get_client_name(client->ptr));
        return 1;
    }
    else {
        pushStatusBitsToString(L, status);
        return luaL_error(L, "cannot create client (%s)", lua_tostring(L, -1));
    }
}

static int client_name_size(lua_State* L)
{
    lua_pushinteger(L, jack_client_name_size()); 
    return 1;
}

static int is_realtime(lua_State *L)
{
    JackClient* client = getCheckedClient(L, 1);
    lua_pushboolean(L, jack_is_realtime(client->ptr));
    return 1;
}

static int client_name(lua_State* L)
{
    JackClient* client = getCheckedClient(L, 1);
    lua_pushstring(L, jack_get_client_name(client->ptr));
    return 1;
}

static int client_uuid(lua_State* L)
{
    JackClient* client = getCheckedClient(L, 1);
    const char* uuid = jack_client_get_uuid(client->ptr);
    if (!uuid) return 0;
    lua_pushstring(L, uuid);
    return 1;
}

static int client_name_to_uuid(lua_State* L)
{
    JackClient* client = getCheckedClient(L, 1);
    const char* name = luaL_checkstring(L, 2);
    char* uuid = jack_get_uuid_for_client_name(client->ptr, name);
    if (!uuid) return 0;
    lua_pushstring(L, uuid); 
    jack_free(uuid);
    return 1;
}

static int client_uuid_to_name(lua_State* L)
{
    JackClient* client = getCheckedClient(L, 1);
    const char* uuid   = luaL_checkstring(L, 2);
    char* name = jack_get_client_name_by_uuid(client->ptr, uuid);
    if (!name) return 0;
    lua_pushstring(L, name); 
    jack_free(name); 
    return 1;
}

static int activate(lua_State* L)
{
    JackClient* client = getCheckedClient(L, 1);
    if (jack_activate(client->ptr) != 0)
        return luaL_error(L, "cannot activate client");
    client->isActivated = true;
    return 0;
}

static int deactivate(lua_State* L)
{
    JackClient* client = getCheckedClient(L, 1);
    if (jack_deactivate(client->ptr) != 0)
        return luaL_error(L, "cannot deactivate client");
    client->isActivated = false;
    return 0;
}

static int buffer_size(lua_State* L)
{
    JackClient* client = getCheckedClient(L, 1);
    lua_pushinteger(L, jack_get_buffer_size(client->ptr));
    return 1;
}

static int client_sleep(lua_State* L)
{
    JackClient* client = getCheckedClient(L, 1);
    if (!client->isMaster) {
        return luaL_argerror(L, 1, "method can only be called on master client object");
    }
    double seconds = luaL_optnumber(L, 2, -1);
    
    if (client->shared) {
        async_mutex_lock         (&client->shared->mutex);
        async_mutex_wait_millis  (&client->shared->mutex, (int)(seconds * 1000));
        async_mutex_unlock       (&client->shared->mutex);
    }
    return 0;
}

static int client_release(lua_State* L)
{
    JackClient* client = getCheckedClient(L, 1);
    releaseClient(client);
    return 0;
}

static int client_close(lua_State* L)
{
    JackClient* client = getCheckedClient(L, 1);
    if (!client->isMaster) {
        return luaL_error(L, "can only close master client object");
    }
    releaseClient(client);
    return 0;
}


/////////////////////////////////////////////////////////////////////////

static const struct luaL_Reg ClientMethods[] = 
{
    { "ptr",                 client_ptr },
    { "name",                client_name },
    { "uuid",                client_uuid },
    { "is_realtime",         is_realtime },
    { "name_to_uuid",        client_name_to_uuid },
    { "uuid_to_name",        client_uuid_to_name },
    { "activate",            activate },
    { "deactivate",          deactivate },
    { "buffer_size",         buffer_size },
    { "close",               client_close},
    { "sleep",               client_sleep },
    { NULL, NULL } /* sentinel */
};

static const struct luaL_Reg ClientMetaMethods[] = 
{
    { "__tostring", client_toString },
    { "__gc",       client_release  },
    { NULL, NULL } /* sentinel */
};

static const struct luaL_Reg ModuleFunctions[] = 
{
    { "client_open",         client_open },
    { "client_name_size",    client_name_size },
    { "is_realtime",         is_realtime },
    { "client_close",        client_close},
    { "client_name",         client_name },
    { "client_uuid",         client_uuid },
    { "client_name_to_uuid", client_name_to_uuid },
    { "client_uuid_to_name", client_uuid_to_name },
    { "activate",            activate },
    { "deactivate",          deactivate },
    { "buffer_size",         buffer_size },
    { "sleep",               client_sleep },
    { NULL, NULL } /* sentinel */
};



bool luajack_open_client(lua_State* L, int module, int clientMeta, int clientClass)
{
    lua_pushvalue(L, module);
        luaL_setfuncs(L, ModuleFunctions, 0);

        lua_pushvalue(L, clientMeta);
            luaL_setfuncs(L, ClientMetaMethods, 0);
    
            lua_pushvalue(L, clientClass);
                luaL_setfuncs(L, ClientMethods, 0);
    
    lua_pop(L, 3);

    return true;
}
