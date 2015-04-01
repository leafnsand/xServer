#include "xServerInclude.h"

#define DEFAULT_SERVER_LOG_SIZE 1024

void x_log(const char *format, ...) {
    char *data = x_malloc(DEFAULT_SERVER_LOG_SIZE);
    va_list list;
    va_start(list, format);
    size_t size = vsnprintf(data, DEFAULT_SERVER_LOG_SIZE, format, list);
    va_end(list);
    size_t i = 1;
    while (size >= DEFAULT_SERVER_LOG_SIZE * i) {
        x_free(data);
        data = x_malloc(DEFAULT_SERVER_LOG_SIZE * i);
        va_start(list, format);
        size = vsnprintf(data, 1024 * i, format, list);
        va_end(list);
        i++;
    }
    printf("%s\n", data);
    x_free(data);
}