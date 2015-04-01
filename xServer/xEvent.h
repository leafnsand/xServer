#ifndef __X_EVENT_H__
#define __X_EVENT_H__

#include "xServerInclude.h"

#define x_event_invalid_fd -1
#define x_event_type_read 0x0f
#define x_event_type_write 0xf0

typedef int32_t x_event_fd;

struct x_event {
	void *data;
	int32_t type;
};

x_event_fd event_create(int32_t max);

void event_destroy(x_event_fd fd);

int32_t event_add_read(x_event_fd fd, x_socket_fd socket_fd, void *data);

void event_remove_read(x_event_fd fd, x_socket_fd socket_fd, void *data);

void event_add_write(x_event_fd fd, x_socket_fd socket_fd, void *data);

void event_remove_write(x_event_fd fd, x_socket_fd socket_fd, void *data);

int32_t event_dispatch(x_event_fd fd, struct x_event *event);

#endif