#include "thread.h"
#include "thread_util.h"



static const struct luaL_Reg ThreadMetaMethods[] = 
{
//    { "__tostring", thread_toString },
    { NULL, NULL } /* sentinel */
};

static const struct luaL_Reg ThreadMethods[] = 
{
//    { "ptr",        thread_ptr},
    { NULL, NULL } /* sentinel */
};

static const struct luaL_Reg ClientMethods[] = 
{
//    { "thread_load", thread_load }, TODO...
    { NULL, NULL } /* sentinel */
};

static const struct luaL_Reg ModuleFunctions[] = 
{
//    { "thread_load", thread_load }, TODO...
    { NULL, NULL } /* sentinel */
};

bool luajack_open_thread(lua_State* L, int module, int clientMeta, int clientClass,
                                                    int threadMeta, int threadClass)
{
    lua_pushvalue(L, module);
        luaL_setfuncs(L, ModuleFunctions, 0);

        lua_pushvalue(L, clientClass);
            luaL_setfuncs(L, ClientMethods, 0);
    
            lua_pushvalue(L, threadMeta);
                luaL_setfuncs(L, ThreadMetaMethods, 0);
        
                lua_pushvalue(L, threadClass);
                    luaL_setfuncs(L, ThreadMethods, 0);
            
    lua_pop(L, 4);
    
    return true;
}


