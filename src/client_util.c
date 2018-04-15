#include <stdlib.h>

#include "util.h"
#include "client_util.h"

//////////////////////////////////////////////////////////////////////////////////////////////
// JackOptionParameters {

static inline void addOption(jack_options_t* flags, jack_options_t flag) {
    *flags = (jack_options_t)(*flags | flag);
}

static void addIfBoolOption(jack_options_t* flags,
                            lua_State*      L, 
                            int             optIndex, 
                            const char*     optionName, 
                            jack_options_t  flag)
{
    lua_getfield(L, optIndex, optionName);
    if (lua_toboolean(L, -1)) addOption(flags, flag); 
    lua_pop(L, 1);
}

static const char* addIfStringOption(jack_options_t* flags,
                                     lua_State*      L, 
                                     int             optIndex, 
                                     const char*     optionName, 
                                     jack_options_t  flag)
{
    lua_getfield(L, optIndex, optionName);
    const char* value = luaL_optstring(L, -1, NULL);
    if (value) addOption(flags, flag);
    // lua_pop(L, 1); dont't pop to keep value alive for this stack
    return value;
}

JackOptionParameters getJackOptionParameters(lua_State* L, int optIndex)
{
    JackOptionParameters rslt;
    rslt.flags      = (jack_options_t)0;
    rslt.serverName = NULL;
    rslt.sessionId  = NULL;
    
    if (lua_istable(L, optIndex)) // options
    {
        lua_checkstack(L, LUA_MINSTACK);

        addIfBoolOption(&rslt.flags, L, optIndex, "use_exact_name",  JackUseExactName);
        addIfBoolOption(&rslt.flags, L, optIndex, "no_start_server", JackNoStartServer);

        rslt.serverName = addIfStringOption(&rslt.flags, L, optIndex, "server_name", JackServerName);
        rslt.sessionId  = addIfStringOption(&rslt.flags, L, optIndex, "session_id",  JackSessionID);
    }
    return rslt;
}

// JackOptionParameters }
//////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////
// JackStatus {

static inline void addStatus(luaL_Buffer* buf, int* n, const char* s)
{
    if ((*n)++ > 0) luaL_addstring(buf, "|");
    luaL_addstring(buf, s);
}

void pushStatusBitsToString(lua_State* L, jack_status_t bits)
{
    luaL_checkstack(L, 30, "cannot grow Lua stack");
    
    luaL_Buffer buf;
    luaL_buffinit(L, &buf);
    
    int n = 0;
    
    if (bits == 0) {
        addStatus(&buf, &n, "no_error"); 
    } else {
        if (bits & JackFailure)       addStatus(&buf, &n, "failure");
        if (bits & JackInvalidOption) addStatus(&buf, &n, "invalid_option");
        if (bits & JackNameNotUnique) addStatus(&buf, &n, "name_not_unique");
        if (bits & JackServerStarted) addStatus(&buf, &n, "server_started");
        if (bits & JackServerFailed)  addStatus(&buf, &n, "server_failed");
        if (bits & JackServerError)   addStatus(&buf, &n, "server_error");
        if (bits & JackNoSuchClient)  addStatus(&buf, &n, "no_such_client");
        if (bits & JackLoadFailure)   addStatus(&buf, &n, "load_failure");
        if (bits & JackInitFailure)   addStatus(&buf, &n, "init_failure");
        if (bits & JackShmFailure)    addStatus(&buf, &n, "shm_failure");
        if (bits & JackVersionError)  addStatus(&buf, &n, "version_error");
        if (bits & JackBackendError)  addStatus(&buf, &n, "backend_error");
        if (bits & JackClientZombie)  addStatus(&buf, &n, "client_zombie");
    }
    luaL_pushresult(&buf);
}

// JackStatus }
//////////////////////////////////////////////////////////////////////////////////////////////

void releaseClient(JackClient* client)
{
    if (client) 
    {
        if (client->isMaster && client->ptr) 
        {
            if (client->isActivated) {
                verbosePrintf("Deactivate client\n");
                jack_deactivate(client->ptr);
                client->isActivated = false;
            }

            if (client->shared->processContext) {
                verbosePrintf("Close process context\n");
                lua_close(client->shared->processContext);
                client->shared->processContext = NULL;
                client->shared->processCallbackRef = LUA_NOREF;               
            }
            
            verbosePrintf("close client '%s'\n", jack_get_client_name(client->ptr));

            jack_client_close(client->ptr);

            verbosePrintf("client closed\n");
            
            if (client->shared) {
                client->shared->ptr = NULL;
            }
            client->ptr = NULL;
        }
        
        releaseClientShared(client->shared);
        client->shared = NULL;
        client->ptr    = NULL;
    }
}


void releaseClientShared(JackClientShared* shared)
{
    if (shared && atomic_dec(&shared->refCounter) == 0) {
        async_mutex_destruct(&shared->mutex);
        if (shared->processContextChunkName) {
            free(shared->processContextChunkName);
        }
        if (shared->errorInProcessContext) {
            free(shared->errorInProcessContext);
        }
        free(shared);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////

bool transferClient(lua_State* T, JackClient* client)
{
    JackClient* processClient = (JackClient*) lua_newuserdata(T, sizeof(JackClient));
    memset(processClient, 0, sizeof(JackClient));

    luaL_setmetatable(T, CLIENT_TYPE_NAME);
    
    processClient->ptr                     = client->ptr;
    processClient->isInProcessContext      = true;
    processClient->shared                  = client->shared;
    
    if (client->shared) {
        atomic_inc(&client->shared->refCounter);
    }
    
    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////

