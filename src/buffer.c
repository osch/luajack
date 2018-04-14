#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "util.h"
#include "buffer.h"

static int buffer_clear(lua_State* L)
{
    JackPort* dstPort = getCheckedPort(L, 1);
    
    if (dstPort->ptr && dstPort->shared && dstPort->shared->client) {
        lua_Integer nframes = dstPort->shared->client->currentProcessNframes;
        jack_default_audio_sample_t* out  = jack_port_get_buffer (dstPort->ptr, nframes);;
        memset(out, 0, sizeof(jack_default_audio_sample_t) * nframes);
    }
    return 0;
}

static int buffer_copy(lua_State* L)
{
    JackPort*     dstPort = getCheckedPort(L, 1);
    JackPort*     srcPort = getCheckedPort(L, 2);
    
    if (dstPort->ptr && srcPort->ptr && dstPort->shared && dstPort->shared->client) {
        lua_Integer nframes;
        if (lua_isnumber(L, 3)) {
            nframes = lua_tointeger(L, 3);
            if (nframes < 0) {
                nframes = 0;
            } else if (nframes > dstPort->shared->client->currentProcessNframes) {
                nframes = dstPort->shared->client->currentProcessNframes; // TODO error
            }
        } else {
            nframes = dstPort->shared->client->currentProcessNframes;
        }
        jack_default_audio_sample_t* in   = jack_port_get_buffer (srcPort->ptr, nframes);
        jack_default_audio_sample_t* out  = jack_port_get_buffer (dstPort->ptr, nframes);;
        
        memcpy (out, in, sizeof(jack_default_audio_sample_t) * nframes);
    }
    return 0;
}


static const struct luaL_Reg PortMethods[] = 
{
    { "clear",      buffer_clear },
    { "copy_from",  buffer_copy },
    { NULL, NULL } /* sentinel */
};

static const struct luaL_Reg ModuleFunctions[] = 
{
    { "clear", buffer_clear },
    { "copy",  buffer_copy },
    { NULL, NULL } /* sentinel */
};

bool luajack_open_buffer(lua_State* L, int module, int clientMeta, int clientClass,
                                                   int   portMeta, int   portClass)
{
    lua_pushvalue(L, module);
        luaL_setfuncs(L, ModuleFunctions, 0);

        lua_pushvalue(L, portClass);
            luaL_setfuncs(L, PortMethods, 0);

    lua_pop(L, 2);
    
    return true;
}

