#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <stdio.h>
#include <stdint.h>
#include "../../includes/common/utils.h"
#include "../../includes/common/str_utils.h"

int32_t main(int32_t argc, char **argv) {

    for (int32_t i = 1; i < argc; i++) {
        char *arg_copy = strdup(argv[i]);
        if (!arg_copy) {
            fprintf(stderr, "echo: out of memory\n");
            return 1;
        }

        if (!injectEscape(arg_copy, "echo")) {
            SAFE_FREE(arg_copy);
            return 1;
        }

        puts(arg_copy);
        SAFE_FREE(arg_copy);
    }

    return 0;
}