#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <strings.h>
#include "str_utils.h"

int main(int argc, char **argv) {

    for (int i = 1; i < argc; i++) {
        char *arg_copy = strdup(argv[i]);
        if (!arg_copy) {
            fprintf(stderr, "Error: out of memory\n");
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