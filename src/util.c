#include <string.h>

#include "util.h"
#include "port_util.h"
#include "client_util.h"
#include "rbuf_util.h"
#include "main.h"

//////////////////////////////////////////////////////////////////////////////////////////////

static volatile bool verboseFlag = false;

void setVerbose(bool arg) 
{
    verboseFlag = arg;
}

void verbosePrint(const char* msg)
{
    if (!verboseFlag || !msg) return;
	fprintf(stderr, "%s\n", msg);
}


int verbosePrintf(const char* fmt, ...)
{
    if (!verboseFlag || !fmt) return 0;
    va_list args;
    va_start(args, fmt);
    int rslt = vfprintf(stderr, fmt, args);
    va_end(args);
    return rslt;
}

//////////////////////////////////////////////////////////////////////////////////////////////


bool checkonoff(lua_State* L, int arg)
{
    if (lua_isboolean(L, arg)) {
        return lua_toboolean(L, arg);
    }
    else {
        const char* onoff = luaL_checkstring(L, arg);
        if (strcmp(onoff, "on") == 0)
            return true;
        if (strcmp(onoff, "off") == 0)
            return false;
        return luaL_error(L, "invalid onoff argument");
    }
}


//////////////////////////////////////////////////////////////////////////////////////////////


static inline void setArg(lua_State* T, int argTable, int arg)
{
    lua_pushvalue(T, -1); 
    lua_seti(T, argTable, arg);
}

int luajack_xmove(JackClientShared* client, lua_State* T, lua_State* L, int chunkName, int first_index, int last_index)
/* Checks and copies arguments between unrelated states L and T (L to T)
 * (lua_xmove() cannot be used here, because the states are not related).
 *
 * L[chunkName]    -----> arg[0] script name, or chunk
 * L[first_index    ] --> arg[1]
 * L[first_index  +1] --> arg[2]
 * ...
 * L[last_index]      --> arg[N]
 * Since arguments are to be passed between unrelated states, the only admitted
 * types are: nil, boolean, number and string.
 */ 
{
    int nargs = last_index + 1 - first_index ; /* no. of optional arguments */
    luaL_checkstack(T, nargs, "cannot grow Lua stack for thread");
    
    luaopen_luajack(T);
    lua_setglobal(T, "jack");
    
    lua_newtable(T); /* "arg" table */
    int argTable = lua_gettop(T);

    lua_pushstring(T, lua_tostring(L, chunkName)); /* arg[0] = full scriptname */
    lua_seti(T, argTable, 0);

    int arg = 1;
    int n;
    for (n = first_index; n <= last_index; n++)
    {
        int type = lua_type(L, n);
        switch (type)
        {
            case LUA_TNIL:     lua_pushnil(T);                            setArg(T, argTable, arg++); break;
            case LUA_TBOOLEAN: lua_pushboolean(T, lua_toboolean(L, n));   setArg(T, argTable, arg++); break;
            case LUA_TNUMBER:  if (lua_isinteger(L, n)) {
                                 lua_pushinteger(T, lua_tointeger(L, n));
                               } else {
                                 lua_pushnumber(T, lua_tonumber(L, n)); } setArg(T, argTable, arg++); break;
            case LUA_TSTRING:  lua_pushstring(T, lua_tostring(L, n));     setArg(T, argTable, arg++); break;
            case LUA_TUSERDATA: {
                JackPort* p = getOptionalPort(L, n);
                if (p && p->shared) {
                    if (p->shared->client == client) {
                        transferPort(T, p->shared);
                        break;   
                    } else {
                        lua_pushfstring(L, "port '%s' does not belong to client '%s'", 
                                           jack_port_name(p->ptr),
                                           jack_get_client_name(client->ptr));
                        return 1;
                    }
                }
                JackClient* c = getOptionalClient(L, n);
                if (c && c->shared) {
                    if (c->shared == client) {
                        if (!transferClient(T, c)) {
                            lua_pushfstring(L, "cannot transfer client");
                            return 1;
                        }
                        break;
                    } else {
                        lua_pushfstring(L, "client '%s' cannot be transferred to process context for client '%s'", 
                                           jack_get_client_name(c->ptr),
                                           jack_get_client_name(client->ptr));
                        return 1;
                    }
                }
                JackRbuf* b = getOptionalRbuf(L, n);
                if (b && b->shared) {
                    transferRbuf(T, b->shared);
                    break;
                }
                // FALLTHROUGH
            }
            default:
                lua_pushfstring(L, "invalid type '%s' of argument #%d ", 
                                    lua_typename(L, type), n - first_index + 1);
                return 1;
        }
    }
    lua_pushvalue(T, argTable);
    lua_setglobal(T, "arg");
    lua_remove(T, argTable);
    return 0;
}
