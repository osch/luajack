#ifndef LUAJACK_THREAD_H
#define LUAJACK_THREAD_H

#include "util.h"

bool luajack_open_thread(lua_State* L, int module, int clientMeta, int clientClass,
                                                    int threadMeta, int threadClass);

#endif // LUAJACK_THREAD_H
