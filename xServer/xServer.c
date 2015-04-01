#include "xServerInclude.h"

#define DEFAULT_SERVER_LOG_SIZE 1024

// api for server instance management

X_SERVER_API int32_t x_instance_launch(const char *module_name, const char *parameter) {
    if (module_name == NULL) {
        x_log("no module name.");
        return INVALID_INSTANCE;
    }
    struct x_module *module = global_module_load(module_name);
    if (module == NULL) {
        x_log("module not found.");
        return INVALID_INSTANCE;
    }
    struct x_instance *instance = global_instance_create(module);
    if (instance == NULL) {
        x_log("create instance fail.");
        return INVALID_INSTANCE;
    }
    if (instance_init(instance, parameter) == 0) {
        x_log("init instance fail.");
        instance_destroy(instance);
        return INVALID_INSTANCE;
    }
    return instance->id;
}

X_SERVER_API void x_instance_kill(int32_t instance_id) {
    if (instance_id <= INVALID_INSTANCE) {
        x_log("kill wrong instance id.");
        return;
    }
    struct x_instance *instance = global_instance_find(instance_id);
    if (instance == NULL) {
        x_log("kill instance fail.");
        return;
    }
    instance_destroy(instance);
}

// api for server logger

X_SERVER_API void x_server_log(int32_t instance_id, const char *format, ...) {
    if (format) {
        char *data = x_malloc(DEFAULT_SERVER_LOG_SIZE);
        va_list list;
        va_start(list, format);
        size_t size = vsnprintf(data, DEFAULT_SERVER_LOG_SIZE, format, list);
        va_end(list);
        size_t i = 1;
        while (size >= DEFAULT_SERVER_LOG_SIZE * i) {
            x_free(data);
            data = x_malloc(DEFAULT_SERVER_LOG_SIZE * i);
            va_start(list, format);
            size = vsnprintf(data, 1024 * i, format, list);
            va_end(list);
            i++;
        }
        x_log("[instance log] %s\n", data);
        x_free(data);
    }
}

// api for server environment varribles

X_SERVER_API void x_env_set(const char *key, const char *value) {
    if (key == NULL || value == NULL) {
        x_log("set env error.");
        return;
    }
    global_environment_set(key, value);
}

X_SERVER_API const char *x_env_get_value(const char *key) {
    if (key == NULL) {
        x_log("get env value error.");
        return NULL;
    }
    return global_environment_get_value(key);
}

X_SERVER_API const char *x_env_get_key(const char *value) {
    if (value == NULL) {
        x_log("get env key error.");
        return NULL;
    }
    return global_environment_get_key(value);
}

// api for server message queue

void x_server_internal_message(int32_t instance_id, int32_t type, int32_t to, void *data, size_t size) {
    struct x_message *message = x_malloc(sizeof(struct x_message));
    message->type = type;
    message->from = instance_id;
    message->to = to;
    message->data = data;
    message->size = size;
    global_message_queue_push(message);
}

X_SERVER_API void x_server_message(int32_t instance_id, int32_t to, void *data, size_t size) {
    struct x_message *message = x_malloc(sizeof(struct x_message));
    message->type = message_type_message;
    message->from = instance_id;
    message->to = to;
    message->data = data;
    message->size = size;
    global_message_queue_push(message);
}

X_SERVER_API void x_register_message_callback(int32_t instance_id, x_message_callback callback) {
    struct x_instance *instance = global_instance_find(instance_id);
    if (instance == NULL) {
        x_log("register message callback fail.");
        return;
    }
    instance->message_callback = callback;
}

// api for server timmer

X_SERVER_API uint32_t x_timer_schedule(int32_t instance_id, uint32_t tick) {
    global_timer_register(instance_id, tick);
    return global_timer_now();
}

X_SERVER_API uint32_t x_timer_now() {
    return global_timer_now();
}

X_SERVER_API void x_register_timer_callback(int32_t instance_id, x_timer_callback callback) {
    struct x_instance *instance = global_instance_find(instance_id);
    if (instance == NULL) {
        x_log("register timer callback fail.");
        return;
    }
    instance->timer_callback = callback;
}

// api for server socket

X_SERVER_API int32_t x_socket_create(int32_t instance_id) {
    struct x_network *network = global_network_create(instance_id);
    if (network == NULL) {
        x_log("create socket fail.");
        return -1;
    }
    return network_id(network);
}

X_SERVER_API int32_t x_socket_listen(int32_t instance_id, int32_t socket_id, const char *ip, const uint16_t port, int32_t backlog) {
    struct x_network *network = global_network_find(socket_id);
    if (network == NULL) {
        x_log("socket listen fail.");
        return -1;
    }
    return network_listen(network, ip, port, backlog);
}

X_SERVER_API int32_t x_socket_connect(int32_t instance_id, int32_t socket_id, const char *ip, const uint16_t port) {
    struct x_network *network = global_network_find(socket_id);
    if (network == NULL) {
        x_log("socket connect fail.");
        return -1;
    }
    return network_connect(network, ip, port);
}

X_SERVER_API void x_socket_send(int32_t instance_id, int32_t socket_id, const void *data, const size_t size) {
    struct x_network *network = global_network_find(socket_id);
    if (network == NULL) {
        x_log("socket send fail.");
        return;
    }
    network_send(network, data, size);
}

X_SERVER_API void x_socket_close(int32_t instance_id, int32_t socket_id) {
    struct x_network *network = global_network_find(socket_id);
    if (network == NULL) {
        x_log("socket send fail.");
        return;
    }
    global_network_destroy(network);
}

X_SERVER_API void x_register_socket_callback(int32_t instance_id, x_socket_callback callback) {
    struct x_instance *instance = global_instance_find(instance_id);
    if (instance == NULL) {
        x_log("register socket callback fail.");
        return;
    }
    instance->socket_callback = callback;
}