#ifndef LUAJACK_UTIL_H
#define LUAJACK_UTIL_H

#include <string.h>
#include <stdlib.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <jack/jack.h>
#include <jack/ringbuffer.h>

#include "async_util.h"

/////////////////////////////////////////////////////////////////////////////////

typedef int bool;
#ifndef true
    #define true  1
    #define false 0
#endif

/////////////////////////////////////////////////////////////////////////////////

#define DECLARE_NEW_OBJ(Type, typeName) \
\
static inline Type* pushNew##Type(lua_State* L) \
{ \
    Type* obj = (Type*) lua_newuserdata(L, sizeof(Type)); \
    memset(obj, 0, sizeof(Type)); \
    obj->shared = (Type##Shared*) malloc(sizeof(Type##Shared)); \
    if (!obj->shared) { \
        luaL_error(L, "cannot create object of type %s", typeName); \
        return NULL; } \
    memset(obj->shared, 0, sizeof(Type##Shared)); \
    obj->shared->refCounter = 1; \
    luaL_setmetatable(L, typeName); \
    return obj; \
}

#define pushNew(L, Type) pushNew##Type(L)

/////////////////////////////////////////////////////////////////////////////////

#define CLIENT_TYPE_NAME "luajack.client"
#define PORT_TYPE_NAME   "luajack.port"
#define RBUF_TYPE_NAME   "luajack.ringbuffer"
#define THREAD_TYPE_NAME "luajack.thread"

/////////////////////////////////////////////////////////////////////////////////

/* If this is printed, it denotes a suspect LuaJack bug: */
#define UNEXPECTED_ERROR "unexpected error (%s, %d)", __FILE__, __LINE__

/////////////////////////////////////////////////////////////////////////////////

#define setVerbose luajack_setVerbose
void setVerbose(bool verbose);

#define verbosePrint luajack_verbosePrint
void verbosePrint(const char* msg);

#define verbose luajack_verbosePrintf
int verbosePrintf(const char* fmt, ...);

#define checkonoff luajack_checkonoff
bool checkonoff(lua_State* L, int arg);

/////////////////////////////////////////////////////////////////////////////////

typedef struct {
    jack_client_t*           ptr;
    AtomicCounter            refCounter;
    Mutex                    mutex;
    lua_State*               mainContext;
    lua_State*               processContext;
    int                      processCallbackRef;
    jack_nframes_t           currentProcessNframes;
}
JackClientShared;

typedef struct {
    jack_client_t*    ptr;
    bool              isMaster;
    bool              isActivated;
    bool              isInProcessContext;
    JackClientShared* shared;
}
JackClient;

DECLARE_NEW_OBJ(JackClient, CLIENT_TYPE_NAME);

static inline JackClient* getCheckedClient(lua_State* L, int stackIndex)
{
    JackClient* client = (JackClient*) luaL_checkudata(L, stackIndex, CLIENT_TYPE_NAME);
    return client;
}
    
static inline JackClient* getOptionalClient(lua_State* L, int stackIndex)
{
    JackClient* client = (JackClient*) luaL_testudata(L, stackIndex, CLIENT_TYPE_NAME);
    return client;
}
    

/////////////////////////////////////////////////////////////////////////////////

typedef struct {
    jack_port_t*      ptr;
    AtomicCounter     refCounter;
    JackClientShared* client;
}
JackPortShared;

typedef struct {
    jack_port_t*    ptr;
    lua_State*      mainContext;
    bool            isInProcessContext;
    JackPortShared* shared;
} 
JackPort;

DECLARE_NEW_OBJ(JackPort, PORT_TYPE_NAME);

static inline JackPort* getCheckedPort(lua_State* L, int stackIndex)
{
    JackPort* port = (JackPort*)luaL_checkudata(L, stackIndex, PORT_TYPE_NAME);
    return port;
}
    
static inline JackPort* getOptionalPort(lua_State* L, int stackIndex)
{
    JackPort* port = (JackPort*)luaL_testudata(L, stackIndex, PORT_TYPE_NAME);
    return port;
}
    

/////////////////////////////////////////////////////////////////////////////////

typedef struct {
    jack_ringbuffer_t* ptr;
    AtomicCounter refCounter;
}
JackRbufShared;

typedef struct {
    jack_ringbuffer_t* ptr;
    lua_State*         mainContext;
    JackRbufShared*    shared;
} 
JackRbuf;

DECLARE_NEW_OBJ(JackRbuf, RBUF_TYPE_NAME);

static inline JackRbuf* getCheckedRbuf(lua_State* L, int stackIndex)
{
    JackRbuf* rbuf = (JackRbuf*)luaL_checkudata(L, stackIndex, RBUF_TYPE_NAME);
    return rbuf;
}
    
static inline JackRbuf* getOptionalRbuf(lua_State* L, int stackIndex)
{
    JackRbuf* rbuf = (JackRbuf*)luaL_testudata(L, stackIndex, RBUF_TYPE_NAME);
    return rbuf;
}
    
/////////////////////////////////////////////////////////////////////////////////

typedef struct {
    jack_native_thread_t thread;
    lua_State*           mainContext;
} 
JackThread;

/////////////////////////////////////////////////////////////////////////////////

int luajack_xmove(JackClientShared* client, lua_State* T, lua_State* L, int chunkName, int chunck_index, int last_index);

/////////////////////////////////////////////////////////////////////////////////

#endif // LUAJACK_UTIL_H
