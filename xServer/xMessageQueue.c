#include "xServerInclude.h"

struct x_message_queue_node {
	struct x_message *message;
	struct x_message_queue_node *next;
};

struct x_message_queue {
	int32_t lock;
	uint32_t size;
	struct x_message_queue_node head;
	struct x_message_queue_node *tail;
};

struct x_message_queue *message_queue_create() {
	struct x_message_queue *queue = x_malloc(sizeof(struct x_message_queue));
	queue->size = queue->lock = 0;
	queue->head.message = NULL;
	queue->head.next = NULL;
	queue->tail = &queue->head;
	return queue;
}

void message_queue_push(struct x_message_queue *queue, struct x_message *message) {
	spin_lock_lock(&queue->lock);
	struct x_message_queue_node *node = x_malloc(sizeof(struct x_message_queue_node));
	node->message = message;
	node->next = NULL;
	queue->tail->next = node;
	queue->tail = node;
	queue->size++;
	spin_lock_unlock(&queue->lock);
}

int32_t message_queue_pop(struct x_message_queue *queue, struct x_message **message) {
	spin_lock_lock(&queue->lock);
	int32_t result = 0;
	if (queue->head.next == NULL) {
		result = 0;
	} else {
		struct x_message_queue_node *node = queue->head.next;
		queue->head.next = node->next;
		if (queue->head.next == NULL) {
			queue->tail = &queue->head;
		}
		queue->size--;
		*message = node->message;
		x_free(node);
		result = 1;
	}
	spin_lock_unlock(&queue->lock);
	return result;
}

void message_queue_destroy(struct x_message_queue *queue) {
	struct x_message *message;
	while (message_queue_pop(queue, &message));
	x_free(queue);
}

static struct x_message_queue *Q = NULL;

void global_message_queue_init() {
	if (Q == NULL) {
		Q = message_queue_create();
	}
}

void global_message_queue_push(struct x_message *message) {
	message_queue_push(Q, message);
}

int32_t global_message_queue_pop(struct x_message **message) {
	return message_queue_pop(Q, message);
}

void global_message_queue_destroy() {
	message_queue_destroy(Q);
}