#include "xServer.h"
#include "xMalloc.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include <string.h>

static X_INLINE void register_function(lua_State *L, const char *name, lua_CFunction function) {
    lua_pushstring(L, name);
    lua_pushcfunction(L, function);
    lua_settable(L, 1);
}

// api for server instance management

static int l_instance_launch(lua_State *L) {
    const char *module_name = luaL_checkstring(L, 1);
    const char *parameter = luaL_optlstring(L, 2, NULL, NULL);
    lua_pushinteger(L, x_instance_launch(module_name, parameter));
    return 1;
}

static int l_instance_kill(lua_State *L) {
    const int32_t instance_id = (int32_t)luaL_checkinteger(L, 1);
    x_instance_kill(instance_id);
    return 0;
}

// api for server logger

static int l_server_log(lua_State *L) {
    const int32_t instance_id = (int32_t)luaL_checkinteger(L, 1);
    const char *log = luaL_checkstring(L, 2);
    x_server_log(instance_id, log);
    return 0;
}

// api for server environment varribles

static int l_env_set(lua_State *L) {
    const char *key = luaL_checkstring(L, 1);
    const char *value = luaL_checkstring(L, 2);
    x_env_set(key, value);
    return 0;
}

static int l_env_get_value(lua_State *L) {
    const char *key = luaL_checkstring(L, 1);
    lua_pushstring(L, x_env_get_value(key));
    return 1;
}

static int l_env_get_key(lua_State *L) {
    const char *value = luaL_checkstring(L, 1);
    lua_pushstring(L, x_env_get_key(value));
    return 1;
}

// api for server message queue

static int l_server_message(lua_State *L) {
    const int32_t from = (int32_t)luaL_checkinteger(L, 1);
    const int32_t to = (int32_t)luaL_checkinteger(L, 2);
    const char *message = luaL_checklstring(L, 3, NULL);
    size_t size = strlen(message) + 1;
    void *data = x_malloc(size);
    memcpy(data, message, size);
    x_server_message(from, to, data, size);
    return 0;
}

// api for server timmer

static int l_timer_schedule(lua_State *L) {
    const int32_t instance_id = (int32_t)luaL_checkinteger(L, 1);
    const int32_t tick = (int32_t)luaL_optinteger(L, 2, 0);
    lua_pushinteger(L, x_timer_schedule(instance_id, tick));
    return 1;
}

static int l_timer_now(lua_State *L) {
    lua_pushinteger(L, x_timer_now());
    return 1;
}

// api for server socket

static int l_socket_create(lua_State *L) {
    const int32_t instance_id = (int32_t)luaL_checkinteger(L, 1);
    lua_pushinteger(L, x_socket_create(instance_id));
    return 1;
}

static int l_socket_listen(lua_State *L) {
    const int32_t instance_id = (int32_t)luaL_checkinteger(L, 1);
    const int32_t socket_id = (int32_t)luaL_checkinteger(L, 2);
    const char *ip = luaL_optlstring(L, 3, NULL, NULL);
    const uint16_t port = (uint16_t)luaL_checkinteger(L, 4);
    const int32_t backlog = (int32_t)luaL_checkinteger(L, 5);
    lua_pushinteger(L, x_socket_listen(instance_id, socket_id, ip, port, backlog));
    return 1;
}

static int l_socket_connect(lua_State *L) {
    const int32_t instance_id = (int32_t)luaL_checkinteger(L, 1);
    const int32_t socket_id = (int32_t)luaL_checkinteger(L, 2);
    const char *ip = luaL_optlstring(L, 3, NULL, NULL);
    const uint16_t port = (uint16_t)luaL_checkinteger(L, 4);
    lua_pushinteger(L, x_socket_connect(instance_id, socket_id, ip, port));
    return 1;
}

static int l_socket_send(lua_State *L) {
    const int32_t instance_id = (int32_t)luaL_checkinteger(L, 1);
    const int32_t socket_id = (int32_t)luaL_checkinteger(L, 2);
    const char *message = luaL_checklstring(L, 3, NULL);
    size_t size = strlen(message) + 1;
    void *data = x_malloc(size);
    memcpy(data, message, size);
    x_socket_send(instance_id, socket_id, data, size);
    return 0;
}

