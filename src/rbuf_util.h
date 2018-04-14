#ifndef LUAJACK_RBUF_UTIL_H
#define LUAJACK_RBUF_UTIL_H

/////////////////////////////////////////////////////////////////////////////////

#define createJackRbuf luajack_createJackRbuf 

JackRbuf* createJackRbuf(lua_State* L, int size, bool mlocked);


/////////////////////////////////////////////////////////////////////////////////

#define releaseJackRbuf luajack_releaseJackRbuf 

void releaseJackRbuf(JackRbufShared* rbuf);

/////////////////////////////////////////////////////////////////////////////////

#define transferRbuf luajack_transferRbuf 

void transferRbuf(lua_State* T, JackRbufShared* sharedRbuf);

/////////////////////////////////////////////////////////////////////////////////

#define writeRbuf luajack_writeRbuf 

int writeRbuf(jack_ringbuffer_t* rbuf, lua_State* L, int arg);

#define readRbuf luajack_readRbuf 

int readRbuf(jack_ringbuffer_t* rbuf, lua_State* L, int arg);

/////////////////////////////////////////////////////////////////////////////////

#endif // LUAJACK_RBUF_UTIL_H
