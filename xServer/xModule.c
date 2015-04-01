#include "xServerInclude.h"

#if defined(PLATFORM_WIN)

#define MODULE_EXTENSION ".dll"

#elif defined(PLATFORM_LINUX)

#define MODULE_EXTENSION ".so"

#elif defined(PLATFORM_OSX)

#define MODULE_EXTENSION ".dylib"

#endif

#define MAX_MODULE_COUNT 64

typedef void *(*module_api_create)();

typedef int32_t(*module_api_init)(void *context, const int32_t id, const char *parameters);

typedef void(*module_api_destroy)(void *context);

struct x_module {
    const char *name;
    module_api_create create;
    module_api_init init;
    module_api_destroy destroy;
};

struct x_modules {
    int32_t count;
	int32_t lock;
    const char *path;
    struct x_module modules[MAX_MODULE_COUNT];
};

static struct x_modules *M = NULL;

static struct x_module *global_module_find(const char *name) {
    for (int i = 0; i < M->count; i++) {
        if (strcmp(M->modules[i].name, name) == 0) {
            return &M->modules[i];
        }
    }
    return NULL;
}

static void *global_module_open(const char *path) {
#if defined(PLATFORM_WIN)
	return LoadLibrary(path);
#elif defined(PLATFORM_OSX) || defined(PLATFORM_LINUX)
    return dlopen(path, RTLD_NOW | RTLD_GLOBAL);
#endif
}

static void global_module_close(void *module) {
#if defined(PLATFORM_WIN)
	FreeLibrary(module);
#elif defined(PLATFORM_OSX) || defined(PLATFORM_LINUX)
	dlclose(module);
#endif
}

static void *global_module_get_api(void *module, const char *name) {
#if defined(PLATFORM_WIN)
	return GetProcAddress(module, name);
#elif defined(PLATFORM_OSX) || defined(PLATFORM_LINUX)
    return dlsym(module, name);
#endif
}

void global_module_init(const char *path) {
	if (M == NULL) {
		M = x_malloc(sizeof(struct x_modules));
		M->count = 0;
		M->lock = 0;
		M->path = x_strdup(path);
	}
}

struct x_module *global_module_load(const char *name) {
    struct x_module *module = global_module_find(name);
    if (module) {
        return module;
    }
	spin_lock_lock(&M->lock);
	module = global_module_find(name);
    if (module == NULL && M->count < MAX_MODULE_COUNT) {
        module = &M->modules[M->count];
		module->name = x_strdup(name);
        size_t path_len = strlen(M->path);
        size_t name_len = strlen(name);
        char *full_path = x_malloc(path_len + name_len + sizeof(MODULE_EXTENSION));
        strcpy(full_path, M->path);
        strcpy(full_path + path_len, name);
        strcpy(full_path + path_len + name_len, MODULE_EXTENSION);
        void *module_instance = global_module_open(full_path);
		x_free(full_path);
        if (module_instance) {
            char *api_name = x_malloc(name_len + 10);
            strcpy(api_name, name);
            strcpy(api_name + name_len, "_create");
            module->create = global_module_get_api(module_instance, api_name);
			strcpy(api_name + name_len, "_init");
            module->init = global_module_get_api(module_instance, api_name);
			strcpy(api_name + name_len, "_destroy");
            module->destroy = global_module_get_api(module_instance, api_name);
			x_free(api_name);
            if (module->create && module->init && module->destroy) {
                M->count++;
			} else {
				module = NULL;
				global_module_close(module_instance);
				x_log("open module %s api error.", name);
			}
		} else {
			module = NULL;
			x_log("open module %s error.", name);
		}
	} else {
		module = NULL;
		x_log("too many modules opend.");
	}
	spin_lock_unlock(&M->lock);
    return module;
}

void *module_create(struct x_module *module) {
    return module->create();
}

int32_t module_init(struct x_module *module, void *context, const int32_t id, const char *parameter) {
    return module->init(context, id, parameter);
}

void module_destroy(struct x_module *module, void *context) {
	module->destroy(context);
}