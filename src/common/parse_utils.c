#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include "utils.h"
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "str_utils.h"
#include "parse_utils.h"

#ifdef _WIN64
    #include <process.h>
    #define execv _execv
#elif defined(__linux__)
    #include <sys/wait.h>
#endif

char *last_argument = NULL;
int8_t last_exit_code;

static void free_args(char **args, uint16_t count) {
    if (!args) return;

    for (uint16_t i = 0; i < count; i++)
        SAFE_FREE(args[i]);

    SAFE_FREE(args);
}

char **parseData(const char *str, uint16_t *count) {
    if (!str || !count) return NULL;

    char **list = alloc((MAX_ARGS + 1) * sizeof(char *));
    if (!list) return NULL;

    *count = 0;
    size_t i = 0;

    while (str[i]) {
        while (str[i] && isspace((unsigned char)str[i]))
            i++;

        if (!str[i]) break;

        if (*count >= MAX_ARGS)
            break;

        size_t start;
        size_t len;

        if (str[i] == '"') {
            start = ++i;

            while (str[i] && str[i] != '"')
                i++;

            if (str[i] != '"') {
                free_args(list, *count);
                *count = 0;
                return NULL;
            }

            len = i - start;
            i++;
        } else {
            start = i;

            while (str[i] && !isspace((unsigned char)str[i]))
                i++;

            len = i - start;
        }

        list[*count] = alloc(len + 1);

        if (!list[*count]) {
            free_args(list, *count);
            *count = 0;
            return NULL;
        }

        memcpy(list[*count], str + start, len);
        list[*count][len] = '\0';

        (*count)++;
    }

    list[*count] = NULL;
    return list;
}

char **extract_args(char *args, uint16_t *argc, char *firstArg) {
    if (!argc) return NULL;

    *argc = 0;

    char **argv = alloc((MAX_ARGS + 1) * sizeof(char *));
    if (!argv) return NULL;

    if (firstArg) {
        argv[*argc] = strdup(firstArg);

        if (!argv[*argc]) {
            SAFE_FREE(argv);
            return NULL;
        }

        (*argc)++;
    }

    if (args) {
        uint16_t count = 0;
        char **list = parseData(args, &count);

        if (!list) {
            free_args(argv, *argc);
            *argc = 0;
            return NULL;
        }

        for (uint16_t i = 0; i < count && *argc < MAX_ARGS; i++) {
            argv[*argc] = strdup(list[i]);

            if (!argv[*argc]) {
                free_args(list, count);
                free_args(argv, *argc);
                *argc = 0;
                return NULL;
            }

            (*argc)++;
        }

        free_args(list, count);
    }

    argv[*argc] = NULL;
    return argv;
}

int16_t processCommand(
    char **argv,
    uint32_t argc,
    const char **cmds,
    char **address,
    char *history_path,
    char *data_folder,
    bool isInsideBash,
    bool isFromAlias
) {
    (void)cmds;
    (void)address;
    (void)history_path;
    (void)data_folder;
    (void)isInsideBash;
    (void)isFromAlias;

    if (!argv || argc == 0 || !argv[0]) {
        last_exit_code = 0;
        return last_exit_code;
    }

    for (uint32_t i = 0; i < argc; i++) {
        if (strcmp(argv[i], "&&") != 0)
            continue;

        if (i == 0 ||
            i + 1 >= argc ||
            !argv[i + 1] ||
            strcmp(argv[i + 1], "&&") == 0) {
            fprintf(stderr,
                    "bash: syntax error near unexpected token '&&'\n");

            last_exit_code = 2;
            return last_exit_code;
        }

        char *separator = argv[i];
        argv[i] = NULL;

        int16_t status = processCommand(
            argv, i,
            cmds, address,
            history_path, data_folder,
            isInsideBash, isFromAlias
        );

        argv[i] = separator;

        if (status != 0) {
            last_exit_code = status;
            return last_exit_code;
        }

        last_exit_code = processCommand(
            &argv[i + 1], argc - i - 1,
            cmds, address,
            history_path, data_folder,
            isInsideBash, isFromAlias
        );

        return last_exit_code;
    }

#ifdef _WIN64
    fprintf(stderr,
            "bash: process execution not implemented for Windows\n");

    last_exit_code = 1;
    return last_exit_code;
#else
    const char *command_dir = "./src/commands/";

    char **expanded_args = alloc(
        ((size_t)argc + 1) * sizeof(*expanded_args)
    );

    if (!expanded_args) {
        perror("bash: alloc");
        last_exit_code = 1;
        return last_exit_code;
    }

    memset(expanded_args, 0,
           ((size_t)argc + 1) * sizeof(*expanded_args));

    for (uint32_t i = 0; i < argc; i++) {
        if (strchr(argv[i], '$')) {
            expanded_args[i] = stringToVariable(argv[i]);
        } else {
            expanded_args[i] = strdup(argv[i]);
        }

        if (!expanded_args[i]) {
            fprintf(stderr,
                    "bash: failed to expand argument %u\n", i);

            free_args(expanded_args, i);
            last_exit_code = 1;
            return last_exit_code;
        }
    }

    expanded_args[argc] = NULL;

    size_t path_len =
        strlen(command_dir) + strlen(expanded_args[0]) + 1;

    char *executable_path = alloc(path_len);

    if (!executable_path) {
        perror("bash: alloc");
        free_args(expanded_args, argc);
        last_exit_code = 1;
        return last_exit_code;
    }

    snprintf(executable_path, path_len, "%s%s",
            command_dir, expanded_args[0]);

    char **call_args = alloc(
        ((size_t)argc + 1) * sizeof(*call_args)
    );

    if (!call_args) {
        perror("bash: alloc");
        SAFE_FREE(executable_path);
        free_args(expanded_args, argc);
        last_exit_code = 1;
        return last_exit_code;
    }

    memset(call_args, 0,
           ((size_t)argc + 1) * sizeof(*call_args));

    call_args[0] = executable_path;

    for (uint32_t i = 1; i < argc; i++) {
        call_args[i] = expanded_args[i];
        expanded_args[i] = NULL;
    }

    SAFE_FREE(expanded_args[0]);
    SAFE_FREE(expanded_args);

    call_args[argc] = NULL;

    if (argc > 1) {
        char *new_last_argument = strdup(call_args[argc - 1]);

        if (!new_last_argument) {
            perror("bash: strdup");
            free_args(call_args, argc);
            last_exit_code = 1;
            return last_exit_code;
        }

        SAFE_FREE(last_argument);
        last_argument = new_last_argument;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("bash: fork");
        free_args(call_args, argc);
        last_exit_code = 1;
        return last_exit_code;
    }

    if (pid == 0) {
        execv(call_args[0], call_args);

        fprintf(stderr,
                "bash: command not found: '%s'\n",
                strrchr(call_args[0], '/')
                    ? strrchr(call_args[0], '/') + 1
                    : call_args[0]);

        _exit(127);
    }

    int status;
    int32_t result = 1;

    while (waitpid(pid, &status, 0) < 0) {
        if (errno == EINTR)
            continue;

        perror("bash: waitpid");
        goto cleanup;
    }

    if (WIFEXITED(status))
        result = WEXITSTATUS(status);
    else if (WIFSIGNALED(status))
        result = 128 + WTERMSIG(status);

cleanup:
    free_args(call_args, argc);

    last_exit_code = (int8_t)result;
    return last_exit_code;
#endif
}