#include "xServerInclude.h"

#define MAX_THREAD 64

static void server_init_instance(char *message) {
	while (*message == ' ' || *message == '\t' || *message == '\r') message++;
	char *temp = message + (int32_t)strlen(message) - 1;
	while (*temp == ' ' || *temp == '\t' || *temp == '\r') *temp-- = 0;
    char *parameter = strchr(message, ' ');
    if (parameter) {
        *parameter++ = 0;
    }
	if (*message != '\0') {
        x_instance_launch(message, parameter);
	}
}

static int32_t server_message_dispatch() {
	struct x_message *message = NULL;
	if (global_message_queue_pop(&message)) {
		struct x_instance *instance = global_instance_find(message->to);
		if (instance) {
			instance_message(instance, message);
            return 1;
		}
	}
	return 0;
}

static
#if defined(PLATFORM_WIN)
unsigned int __stdcall
#elif defined(PLATFORM_OSX) || defined(PLATFORM_LINUX)
void *
#endif
timer_loop(void *p) {
	for (;;) {
		global_timer_update();
#if defined(PLATFORM_WIN)
		Sleep(2);
#elif defined(PLATFORM_OSX) || defined(PLATFORM_LINUX)
		usleep(2500);
#endif
	}
#if defined(PLATFORM_WIN)
	return 0;
#elif defined(PLATFORM_OSX) || defined(PLATFORM_LINUX)
	return NULL;
#endif
}

static
#if defined(PLATFORM_WIN)
unsigned int __stdcall
#elif defined(PLATFORM_OSX) || defined(PLATFORM_LINUX)
void *
#endif
message_loop(void *p) {
	for (;;) {
		if (server_message_dispatch() == 0) {
#if defined(PLATFORM_WIN)
			Sleep(1);
#elif defined(PLATFORM_OSX) || defined(PLATFORM_LINUX)
			usleep(1000);
#endif
		}
	}
#if defined(PLATFORM_WIN)
	return 0;
#elif defined(PLATFORM_OSX) || defined(PLATFORM_LINUX)
	return NULL;
#endif
}

static
#if defined(PLATFORM_WIN)
unsigned int __stdcall
#elif defined(PLATFORM_OSX) || defined(PLATFORM_LINUX)
void *
#endif
network_loop(void *p) {
	for (;;) {
		if (global_network_dispatch() == 0) {
#if defined(PLATFORM_WIN)
			Sleep(1);
#elif defined(PLATFORM_OSX) || defined(PLATFORM_LINUX)
			usleep(1000);
#endif
		}
	}
#if defined(PLATFORM_WIN)
	return 0;
#elif defined(PLATFORM_OSX) || defined(PLATFORM_LINUX)
	return NULL;
#endif
}

static void start_thread(int32_t thread) {
#if defined(PLATFORM_WIN)
	int32_t i = 0;
	HANDLE pid[MAX_THREAD];
	pid[i++] = (HANDLE)_beginthreadex(NULL, 0, timer_loop, NULL, 0, NULL);
	pid[i++] = (HANDLE)_beginthreadex(NULL, 0, network_loop, NULL, 0, NULL);
	for (; i < thread; i++) {
		pid[i] = (HANDLE)_beginthreadex(NULL, 0, message_loop, NULL, 0, NULL);
	}
	WaitForMultipleObjects(thread, pid, TRUE, INFINITE);
#elif defined(PLATFORM_OSX) || defined(PLATFORM_LINUX)
	int32_t i = 0;
	pthread_t pid[MAX_THREAD];
	pthread_create(&pid[i++], NULL, timer_loop, NULL);
	pthread_create(&pid[i++], NULL, network_loop, NULL);
	for (; i < thread; i++) {
		pthread_create(&pid[i], NULL, message_loop, NULL);
	}
	for (i = 0; i < thread; i++) {
		pthread_join(pid[i], NULL);
	}
#endif
}

X_SERVER_API void server_start() {
	int32_t daemon = 0;
	const char *daemon_string = global_environment_get_value("daemon");
	if (daemon_string != NULL) {
		daemon = atoi(daemon_string);
	}
	int32_t thread = 0;
	const char *thread_string = global_environment_get_value("thread");
	if (thread_string == NULL) {
		x_log("thread count not specified in config!");
		return;
	}
	thread = atoi(thread_string);
	if (thread < 3 || thread > MAX_THREAD) {
		x_log("thread count(%d) error, less than 3 or bigger than %d!", thread, MAX_THREAD);
		return;
	}
	const char *module_path = global_environment_get_value("module_path");
	if (module_path == NULL) {
		x_log("module path not specified in config!");
		return;
	}
	
	global_message_queue_init();
	global_module_init(module_path);
	global_instance_init();
	global_timer_init();
	global_network_init();

	const char *messages = global_environment_get_value("messages");
	if (messages != NULL) {
		void *p = x_strdup(messages);
		char *message = p;
		int32_t loop = 1;
		do {
			char *separator = strchr(message, '\n');
			if (separator) {
				*separator = '\0';
			} else {
				loop = 0;
			}
			server_init_instance(message);
			message = ++separator;
		} while (loop);
		x_free(p);
	}
	
	start_thread(thread);
}