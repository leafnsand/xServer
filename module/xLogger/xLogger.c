#include "xServer.h"
#include "xMalloc.h"
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

struct xLogger_context {
	int32_t id;
	char *path;
};

void message_callback(void *context, const int32_t from, const void *message, const size_t size) {
    printf("%s", (char *)message);
    x_free((void *)message);
}

X_MODULE_API struct xLogger_context *xLogger_create() {
	struct xLogger_context *context = x_malloc(sizeof(struct xLogger_context));
	context->id = INVALID_INSTANCE;
	context->path = NULL;
	return context;
}

X_MODULE_API int32_t xLogger_init(struct xLogger_context *context, const int32_t id, const char *parameters) {
	context->id = id;
	context->path = x_strdup(x_env_get_value("log_path"));
    x_register_message_callback(id, message_callback);
	return 1;
}

X_MODULE_API void xLogger_destroy(struct xLogger_context *context) {
    x_free(context);
}