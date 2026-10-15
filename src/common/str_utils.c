#define _GNU_SOURCE

#include "utils.h"
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <strings.h>
#include "str_utils.h"
#include "parse_utils.h"

char *get_user(void);

#ifdef _WIN64
    #define strcasecmp _stricmp
#endif

static char *getSpecialVariable(const char *name) {

    if (strcasecmp(name, "path") == 0)
        return get_env_var("PATH");

    if (strcasecmp(name, "home") == 0)
        return get_env_var("HOME");

    if (strcasecmp(name, "username") == 0 ||
        strcasecmp(name, "user") == 0) {
        const char *user = get_user();
        return user ? strdup(user) : NULL;
    }

    if (strcasecmp(name, "temp") == 0) {
#ifdef _WIN64
        const char *tmp = getenv("TEMP");
        if (!tmp)
            tmp = getenv("TMP");
#else
        const char *tmp = getenv("TMPDIR");
        if (!tmp)
            tmp = "/tmp";
#endif
        return tmp ? strdup(tmp) : NULL;
    }

    if (strcmp(name, "?") == 0) {
        int32_t length = snprintf(NULL, 0, "%" PRId16, last_exit_code);

        if (length < 0)
            return NULL;

        char *value = alloc((size_t)length + 1);

        snprintf(value, (size_t)length + 1,
                "%"PRId16, last_exit_code);

        return value;
    }

    if (strcmp(name, "_") == 0)
        return last_argument ? strdup(last_argument) : strdup("");

#ifndef _WIN64
    if (strcasecmp(name, "shell") == 0)
        return get_env_var("SHELL");

    if (strcasecmp(name, "lang") == 0)
        return get_env_var("LANG");

    if (strcasecmp(name, "pwd") == 0)
        return get_env_var("PWD");

    if (strcasecmp(name, "editor") == 0)
        return get_env_var("EDITOR");
#else
    if (strcasecmp(name, "comspec") == 0)
        return get_env_var("COMSPEC");

    if (strcasecmp(name, "systemroot") == 0)
        return get_env_var("SystemRoot");

    if (strcasecmp(name, "appdata") == 0)
        return get_env_var("APPDATA");

    if (strcasecmp(name, "localappdata") == 0)
        return get_env_var("LOCALAPPDATA");

    if (strcasecmp(name, "programdata") == 0)
        return get_env_var("PROGRAMDATA");

    if (strcasecmp(name, "public") == 0)
        return get_env_var("PUBLIC");

    if (strcasecmp(name, "os") == 0)
        return get_env_var("OS");

    if (strcasecmp(name, "number_of_processors") == 0)
        return get_env_var("NUMBER_OF_PROCESSORS");

    if (strcasecmp(name, "processor_architecture") == 0)
        return get_env_var("PROCESSOR_ARCHITECTURE");
#endif

    return NULL;
}

char *stringToVariable(char *str){
    if (!str)
        return NULL;

    size_t input_len = strlen(str);

    char *result = alloc(input_len + 1);
    size_t result_len = 0;

    result[0] = '\0';

    for (size_t i = 0; i < input_len;) {
        if (str[i] != '$' || i + 1 >= input_len) {
            size_t new_len = result_len + 1;
            char *tmp = alloc(new_len + 1);

            memcpy(tmp, result, result_len);
            tmp[result_len] = str[i];
            tmp[new_len] = '\0';

            SAFE_FREE(result);
            result = tmp;
            result_len = new_len;

            i++;
            continue;
        }

        size_t name_start = i + 1;
        size_t name_end = name_start;

        if (str[name_start] == '?')
            name_end++;
        else {
            while (name_end < input_len &&
                (isalnum((unsigned char)str[name_end]) ||
                    str[name_end] == '_')) {
                name_end++;
            }
        }

        if (name_end == name_start) {
            size_t new_len = result_len + 1;
            char *tmp = alloc(new_len + 1);

            memcpy(tmp, result, result_len);
            tmp[result_len] = '$';
            tmp[new_len] = '\0';

            SAFE_FREE(result);
            result = tmp;
            result_len = new_len;

            i++;
            continue;
        }

        size_t name_len = name_end - name_start;

        char *name = alloc(name_len + 1);
        memcpy(name, str + name_start, name_len);
        name[name_len] = '\0';

        char *value = getSpecialVariable(name);

        SAFE_FREE(name);

        if (!value) {
            size_t token_len = name_end - i;
            size_t new_len = result_len + token_len;
            char *tmp = alloc(new_len + 1);

            memcpy(tmp, result, result_len);
            memcpy(tmp + result_len, str + i, token_len);
            tmp[new_len] = '\0';

            SAFE_FREE(result);
            result = tmp;
            result_len = new_len;

            i = name_end;
            continue;
        }

        size_t value_len = strlen(value);
        size_t new_len = result_len + value_len;

        char *tmp = alloc(new_len + 1);

        memcpy(tmp, result, result_len);
        memcpy(tmp + result_len, value, value_len);
        tmp[new_len] = '\0';

        SAFE_FREE(result);
        SAFE_FREE(value);

        result = tmp;
        result_len = new_len;
        i = name_end;
    }

    return result;
}

char *get_env_var(const char *name) {
    const char *val = getenv(name);
    if (!val) val = "Unknown";
    return strdup(val);
}

uint16_t injectEscape(char *str, const char *error_str) {
    bool inQuotes = false;
    for (size_t i = 0; str[i]; i++) {
        if (str[i] == '\\' && str[i + 1]) {
            char next = str[i + 1];
            char replace = 0;
            switch(next) {
                case 'n': replace = '\n'; break;
                case 't': replace = '\t'; break;
                case 'b': replace = '\b'; break;
                case 'r': replace = '\r'; break;
                case 'a': replace = '\a'; break;
                case '\'': replace = '\''; break;
                case '"': replace = '"'; break;
                case '?': replace = '?'; break;
                case '\\': replace = '\\'; break;
                case 'f': replace = '\f'; break;
                case 'v': replace = '\v'; break;
                case '0': replace = '\0'; break;
                default: {
                    fprintf(stderr, "%s: invalid escape character: '\\%c'\n", error_str, next);
                    return 0;
                }
            }

            str[i] = replace;
            shiftLeft_at(str, i + 1);

            if (replace == '\0') break;
            continue;
        }

        if ((str[i] == '"' || str[i] == '\'') && (i == 0 || str[i - 1] != '\\')) {
            inQuotes = !inQuotes;
        }
    }
    return 1;
}

void shiftLeft_at(char *str, size_t pos) {
    size_t len = strlen(str);
    if (pos >= len) return;

    memmove(&str[pos], &str[pos + 1], len - pos);
}
