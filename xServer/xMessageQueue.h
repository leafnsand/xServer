#ifndef __X_MESSAGE_QUEUE_H__
#define __X_MESSAGE_QUEUE_H__

#include "xServerInclude.h"

enum x_message_type {
    message_type_message,
    message_type_timer,
    message_type_socket,
};

struct x_message {
    enum x_message_type type;
    int32_t from;
    int32_t to;
    char *data;
    size_t size;
};

struct x_message_queue;

struct x_message_queue *message_queue_create();

void message_queue_push(struct x_message_queue *queue, struct x_message *message);

int32_t message_queue_pop(struct x_message_queue *queue, struct x_message **message);

void message_queue_destroy(struct x_message_queue *queue);

void global_message_queue_init();

void global_message_queue_push(struct x_message *message);

int32_t global_message_queue_pop(struct x_message **message);

void global_message_queue_destroy();

#endif