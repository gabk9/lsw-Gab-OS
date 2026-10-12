#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

uint16_t injectEscape(char *str, const char *error_str);

int main(int argc, char **argv) {

    for (int i = 1; i < argc; i++) {
        if (!injectEscape(argv[i], "echo"))
            return 1;

        if (argv[i][0] == '$') {
            char *arg = stringToVariable(argv[i]);

            puts(arg);

            free(arg);
            continue;
        }

        puts(argv[i]);
    }

    return 0;
}

uint16_t injectEscape(char *str, const char *error_str) {
    bool inQuotes = false;

    for (size_t i = 0; str[i]; i++) {
        if ((str[i] == '"' || str[i] == '\'') && (i == 0 || str[i-1] != '\\')) {
            inQuotes = !inQuotes;
            continue;
        }

        if (!inQuotes)
            continue;

        if (str[i] == '\\' && str[i+1]) {
            char next = str[i+1];
            char replace = 0;

            switch(next) {
                case 'n':  replace = '\n'; break;
                case 't':  replace = '\t'; break;
                case 'b':  replace = '\b'; break;
                case 'r':  replace = '\r'; break;
                case 'a':  replace = '\a'; break;
                case '\'': replace = '\''; break;
                case '"':  replace = '"';  break;
                case '?':  replace = '?';  break;
                case '\\': replace = '\\'; break;
                case 'f':  replace = '\f'; break;
                case 'v':  replace = '\v'; break;
                case '0':  replace = '\0'; break;
                default: {
                    color4 color1 = (strcasecmp(error_str, "eval") == 0) ? BC_PROMPT_COLOR : WHITE;
                    color4 color2 = (color1 == BC_PROMPT_COLOR) ? GET_BASE_COLOR(color1) : WHITE;

                    printc("%s", color1, WHITE, error_str);
                    printf(": ");
                    printc("invalid escape character: '\\%c'\n", color2, WHITE, next);

                    return 0;
                }
            }

            str[i] = replace;
            shiftLeft_at(str, i+1);
        }
    }
    return 1;
}