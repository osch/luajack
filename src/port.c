#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <jack/session.h>

#include "util.h"
#include "port.h"
#include "port_util.h"

static int port_ptr(lua_State* L)
{
    JackPort* port = getCheckedPort(L, 1);
    lua_pushlightuserdata(L, port->ptr);
    return 1;
}

static int port_toString(lua_State* L)
{
    JackPort* port = getCheckedPort(L, 1);
    lua_pushfstring(L, "%s: \"%s\" (%p)", PORT_TYPE_NAME, 
                                          jack_port_name(port->ptr),
                                          port->ptr);
    return 1;
}

static int port_type_size (lua_State* L)
{
    lua_pushinteger(L, jack_port_type_size()); 
    return 1; 
}

static int port_name_size(lua_State* L)
{
    lua_pushinteger(L, jack_port_name_size()); 
    return 1; 
}


static int input_audio_port(lua_State* L)
{
    return registerPort(L, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput);
}    

static int output_audio_port(lua_State* L)
{
    return registerPort(L, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput);
}    

static int input_midi_port(lua_State* L)
{
    return registerPort(L, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput);
}    

static int output_midi_port(lua_State* L)
{
    return registerPort(L, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput);
}    


static int port_release(lua_State* L)
{
    JackPort* port = getCheckedPort(L, 1);
    releasePort(port->shared);
    port->ptr    = NULL;
    port->shared = NULL;
    return 0;
}

static const struct luaL_Reg PortMetaMethods[] = 
{
    { "__tostring", port_toString },
    { "__gc",       port_release  },
    { NULL, NULL } /* sentinel */
};

static const struct luaL_Reg PortMethods[] = 
{
    { "ptr",                 port_ptr },
    { NULL, NULL } /* sentinel */
};

static const struct luaL_Reg ClientMethods[] = 
{
    { "input_audio_port",   input_audio_port },
    { "output_audio_port",  output_audio_port },
    { "input_midi_port",    input_midi_port },
    { "output_midi_port",   output_midi_port },
    { NULL, NULL } /* sentinel */
};

static const struct luaL_Reg ModuleFunctions[] = 
{
        { "port_type_size",     port_type_size },
        { "port_name_size",     port_name_size },

        { "input_audio_port",   input_audio_port },
        { "output_audio_port",  output_audio_port },
        { "input_midi_port",    input_midi_port },
        { "output_midi_port",   output_midi_port },
#if 0
    #if 0 /* @@ CUSTOM PORT TYPE */
        { "input_custom_port", InputCustomPort }, 
        { "output_custom_port", OutputCustomPort }, 
    #endif
        { "port_unregister", PortUnregister },
        { "port_connect", PortConnect },
        { "nport_connect", PortnameConnect },
        { "connect", PortnameConnect },
        { "port_disconnect", PortDisconnect },
        { "nport_disconnect", PortnameDisconnect },
        { "disconnect", PortnameDisconnect },
        { "port_name", PortName },
        { "port_short_name", PortShortName },
        { "port_flags", PortFlags },
        { "nport_flags", PortnameFlags },
        { "port_uuid", PortUuid },
        { "nport_uuid", PortnameUuid },
        { "port_type", PortType },
        { "nport_type", PortnameType },
        { "nport_exists", PortnameExists },
        { "port_is_mine", PortIsMine },
        { "nport_is_mine", PortnameIsMine },
/*      { "port_set_name", PortSetName }, DEPRECATED */
        { "get_ports", GetPorts },
        { "port_set_alias", PortSetAlias },
        { "nport_set_alias", PortnameSetAlias },
        { "port_unset_alias", PortUnsetAlias },
        { "nport_unset_alias", PortnameUnsetAlias },
        { "port_aliases", PortAliases },
        { "nport_aliases", PortnameAliases },
        { "port_monitor", PortMonitor },
        { "nport_monitor", PortnameMonitor },
        { "port_monitoring", PortMonitoring },
        { "nport_monitoring", PortnameMonitoring },
        { "port_connections", PortConnections },
        { "nport_connections", PortnameConnections },
        { "port_connected", PortConnectedTo },
        { "nport_connected", PortnameConnectedTo },
#endif
    { NULL, NULL } /* sentinel */
};

bool luajack_open_port(lua_State* L, int module, int clientMeta, int clientClass,
                                                  int   portMeta, int   portClass)
{
    lua_pushvalue(L, module);
        luaL_setfuncs(L, ModuleFunctions, 0);

        lua_pushvalue(L, clientClass);
            luaL_setfuncs(L, ClientMethods, 0);
    
            lua_pushvalue(L, portMeta);
                luaL_setfuncs(L, PortMetaMethods, 0);
        
                lua_pushvalue(L, portClass);
                    luaL_setfuncs(L, PortMethods, 0);
            
    lua_pop(L, 4);
    
    return true;
}


