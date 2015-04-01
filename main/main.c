#include "xMain.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
	if (argc <= 1) {
		printf("arguments error, need a config file.\n");
		return 0;
	}
	char *file = argv[1];
	struct lua_State *L = luaL_newstate();
	luaL_openlibs(L);
	if (luaL_dofile(L, file)) {
		printf("parse config file error.\n");
		return 0;
	}
	lua_pushnil(L);
	while (lua_next(L, -2) != 0) {
		if (lua_isstring(L, -2) && lua_isstring(L, -1)) {
			const char *key = lua_tostring(L, -2);
			const char *value = lua_tostring(L, -1);
			x_env_set(key, value);
		} else {
			printf("config file data error.\n");
			return 0;
		}
		lua_pop(L, 1);
	}
	lua_close(L);
	server_start();
	return 1;
}