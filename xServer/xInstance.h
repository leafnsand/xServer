#ifndef __X_INSTANCE_H__
#define __X_INSTANCE_H__

#include "xServerInclude.h"

struct x_instance {
    int32_t id;
    int32_t lock;
    int32_t ref;
    char result[32];
    void *context;
    struct x_module *module;
    struct x_message_queue *message_queue;
    x_message_callback message_callback;
    x_timer_callback timer_callback;
    x_socket_callback socket_callback;
};

int32_t instance_init(struct x_instance *instance, const char *parameter);

void instance_message(struct x_instance *instance, struct x_message *message);

void instance_destroy(struct x_instance *instance);

void global_instance_init();

struct x_instance *global_instance_create(struct x_module *module);

struct x_instance *global_instance_find(int32_t id);

void global_instance_destroy(int32_t id);

#endif