#define _GNU_SOURCE

#include <strings.h>
#include "../../includes/common/str_utils.h"

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