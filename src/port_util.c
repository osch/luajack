#include <stdlib.h>

#include "util.h"
#include "port_util.h"
#include "client_util.h"

//////////////////////////////////////////////////////////////////////////////////////////////
// PortOptions {

unsigned long getPortOptions(lua_State* L, int optionsIndex)
{
    unsigned long flags = 0;

    if (lua_istable(L, 3))
    {
        lua_getfield(L, optionsIndex, "is_physical");
        if (lua_toboolean(L, -1)) flags |= JackPortIsPhysical;

        lua_getfield(L, optionsIndex, "can_monitor");
        if (lua_toboolean(L, -1)) flags |= JackPortCanMonitor;

        lua_getfield(L, optionsIndex, "is_terminal");
        if (lua_toboolean(L, -1)) flags |= JackPortIsTerminal;
        
        lua_pop(L, 3);
    }
    return flags;
}

// PortOptions }
//////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////

int registerPort(lua_State* L, const char* port_type, unsigned long inoutFlags)
{
    JackClient* client   = getCheckedClient(L, 1);
    const char* portName = luaL_checkstring(L, 2);
    unsigned long flags  = getPortOptions(L, 3);

    lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_MAINTHREAD);
    lua_State* thisContext = lua_tothread(L, -1);
    lua_pop(L, 1);
    
    JackPort* port = pushNew(L, JackPort);
    
    port->ptr = jack_port_register(client->ptr, 
                                   portName, 
                                   port_type, 
                                   flags | inoutFlags,
                                   0);
    if (!port->ptr) {
        // lua will release the JackPort-Object
        return luaL_error(L, "cannot register port");
    }
    
    verbosePrintf("registered port '%s'\n", jack_port_name(port->ptr));
    
    port->mainContext    = thisContext;

    port->shared->ptr    = port->ptr;
    port->shared->client = client->shared;

    atomic_inc(&client->shared->refCounter);
    
    return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////

void releasePort(JackPortShared* sharedPort)
{
    if (sharedPort) {
        if (atomic_dec(&sharedPort->refCounter) == 0) {
            if (sharedPort->client) {
                releaseClientShared(sharedPort->client);
                sharedPort->client = NULL;
            }
            free(sharedPort);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////

void transferPort(lua_State* T, JackPortShared* sharedPort)
{
    lua_rawgeti(T, LUA_REGISTRYINDEX, LUA_RIDX_MAINTHREAD);
    lua_State* thisContext = lua_tothread(T, -1);
    lua_pop(T, 1);

    JackPort* port = (JackPort*) lua_newuserdata(T, sizeof(JackPort));
    memset(port, 0, sizeof(JackPort));

    luaL_setmetatable(T, PORT_TYPE_NAME);

    port->ptr                     = sharedPort->ptr;
    port->mainContext             = thisContext;
    port->isInProcessContext      = true;
    port->shared                  = sharedPort;
    
    atomic_inc(&sharedPort->refCounter);
}

//////////////////////////////////////////////////////////////////////////////////////////////
