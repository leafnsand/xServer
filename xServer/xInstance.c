#include "xServerInclude.h"

#define DEFAULT_INSTANCE_COUNT 8

struct x_instance_cache {
	int32_t next_index;
	int32_t pool_size;
	struct x_lock lock;
	struct x_instance **instance_pool;
};

static X_INLINE void dispatch_message(struct x_instance *instance, struct x_message *message) {
    switch (message->type) {
        case message_type_timer:
            if (instance->timer_callback) {
                instance->timer_callback(instance->context, global_timer_now());
            }
            break;
        case message_type_socket:
            if (instance->socket_callback) {
                instance->socket_callback(instance->context, message->from, message->data, message->size);
            }
            break;
        case message_type_message:
        default:
            if (instance->message_callback) {
                instance->message_callback(instance->context, message->from, message->data, message->size);
            }
            break;
    }
    x_free(message);
}

static struct x_instance *instance_create(int32_t id, struct x_module *module) {
	struct x_instance *instance = x_malloc(sizeof(struct x_instance));
	instance->id = id;
	instance->lock = 0;
	instance->ref = 0;
	instance->module = module;
	instance->context = module_create(module);
	instance->message_queue = message_queue_create();
    instance->message_callback = NULL;
    instance->timer_callback = NULL;
    instance->socket_callback = NULL;
	lock_atomic_add(&instance->ref);
	x_log("instance(%d) created.", id);
	return instance;
}

int32_t instance_init(struct x_instance *instance, const char *parameter) {
	spin_lock_lock(&instance->lock);
	int32_t result = module_init(instance->module, instance->context, instance->id, parameter);
	spin_lock_unlock(&instance->lock);
	x_log("instance(%d) init, parameter:%s.", instance->id, parameter);
	return result;
}

void instance_message(struct x_instance *instance, struct x_message *message) {
	lock_atomic_add(&instance->ref);
	if (spin_lock_try_lock(&instance->lock) == 0) {
        dispatch_message(instance, message);
		while (message_queue_pop(instance->message_queue, &message)) {
            dispatch_message(instance, message);
		}
		spin_lock_unlock(&instance->lock);
	} else {
		message_queue_push(instance->message_queue, message);
	}
	instance_destroy(instance);
}

void instance_destroy(struct x_instance *instance) {
	if (lock_atomic_sub(&instance->ref) == 0) {
		x_log("instance(%d) destroy.", instance->id);
		module_destroy(instance->module, instance->context);
		message_queue_destroy(instance->message_queue);
		global_instance_destroy(instance->id);
		x_free(instance);
	}
}

static struct x_instance_cache *I = NULL;

void global_instance_init() {
	if (I == NULL) {
		I = x_malloc(sizeof(struct x_instance_cache));
		I->next_index = 0;
		I->pool_size = DEFAULT_INSTANCE_COUNT;
		lock_init(&I->lock);
		I->instance_pool = x_malloc(I->pool_size * sizeof(struct x_instance *));
		memset(I->instance_pool, 0, I->pool_size * sizeof(struct x_instance *));
	}
}

struct x_instance *global_instance_create(struct x_module *module) {
	struct x_instance *instance = NULL;
    lock_write_lock(&I->lock);
    while (instance == NULL) {
		for (int32_t i = 0; i < I->pool_size; i++) {
			int32_t index = (I->next_index + i) & (I->pool_size - 1);
            if (I->instance_pool[index] == NULL) {
                int32_t id = I->next_index + i;
                instance = instance_create(id, module);
                I->instance_pool[index] = instance;
                I->next_index = id + 1;
                break;
            }
        }
        if (instance == NULL) {
			struct x_instance **new_pool = x_malloc(2 * I->pool_size * sizeof(struct x_instance *));
			memset(new_pool, 0, 2 * I->pool_size * sizeof(struct x_instance *));
			memcpy(new_pool, I->instance_pool, I->pool_size * sizeof(struct x_instance *));
            x_free(I->instance_pool);
			I->instance_pool = new_pool;
			I->pool_size *= 2;
        }
    }
    lock_write_unlock(&I->lock);
	return instance;
}

struct x_instance *global_instance_find(int32_t id) {
	lock_read_lock(&I->lock);
	struct x_instance *instance = NULL;
	instance = I->instance_pool[id & (I->pool_size - 1)];
	lock_read_unlock(&I->lock);
	return instance;
}

void global_instance_destroy(int32_t id) {
	lock_write_lock(&I->lock);
	I->instance_pool[id & (I->pool_size - 1)] = NULL;
	lock_write_unlock(&I->lock);
}