#include <stdlib.h>

#include "util.h"
#include "rbuf.h"
#include "rbuf_util.h"

static int rbuf_ptr(lua_State* L)
{
    JackRbuf* rbuf = getCheckedRbuf(L, 1);
    lua_pushlightuserdata(L, rbuf->ptr);
    return 1;
}

static int rbuf_toString(lua_State* L)
{
    JackRbuf* rbuf = getCheckedRbuf(L, 1);
    lua_pushfstring(L, "%s: %p", RBUF_TYPE_NAME, 
                                 rbuf->ptr);
    return 1;
}

static int rbuf_new(lua_State* L)
{
    int nargs = lua_gettop(L);
    int arg = 1;
    
    if (getOptionalClient(L, arg))  ++arg; // for compatibility to luajack1
    int  size   = luaL_checkinteger(L, arg++);
    bool mlocked = false;

    if (nargs >= arg) {
        mlocked = lua_toboolean(L, arg++);
    }

    JackRbuf* rbuf = createJackRbuf(L, size, mlocked);

    if (rbuf) {
        return 1;
    } else {   
        return luaL_error(L, "cannot create ringbuffer");
    }
}

static int rbuf_release(lua_State* L)
{
    JackRbuf* rbuf = getCheckedRbuf(L, 1);
    releaseJackRbuf(rbuf->shared);
    rbuf->ptr    = NULL;
    rbuf->shared = NULL;
    return 0;
}

static int rbuf_write(lua_State* L)
{
    JackRbuf* rbuf = getCheckedRbuf(L, 1);
    return writeRbuf(rbuf->ptr, L, 2);
}

static int rbuf_read(lua_State* L)
{
    JackRbuf* rbuf = getCheckedRbuf(L, 1);
    return readRbuf(rbuf->ptr, L, 2);
}


static const struct luaL_Reg RbufMetaMethods[] = 
{
    { "__tostring", rbuf_toString },
    { "__gc",       rbuf_release }, 
    { NULL, NULL } /* sentinel */
};

static const struct luaL_Reg RbufMethods[] = 
{
    { "ptr",        rbuf_ptr   },
    { "write",      rbuf_write },
    { "read",       rbuf_read  },
    { NULL, NULL } /* sentinel */
};

static const struct luaL_Reg ClientMethods[] = 
{
    { NULL, NULL } /* sentinel */
};

static const struct luaL_Reg ModuleFunctions[] = 
{
    { "ringbuffer",        rbuf_new   },
    { "ringbuffer_write",  rbuf_write },
    { "ringbuffer_read",   rbuf_read  },
    { NULL, NULL } /* sentinel */
};

bool luajack_open_rbuf(lua_State* L, int module, int clientMeta, int clientClass,
                                                  int   rbufMeta, int   rbufClass)
{
    lua_pushvalue(L, module);
        luaL_setfuncs(L, ModuleFunctions, 0);

        lua_pushvalue(L, clientClass);
            luaL_setfuncs(L, ClientMethods, 0);
    
            lua_pushvalue(L, rbufMeta);
                luaL_setfuncs(L, RbufMetaMethods, 0);
        
                lua_pushvalue(L, rbufClass);
                    luaL_setfuncs(L, RbufMethods, 0);
            
    lua_pop(L, 4);
    
    return true;

}


