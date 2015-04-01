#ifndef __X_ENVIRONMENT_H__
#define __X_ENVIRONMENT_H__

void global_environment_init();

const char *global_environment_get_key(const char *value);

const char *global_environment_get_value(const char *key);

void global_environment_set(const char *key, const char *value);

#endif