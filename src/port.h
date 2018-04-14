#ifndef LUAJACK_PORT_H
#define LUAJACK_PORT_H

bool luajack_open_port(lua_State* L, int module, int clientMeta, int clientClass,
                                                  int   portMeta, int   portClass);


#endif // LUAJACK_PORT_H
