#include <stdlib.h>

#include "util.h"
#include "rbuf_util.h"

/////////////////////////////////////////////////////////////////////////////////

JackRbuf* createJackRbuf(lua_State* L, int size, bool mlocked)
{
    lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_MAINTHREAD);
    lua_State* thisContext = lua_tothread(L, -1);
    lua_pop(L, 1);

    JackRbuf* rbuf = pushNew(L, JackRbuf);

    rbuf->ptr = jack_ringbuffer_create(size);

    if (rbuf->ptr) {
        rbuf->mainContext = thisContext;
        rbuf->shared->ptr = rbuf->ptr;
        if (mlocked)  {
            jack_ringbuffer_mlock(rbuf->ptr);
        }
        return rbuf;
    } else {
        return NULL;
    }
}

/////////////////////////////////////////////////////////////////////////////////

void releaseJackRbuf(JackRbufShared* sharedRbuf)
{
    if (sharedRbuf) {
        if (atomic_dec(&sharedRbuf->refCounter) == 0) {
            if (sharedRbuf->ptr) {
                jack_ringbuffer_free(sharedRbuf->ptr);
                sharedRbuf->ptr = NULL;
            }
            free(sharedRbuf);
        }
    }
}


/////////////////////////////////////////////////////////////////////////////////


void transferRbuf(lua_State* T, JackRbufShared* sharedRbuf)
{
    lua_rawgeti(T, LUA_REGISTRYINDEX, LUA_RIDX_MAINTHREAD);
    lua_State* thisContext = lua_tothread(T, -1);
    lua_pop(T, 1);

    JackRbuf* rbuf = (JackRbuf*) lua_newuserdata(T, sizeof(JackRbuf));
    memset(rbuf, 0, sizeof(JackRbuf));

    luaL_setmetatable(T, RBUF_TYPE_NAME);

    rbuf->ptr         = sharedRbuf->ptr;
    rbuf->mainContext = thisContext;
    rbuf->shared      = sharedRbuf;
    
    atomic_inc(&sharedRbuf->refCounter);
}


/////////////////////////////////////////////////////////////////////////////////

typedef struct {
	/* use fixed size types, so to ease the dimensioning of the ringbuffer ... */
	int32_t	tag;
	uint32_t len;	/* length of data that follow */
} hdr_t;


int writeRbuf(jack_ringbuffer_t *rbuf, lua_State *L, int arg)
/* bool, errmsg = write(..., tag, data)
 * expects 
 * tag (integer) at index 'arg' of the stack, and 
 * data (string) at index 'arg+1' (optional)
 *
 * if there is not enough space available, it returns 'false, "no space"';
 * data may be an empty string ("") or nil, in which case it defaults
 * to the empty string (i.e., the message has only the header).
 */
{
    jack_ringbuffer_data_t vec[2];
    hdr_t hdr;
    int isnum;
    size_t space, cnt;
    size_t len;
    const char *data;
    hdr.tag = (uint32_t)lua_tointegerx(L, arg, &isnum);
    if(!isnum)
        luaL_error(L, "invalid tag");

    data = luaL_optlstring(L, arg + 1, NULL, &len);
    if(!data)
        hdr.len = 0;
    else
        hdr.len = len; /*@@*/

    space = jack_ringbuffer_write_space(rbuf);
    if((sizeof(hdr) + hdr.len) > space)
        { lua_pushboolean(L, 0); return 1; }

    /* write header first (this automatically advances) */
    cnt = jack_ringbuffer_write(rbuf, (const char *)&hdr, sizeof(hdr));
    if(cnt != sizeof(hdr))
        return luaL_error(L, UNEXPECTED_ERROR);

    if(hdr.len)
        {
        /* write data */
        jack_ringbuffer_get_write_vector(rbuf, vec);
        if((vec[0].len+vec[1].len) < hdr.len)
            return luaL_error(L, UNEXPECTED_ERROR);
        if(vec[0].len >= hdr.len)
            memcpy(vec[0].buf, data, hdr.len);
        else
            {
            memcpy(vec[0].buf, data, vec[0].len);
            memcpy(vec[1].buf, data + vec[0].len, hdr.len - vec[0].len);
            }
        jack_ringbuffer_write_advance(rbuf, hdr.len);
        }
    lua_pushboolean(L, 1);
    return 1;
}

/////////////////////////////////////////////////////////////////////////////////

int readRbuf(jack_ringbuffer_t *rbuf, lua_State *L, int advance)
/* tag, data = read()
 * returns tag=nil if there is not a complete message (header+data) in
 * the ringbuffer
 * if the header.len is 0, data is returned as an empty string ("")
 */
	{
	jack_ringbuffer_data_t vec[2];
	hdr_t hdr;
	uint32_t len;
	size_t cnt;

	/* peek for header */
	cnt = jack_ringbuffer_peek(rbuf, (char*)&hdr, sizeof(hdr));
	if(cnt != sizeof(hdr))
		{ lua_pushnil(L); return 1; }
	
	/* see if there are 'len' bytes of data available */
	cnt = jack_ringbuffer_read_space(rbuf);
	if( cnt < (sizeof(hdr) + hdr.len) )
		{ lua_pushnil(L); return 1; }
	
	lua_pushinteger(L, hdr.tag);

	if(hdr.len == 0) /* header only */
		{ 
		if(advance)
			jack_ringbuffer_read_advance(rbuf, sizeof(hdr));
		lua_pushstring(L, ""); 
		return 2; 
		}
		
	/* get the read vector */
	jack_ringbuffer_get_read_vector(rbuf, vec);

	//printf("vec[0].len=%u, vec[1].len=%u hdr.len=%u\n",vec[0].len,vec[1].len,hdr.len);
	if(vec[0].len >= (sizeof(hdr) + hdr.len)) /* data fits in vec[0] */
		lua_pushlstring(L, vec[0].buf + sizeof(hdr), hdr.len);
	else if(vec[0].len > sizeof(hdr))
		{
		len = vec[0].len - sizeof(hdr);
		lua_pushlstring(L, vec[0].buf + sizeof(hdr), len);
		lua_pushlstring(L, vec[1].buf, hdr.len - len);
		lua_concat(L, 2);
		}
	else /* vec[0] contains only the header or part of it (data is all in vec[1]) */
		{
		len = sizeof(hdr) - vec[0].len; /* the first len bytes in vec1 are part of the header */
		lua_pushlstring(L, vec[1].buf + len, hdr.len);
		}
	if(advance)
		jack_ringbuffer_read_advance(rbuf, sizeof(hdr) + hdr.len);
	return 2;
	}

/////////////////////////////////////////////////////////////////////////////////
