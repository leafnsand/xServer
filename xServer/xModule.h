#ifndef __X_MODULE_H__
#define __X_MODULE_H__

#include "xServerInclude.h"

struct x_module;

void global_module_init(const char *path);

struct x_module *global_module_load(const char *name);

void *module_create(struct x_module *module);

int32_t module_init(struct x_module *module, void *context, const int32_t id, const char *parameter);

void module_destroy(struct x_module *module, void *context);

#endif