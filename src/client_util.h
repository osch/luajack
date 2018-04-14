#ifndef LUAJACK_CLIENT_UTIL_H
#define LUAJACK_CLIENT_UTIL_H

#include "util.h"

/////////////////////////////////////////////////////////////////////////////////

typedef struct 
{
    jack_options_t flags;
    const char*    serverName;
    const char*    sessionId;
}
JackOptionParameters;

#define getJackOptionParameters luajack_getJackOptionParameters 

JackOptionParameters getJackOptionParameters(lua_State* L, int optIndex);


/////////////////////////////////////////////////////////////////////////////////

#define pushStatusBitsToString luajack_pushStatusBitsToString  

void pushStatusBitsToString(lua_State* L, jack_status_t bits);


/////////////////////////////////////////////////////////////////////////////////

#define releaseClient luajack_releaseClient 

void releaseClient(JackClient* client);

#define releaseClientShared luajack_releaseClientShared

void releaseClientShared(JackClientShared* client);


/////////////////////////////////////////////////////////////////////////////////

#define transferClient luajack_transferClient

bool transferClient(lua_State* T, JackClient* client);


/////////////////////////////////////////////////////////////////////////////////

#endif // LUAJACK_CLIENT_UTIL_H