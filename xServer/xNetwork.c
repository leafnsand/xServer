#include "xServerInclude.h"

#define MAX_SOCKET_COUNT 0x10000
#define SOCKET_INDEX_MASK (MAX_SOCKET_COUNT - 1)

enum x_network_type {
	network_type_invalid,
	network_type_listen,
	network_type_established,
};

struct x_network {
	int32_t id;
	int32_t instance_id;
	enum x_network_type type;
	struct x_socket *socket;
};

struct x_network_cache {
	int32_t lock;
	int32_t next_index;
	x_event_fd event_fd;
	struct x_event *event_cache;
	struct x_network **network_pool;
};

struct x_network *network_create(int32_t id, int32_t instance_id) {
	struct x_network *network = x_malloc(sizeof(struct x_network));
	network->id = id;
	network->instance_id = instance_id;
	network->socket = NULL;
	network->type = network_type_invalid;
	return network;
}

int32_t network_id(struct x_network *network) {
	return network->id;
}

int32_t network_listen(struct x_network *network, const char *ip, const uint16_t port, int32_t backlog) {
	network->socket = socket_create();
    socket_open(network->socket);
	socket_listen(network->socket, ip, port, backlog);
	if (network->socket->status != socket_status_listening) {
		x_log("network listen error.");
		global_network_destroy(network);
		return 0;
	}
#ifndef PLATFORM_WIN
	if (global_network_add_read(network) != 1) {
		x_log("network listen add event error.");
		global_network_destroy(network);
		return 0;
	}
#endif
	network->type = network_type_listen;
	return 1;
}

int32_t network_connect(struct x_network *network, const char *ip, const uint16_t port) {
	network->socket = socket_create();
    socket_open(network->socket);
	socket_connect(network->socket, ip, port);
	if (network->socket->status != socket_status_connected) {
		x_log("network connect error.");
		global_network_destroy(network);
		return 0;
	}
	if (global_network_add_read(network) != 1) {
		x_log("network connect add event error.");
		global_network_destroy(network);
		return 0;
	}
	network->type = network_type_established;
	return 1;
}

void network_send(struct x_network *network, const void *data, const size_t size) {
	if (network->type == network_type_established) {
		if (socket_send(network->socket, data, size) != 1) {
			global_network_add_write(network);
		} else {
			global_network_remove_write(network);
		}
		if (network->socket->status == socket_status_closed) {
			x_server_internal_message(network->id, message_type_socket, network->instance_id, NULL, 0);
			global_network_destroy(network);
		}
	}
}

void network_destroy(struct x_network *network) {
	socket_destroy(network->socket);
	network->type = network_type_invalid;
	x_free(network);
}

void network_dispatch(struct x_network *network, int32_t type) {
	if (type & x_event_type_read) {
		if (network->type == network_type_listen) {
			for (;;) {
				struct x_socket *s = socket_accept(network->socket);
				if (s == NULL) {
					break;
				}
				struct x_network *nn = global_network_create(network->instance_id);
				if (nn == NULL) {
					x_log("network accept error, too many connections.");
					continue;
				}
				nn->socket = s;
				if (global_network_add_read(nn) == 0) {
					x_log("network accept error, can not add event.");
					global_network_destroy(nn);
					continue;
				}
				nn->type = network_type_established;
				x_server_internal_message(network->id, message_type_socket, nn->instance_id, NULL, 0);
			}
		} else {
			struct x_socket_receive_data receive_data;
			while (socket_receive(network->socket, &receive_data) == 1) {
				x_server_internal_message(network->id, message_type_socket, network->instance_id, receive_data.buffer, receive_data.size);
			}
			if (network->socket->status == socket_status_closed) {
				x_server_internal_message(network->id, message_type_socket, network->instance_id, NULL, 0);
				global_network_destroy(network);
			}
		}
	}
	if (type & x_event_type_write) {
		if (socket_send(network->socket, NULL, 0) != 1) {
			global_network_add_write(network);
		} else {
			global_network_remove_write(network);
		}
		if (network->socket->status == socket_status_closed) {
			x_server_internal_message(network->id, message_type_socket, network->instance_id, NULL, 0);
			global_network_destroy(network);
		}
	}
}

