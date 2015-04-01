#ifndef __X_NETWORK_H__
#define __X_NETWORK_H__

#include "xServerInclude.h"

struct x_network;

struct x_network *network_create(int32_t id, int32_t instance_id);

int32_t network_id(struct x_network *network);

int32_t network_listen(struct x_network *network, const char *ip, const uint16_t port, int32_t backlog);

int32_t network_connect(struct x_network *network, const char *ip, const uint16_t port);

void network_send(struct x_network *network, const void *data, const size_t size);

void network_destroy(struct x_network *network);

void network_dispatch(struct x_network *network, int32_t type);

void global_network_init();

struct x_network *global_network_create(int32_t instance_id);

struct x_network *global_network_find(int32_t network_id);

int32_t global_network_add_read(struct x_network *network);

void global_network_remove_read(struct x_network *network);

void global_network_add_write(struct x_network *network);

void global_network_remove_write(struct x_network *network);

void global_network_destroy(struct x_network *network);

int32_t global_network_dispatch();

#endif