#ifndef STR_UTILS_H
#define STR_UTILS_H

#include <stdint.h>
#include <stddef.h>

char *get_env_var(const char *name);
char *stringToVariable(const char *str);
void shiftLeft_at(char *str, size_t pos);
uint16_t injectEscape(char *str, const char *error_str);

#endif