static struct x_network_cache *N = NULL;

void global_network_init() {
	if (N == NULL) {
		global_socket_init();
		N = x_malloc(sizeof(struct x_network_cache));
		N->lock = 0;
		N->next_index = 0;
		N->event_fd = event_create(MAX_SOCKET_COUNT);
		N->event_cache = x_malloc(MAX_SOCKET_COUNT * sizeof(struct x_event));
		N->network_pool = x_malloc(MAX_SOCKET_COUNT * sizeof(struct x_network));
		memset(N->network_pool, 0, MAX_SOCKET_COUNT * sizeof(struct x_network));
	}
}

struct x_network *global_network_create(int32_t instance_id) {
	spin_lock_lock(&N->lock);
	struct x_network *network = NULL;
	for (int32_t i = 0; i < MAX_SOCKET_COUNT; i++) {
		int32_t index = (N->next_index + i) & SOCKET_INDEX_MASK;
		if (N->network_pool[index] == NULL) {
			int32_t id = N->next_index + i;
			network = network_create(id, instance_id);
			N->network_pool[index] = network;
			N->next_index = id + 1;
			break;
		}
	}
	if (network == NULL) {
		x_log("too many network instances.");
	}
	spin_lock_unlock(&N->lock);
	return network;
}

struct x_network *global_network_find(int32_t network_id) {
	int32_t index = network_id & SOCKET_INDEX_MASK;
	return N->network_pool[index];
}

int32_t global_network_add_read(struct x_network *network) {
	return event_add_read(N->event_fd, network->socket->fd, network);
}

void global_network_remove_read(struct x_network *network) {
	event_remove_read(N->event_fd, network->socket->fd, network);
}

void global_network_add_write(struct x_network *network) {
	event_add_write(N->event_fd, network->socket->fd, network);
}

void global_network_remove_write(struct x_network *network) {
	event_remove_write(N->event_fd, network->socket->fd, network);
}

void global_network_destroy(struct x_network *network) {
	spin_lock_lock(&N->lock);
	N->network_pool[network->id & SOCKET_INDEX_MASK] = NULL;
	global_network_remove_read(network);
	global_network_remove_write(network);
	network_destroy(network);
	spin_lock_unlock(&N->lock);
}

int32_t global_network_dispatch() {
#if defined(PLATFORM_WIN)
	fd_set fds_read;
	fd_set fds_write;
	FD_ZERO(&fds_read);
	FD_ZERO(&fds_write);
	struct timeval time = {0, 0};
	for (int32_t i = 0; i < MAX_SOCKET_COUNT; i++) {
		struct x_network *network = N->network_pool[(N->next_index + i) & SOCKET_INDEX_MASK];
		if (network) {
			FD_SET(network->socket->fd, &fds_read);
			if (network->type == network_type_established) {
				FD_SET(network->socket->fd, &fds_write);
			}
		}
	}
	int32_t count = select(MAX_SOCKET_COUNT, &fds_read, &fds_write, NULL, &time);
	if (count <= 0) {
		return 0;
	}
	for (int32_t i = 0; i < MAX_SOCKET_COUNT; i++) {
		struct x_network *network = N->network_pool[(N->next_index + i) & SOCKET_INDEX_MASK];
		if (network) {
			if (FD_ISSET(network->socket->fd, &fds_read)) {
				network_dispatch(network, x_event_type_read);
			}
			if (FD_ISSET(network->socket->fd, &fds_write)) {
				network_dispatch(network, x_event_type_write);
			}
		}
	}
	return 1;
#elif defined(PLATFORM_OSX) || defined(PLATFORM_LINUX)
	int32_t count = event_dispatch(N->event_fd, N->event_cache);
	if (count > 0) {
		for (int32_t i = 0; i < count; i++) {
			network_dispatch(N->event_cache[i].data, N->event_cache[i].type);
		}
		return 1;
	}
	return 0;
#endif
}