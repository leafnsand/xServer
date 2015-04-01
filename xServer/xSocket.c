#include "xServerInclude.h"

#if defined(PLATFORM_WIN)

#define X_INVALID_SOCKET INVALID_SOCKET
#define x_setsockopt(a, b, c, d, e) setsockopt(a, b, c, (const char *)d, e)
typedef uint32_t socklen_t;

#elif defined(PLATFORM_OSX)

#define X_INVALID_SOCKET -1
#define x_setsockopt setsockopt

#elif defined(PLATFORM_LINUX)

#define X_INVALID_SOCKET -1
#define x_setsockopt setsockopt

#endif

#define MIN_RECEIVE_BUFFER_SIZE 64

static void socket_append_data(struct x_socket *s, char *buffer, const size_t size, const int32_t offset) {
	struct x_socket_data *data = x_malloc(sizeof(struct x_socket_data));
	data->next = NULL;
	data->buffer = buffer;
	data->unsent_buffer = buffer + offset;
	data->size = size - offset;
	s->tail->next = data;
	s->tail = data;
}

static int32_t socket_get_error() {
#if defined(PLATFORM_WIN)
	return WSAGetLastError();
#elif defined(PLATFORM_LINUX) || defined(PLATFORM_OSX)
	return errno;
#endif
}

static void socket_close_fd(x_socket_fd fd) {
#if defined(PLATFORM_WIN)
	closesocket(fd);
#elif defined(PLATFORM_LINUX) || defined(PLATFORM_OSX)
	close(fd);
#endif
}

static int32_t socket_set_non_blocking(x_socket_fd fd) {
#if defined(PLATFORM_WIN)
	uint32_t mode = 1;
	return ioctlsocket(fd, FIONBIO, &mode);
#elif defined(PLATFORM_LINUX) || defined(PLATFORM_OSX)
	int32_t mode = fcntl(fd, F_GETFL, 0);
	if (-1 == mode) {
		return -1;
	}
	return fcntl(fd, F_SETFL, mode | O_NONBLOCK);
#endif
}

static int32_t socket_eagain() {
	int32_t error = socket_get_error();
#if defined(PLATFORM_WIN)
	return (error == WSAEWOULDBLOCK || error == WSAEINTR) ? 1 : 0;
#elif defined(PLATFORM_LINUX) || defined(PLATFORM_OSX)
	return (error == EAGAIN || error == EINTR) ? 1 : 0;
#endif
}

static int32_t socket_keepalive(x_socket_fd fd) {
	int32_t flag = 1;
	return x_setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &flag, sizeof(flag));
}

static int32_t socket_reuseaddr(x_socket_fd fd) {
	int32_t flag = 1;
	return x_setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
}

static int32_t socket_nodelay(x_socket_fd fd) {
	int32_t flag = 1;
	return x_setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
}

static int32_t socket_set_ling(x_socket_fd fd) {
	struct linger ling = {0, 0};
	return x_setsockopt(fd, SOL_SOCKET, SO_LINGER, &ling, sizeof(ling));
}

struct x_socket *socket_create() {
	struct x_socket *s = x_malloc(sizeof(*s));
	s->fd = X_INVALID_SOCKET;
	s->status = socket_status_closed;
	s->receive_buffer_size = MIN_RECEIVE_BUFFER_SIZE;
	s->head.next = NULL;
	s->head.buffer = NULL;
	s->head.size = 0;
	s->tail = &s->head;
	return s;
}

void socket_open(struct x_socket *s) {
	if (s->status != socket_status_closed) {
		x_log("socket open error. status: %d.", s->status);
	}
	s->fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (X_INVALID_SOCKET == s->fd) {
		x_log("socket open fail. error: %d.", socket_get_error());
	} else {
		if (socket_set_ling(s->fd) == -1) {
			x_log("socket set ling fail. error: %d.", socket_get_error());
			socket_close_fd(s->fd);
		} else {
			x_log("socket(%d) open success.", s->fd);
			s->status = socket_status_opened;
		}
	}
}

