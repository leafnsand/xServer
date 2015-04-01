#ifndef __X_SOCKET__
#define __X_SOCKET__

#include "xServerInclude.h"

#if defined(PLATFORM_WIN)

typedef SOCKET x_socket_fd;

#elif defined(PLATFORM_OSX)

typedef int32_t x_socket_fd;

#elif defined(PLATFORM_LINUX)

typedef int32_t x_socket_fd;

#endif

enum x_socket_status {
    socket_status_closed,
    socket_status_opened,
    socket_status_listening,
	socket_status_accept,
    socket_status_connected,
};

struct x_socket_data {
    struct x_socket_data *next;
    char *buffer;
	char *unsent_buffer;
    size_t size;
};

struct x_socket {
	x_socket_fd fd;
	enum x_socket_status status;
	uint32_t receive_buffer_size;
	struct x_socket_data head;
	struct x_socket_data *tail;
};

struct x_socket_receive_data {
    int32_t size;
    char *buffer;
};

struct x_socket *socket_create();

void socket_open(struct x_socket *s);

void socket_listen(struct x_socket *s, const char *ip, const uint16_t port, int32_t backlog);

struct x_socket *socket_accept(struct x_socket *s);

void socket_connect(struct x_socket *s, const char *ip, const uint16_t port);

int32_t socket_send(struct x_socket *s, const void *buffer, const size_t size);

int32_t socket_receive(struct x_socket *s, struct x_socket_receive_data *result);

void socket_close(struct x_socket *s);

void socket_destroy(struct x_socket *s);

void global_socket_init();

void global_socket_destroy();

#endif