#include <stdlib.h>
#include "libs/utils.h"
#include "libs/types.h"
#include "libs/terminal.h"

#ifdef _WIN64
    #define rmdir _rmdir
    #define chdir _chdir
    #define getcwd _getcwd
    HANDLE hConsole;
#elif !defined(_WIN64) && !defined(__linux__) && !defined(__APPLE__) && !defined(__ANDROID__)
    #error "Operational system not recognized, terminating program!!"
#endif

int32_t main(int32_t argc, char **argv) {
#ifdef _WIN64
    SetUnhandledExceptionFilter(handler);
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    setup_console();
#endif
    initRandom();

    const char *cmds[] = {
        "clear", "exit", "echo", "neofetch", "logs",
        "cmds", "cd", "ls", "man", "whoami", "date", "pwd",
        "mkdir", "rmdir", "cat", "touch", "rm", "history", 
        "uname", "grep", "bc", "drives", "clearhistory", "rand",
        "alias", "rename", "bash", "head", "tail", "lc", "sleep",
        "randstr", "rev", NULL
    };

    char *input = calloc(MAX_CHAR, sizeof(char));
    if (!input) {
        perror("bash: Memory allocation error!!\n");
        return 1;
    }

    char *address = get_default_address();

    char *program_root = calloc(MAX_CHAR, sizeof(char));
    char *data_folder = calloc(MAX_CHAR, sizeof(char));
    char *history_path = calloc(0x4B0, sizeof(char));

    GetProjDir(program_root, MAX_CHAR, data_folder, MAX_CHAR, history_path, 0x4B0);

    if (argc > 1) {
        checkLswrcSyntax(data_folder);
        SAFE_FREE(input);

        for (int32_t i = 1; i < argc; i++) {
            if (*argv[i] == '#')
                break;

            if (parenthesis_check(argv[i]) == PAREN_UNCLOSED_QUOTE) {
                printf("LSW: unclosed quote\n");
                continue;
            }

            if (*argv[i] == '-')
                bashCmd(2, (char *[]){"bash", argv[i]}, cmds, false);
            else {
                saveHist(argv[i], history_path, data_folder);
                processCommand(argv[i], cmds, &address, history_path, data_folder, false, false);
            }
        }

        SAFE_FREE(data_folder);
        SAFE_FREE(program_root);
        SAFE_FREE(history_path);
        SAFE_FREE(address);
        return 0;
    }

    char *userName;
    userName = get_user();

    char *hostName;
    hostName = get_hostname();

    puts("Type 'cmds' to see the command list");

    while (true) {
        checkLswrcSyntax(data_folder);
        char *wd = defaultAddressReplace(address);

        printc("❯ lsw ❮ ", CYAN, WHITE);
        printc("%s@%s", LIGHT_GREEN, WHITE, userName, hostName);
        putchar(':');
        printc("%s", LIGHT_BLUE, WHITE, wd);
        printf("$ ");

        SAFE_FREE(wd);

        if (!fgets(input, MAX_CHAR, stdin))
            break;

        if (!strchr(input, '\n')) {  
            int16_t c;
            while ((c = getchar()) != '\n' && c != EOF);
        }

        removeComments(input);

        input[strcspn(input, "\n")] = '\0';
        trim(input);
        trimEnd(input);

        if (parenthesis_check(input) == PAREN_UNCLOSED_QUOTE) {
            printf("bash: unclosed quote\n");
            continue;
        }

        if (!*input)
            continue;

        saveHist(input, history_path, data_folder);

        processCommand(input, cmds, &address, history_path, data_folder, true, false);
    }

    SAFE_FREE(data_folder);
    SAFE_FREE(program_root);
    SAFE_FREE(history_path);
    SAFE_FREE(input);
    SAFE_FREE(address);
    return 0;
}