void socket_listen(struct x_socket *s, const char *ip, uint16_t const port, int32_t backlog) {
	if (s->status != socket_status_opened) {
		x_log("socket not opend, status: %d.", s->status);
		return;
	}
    uint32_t host = INADDR_ANY;
    if (ip[0]) {
        host = inet_addr(ip);
    }
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = host;
	if (socket_reuseaddr(s->fd) == -1) {
		x_log("socket(%d) listen reuseaddr fail. error: %d.", s->fd, socket_get_error());
		socket_close(s);
		return;
	}
	if (socket_set_non_blocking(s->fd) == -1) {
		x_log("socket(%d) listen set non blocking fail. error: %d.", s->fd, socket_get_error());
		socket_close(s);
		return;
	}
	if (socket_keepalive(s->fd) == -1) {
		x_log("socket(%d) listen keep alive fail. error: %d.", s->fd, socket_get_error());
		socket_close(s);
		return;
	}
	if (socket_nodelay(s->fd) == -1) {
		x_log("socket(%d) listen set no delay fail. error: %d.", s->fd, socket_get_error());
		socket_close(s);
		return;
	}
    if (bind(s->fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
		x_log("socket(%d) bind fail. error: %d.", s->fd, socket_get_error());
		socket_close(s);
        return;
    }
	x_log("socket(%d) bind %s.", s->fd, ip);
    if (listen(s->fd, backlog) == -1) {
		x_log("socket(%d) listen fail. error: %d.", s->fd, socket_get_error());
		socket_close(s);
        return;
    }
	x_log("socket(%d) listening at %d.", s->fd, port);
    s->status = socket_status_listening;
}

struct x_socket *socket_accept(struct x_socket *s) {
	struct x_socket *ns = socket_create();
	struct sockaddr_in address;
	socklen_t len = sizeof(address);
	ns->fd = accept(s->fd, (struct sockaddr *)&address, &len);
	if (ns->fd == X_INVALID_SOCKET) {
		return NULL;
	}
	if (socket_keepalive(ns->fd) == -1) {
		x_log("socket(%d) accept keep alive fail. error: %d.", ns->fd, socket_get_error());
		socket_destroy(ns);
		return NULL;
	}
	if (socket_set_non_blocking(ns->fd) == -1) {
		x_log("socket(%d) accept set non blocking fail. error: %d.", ns->fd, socket_get_error());
		socket_destroy(ns);
		return NULL;
	}
	ns->status = socket_status_connected;
	return ns;
}

void socket_connect(struct x_socket *s, const char *ip, uint16_t const port) {
	if (s->status != socket_status_opened) {
		x_log("socket not opend, status: %d.", s->status);
		return;
	}
	uint32_t host = INADDR_ANY;
	if (ip[0]) {
		host = inet_addr(ip);
	}
	struct sockaddr_in address;
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	address.sin_addr.s_addr = host;
	if (connect(s->fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
		x_log("socket(%d) connect fail. error: %d.", s->fd, socket_get_error());
		socket_close(s);
		return;
	}
	x_log("socket(%d) connected to %s:%d.", s->fd, ip, port);
	s->status = socket_status_connected;
    if (socket_keepalive(s->fd) == -1) {
        x_log("socket(%d) connect keep alive fail. error: %d.", s->fd, socket_get_error());
        socket_close(s);
        return;
    }
    if (socket_set_non_blocking(s->fd) == -1) {
        x_log("socket(%d) connect set non blocking fail. error: %d.", s->fd, socket_get_error());
        socket_close(s);
        return;
    }
}

int32_t socket_send(struct x_socket *s, const void *buffer, const size_t size) {
	if (s->status != socket_status_connected) {
		x_log("socket send error. status: %d.", s->status);
		return 0;
	}
	if (buffer && size > 0) {
		socket_append_data(s, (char *)buffer, size, 0);
	}
	while (s->head.next) {
		struct x_socket_data *data = s->head.next;
		int32_t sent_size = (int32_t)send(s->fd, data->unsent_buffer, data->size, 0);
		if (sent_size < 0) {
			if (socket_eagain() == 1) {
				return 1;
			} else {
				x_log("socket send data fail. error: %d.", socket_get_error());
				socket_close(s);
				return 0;
			}
		}
		if (sent_size == data->size) {
			s->head.next = data->next;
			x_free(data->buffer);
			x_free(data);
		} else {
			data->unsent_buffer = data->unsent_buffer + sent_size;
			data->size -= sent_size;
			return 0;
		}
	}
	s->tail = &s->head;
	return 1;
}

int32_t socket_receive(struct x_socket *s, struct x_socket_receive_data *result) {
	if (s->status != socket_status_connected) {
        x_log("socket receive error. status: %d.", s->status);
        return -1;
    }
    result->size = 0;
	char *buffer = x_malloc(s->receive_buffer_size);
    int32_t receive_size = (int32_t)recv(s->fd, buffer, s->receive_buffer_size, 0);
    if (receive_size < 0) {
        x_free(buffer);
        if (socket_eagain() == 1) {
			return 0;
        } else {
            x_log("socket receive data fail. error: %d.", socket_get_error());
            socket_close(s);
			return -1;
        }
    }
    if (receive_size == 0) {
        x_free(buffer);
        x_log("socket(%d) peer closed.", s->fd);
        socket_close(s);
		return -1;
    }
    if (receive_size == s->receive_buffer_size) {
        s->receive_buffer_size *= 2;
    } else if (receive_size > MIN_RECEIVE_BUFFER_SIZE && receive_size * 2 < (int32_t)s->receive_buffer_size) {
        s->receive_buffer_size /= 2;
    }
    result->buffer = buffer;
    result->size = receive_size;
	return 1;
}

// TODO 在关闭连接的时候如果还有未发送的数据
void socket_close(struct x_socket *s) {
	if (s->fd != X_INVALID_SOCKET) {
		socket_close_fd(s->fd);
		x_log("socket(%d) closed.", s->fd);
	}
    struct x_socket_data *data = s->head.next;
    while (data != NULL)
    {
        struct x_socket_data *tmp = data;
        data = data->next;
        x_free(tmp->buffer);
        x_free(tmp);
    }
	s->head.next = NULL;
	s->tail = &s->head;
	s->fd = X_INVALID_SOCKET;
	s->status = socket_status_closed;
}

void socket_destroy(struct x_socket* s) {
	if (s->status != socket_status_closed) {
		socket_close(s);
	}
	x_free(s);
	x_log("socket destroied.");
}

void global_socket_init() {
#if defined(PLATFORM_WIN)
	WSADATA data;
	WSAStartup(MAKEWORD(2, 2), &data);
#endif
}

void global_socket_destroy() {
#if defined(PLATFORM_WIN)
	WSACleanup();
#endif
}