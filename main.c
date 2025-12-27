#include "libs/utils.h"
#include "libs/CheckCmd.h"
#include "libs/terminal.h" 

#ifdef _WIN32
    #define rmdir _rmdir
    #define chdir _chdir
    #define getcwd _getcwd
    HANDLE hConsole;
#elif !defined(_WIN32) && !defined(__linux__) && !defined(__APPLE__)
    #error "Operational system not recognized, terminating program!!"
#endif

int main(int argc, char **argv) {
#ifdef _WIN32
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    setup_console();
#endif
    initRandom();

    const char *cmds[] = {
        "clear", "exit", "echo", "neofetch", "updatehistory",
        "cmds", "cd", "ls", "man", "whoami", "date", "pwd",
        "mkdir", "rmdir", "cat", "touch", "rm", "history", 
        "uname", "grep", "bc", "drives", "clearhistory", "rand",
        "alias", "rename", "bash", "head", "tail", "lc", "yes",
        "sleep", "randstr", "rev"
    };

    uint16_t cmdCount = sizeof(cmds) / sizeof(cmds[0]);

    char *input = calloc(MAX_CHAR, sizeof(char));
    if (!input) {
        perror("Error: Memory allocation error!!\n");
        return 1;
    }

    char *address = get_default_address();

    char *program_root = calloc(0x0400, sizeof(char));
    char *data_folder = calloc(0x0400, sizeof(char));
    char *history_path = calloc(0x04B0, sizeof(char));

    GetProjDir(program_root, 0x0400, data_folder, 0x0400, history_path, 0x04B0);

    if (argc > 1) {
        uint16_t total_len = 0;
        for (uint16_t i = 1; i < argc; i++)
            total_len += strlen(argv[i]) + 1;

        SAFE_FREE(input);
        input = calloc(total_len + 1, sizeof(char));

        for (uint16_t i = 1; i < argc; i++) {
            strcat(input, argv[i]);
            if (i + 1 < argc) strcat(input, " ");
        }
        
        FILE *f = fopen(history_path, "a");
        if (f) {
            fprintf(f, "%s\n", input);
            SAFE_FCLOSE(f);
        }

        char *arguments = strchr(input, ' ');
        if (arguments) {
            while (*arguments == ' ') arguments++;
            if (*arguments == '\0') arguments = NULL;
        }

        removeComments(arguments);
        removeComments(input);
        
        trim(input); trimEnd(input);

        if (arguments) {
            trim(arguments); trimEnd(arguments);
        }
        
        if (input[0] == '-')
            bashCmd(input);
        else
            processCommand(input, arguments, cmds, cmdCount, &address, history_path, data_folder, false);

        SAFE_FREE(data_folder);
        SAFE_FREE(program_root);
        SAFE_FREE(history_path);
        SAFE_FREE(input);
        SAFE_FREE(address);
        return 0;
    }
    
    char *userName;
    userName = get_user();

    char *hostName;
    hostName = get_hostname();

    while (true) {
        char *wd = defaultAddressReplace(address);
    #ifdef _WIN32
        charReplace(wd, '/', '\\');
    #endif
        printc("❯ lsw ❮ ", CYAN, WHITE);
        printc("%s@%s", LIGHT_GREEN, WHITE, userName, hostName);
        putchar(':');
        printc("%s", LIGHT_BLUE, WHITE, wd);
        printf("$ ");

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

        if (!strlen(input))
            continue;

        FILE *f = fopen(history_path, "a");
        if (f) {
            fprintf(f, "%s\n", input);
            SAFE_FCLOSE(f);
            SAFE_FCLOSE(f);
        }

        char *args = strchr(input, ' ');
        if (args) {
            while (*args == ' ') args++;
            if (*args == '\0') args = NULL;
        }

        processCommand(input, args, cmds, cmdCount, &address, history_path, data_folder, true);
    }
    
    SAFE_FREE(data_folder);
    SAFE_FREE(program_root);
    SAFE_FREE(history_path);
    SAFE_FREE(input);
    SAFE_FREE(address);
    return 0;
}