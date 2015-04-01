#include "xServerInclude.h"

#define DEFAULT_DATA_MAX_COUNT 128

struct x_environment_node {
	const char *key;
	const char *value;
};

struct x_environment {
	int32_t lock;
	int32_t count;
	int32_t max;
	struct x_environment_node *data;
};

static struct x_environment *D = NULL;

static int32_t environment_find_key(struct x_environment *data, const char *key) {
	for (int32_t i = 0; i < data->count; i++) {
		if (strcmp(key, data->data[i].key) == 0) {
			return i;
		}
	}
	return -1;
}

static int32_t environment_find_value(struct x_environment *data, const char *value) {
    for (int32_t i = 0; i < data->count; i++) {
        if (strcmp(value, data->data[i].value) == 0) {
            return i;
        }
    }
    return -1;
}

static const char *environment_get_key(struct x_environment *data, const char *value) {
    spin_lock_lock(&data->lock);
    const char *key = NULL;
    int32_t index = environment_find_value(data, value);
    if (index > -1) {
        key = data->data[index].key;
    }
    spin_lock_unlock(&data->lock);
    return key;
}

static const char *environment_get_value(struct x_environment *data, const char *key) {
	spin_lock_lock(&data->lock);
	const char *value = NULL;
	int32_t index = environment_find_key(data, key);
	if (index > -1) {
		value = data->data[index].value;
	}
	spin_lock_unlock(&data->lock);
	return value;
}

static void environment_set(struct x_environment *data, const char *key, const char *value) {
	spin_lock_lock(&data->lock);
	int32_t key_index = environment_find_key(data, key);
    int32_t value_index = environment_find_value(data, value);
    if (key_index > -1 && value_index > -1) {
        x_log("server environment, both key(%s) value(%s) exist.", key, value);
    } else if (key_index > -1) {
        x_free((void *)data->data[key_index].value);
        data->data[key_index].value = x_strdup(value);
    } else if (value_index > -1) {
        x_free((void *)data->data[value_index].key);
        data->data[value_index].key = x_strdup(key);
    } else {
		if (data->count == data->max) {
			struct x_environment_node *new_data = x_malloc(2 * data->max * sizeof(struct x_environment_node));
			memcpy(new_data, data->data, data->max);
			x_free(data->data);
			data->data = new_data;
			data->max *= 2;
		}
		data->data[data->count].key = x_strdup(key);
		data->data[data->count].value = x_strdup(value);
		data->count++;
    }
	spin_lock_unlock(&data->lock);
}

void global_environment_init() {
	if (D == NULL) {
		D = x_malloc(sizeof(struct x_environment));
		D->lock = 0;
		D->count = 0;
		D->max = DEFAULT_DATA_MAX_COUNT;
		D->data = x_malloc(DEFAULT_DATA_MAX_COUNT * sizeof(struct x_environment_node));
	}
}

const char *global_environment_get_key(const char *value) {
    if (D == NULL) global_environment_init();
    return environment_get_key(D, value);
}

const char *global_environment_get_value(const char *key) {
	if (D == NULL) global_environment_init();
	return environment_get_value(D, key);
}

void global_environment_set(const char *key, const char *value) {
	if (D == NULL) global_environment_init();
	environment_set(D, key, value);
}