#ifndef __X_SERVER_H__
#define __X_SERVER_H__

#include "xPlatform.h"
#include <stddef.h>
#include <stdint.h>

#define INVALID_INSTANCE -1

// api for server instance management

X_SERVER_API int32_t x_instance_launch(const char *module_name, const char *parameter);

X_SERVER_API void x_instance_kill(int32_t instance_id);

// api for server logger

X_SERVER_API void x_server_log(int32_t instance_id, const char *format, ...);

// api for server environment varribles

X_SERVER_API void x_env_set(const char *key, const char *value);

X_SERVER_API const char *x_env_get_value(const char *key);

X_SERVER_API const char *x_env_get_key(const char *value);

// api for server message queue

X_SERVER_API void x_server_message(int32_t instance_id, int32_t to, void *data, size_t size);

typedef void(*x_message_callback)(void *context, const int32_t from, const void *message, const size_t size);

X_SERVER_API void x_register_message_callback(int32_t instance_id, x_message_callback callback);

// api for server timmer

X_SERVER_API uint32_t x_timer_schedule(int32_t instance_id, uint32_t tick);

X_SERVER_API uint32_t x_timer_now(void);

typedef void(*x_timer_callback)(void *context, uint32_t now);

X_SERVER_API void x_register_timer_callback(int32_t instance_id, x_timer_callback callback);

// api for server socket

X_SERVER_API int32_t x_socket_create(int32_t instance_id);

X_SERVER_API int32_t x_socket_listen(int32_t instance_id, int32_t socket_id, const char *ip, const uint16_t port, int32_t backlog);

X_SERVER_API int32_t x_socket_connect(int32_t instance_id, int32_t socket_id, const char *ip, const uint16_t port);

X_SERVER_API void x_socket_send(int32_t instance_id, int32_t socket_id, const void *data, const size_t size);

X_SERVER_API void x_socket_close(int32_t instance_id, int32_t socket_id);

typedef void(*x_socket_callback)(void *context, int32_t socket_id, const void *data, const size_t size);

X_SERVER_API void x_register_socket_callback(int32_t instance_id, x_socket_callback callback);

#endif
