#include <lua.h>

int luaopen_hello(lua_State* L) {
    lua_getglobal(L, "print");
    lua_pushliteral(L, "Hello, world!");
    lua_call(L, 1, 0);
    lua_pushboolean(L, 1);
    
    return 1;
};