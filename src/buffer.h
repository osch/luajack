#ifndef LUAJACK_BUFFER_H
#define LUAJACK_BUFFER_H

bool luajack_open_buffer(lua_State* L, int module, int clientMeta, int clientClass,
                                                   int   portMeta, int   portClass);

#endif // LUAJACK_BUFFER_H