static int l_socket_close(lua_State *L) {
    const int32_t instance_id = (int32_t)luaL_checkinteger(L, 1);
    const int32_t socket_id = (int32_t)luaL_checkinteger(L, 2);
    x_socket_close(instance_id, socket_id);
    return 0;
}

void message_callback(void *L, const int32_t from, const void *message, const size_t size) {
    lua_getglobal(L, "xServer");
    lua_pushstring(L, "message_callback");
    lua_gettable(L, 1);
    if (lua_isfunction(L, -1)) {
        lua_pushinteger(L, from);
        lua_pushstring(L, message);
        lua_pcall(L, 2, 0, 0);
    }
    x_free((void *)message);
}

void timer_callback(void *L, uint32_t now) {
    lua_getglobal(L, "xServer");
    lua_pushstring(L, "timer_callback");
    lua_gettable(L, 1);
    if (lua_isfunction(L, -1)) {
        lua_pushinteger(L, now);
        lua_pcall(L, 1, 0, 0);
    }
}

void socket_callback(void *L, const int32_t socket_id, const void *message, const size_t size) {
    lua_getglobal(L, "xServer");
    lua_pushstring(L, "socket_callback");
    lua_gettable(L, 1);
    if (lua_isfunction(L, -1)) {
        lua_pushinteger(L, socket_id);
        lua_pushstring(L, message);
        lua_pcall(L, 2, 0, 0);
    }
    x_free((void *)message);
}

X_MODULE_API void *xLua_create() {
    return luaL_newstate();
}

X_MODULE_API int32_t xLua_init(lua_State *L, const int32_t id, const char *parameters) {
	luaL_openlibs(L);
	lua_newtable(L);
	lua_pushstring(L, "id");
	lua_pushinteger(L, id);
	lua_settable(L, 1);
    // api for server instance management
    register_function(L, "instance_launch", l_instance_launch);
    register_function(L, "instance_kill", l_instance_kill);
    // api for server logger
    register_function(L, "server_log", l_server_log);
    // api for server environment varribles
    register_function(L, "env_set", l_env_set);
    register_function(L, "env_get_value", l_env_get_value);
    register_function(L, "env_get_key", l_env_get_key);
    // api for server message queue
    register_function(L, "server_message", l_server_message);
    // api for server timmer
    register_function(L, "timer_schedule", l_timer_schedule);
    register_function(L, "timer_now", l_timer_now);
    // api for server socket
    register_function(L, "socket_create", l_socket_create);
    register_function(L, "socket_listen", l_socket_listen);
    register_function(L, "socket_connect", l_socket_connect);
    register_function(L, "socket_send", l_socket_send);
    register_function(L, "socket_close", l_socket_close);
    lua_setglobal(L, "xServer");
	char filename[256];
	sprintf(filename, "%s%s", x_env_get_value("lua_path"), parameters);
	if (LUA_OK != luaL_loadfile(L, filename)) {
		x_server_log(id, "lua load file %s error.\n", filename);
		return 0;
	}
	int lua_error = lua_pcall(L, 0, 0, 0);
	switch (lua_error) {
		case LUA_OK:
			break;
		case LUA_ERRRUN:
			x_server_log(id, "lua do [%s] error : %s", filename, lua_tostring(L, -1));
			return 0;
		case LUA_ERRMEM:
			x_server_log(id, "lua memory error : %s", filename);
			return 0;
		case LUA_ERRERR:
			x_server_log(id, "lua message error : %s", filename);
			return 0;
		case LUA_ERRGCMM:
			x_server_log(id, "lua gc error : %s", filename);
			return 0;
        default:
            x_server_log(id, "lua unknown error: %d", lua_error);
            return 0;

	};
    x_register_message_callback(id, message_callback);
    x_register_timer_callback(id, timer_callback);
    x_register_socket_callback(id, socket_callback);
    return 1;
}

X_MODULE_API void xLua_destroy(lua_State *L) {
	lua_close(L);
}