#define _GNU_SOURCE

#include "utils.h"
#include <stdio.h>
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

char *stringToVariable(const char *str) {
    if (strcasecmp(str, "$path") == 0) {
        return get_env_var("PATH");
    }
    else if (strcasecmp(str, "$home") == 0) {
        return get_env_var("HOME");
    }
    else if (strcasecmp(str, "$username") == 0 || strcasecmp(str, "$user") == 0) {
        const char *user = get_user();
        return strdup(user);
    }
    else if (strcasecmp(str, "$temp") == 0) {
        const char *tmp = getenv("TMP");
        if (!tmp) tmp = getenv("TEMP");
        if (!tmp) tmp = "/tmp";
        return strdup(tmp);
    }
    else if (strcmp(str, "$?") == 0) {
        int16_t length = snprintf(NULL, 0, "%" PRId16, last_exit_code);

        char *tmp = alloc(length + 1);
        if (!tmp) return NULL;

        snprintf(tmp, length + 1, "%"PRId16, last_exit_code);

        return tmp;
    }

#ifndef _WIN64
    else if (strcasecmp(str, "$shell") == 0) {
        return get_env_var("SHELL");
    }
    else if (strcasecmp(str, "$lang") == 0) {
        return get_env_var("LANG");
    }
    else if (strcasecmp(str, "$pwd") == 0) {
        return get_env_var("PWD");
    }
    else if (strcasecmp(str, "$editor") == 0) {
        return get_env_var("EDITOR");
    }

#else
    else if (strcasecmp(str, "$comspec") == 0) {
        return get_env_var("COMSPEC");
    }
    else if (strcasecmp(str, "$systemroot") == 0) {
        return get_env_var("SystemRoot");
    }
    else if (strcasecmp(str, "$appdata") == 0) {
        return get_env_var("APPDATA");
    }
    else if (strcasecmp(str, "$localappdata") == 0) {
        return get_env_var("LOCALAPPDATA");
    }
    else if (strcasecmp(str, "$programdata") == 0) {
        return get_env_var("PROGRAMDATA");
    }
    else if (strcasecmp(str, "$public") == 0) {
        return get_env_var("PUBLIC");
    }
    else if (strcasecmp(str, "$os") == 0) {
        return get_env_var("OS");
    }
    else if (strcasecmp(str, "$number_of_processors") == 0) {
        return get_env_var("NUMBER_OF_PROCESSORS");
    }
    else if (strcasecmp(str, "$processor_architecture") == 0) {
        return get_env_var("PROCESSOR_ARCHITECTURE");
    }
#endif
    else if (str[0] == '$' && str[1] != '\0' && str[1] != ' ') {
        return strdup("\n");
    }

    return strdup(str);
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
