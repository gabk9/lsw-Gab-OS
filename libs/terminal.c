#define _GNU_SOURCE

#include <math.h>
#include "utils.h"
#include <stdlib.h>
#include "s_math.h"
#include <inttypes.h>

#ifndef __APPLE__
    #include <ctype.h>
#endif

#if !defined(_WIN64) && !defined(__linux__) && !defined(__APPLE__) && !defined(__ANDROID__)
    #error "Operational system not recognized, terminating program!!"
#endif

#define VERSION "r2.5.23"

void revCmd(char *instruction) {

    if (!*instruction) {
        char string[MAX_CHAR] = {0};

        uint8_t appear = 1;

        while (true) {
            if (appear) {
                printf("Reading from the input, type 'stop' or 'quit' to exit ");
                puts("and use 'clear' or 'cls' to clear the screen and the scrollback buffer\n");
            }

            appear = 0;

            fgets(string, sizeof(string), stdin);
            string[strcspn(string, "\n")] = '\0';

            if (!*string) {
                putchar('\n');
                continue;
            }

            trim(string);
            trimEnd(string);

            char *copy = revStr(string);
            uint8_t stop  = (isValidBcCommand(string, "exit") ||
                            isValidBcCommand(string, "quit"));

            uint8_t clean = (isValidBcCommand(string, "clear") ||
                            isValidBcCommand(string, "cls"));

            char lastChr = (clean || stop) ? '\0' : '\n';

            printf("%s\n%c", copy, lastChr);

            if (stop) {
                SAFE_FREE(copy);
                break;
            }

            if (clean) {
                sleepF(1);
                cls();
                appear = 1;
            }
        }
    } else {

        int16_t hasRedirect = strrchar(instruction, '>');

        if (hasRedirect != -1) {

            char *save;
            char *sourceFile = strtok_r(instruction, ">", &save);
            char *destFile = strtok_r(NULL, ">", &save);

            if (!sourceFile || !destFile) {
                puts("rev: missing operand\nUse \"man rev\" to check the manual");
                return;
            }

            trim(destFile);
            trimEnd(sourceFile);

            uint8_t isPath = (strchar(destFile, '/') != -1 || strchar(sourceFile, '/') != -1 ||
                            strchar(destFile, '\\') != -1 || strchar(sourceFile, '\\') != -1);

            if (isPath) {
                printf("rev: currently it does not support paths\n");
                return;
            }

            if (!isValidFolderOrFileName(sourceFile) ||
                !isValidFolderOrFileName(destFile)) {
                printf("rev: invalid file name\n");
                return;
            }

            FILE *source = fopen(sourceFile, "r");

            if (!source) {
                printf("rev: could not open '%s'\n", sourceFile);
                return;
            }

            char line[MAX_CHAR];

            FILE *dest = fopen(destFile, "w");
            if (!dest) {
                printf("rev: could not create '%s'\n", destFile);
                SAFE_FCLOSE(source);
                return;
            }

            while (fgets(line, MAX_CHAR, source)) {
                trim(line);
                trimEnd(line);
                line[strcspn(line, "\n")] = '\0';

                char *rev = revStr(line);

                fprintf(dest, "%s\n", rev);
                SAFE_FREE(rev);
            }

            SAFE_FCLOSE(source);
            SAFE_FCLOSE(dest);
        } else {

            uint8_t isPath = (strchar(instruction, '/') != -1 || strchar(instruction, '/') != -1);

            if (isPath) {
                printf("rev: currently it does not support paths\n");
                return;
            }

            if (!isValidFolderOrFileName(instruction)) {
                printf("rev: invalid file name\n");
                return;
            }

            FILE *source = fopen(instruction, "r");

            if (!source) {
                printf("rev: could not open '%s'\n", instruction);
                return;
            }

            char line[MAX_CHAR];

            while (fgets(line, MAX_CHAR, source)) {
                trim(line);
                trimEnd(line);
                line[strcspn(line, "\n")] = '\0';

                char *rev = revStr(line);

                puts(rev);
                SAFE_FREE(rev);
            }
        }

    }
}

char *randstrCmd(char *instruction) {
    float64 len = 1;

    if (*instruction != '\0') {

        if (instruction[0] != '-') {
            printf("randstr: invalid argument: '%s'\n", instruction);
            return NULL;
        }

        if (instruction[1] == '-') {
            if (strncasecmp(instruction, "--len", 5) == 0)
                len = parse_len(instruction + 5);
            else {
                printf("randstr: invalid option: '%s'\n", instruction);
                return NULL;
            }

        } else  {
            char opt = tolower((unsigned char)instruction[1]);
            switch (opt) {
                case 'l': len = parse_len(instruction + 2); break;
                default:
                    printf("randstr: invalid option: '-%c'\n", instruction[1]);
                    return NULL;
            }
        }
    }

    if (isnan(len))
        return NULL;

    if (len <= 0 || len >= 65536) {
        printf("randstr: length must be > 0 and < 65536\n");
        return NULL;
    }

    if ((int64_t)len != len) {
        printf("randstr: must be integer!\n");
        return NULL;
    }

    char *str = malloc(len + 1);
    if (!str) {
        puts("randstr: memory allocation error!!");
        return NULL;
    }

    for (int32_t i = 0; i < len; i++)
        str[i] = randChr();

    str[(uint16_t)len] = '\0';
    return str;
}

void sleepCmd(char *instruction) {

    if (!*instruction) {
        puts("sleep: missing operand\nUse \"man sleep\" to check the manual");
        return;
    }

    uint16_t len = strlen(instruction);
    uint32_t unit = 1;

    char last = tolower((unsigned char)instruction[len-1]);

    if (isalpha(last)) {
        switch (last) {
            case 's':
                instruction[len-1] = '\0';
                break;
            case 'm':
                unit = 60;
                instruction[len-1] = '\0';
                break;
            case 'h':
                unit = 3600;
                instruction[len-1] = '\0';
                break;
            case 'd':
                unit = 216000;
                instruction[len-1] = '\0';
                break;
            default:
                puts("sleep: invalid suffix\nUse \"man sleep\" to check the manual");
                return;
        }
    }

    float64 time;
    char *buff = eval(instruction, true);

    var tmp = h_atof(buff, true);
    time = (tmp.type == BC_BOOL) ? (float64)tmp.data.b : tmp.data.f;

    SAFE_FREE(buff);

    if (isnan(time))
        return;

    time *= unit;

    if (time < 0) {
        puts("sleep: must be greater than 0");
        return;
    }

    sleepF(time);
}

int32_t lcCmd(char *instruction) {
    char buffer[MAX_CHAR];
    strncpy(buffer, instruction, sizeof(buffer));
    buffer[sizeof(buffer)-1] = '\0';

    char *rest = buffer;

    if (buffer[0] == '-') {
        rest = strrchr(buffer, ' ');
    }

    if (!rest || strlen(rest) == 0) {
        puts("lc: missing operand\nUse \"man lc\" to check the manual");
        return U32_NAN;
    }

    uint32_t lines = 0;
    uint16_t fileCount = 0;

    char **files = parseData(rest, &fileCount);

    for (uint16_t i = 0; i < fileCount; i++) {
        FILE *f = fopen(files[i], "r");
        if (!f) {
            perror(files[i]);
            return U32_NAN;   
        }

        while (fgets(buffer, sizeof(buffer), f)) {
            lines++;
        }
        SAFE_FCLOSE(f);
    }

    for (uint16_t i = 0; i < fileCount; i++)
        SAFE_FREE(files[i]);
    SAFE_FREE(files);

    return lines;
}

void bashCmd(uint16_t argc, char **argv, const char **cmds, bool insideBash) {
    if (argc < 2) 
        return;

    uint8_t flags = 0;

    char *shell = insideBash ? "bash" : "LSW";

    for (uint16_t i = 1; i < argc; i ++) {

        if (argv[i][0] != '-') {
            printf("%s: invalid argument: '%s'\n", shell, argv[i]);
            return;
        }

        if (argv[i][1] == '-') {
            if (strcasecmp(argv[i], "--version") == 0)
                flags |= BASH_VERSION;
            else if (strcasecmp(argv[i], "--help") == 0)
                flags |= BASH_HELP;
            else if (strcasecmp(argv[i], "--all") == 0)
                flags |= BASH_ALL;
            else
                printf("%s: invalid option: '%s'\n", shell, argv[i]);
        } else {
            for (uint16_t j = 1; argv[i][j]; j++) {
                char opt = tolower((unsigned char)argv[i][j]);
                switch (opt) {
                    case 'a': flags |= BASH_ALL; break;
                    case 'h': flags |= BASH_HELP; break;
                    case 'v': flags |= BASH_VERSION; break;
                    default:
                        printf("%s: invalid option: '-%c'\n", shell, argv[i][j]);
                        return;
                }
            }
        }
    }

    if (flags & BASH_VERSION)
        puts("LSW - Gab-OS  "VERSION"");

    if (flags & BASH_HELP) {
        printf("LSW - Gab OS, a Linux like shell (Linux Subsystem for Windows)\n\n");
        
        if (!insideBash)
            printf("You can run commands using 'lsw [COMMAND...]', or you can use options with 'lsw [OPTION...]'\n");
        else
            manCmd("bash", cmds, true);

        printf("\n\nQuick manual of LSW:\n"
            "\tUse '&&' to run multiple commands\n\tExample: echo \"Hello World\" && neofetch && sleep 5m && clear\n"
            "\n"
            "\tSupposing you created an alias with spaces in it, to run it you will have to run the command with matching quotes\n\tExample: <prompt> \"command testing\"\n"
        );

        if (!insideBash) {
            printf("\nOptions:\n");
            printf("\t'-v', '--version'   show version information\n"
                "\t'-h', '--help'      display manual\n"
                "\t'-a', '--all'       displays everything\n\n"
            );
            printf("Commands:\n");
            char **copy = NULL;
            static int8_t initialized = 0;
    
            if (!initialized) {
                
                uint16_t cmdCount = 0;
                for (uint16_t i = 0; cmds[i]; i++)
                    cmdCount++;

                copy = malloc(cmdCount * sizeof(char *));

                for (uint16_t i = 0; i < cmdCount; i++)
                    copy[i] = strdup(cmds[i]);
    
                bsort(copy, cmdCount);
                initialized = 1;
            }
    
            for (uint16_t i = 0; cmds[i]; i++)
                printf("\t%s\n", copy[i]);
    
            for (uint16_t i = 0; cmds[i]; i++)
                SAFE_FREE(copy[i]);
            SAFE_FREE(copy);
        }
    }
}

void renameCmd(char *instruction) {
    if (!*instruction) {
        puts("rename: missing operand\nUse \"man rename\" to check the manual");
        return;
    }

    trim(instruction);
    trimEnd(instruction);
    
    char *oldName = extractPath(&instruction);
    if (!oldName || !*oldName) {
        puts("rename: missing operand\nUse \"man rename\" to check the manual");
        return;
    }

    trim(instruction);
    trimEnd(instruction);

    char *newName = extractPath(&instruction);
    if (!newName || !*newName) {
        puts("rename: missing operand\nUse \"man rename\" to check the manual");
        return;
    }

    if (strchr(oldName, '/') || strchr(newName, '/')) {
        puts("rename: renaming across directories is not allowed");
        return;
    }

    if (rename(oldName, newName) != 0)
        perror("rename");

}

void manCmdMulti(char *instruction, const char **cmds, uint8_t isInsideBash) {
    char buffer[MAX_CHAR];
    strncpy(buffer, instruction, sizeof(buffer));
    buffer[sizeof(buffer)-1] = '\0';

    char *rest = buffer;

    if (buffer[0] == '-') {
        strtok(buffer, " ");
        rest = strtok(NULL, " ");
    }

    if (!rest || strlen(rest) == 0) {
        puts("man: missing operand\nUse \"man man\" to check the manual");
        return;
    }

    uint16_t cmdCount = 0;
    char **commands = parseData(rest, &cmdCount);

    if (!cmdCount) {
        puts("man: missing operand\nUse \"man man to check the manual");
        return;
    }

    for (int32_t i = 0; i < cmdCount; i++) {
        manCmd(commands[i], cmds, isInsideBash);

        if (i < cmdCount - 1) {
            puts("\n────────────────────────────────────────────────────────────────────────\n");
        }
    }

}

void clearHistoryCmd(const char *path) {
    char answer[0x20];
    
    printf("Are you sure you want to clear 'history.txt'? (y/n): ");
    if (!fgets(answer, sizeof(answer), stdin)) {
        puts("Error reading input");
        return;
    }
    answer[strcspn(answer, "\n")] = '\0';

    answer[strcspn(answer, "\n")] = '\0';
    safe_lower_inplace(answer);
    removeComments(answer);

    if (answer[0] == 'y' && (!answer[1] || answer[1] == ' ')) {

        FILE *f = fopen(path, "w");

        if (!f) 
            perror("clearhistory");
        else
            puts("clearhistory: 'history.txt' cleared successfully");

    } else {
        puts("clearhistory: deletion cancelled");
    }
}

void bcCmd(uint16_t argc, char **argv) {
    setup_console();
    char *result;
    uint8_t appear = 0;

    uint8_t flags = 0;

    for (uint16_t i = 1; i < argc; i++) {
        char *opt = argv[i];

        if (*opt != '-') {
            printf("bc: invalid argument: '%s'\n", opt);
            return;
        }

        if (opt[1] == '-') {
            if (strcasecmp(opt, "--quiet") == 0)
                flags |= BC_QUIET;
            else if (strcasecmp(opt, "--mathlib") == 0)
                flags |= BC_MATHLIB;
            else {
                printf("bc: invalid option: '%s'\n", opt);
                return;
            }
        }
        else {
            for (uint16_t j = 1; opt[j]; j++) {
                char chr = tolower((unsigned char)opt[j]);
                switch (chr) {
                    case 'q': flags |= BC_QUIET; break;
                    case 'l': flags |= BC_MATHLIB; break;
                    default:
                        printf("bc: invalid option: '-%c'\n", opt[j]);
                        return;
                }
            }
        }
    }

    bool quiet = flags & BC_QUIET;
    bool mathlib = flags & BC_MATHLIB;
    initRandom();

    char operation[MAX_CHAR] = {0};

    if (Ans.type == BC_STR && Ans.data.s)
        SAFE_FREE(Ans.data.s);
    else
        Ans.type = BC_NONE;

    while (true) {
        if (!appear) {
            if (!quiet) {
                printf("A simple calculator, note that without parenthesis operand precedence does not work,\n");
                printf("type 'quit' or 'exit' to exit and type 'clear' or 'cls' to clear the screen and scrollback buffer\n");
                printf("PS: mathlib is off by default, type 'mathlib' to turn it on/off if you're inside bc\n");
            }
            printf("Mathlib status: ");
            if (mathlib)
                printc("on\n\n", GREEN, WHITE);
            else 
                printc("off\n\n", RED, WHITE);
        }

        appear = 1;

        printc(">>> ", BC_PROMPT_COLOR, WHITE);
        fgets(operation, sizeof(operation), stdin);
        operation[strcspn(operation, "\n")] = '\0';

        trim(operation);
        trimEnd(operation);

        if (!*operation) {
            putchar('\n');
            continue;
        }

        if (isValidBcCommand(operation, "quit") ||
            isValidBcCommand(operation, "exit")) {
            break;

        } else if (isValidBcCommand(operation, "clear")) {
            cls();
            appear = 0;
            continue;
        } else if (isValidBcCommand(operation, "mathlib")) {
            cls();
            mathlib ^= 1;
            appear = 0;
            continue;
        } else if (isValidBcCommand(operation, "cls")) {
            cls();
            appear = 0;
            continue;
        }

        result = eval(operation, mathlib);

        if (!result) {
            putchar('\n');
            SAFE_FREE(result);
            fflush(stdout);
            continue;
        }

        printf("%s\n\n", result);

        SAFE_FREE(result);
        fflush(stdout);
    }
}

void grepCmd(char *instruction) {

    char buffer[MAX_CHAR];
    strncpy(buffer, instruction, sizeof(buffer));
    buffer[sizeof(buffer)-1] = '\0';

    
    int8_t ignoreCase = 0;
    char *option;
    char *rest = buffer;

    if (buffer[0] == '-' && buffer[1]) {
        option = strtok(buffer, " ");
        rest = strtok(NULL, "");

        if (option && option[1] == '-') {
            if (strcasecmp(option, "--ignore-case") == 0)
                ignoreCase = 1;
            else {
                printf("grep: invalid option: '%s'\n", option);
                return;
            }
        } else if (option && option [1] != '-') {
            for (uint16_t i = 1; option[i]; i++) {
                char opt = tolower((unsigned char)option[i]);
                switch (opt) {
                    case 'i': ignoreCase = 1; break;
                    default: 
                        printf("grep: invalid option: '-%c'\n", option[i]);
                        return;
                }
            }
        }
    }

    if (!rest || strlen(rest) == 0) {
        puts("grep: missing operand\nUse \"man grep\" to check the manual");
        return;
    }

    instruction = rest;
    trim(instruction);

    char pattern[0x100] = {0};
    char *file = NULL;

    if (instruction[0] == '"') {
        char *endQuote = strchr(instruction + 1, '"');
        if (!endQuote) {
            puts("grep: missing closing quote");
            return;
        }

        uint32_t len = endQuote - (instruction + 1);
        strncpy(pattern, instruction + 1, len);
        pattern[len] = '\0';

        instruction = endQuote + 1;
        trim(instruction);
        if (*instruction)
            file = instruction;
    } else {
        char *lastSpace = strrchr(instruction, ' ');
        if (lastSpace) {
            uint32_t len = lastSpace - instruction;
            strncpy(pattern, instruction, len);
            pattern[len] = '\0';

            file = lastSpace + 1;
            trim(file);
        } else {
            strcpy(pattern, instruction);
        }
    }

    if (!file || !*file) {
        puts("grep: missing file operand\nUse \"man grep\" to check the manual");
        return;
    }

    FILE *f = fopen(file, "r");
    if (!f) {
        perror(file);
        return;
    }

    char line[0x200];
    uint8_t found = 0;

    trimEnd(pattern);
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = '\0';

        const char *pos = ignoreCase ? strcasestr_ptr(line, pattern)
                                    : strstr(line, pattern);

        uint8_t match = (pos != NULL);

        if (match) {
            printTarg(line, pattern, RED, ignoreCase);
            found = 1;
        }
    }

    if (!found)
        printf("grep: pattern '%s' not found in '%s'\n", pattern, file);

    SAFE_FCLOSE(f);
}

void historyCmd(char *operation, const char *path) {
    FILE *f = fopen(path, "r");

    if (!f) {
        perror("history");
        return;
    }

    uint32_t lineCount = 1;

    //* temporary, ig
    // char **lines = readHistory(path, &lineCount);

    // if (!lines) {
    //     puts("Error: failed to read history file");
    //     return;
    // }

    // for (uint32_t i = 0; i < lineCount; i++) {
    //     charReplace(lines[i], '\n', '\0');
    //     printf("%05u  %s\n", lineCount++, lines[i]);
    // }

    // for (uint32_t i = 0; i < lineCount; i++)
    //     SAFE_FREE(lines[i]);

    // SAFE_FREE(lines);


    if (!*operation) {
        uint32_t size = getFileLength(f);
        char *fileBuffer = malloc(size + 1);
        fread(fileBuffer, 1, size, f);
        fileBuffer[size] = '\0';
    
        char *line = strtok(fileBuffer, "\n");
        while (line) {
            printf("%5u  %s\n", lineCount++, line);
            line = strtok(NULL, "\n");
        }
        SAFE_FREE(fileBuffer);
        SAFE_FCLOSE(f);

        return;
    }

    char *tmp = eval(operation, true);

    var debug1 = h_atof(tmp, true);
    float64 num = (debug1.type == BC_BOOL) ? (float64)debug1.data.b : debug1.data.f;

    SAFE_FREE(tmp);

    if (isnan(num)) {
        SAFE_FCLOSE(f);
        return;
    }

    if (debug1.type != BC_INT && debug1.type != BC_BOOL) {
        printf("history: must be integer\n");
        SAFE_FCLOSE(f);
        return;
    }

    if (num <= 0 || num >= 10000) {
        printf("history: must be greater than 0 and less than 10000 (10e+4)\n");
        SAFE_FCLOSE(f);
        return;
    }

    uint16_t i = 1;

    char buff[MAX_CHAR];

    while (i <= num) {
        if (!fgets(buff, sizeof(buff), f)) 
            break;

        buff[strcspn(buff, "\n")] = '\0';
        if (!*buff) 
            continue;
    
        printf("%5u  %s\n", i, buff);

        i++;
    }
        
    SAFE_FCLOSE(f);
}

void rmCmd(uint16_t argc, char **argv) {

    uint8_t flags = 0;
    uint16_t fileCount = 0;

    if (argc < 2) {
        puts("rm: missing operand\nUse \"man rm\" to check the manual");
        return;
    }

    for (uint16_t i = 1; i < argc; i++) {
        char *arg = argv[i];

        if (arg[0] == '-' && arg[1]) {
            if (arg[1] == '-') {
                if (strcasecmp(arg, "--force") == 0)
                    flags |= RM_FORCE;
                else if (strcasecmp(arg, "--interactive") == 0)
                    flags &= ~RM_FORCE;
                else if (strcasecmp(arg, "--recycle-bin") == 0)
                    flags |= RM_BIN;
                else if (strcasecmp(arg, "--erase") == 0)
                    flags &= ~RM_BIN;
                else {
                    printf("rm: invalid option: '%s'\n", arg);
                    return;
                }

            } else {
                for (size_t i = 1; arg[i]; i++) {
                    arg[i] = tolower((unsigned char)arg[i]);
                    switch (arg[i]) {
                        case 'f': flags |= RM_FORCE; break;
                        case 'i': flags &= ~RM_FORCE; break;
                        case 'b': flags |= RM_BIN; break;
                        case 'e': flags &= ~RM_BIN; break;
                        default:
                            printf("rm: invalid option: '-%c'\n", arg[i]);
                            return;
                    }
                }
            }
        }
        else {
            argv[fileCount++] = arg;
        }
    }

    if (!fileCount) {
        puts("rm: missing operand\nUse \"man rm\" to check the manual");
        return;
    }

    if (!(flags & RM_FORCE)) {
        char answer[0x20];
        printf("Are you sure you want to delete %d file(s)? (y/n): ", fileCount);

        if (!fgets(answer, sizeof(answer), stdin)) {
            puts("rm: reading input error");
            return;
        }

        answer[strcspn(answer, "\n")] = '\0';
        safe_lower_inplace(answer);

        if (answer[0] != 'y') {
            puts("rm: deletion cancelled");
            return;
        }
    }

    for (uint16_t i = 0; i < fileCount; i++) {
        if (rm_delete(argv[i], flags) != 0) {
            perror(argv[i]);
        }
        else {
            if (flags & RM_BIN)
                printf("rm: '%s' moved to recycle bin\n", argv[i]);
            else
                printf("rm: '%s' deleted successfully\n", argv[i]);
        }
    }
}

void touchCmd(char *instruction) {

    if (!*instruction) {
        puts("touch: missing operand\nUse \"man touch\" to check the manual");
        return;
    }
    
    trim(instruction);

    int16_t string = strrchar(instruction, '<');

    char *copy = strdup(instruction);
    char *save;


    if (string == -1) {

        if (!isValidFolderOrFileName(instruction)) {
            printf("touch: invalid file name\n");
            return;
        }

        FILE *f = fopen(instruction, "w");
        SAFE_FCLOSE(f);
    } else {
        char *filename = strtok_r(instruction, "<", &save);
        char *inFile = strtok_r(NULL, ">", &save);

        trim(inFile);
        trimEnd(inFile);

        trim(filename);
        trimEnd(filename);

        if (!isValidFolderOrFileName(filename)) {
            printf("touch: invalid file name\n");
            SAFE_FREE(copy);
            return;
        }

        int32_t changed = 0;
        char *new = stringToVariable(inFile, &changed);

        if (!(changed && strcasecmp(inFile, "$path") == 0)) {
            new = echoHandler(new);

            if (!new)
                return;

        }

        FILE *f = fopen(filename, "w");

        fprintf(f, "%s", new);

        SAFE_FCLOSE(f);
    }

    SAFE_FREE(copy);
}

void catCmd(char *instruction, uint32_t max_lines, const char *cmdName) {

    if (!*instruction) {
        printf("%s: missing operand\nUse \"man %s\" to check the manual\n", cmdName, cmdName);
        return;
    }

    trim(instruction);

    char *rest = strdup(instruction);


    char line[0x1000];
    uint32_t lines = 0;

    uint16_t fileCount = 0;
    char **files = parseData(rest, &fileCount);

    for (uint16_t i = 0; i < fileCount; i++) {
        
        FILE *f = fopen(files[i], "rb");
        if (!f) {
            fprintf(stderr, "cat: could not open '%s'\n", files[i]);
            for (uint16_t j = 0; j < fileCount; j++)
                SAFE_FREE(files[j]);
            SAFE_FREE(files);
            return;
        }

        while (fgets(line, sizeof(line), f)) {

            if (max_lines > 0 && lines >= max_lines)
                break;

            size_t len = strlen(line);
            if (len && line[len - 1] == '\n')
                line[len - 1] = '\0';

            for (size_t i = 0; line[i]; i++) {
                unsigned char c = (unsigned char)line[i];

                if (isprint(c) || c == '\t' || c == '\r') {
                    putchar(c);
                } else {
                    printf("\\x%02X", c);
                }
            }

            putchar('\n');
            lines++;
        }

        SAFE_FCLOSE(f);
    }

    for (uint16_t i = 0; i < fileCount; i++)
        SAFE_FREE(files[i]);
    SAFE_FREE(files);

    SAFE_FREE(rest);
}

void tailCmd(char *instruction, uint32_t max_lines) {

    if (!instruction || !*instruction) {
        puts("tail: missing operand\nUse \"man tail\" to check the manual");
        return;
    }

    char path[MAX_CHAR];
    strncpy(path, instruction, sizeof(path) - 1);
    path[sizeof(path) - 1] = '\0';

    trim(path);

    FILE *f = fopen(path, "rb");
    if (!f) {
        perror("tail");
        return;
    }

    fseek(f, 0, SEEK_END);
    long pos = ftell(f);

    uint32_t lines = 0;

    while (pos > 0) {
        pos--;
        fseek(f, pos, SEEK_SET);

        int16_t c = fgetc(f);
        if (c == '\n') {
            lines++;
            if (lines == max_lines + 1)
                break;
        }
    }

    if (pos > 0)
        fseek(f, pos + 1, SEEK_SET);
    else
        rewind(f);

    int32_t c;
    while ((c = fgetc(f)) != EOF) {
        if (isprint(c) || c == '\n' || c == '\t' || c == '\r') {
            putchar(c);
        } else {
            printf("\\x%02X", (unsigned char)c);
        }
    }

    putchar('\n');

    SAFE_FCLOSE(f);
}

void rmdirCmd(uint16_t argc, char **argv) {

    uint8_t flags = 0;
    uint16_t objCount = 0;
    char *objects[MAX_ARGS];

    if (argc < 2) {
        puts("rmdir: missing operand\nUse \"man rmdir\" to check the manual");
        return;
    }

    for (uint16_t i = 1; i < argc; i++) {
        char *arg = argv[i];

        if (arg[0] == '-' && arg[1]) {
            if (arg[1] == '-') {
                if (strcasecmp(arg, "--recycle-bin") == 0)
                    flags |= RM_BIN;
                else if (strcasecmp(arg, "--erase") == 0)
                    flags &= ~RM_BIN;
                else {
                    printf("rmdir: invalid option: '%s'\n", arg);
                    return;
                }
            } else {
                for (size_t j = 1; arg[j]; j++) {
                    char c = tolower((unsigned char)arg[j]);
                    switch (c) {
                        case 'b': flags |= RM_BIN; break;
                        case 'e': flags &= ~RM_BIN; break;
                        default:
                            printf("rmdir: invalid option: '-%c'\n", c);
                            return;
                    }
                }
            }
        }
        else {
            objects[objCount++] = arg;
        }
    }

    if (!objCount) {
        puts("rmdir: missing operand\nUse \"man rmdir\" to check the manual");
        return;
    }

    for (uint16_t i = 0; i < objCount; i++) {
        if (rm_delete(objects[i], flags) != 0) {
            perror(objects[i]);
        }
        else {
            if (flags & RM_BIN)
                printf("rmdir: '%s' moved to recycle bin\n", objects[i]);
            else
                printf("rmdir: '%s' deleted successfully\n", objects[i]);
        }
    }
}

void mkdirCmd(char *command) {
    
    char buffer[MAX_CHAR];
    strncpy(buffer, command, sizeof(buffer));
    buffer[sizeof(buffer)-1] = '\0';

    char *rest = buffer;

    if (!*rest) {
        puts("mkdir: missing operand\nUse \"man mkdir\" to check the manual");
        return;
    }

    uint16_t fileCount = 0;
    char **files = parseData(rest, &fileCount);

    if (fileCount == 0) {
        puts("mkdir: missing operand\nUse \"man mkdir\" to check the manual");
        return;
    }

#ifdef _WIN64
    for (uint16_t i = 0; i < fileCount; i++) {
        if (!isValidFolderOrFileName(files[i])) {
            printf("mkdir: invalid folder name: '%s'\n", files[i]);
            for (uint16_t j = 0; j < fileCount; j++) {
                SAFE_FREE(files[i]);
            }
            SAFE_FREE(files);
            return;
        }
        if (_mkdir(files[i]) != 0) {
            perror(files[i]);
        }
        SAFE_FREE(files[i]);
    }        
#else
    uint16_t mode = 0777;

    for (uint16_t i = 0; i < fileCount; i++) {
        if (!isValidFolderOrFileName(files[i])) {
            printf("mkdir: invalid folder name: '%s'\n", files[i]);
            for (uint16_t j = 0; j < fileCount; j++) {
                SAFE_FREE(files[i]);
            }
            SAFE_FREE(files);
            return;
        }
        if (mkdir(files[i], mode) != 0) {
            perror(files[i]);
        }
        SAFE_FREE(files[i]);
    }        
#endif

    SAFE_FREE(files);
}

char *cdCmd(const char *instruction, char *address) {
    char *raw = strdup(instruction);
    char *path = raw;
    while (*path == ' ') path++;

#ifdef _WIN64
    charReplace(path, '\\', '/');
#endif

    if (strcmp(path, "-") == 0) {
        SAFE_FREE(raw);
        return handle_cd_dash(address);
    }

    update_last_directory(address);

    char *buffer = NULL;

    if (*path == '~') {
        char *Default = get_default_address();

        *path = ' ';
        trim(path);

        size_t extra = strlen(Default) + strlen(path) + 1;
        buffer = malloc(extra);
        *buffer = '\0';
        snprintf(buffer, extra, "%s%s", Default, path);

        SAFE_FREE(Default);
    }

    char *final = buffer ? buffer : path;
    char *result = handle_normal_cd(final, address);

    SAFE_FREE(raw);
    if (buffer) SAFE_FREE(buffer);

    return result;
}

void listDrives(void) {
#ifdef _WIN64
    DWORD drives = GetLogicalDrives();
    if (drives == 0) {
        puts("drives: could not get logical drives");
        return;
    }

    puts("Available drives:");
    for (char letter = 'A'; letter <= 'Z'; letter++) {
        if (drives & (1 << (letter - 'A')))
            printf("  %c:\\\n", letter);
    }

#else
    char *user = get_user();
    char path[0x100];
    snprintf(path, sizeof(path), "/media/%s", user);

    DIR *dir = opendir(path);
    if (!dir) {
        puts("drives: could not open /media folder");
        return;
    }

    printf("Mounted drives in %s:\n", path);

    struct dirent *entry;
    bool found = false;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        printf("  %s/\n", entry->d_name);
        found = true;
    }

    if (!found)
        puts("drives: (no mounted drives found)");

    closedir(dir);
#endif
}

void cmdsCommand(const char **cmds, uint8_t isInsideBash) {
    static char **copy = NULL;
    static int8_t initialized = 0;
    
    uint16_t cmdCount = 0;

    for (uint16_t i = 0; cmds[i]; i++)
        cmdCount++;

    if (!initialized) {
        copy = malloc(cmdCount * sizeof(char *));
        for (uint16_t i = 0; i < cmdCount; i++)
            copy[i] = strdup(cmds[i]);

        bsort(copy, cmdCount);
        initialized = 1;
    }

    for (uint16_t i = 0; i < cmdCount; i++) {
        if (strcmp(copy[i], "bash") == 0 && !isInsideBash)
            continue;

        puts(copy[i]);
    }

    cmdCount = isInsideBash ? cmdCount : cmdCount - 1;

    printf("\n\nTotal commands: %"PRIu16"\n", cmdCount);
}

void updatehistory(void) {
    const char *logs[] = {
        "a0.0.40 - terminal creation\n\tAdded: clear, echo and exit command\n",
        "a0.0.55 - minor changes\n\tAdded: neofetch cmd\n",
        "a0.0.75 - minor changes\n\tAdded: history command\n",
        "a0.0.80 - minor changes\n\tEdited: optimized the sort algorithm\n",
        "a0.0.95 - minor changes\n\tEdited: now the neofetch menu displays the cpu and mem\n",
        "a0.0.975 - minor changes\n\tRemoved: system instruction\n",
        "a0.1.35 - big changes\n\tAdded: cd instruction\n",
        "b0.1.55 - big changes\n\tAdded: ls instruction\n",
        "b0.1.60 - minor changes\n\tAdded: 'initguid.h' and 'knownfolders.h' windows libs \n",
        "b0.1.65 - minor changes\n\tEdited: neofetch function\n",
        "b0.1.82 - big changes\n\tAdded: man command\n\tEdited: now neofetch displays the author of this terminal\n",
        "b0.2.00 - big changes\n\tEdited: get_time function and neofetch command\n",
        "b0.2.10 - big changes\n\tAdded: date command\n",
        "b0.2.23 - big changes\n\tAdded: pwd command\n",
        "b0.2.34 - big changes\n\tAdded: mkdir and rmdir command\n",
        "b0.2.36 - minor changes\n\tEdited: now neofetch displays the host\n",
        "b0.2.71 - big changes\n\tAdded: cat, touch and rm command\n",
        "b0.2.73 - minor changes\n\tEdited: echo cmd, changed how it works\n",
        "b0.2.78 - minor changes\n\tEdited: now you can write in files with the echo cmd\n",
        "b0.2.80 - minor changes\n\tEdited: now the touch command can also write in the file at the same time it's being created\n",
        "b0.2.91 - minor changes\n\tAdded: history command\n",
        "b0.2.912 - minor changes\n\tAdded: uname command\n",
        "b0.3.140 - big changes\n\tFixed: history command\n\tAdded: grep command\n",
        "b0.3.1415 (pi) - small changes\n\tEdited: optimized the command and manual identifier\n",
        "b0.3.33 - minor changes\n\tEdited: now you can multiply the text to how many times you want with touch and echo command\n",
        "b0.3.66 - big changes\n\tAdded: bc command\n",
        "b0.3.92 - big changes\n\tEdited: now it handles a lot more of operations\n",
        "b0.4.00 - minor changes\n\tEdited: the neofetch command now displays the line number of the code\n",
        "b0.4.10 - minor changes\n\tEdited: now the uname command has more options\n",
        "b0.4.37 - big changes\n\tAdded: a lot of new operators and functions to the bc command\n",
        "b0.4.50 - small changes\n\tAdded: floor(), ceil(), round() and fact() function to the bc command\n",
        "b0.4.59 - small changes\n\tAdded: drives command\n",
        "b0.4.62 - small changes\n\tEdited: added -i to grep command\n",
        "b0.4.70 - small changes\n\tEdited: added clear, deg and rad to bc command\n",
        "b0.4.89 - big changes\n\tAdded: terminal command\n",
        "b0.4.97 - small changes\n\tAdded: clearhistory and rand command\n",
        "b0.5.01 - minor changes\n\tEdited: optimized most of the 's_math.c' code\n",
        "b0.5.15 - small changes\n\tAdded: now you can run more than one command using '&&'\n",
        "b0.5.17 - minor changes\n\tEdited: now the terminal shows the total amount of commands and logs\n",
        "b0.5.20 - minor changes\n\tEdited: created 'CheckCmd.c' and 'CheckCmd.h' to handle the commands and eval functions\n",
        "b0.5.30 - small changes\n\tAdded: rand() function to the calculator\n",
        "b0.5.53 - big changes\n\tAdded: alias command\n",
        "b0.5.57 - minor changes\n\tFixed:  now the cd aliases works properly on windows\n",
        "b0.5.60 - minor changes\n\tEdited: now you cand check the manual of multiple commands at once\n",
        "b0.5.65 - minor changes\n\tEdited: now echo and touch breaks lines at each space if the string doesn't have any quotations marks at the beginning and in the end\n",
        "b0.5.70 - big changes\n\tAdded: rename command\n",
        "b0.5.72 - minor changes\n\tEdited: echoHandler() now works properly\n",
        "b0.5.75 - big changes\n\tEdited: improved alias to work more like on linux\n\tFixed: echo command\n",
        "b0.5.76 - minor changes\n\tEdited: changed echoHandler()\n",
        "b0.5.79 - minor changes\n\tEdited: changed int32_t variables to optimize the code, using 'inttypes.h'\n",
        "b0.5.85 - small changes\n\tEdited: now on windows you can generate 32bit numbers with rand command, or rand() in the calculator\n",
        "b0.5.90 - small changes\n\tEdited: now the calculator supports hex and octal numbers\n",
        "b0.6.00 - small changes\n\tAdded: hex(), int() and oct() to the calculator\n",
        "b0.6.07 - minor changes\n\tEdited: now echo and touch supports hex and oct numbers and the calculator was improved\n\n",
        "b0.6.15 - big changes\n\tEdited: improved ls command\n",
        "b0.6.18 - small changes\n\tEdited: improved functionHandler()\n",
        "b0.6.21 - minor changes\n\tEdited: improved echoCmd() and touchCmd()\n",
        "b0.6.24 - small changes\n\tAdded: euler constant\n",
        "b0.6.26 - big changes\n\tAdded: bash command (available if you're inside the terminal)\n",
        "b0.6.29 - minor changes\n\tEdited: a few tweaks to the calculator, now you can use -PI or -E, still not case sensitive\n",
        "b0.6.35 - small changes\n\tEdited: now to convert octal to integer or hex to integer, you just type it in\n\tRemoved: int() calculator function\n",
        "b0.6.39 - minor changes\n\tEdited: a few more tweaks to the calculator\n",
        "b0.6.45 - small changes\n\tEdited: improved sin(), cos() and tan() suffix identifier\n",
        "b0.6.50 - small changes\n\tEdited: now you can type things like '3pi' or '-pi' and it will work, including hex and oct numbers\n",
        "b0.6.52 - minor changes\n\tEdited: replaced some of the strtok to strtok_r, which is thread safe\n",
        "b0.6.58 - small changes\n\tFixed: now you can echo with another command using '&&' without any bugs\n",
        "b0.6.65 - small changes\n\tFixed: an annoying asf calculator bug\n",
        "b0.6.75 - big changes\n\tAdded: now you can choose the difference with the sum() function\n\tRemoved: that stupid thing of printing in columns of the ls command\n",
        "b0.6.78 - small changes\n\tFixed: now RAND_MAX keyword works properly\n",
        "b0.6.89 - changes\n\tAdded: more options to grep, uname and rm command\n\tEdited: now you can remove multiple files or empty folders using the rm and rmdir command\n",
        "b0.6.94 - small changes\n\tAdded: K, M, B and T suffixes for the numbers on the calculator (not case sensitive) and also 'exit' is now a valid calculator command\n",
        "b0.7.00 - small changes\n\tEdited: now you can create multiple folders with mkdir command\n",
        "b0.7.10 - big changes\n\tFixed: thought that only echo was affected by the '&&' bug, but it was every single command, it's now fixed\n",
        "b0.7.22 - big changes\n\tEdited: now bc and uname can combine options\n\tAdded: options to bc command\n",
        "b0.7.30 - small changes\n\tAdded: head and tail command\n",
        "b0.7.33 - small changes\n\tEdited: improved the neofetch ascii art\n",
        "b0.7.37 - small changes\n\tFixed: now '&&' should work properly...\n",
        "b0.7.40 - minor changes\n\tEdited: now you use multiple commands and change disk at the same time on windows\n",
        "b0.7.46 - minor changes\n\tAdded: comment support\n",
        "b0.7.53 - big changes\n\tAdded: lc command\n",
        "b0.7.57 - small changes\n\tAdded: fah() and cel() functions to the calculator\n",
        "b0.7.65 - big changes\n\tEdited: now lc works with multiple files, adding their line count\n",
        "b0.7.74 - big changes\n\tEdited: now the prompt looks just like linux\n",
        "b0.7.80 - small changes\n\tEdited: now the neofetch displays kernel release and architecture\n",
        "b0.7.83 - small changes\n\tAdded: bin() function to the calculator, it support binary numbers as well\n",
        "b0.7.89 - big changes\n\tEdited: changed prompt to avoid confusion\n",
        "b0.7.92 - minor changes\n\tRemoved: now the calculator no longer supports comma instead of dot\n",
        "b0.8.00 - big changes\n\tEdited: Now mathmatical expressions and another functions work inside some functions\n",
        "b0.8.10 - small changes\n\tEdited: now sin(), cos(), tan() and scale() works with another functions and operations\n",
        "b0.8.13 - minor changes\n\tEdited: now you make operations with the result of functions, i think everything else works as usual, at least i hope so\n",
        "b0.8.17 - small changes\n\tAdded: root() function to the calculator\n",
        "b0.8.20 - small changes\n\tRemoved: terminal command\n\tEdited: now to write in the files using touch, instead of '>' it is '<' now, and also changed the **argv thingy\n",
        "b0.8.25 - big changes\n\tAdded: randf() function\n\tEdited: rand() now generates integers\n",
        "b0.8.37 - big changes\n\tAdded: mi(), km(), lb() and kg() functions to the calculator\n",
        "b0.8.40 - minor changes\n\tEdited: edited echoHandler() with the new function 'trimBetween()', and now createShortcut() now has an implementation of that same function\n",
        "b0.8.45 - minor changes\n\tAdded: more options to uname command\n\tEdited: neofetch now shows the size in megabytes\n",
        "b0.8.54 - big changes\n\tEdited: now functions should work properly with another functions inside\n",
        "b0.8.59 - minor changes\n\tAdded: yes command\n",
        "b0.8.71 - big changes\n\tAdded: sleep command\n",
        "b0.8.82 - big changes\n\tAdded: randstr command\n",
        "b0.9.00 - big changes\n\tAdded: rev command\n\tEdited: now the manual for a few commands gives more explanations\n",
        "b0.9.07 - minor changes\n\tAdded: file name verification\n\tEdited: now randChr() is able to randomize numbers\n",
        "b0.9.12 - minor changes\n\tEdited: optimized the code\n",
        "b0.9.17 - small changes\n\tAdded: '-o' option to uname\n",
        "b0.9.28 - big changes\n\tEdited: now instead of unknown, the size and lines show a approximated number\n",
        "b0.9.35 - small changes\n\tEdited: improved man\n",
        "b0.9.39 - minor changes\n\tEdited: now neofetch displays the creation date\n",
        "b0.9.51 - big changes\n\tEdited: the calculator now works properly when dealing with the wrong data type\n",
        "b0.9.57 - small changes\n\tEdited: oct() function now works properly\n",
        "b0.9.63 - minor changes\n\tRemoved: some useless functions from the source code\n",
        "b0.9.70 - big changes\n\tEdited: improved the sleep() function, now it has more precision and works with sigle point precision numbers\n",
        "b0.9.76 - small changes\n\tEdited: in neofetch KERNEL -> KERNEL-RELEASE + KERNEL-VERSION\n",
        "b0.9.85 - big changes\n\tFixed: now commands that randomizes values works properly outside the terminal\n",
        "b0.9.89 - minor changes\n\tEdited: now the source code is a little more safe\n",
        "b0.9.95 - small changes\n\tFixed: now echo and touch works a lot better when multiplying strings\n",
        "r1.0.40 - big changes\n\tEdited: edited the calculator initial message\n",
        "r1.0.50 - big changes\n\tEdited: file headers organization\n",
        "r1.0.54 - minor changes\n\tEdited: now the source code is safer\n",
        "r1.0.60 - big changes\n\tEdited: made some preparations for the future update\n",
        "r1.0.64 - minor changes\n\tFixed: echo and touch behavior when multiplying strings with quotes\n",
        "r1.0.69 - small changes\n\tAdded: help message when initializing the program\n",
        "r1.0.80 - big changes\n\tEdited: improved echo behavior once again\n\tFixed: freed some pointers that I had forgotten to and also the sleep suffix identifier\n",
        "r1.0.85 - small changes\n\tRemoved: Kernel version from neofetch\n",
        "r1.1.00 - big changes\n\tAdded: now rm can move to the recycle bin\n",
        "r1.1.10 - minor changes\n\tEdited: time format\n",
        "r1.1.13 - minor changes\n\tEdited: stop and clean in rev\n",
        "r1.1.18 - small changes\n\tFixed: now you can use hex(), oct() or bin() as parameters\n",
        "r1.1.23 - minor changes\n\tEdited: factored the math code\n",
        "r1.1.30 - big changes\n\tAdded: rmdir can now also move to the recycle bin\n",
        "r1.1.34 - small changes\n\tFixed: early freed pointers\n",
        "r1.1.36 - minor changes\n\tEdited: bc manual\n",
        "r1.1.45 - big changes\n\tEdited: improved the option identifier for all commands\n",
        "r1.1.51 - small changes\n\tEdited: optimized the history command since that 'future update' isn't coming any time soon\n",
        "r1.1.60 - big changes\n\tAdded: now the terminal works with commands with spaces, using quotes, e.g: '[COMMAND WITH SPACES]' [ARGS...]\n",
        "r1.1.63 - minor changes\n\tEdited: linesNumber() refactor\n",
        "r1.1.70 - big changes\n\tFixed: bc, rm and uname seg-fault\n",
        "r1.1.79 - big changes\n\tAdded: seg-fault message for windows\n",
        "r1.1.83 - small changes\n\tEdited: uname and randstr option identifier\n",
        "r1.1.92 - big changes\n\tEdited: now single characters options are no longer case sensitive, and also upgraded the file/folder name verification\n",
        "r1.2.00 - minor changes\n\tEdited: fact() function\n",
        "r1.2.10 - small changes\n\tFixed: man seg-fault\n",
        "r1.2.16 - small changes\n\tEdite: echo function refactor\n",
        "r1.2.27 - big changes\n\tAdded: append in echo command\n",
        "r1.2.34 - small changes\n\tEdited: echoHandler()\n",
        "r1.2.43 - big changes\n\tAdded: help option to bash command\n\tEdited: bash now uses argc and argv\n",
        "r1.2.48 - small changes\n\tAdded: all option to bash command\n",
        "r1.2.52 - minor changes\n\tEdited: rev\n",
        "r1.2.64 - big changes\n\tAdded: tetration operator to the calculator\n",
        "r1.2.70 - small changes\n\tEdited: bc behavior with comments\n\tRemoved: comments from rev command\n",
        "r1.2.79 - big changes\n\tAdded: fabs() and abs() function to the calculator\n",
        "r1.2.91 - big changes\n\tFixed: early freed pointers in the calculator and negative numbers not working with mathlib turned on\n\tEdited: some calculator error messages\n",
        "r1.2.97 - small changes\n\tEdited: bash command strings\n",
        "r1.3.00 - minor changes\n\tEdited: bc initial message\n",
        "r1.3.05 - minor changes\n\tEdited: the command list is now sorted with 'lsw --help'\n",
        "r1.3.10 - small changes\n\tEdited: improved history command output speed\n",
        "r1.3.17 - small changes\n\tEdited: argv and argc extractor refactored\n",
        "r1.3.28 - big changes\n\tEdited: now binaries work with negative numbers and they can be converted from them\n",
        "r1.3.40 - big changes\n\tAdded: '~' unary operator to the calculator\n",
        "r1.3.44 - small changes\n\tEdited: bc manual\n",
        "r1.3.50 - small changes\n\tEdited: bc manual once again\n",
        "r1.3.61 - big changes\n\tEdited: now the code works on arm64 aka aarch64 devices\n",
        "r1.3.70 - big changes\n\tAdded: support for android\n",
        "r1.3.79 - big changes\n\tEdited: now you can cat multiple files\n",
        "r1.3.84 - minor changes\n\tEdited: uname should work on mac, supposedly\n",
        "r1.3.95 - big changes\n\tEdited: improved eval() so you can make operations like 'sqrt(4) + sqrt(4)'\n",
        "r1.4.06 - big changes\n\tAdded: support for negative hex\n\tEdited: octal prefix\n",
        "r1.4.10 - minor changes\n\tEdited: optimized eval()\n",
        "r1.4.13 - minor changes\n\tEdited bash cmd and version string\n",
        "r1.4.17 - small changes\n\tEdited: shortcut.txt --> lswrc.txt, planning to make it work like .bashrc and .zshrc\n",
        "r1.4.21 - small changes\n\tEdited: a simple thing in neofetch function\n",
        "r1.4.30 - big changes\n\tAdded: now you can choose how many lines of command history you want to see\n",
        "r1.4.44 - big changes\n\tAdded: HISTSIZE in lswrc\n\tEdited: the history.txt file is now dynamically edited when it reaches the HISTSIZE\n",
        "r1.4.50 - small changes\n\tEdited: improved the lswrc syntax analyzer\n",
        "r1.4.56 - small changes\n\tAdded: neofetch now displays HISTSIZE\n",
        "r1.4.67 - big changes\n\tAdded: support for system variables\n\tEdited: almost every manual\n",
        "r1.4.71 - small changes\n\tFixed: fixed calc() garbage values\n",
        "r1.4.76 - small changes\n\tEdited: manual structure\n",
        "r1.4.80 - minor changes\n\tRemoved: yes command\n",
        "r1.4.85 - small changes\n\tEdited: now instead of checking the syntax of lswrc only once, it checks every time\n",
        "r1.4.90 - small changes\n\tFixed: getHistSizeConfig() function bug\n",
        "r1.4.93 - minor changes\n\tEdited: renamed 'updatehistory' command to 'logs'\n",
        "r1.5.04 - big changes\n\tAdded: strlen() function to the calculator\n\tEdited: functions error messages and now eval() no longer remove spaces\n",
        "r1.5.08 - minor changes\n\tEdited: optimized stringToVariable() function and the environment variables\n",
        "r1.5.16 - small changes\n\tEdited: improved the alias syntax analyzer\n",
        "r1.5.23 - small changes\n\tEdited: improved stringToVariable() function\n",
        "r1.5.34 - big changes\n\tFixed: forgot to free and fclose in some functions\n\tAdded: now the checkLswrcSyntax() function checks for duplicated keys\n",
        "r1.5.38 - small changes\n\tEdited: improved strlen() calculator function\n",
        "r1.5.46 - small changes\n\tEdited: improved eval()\n",
        "r1.5.53 - small changes\n\tEdited: improved calculator strlen() once again\n",
        "r1.5.59 - minor changes\n\tFixed: forgot to add variable checking in some places\n",
        "r1.5.70 - big changes\n\tAdded: now apparently the calculator works with more than 2 numbers, but without operand precedence\n",
        "r1.5.73 - minor changes\n\tEdited: just a few optimizations\n",
        "r1.5.76 - small changes\n\tAdded: function attribute to printc()\n",
        "r1.5.80 - small changes\n\tEdited: improved isBcVariable()\n",
        "r1.5.84 - minor changes\n\tEdited: the variable warning when used with other commands now appears without the extra '\\n'\n",
        "r1.5.89 - small changes\n\tEdited: renamed strlen() to len() in the calculator\n",
        "r1.5.94 - small changes\n\tFixed: echo multiplier bug\n\tEdited: bc manual\n",
        "r1.6.05 - big changes\n\tAdded: bmi(), feet() and meter() functions to bc\n",
        "r1.6.07 - minor changes\n\tEdited: type annotation\n",
        "r1.6.13 - small changes\n\tAdded: cot() to bc\n",
        "r1.6.18 - small changes\n\tFixed: constant parser\n",
        "r1.6.27 - big changes\n\tFixed: cel() conversion formula\n",
        "r1.6.33 - small changes\n\tEdited: improved functionHandler()\n",
        "r1.6.37 - small changes\n\tAdded: added gradians 'gon()', an another angle measurement to bc\n",
        "r1.6.49 - big changes\n\tAdded: Ans, a builtin variable that stores the result of the last operation\n",
        "r1.6.55 - small changes\n\tEdited: improved Ans behavior\n",
        "r1.6.60 - small changes\n\tAdded: error to negative numbers to oct() in bc\n",
        "r1.6.69 - big changes\n\tAdded: improved the parser to work with expressions like: '2ans'\n",
        "r1.6.73 - minor changes\n\tFixed: forgot to add type annotations\n",
        "r1.6.78 - small changes\n\tEdited: now the parser should work with octal with e or pi expressions, example: '0o2pi'\n",
        "r1.6.81 - minor changes\n\tRemoved: empty input bc error message\n",
        "r1.6.90 - big changes\n\tEdited: improved mostly of eval() parser\n",
        "r1.6.95 - small changes\n\tAdded: more instructions to lsw manual\n",
        "r1.7.00 - small changes\n\tFixed: eval() parser not working with '-' as an unary operator\n",
        "r1.7.09 - big changes\n\tFixed: Ans not working with unary '-'\n",
        "r1.7.14 - small changes\n\tEdited: bc manual and bc initial string\n",
        "r1.7.18 - small changes\n\tEdited: the user should now appear on android\n",
        "r1.7.30 - big changes\n\tAdded: ascii characters support to bc\n",
        "r1.7.38 - big changes\n\tEdited: improved bc suffix and ascii parser\n",
        "r1.7.49 - big changes\n\tAdded: chr() function to bc\n",
        "r1.7.55 - small changes\n\tEdited: improved the bc parser once again\n",
        "r1.7.63 - small changes\n\tEdited: improved the str functions parsers (bin(), chr(), hex() and() oct())\n",
        "r1.7.74 - big changes\n\tEdited: most of the error strings\n",
        "r1.7.80 - small changes\n\tFixed: Ans not working\n",
        "r1.7.85 - small changes\n\tFixed: now atof will not convert 'nan' or 'inf', it will just return 0\n",
        "r1.8.00 - big changes\n\tAdded: acos(), acot(), asin(), atan() and now 'inf' returned\n",
        "r1.8.09 - big changes\n\tRemoved: useless code\n",
        "r1.8.15 - small changes\n\tEdited: now the alias is checked first, allowing you to create aliases with the terminal's command names\n",
        "r1.8.25 - big changes\n\tAdded: new option to rm and rmdir\n\tEdited: double '-' options are no longer case sensitive\n",
        "r1.8.30 - small changes\n\tEdited: ls now works by argv and argc\n",
        "r1.8.34 - minor changes\n\tFixed: now bc returns 0 properly when you type random characters\n",
        "r1.8.43 - big changes\n\tFixed: '==' not working properly\n\tEdited: improved the calc precision\n",
        "r1.8.46 - minor changes\n\tFixed: ans returning 0 when set to NAN\n",
        "r1.8.52 - small changes\n\tFixed: 'get_cpu_model()' should now work on android\n",
        "r1.8.57 - minor changes\n\tFixed: inf not working properly\n",
        "r1.8.62 - small changes\n\tEdited: improved bc function identifier\n",
        "r1.8.65 - minor changes\n\tFixed: eval() returning nan instead of 0 at some point\n",
        "r1.8.69 - minor changes\n\tEdited: improved options behavior\n",
        "r1.8.74 - small changes\n\tFixed: eval() returning the wrong value at some point\n",
        "r1.8.86 - big changes\n\tRemoved: useless code\n\tEdited: improved the parenthesis balance analyzer\n",
        "r1.9.00 - big changes\n\tEdited: huge eval() refactor\n",
        "r1.9.03 - minor changes\n\tEdited: improved unary parser with invalid numbers\n",
        "r1.9.05 - minor changes\n\tEdited: those imbalanced error strings\n",
        "r1.9.10 - small changes\n\tFixed: euler not working with the suffixes\n",
        "r1.9.14 - minor changes\n\tRemoved: scientific notation, since I only managed it to work with only positive and without the plus sign on the power\n",
        "r1.9.19 - small changes\n\tEdited: more error strings\n",
        "r1.9.25 - small changes\n\tEdited: rmdir now works by argc and argv correctly\n",
        "r1.9.30 - small changes\n\tEdited: attempted to fix permission being denied when removing folders/files on windows\n",
        "r1.9.34 - minor changes\n\tAdded: error when trying to use the bitwise not with inf\n",
        "r1.9.45 - big changes\n\tAdded: isprime() to bc\n",
        "r1.9.54 - big changes\n\tEdited: improved the behavior with lsw argv and argc\n",
        "r1.9.66 - big changes\n\tEdited: improved the main argv and argc, optimized the suffix analyzer and improved the bc oct() function\n",
        "r1.9.73 - small changes\n\tEdited: improved the main argv and argc behavior with comments\n",
        "r1.9.77 - small changes\n\tEdited: improved '==' operand\n",
        "r1.9.85 - small changes\n\tEdited: replaced strcat uses with snprintf\n",
        "r1.9.91 - small changes\n\tEdited: improved the behavior with comments\n\tRemoved: '//' comments\n",
        "r2.0.00 - big changes\n\tEdited: improved bc parser and edited its variables syntax\n",
        "r2.0.03 - minor changes\n\tRemoved: unnecessary error messages\n",
        "r2.0.09 - small changes\n\tEdited: bc initial message\n",
        "r2.0.15 - small changes\n\tEdited: improved the variable analyzer\n",
        "r2.0.17 - minor changes\n\tEdited: improved the variable analyzer again\n",
        "r2.0.21 - minor changes\n\tFixed: the variable analyzer not skipping spaces\n",
        "r2.0.32 - big changes\n\tRemoved: fact()\n\tAdded: now to factor numbers you will just use '!'\n",
        "r2.0.40 - small changes\n\tEdited: improved eval() parser\n",
        "r2.0.42 - minor changes\n\tEdited: bc manual\n",
        "r2.0.50 - small changes\n\tRemoved: useless math code\n\tEdited: made the bc parser safer\n",
        "r2.0.70 - huge changes\n\tEdited: refactored the function parser\n\tAdded: error messages to string types values/functions\n",
        "r2.0.95 - huge changes\n\tAdded: parenthesis support, you can use it when you are having unexpected result with the lack of precedence\n",
        "r2.1.00 - small changes\n\tEdited: bc initial message\n",
        "r2.1.26 - huge changes\n\tEdited: improved the bc parser by a lot and also refactored all of the function parser to depend less on heap\n",
        "r2.1.32 - small changes\n\tEdited: optimized the function parser and fixed the -Wextra and -Wpedantic compilation flags warnings\n",
        "r2.1.38 - small changes\n\tEdited: renamed the angles functions\n",
        "r2.1.54 - big changes\n\tRemoved: string multiplication with echo and touch\n\tAdded: support to escape characters to lsw, and bc of course\n",
        "r2.1.62 - small changes\n\tEdited: improved the escape characters parser in bc and you can now use the '\\0' character\n",
        "r2.1.65 - minor changes\n\tFixed: seg-fault fixed, now the bc is a slightly safer\n",
        "r2.1.90 - huge changes\n\tEdited: changing the whole eval from double to char *, later I will add full support to strings to Bc\n",
        "r2.2.17 - huge changes\n\tAdded: strings are now fully supported on bc, and also added int(), float() and str() functions (it may have some bugs which with further testing will soon be fixed)\n",
        "r2.2.25 - small changes\n\tFixed: a seg-fault caused by the format i chose to print the numbers on the strings\n",
        "r2.2.31 - small changes\n\tFixed: a a another seg-fault caused by NULL pointers, and also fixed some lost pointers\n",
        "r2.2.39 - small changes\n\tFixed: factorial is now working with parenthesis again\n",
        "r2.2.50 - big changes\n\tFixed: int(), float() and ans not working with strings\n\tRemoved: fabs() from bc\n",
        "r2.2.55 - small changes\n\tFixed: Ans not freeing after an error\n",
        "r2.2.80 - huge changes\n\tEdited: huge refactor on eval()\n\tAdded: support to boolean constants\n",
        "r2.2.85 - small changes\n\tRemoved: numeric a numeric overflow error message\n\tEdited: bc prompt\n",
        "r2.2.93 - small changes\n\tEdited: bc prompt\n\tFixed: Ans now converts true to 1 and false to 0\n",
        "r2.3.25 - huge changes\n\tEdited: lswrc syntax analyzer, string operations handler were all refactored\n\tAdded: boolean operators and multiplication with strings\n",
        "r2.3.33 - small changes\n\tFixed: chr() function seg-fault\n",
        "r2.3.38 - small changes\n\tFixed: operations with strings should work properly now\n",
        "r2.3.50 - big changes\n\tAdded: lower() and upper() to the calculator\n",
        "r2.3.54 - minor changes\n\tFixed: Ans returning it's string type values when mathlib is turned off\n",
        "r2.3.64 - big changes\n\tAdded: more error messages for the other numeric systems\n\tEdited: the builtin variables and constants are no longer case insensitive\n",
        "r2.3.68 - small changes\n\tAdded: none constant to bc\n",
        "r2.3.82 - big changes\n\tEdited: first part of making bc more customizable\n",
        "r2.3.90 - big changes\n\tEdited: finished the eval strings customizations\n",
        "r2.3.95 - small changes\n\tEdited: improved a number parser in bc\n",
        "r2.4.10 - big changes\n\tAdded: logical not(!) operator\n",
        "r2.4.16 - small changes\n\tFixed: wrong logic with some operators\n",
        "r2.4.23 - small changes\n\tFixed: sum() error messages in bc\n",
        "r2.4.27 - small changes\n\tFixed: now bc displays the multi-byte characters properly\n",
        "r2.4.32 - small changes\n\tFixed: true constant not working\n\tEdited: ans now returns true/false when the last answer was boolean type\n",
        "r2.4.37 - small changes\n\tFixed: a bug in the unary parser\n",
        "r2.4.44 - small changes\n\tEdited: improved the lswrc syntax checking\n",
        "r2.4.50 - small changes\n\tEdited: improved ans in bc\n",
        "r2.4.58 - big changes\n\tFixed: ans not saving str types\n\tEdited: improved char type in bc\n",
        "r2.4.65 - small changes\n\tAdded: typeof() to bc\n",
        "r2.4.75 - big changes\n\tFixed: int() and float() not working as expected, and also fixed the parser not parsing str type properly\n",
        "r2.4.81 - small changes\n\tFixed: len() now works with scape '\\0' properly and now the parser trims the spaces that were causing bugs\n",
        "r2.4.94 - big changes\n\tFixed: bc type loss\n\tEdited: improved the number format and precision\n",
        "r2.5.00 - last changes\n\tFixed: factorial buffer overflow\n",
        "r2.5.06 - small changes\n\tEdited: improved rand_max variable\n",
        "r2.5.23 - big changes\n\tEdited: made everything work on MacOS\n"
    };

    uint16_t logCount = sizeof(logs) / sizeof(*logs);

    for (uint16_t i = 0; i < logCount; i++) {
        if (i != logCount - 1)
            puts(logs[i]);
        else 
            printf("\n(current):\n\n%s", logs[i]);
    }

    printf("\n\nTotal updates: %"PRIu16"\n", logCount);
}

char *unameCmd(uint16_t argc, char **argv) {
    uint8_t flags = 0;

    if (argc == 1) {
        flags = U_KERN_NAME;
    } else {
        for (uint16_t i = 1; i < argc; i++) {
            char *opt = argv[i];

            if (*opt != '-') {
                printf("uname: invalid argument: '%s'\n", opt);
                return NULL;
            }

            if (opt[1] == '-') {
                if (strcasecmp(opt, "--all") == 0) {
                    flags = U_ALL;
                }
                else if (strcasecmp(opt, "--kernel-name") == 0) {
                    flags |= U_KERN_NAME;
                }
                else if (strcasecmp(opt, "--kernel-release") == 0) {
                    flags |= U_KERN_RELEASE;
                }
                else if (strcasecmp(opt, "--machine") == 0) {
                    flags |= U_MACHINE;
                }
                else if (strcasecmp(opt, "--kernel-version") == 0) {
                    flags |= U_KERN_VERSION;
                }
                else if (strcasecmp(opt, "--nodename") == 0) {
                    flags |= U_HOST_NAME;
                }
                else if (strcasecmp(opt, "--operating-system") == 0) {
                    flags |= U_OPERATING_SYSTEM;
                }
                else {
                    printf("uname: invalid option: '%s'\n", opt);
                    return NULL;
                }
                continue;
            }

            for (uint16_t j = 1; opt[j]; j++) {
                char chr = tolower((unsigned char)opt[j]);
                switch (chr) {
                    case 'a': flags |= U_ALL; break;
                    case 's': flags |= U_KERN_NAME; break;
                    case 'r': flags |= U_KERN_RELEASE; break;
                    case 'm': flags |= U_MACHINE; break;
                    case 'v': flags |= U_KERN_VERSION; break;
                    case 'n': flags |= U_HOST_NAME; break;
                    case 'o': flags |= U_OPERATING_SYSTEM; break;
                    default:
                        printf("uname: invalid option: '-%c'\n", opt[j]);
                        return NULL;
                }
            }
        }
    }

#ifdef _WIN64
    return unameCmdWin(flags);
#else
    return unameCmdLinux(flags);
#endif
}

void echoCmd(char *instruction) {

    if (!*instruction) {
        putchar('\n');
        return;
    }

    int8_t file = strrchar(instruction, '>');

    char *copy = strdup(instruction);
    char *save;

    if (!copy) {
        perror("strdup");
        return;
    }

    if (file == -1) {
        int32_t changed = 0;
        char *new = stringToVariable(copy, &changed);

        if (!(changed && strcasecmp(copy, "$path") == 0)) {
            new = echoHandler(new);
            if (!new)
                return;
        }

        puts(new);
        
        SAFE_FREE(new);

    } else if (file != -1) {

        char *inFile = strtok_r(copy, ">", &save);
        char *filename = strtok_r(NULL, ">", &save);

        if (!inFile || !filename) {
            puts("echo: invalid syntax");
            SAFE_FREE(copy);
            return;
        }

        trim(inFile); trimEnd(inFile);
        trim(filename); trimEnd(filename);

        bool QuoteAfterAbracket = (file < strrchar(instruction, '\"') ||
                                file < strrchar(instruction, '\''));

        int8_t append = isAppend(instruction);
        char *mode = NULL;

        switch (append) {
            case -1:
                QuoteAfterAbracket = true;
                break;

            case -2:
                puts("echo: invalid redirection syntax");
                return;

            case 1:
                mode = "w";
                break;

            case 2:
                mode = "a";
                break;

            case 0:
                return;
        }

        if (QuoteAfterAbracket) {
            echoHandler(instruction);
            puts(instruction);
            return;
        }

        int32_t changed = 0;
        char *new = stringToVariable(inFile, &changed);

        if (!(changed && strcasecmp(inFile, "$path") == 0)) {
            new = echoHandler(new);
        }

        if (!isValidFolderOrFileName(filename)) {
            printf("echo: invalid file name\n");
            return;
        }

        FILE *f = fopen(filename, mode);
        if (!f) {
            perror("fopen");
            SAFE_FREE(copy);
            return;
        }

        fprintf(f, "%s\n", new);
        SAFE_FCLOSE(f);

        SAFE_FREE(copy);
        return;

    }
}

void lsCmd(char **argv, uint16_t argc, const char *address) {
    const char *dirPath = (address && *address) ? address : ".";

    uint8_t flags = 0;

    if (argc > 1) {
        for (uint16_t i = 1; i < argc; i++) {
            char *opt = argv[i];

            if (*opt != '-') {
                printf("ls: invalid argument: '%s'\n", opt);
                return;
            }

            if (opt[1] == '-') {
                if (strcasecmp(opt, "--all") == 0) {
                    flags |= LS_ALL;
                }
                else {
                    printf("ls: invalid option: '%s'\n", opt);
                    return;
                }
                continue;
            }

            for (uint16_t j = 1; opt[j]; j++) {
                char chr = tolower((unsigned char)opt[j]);
                switch (chr) {
                    case 'a': flags |= LS_ALL; break;
                    default:
                        printf("ls: invalid option: '-%c'\n", opt[j]);
                        return;
                }
            }
        }
    }

    enableAnsiIfNeeded();

#ifdef _WIN64
    lsCmdWin(dirPath, flags);
#else
    lsCmdLinux(dirPath, flags);
#endif
}

void neofetchCmd(char *lswrc_path) {
    setup_console();

    const char *ascii_art[] = {
        " ██████╗  █████╗ ██████╗           ██████╗ ███████╗",
        "██╔════╝ ██╔══██╗██╔══██╗         ██╔═══██╗██╔════╝",
        "██║  ███╗███████║██████╔╝ ██████╗ ██║   ██║███████╗",
        "██║   ██║██╔══██║██╔══██╗ ╚═════╝ ██║   ██║╚════██║",
        "╚██████╔╝██║  ██║██████╔╝         ╚██████╔╝███████║",
        " ╚═════╝ ╚═╝  ╚═╝╚═════╝           ╚═════╝ ╚══════╝",
        "",
        "      G A B   O P E R A T I N G   S Y S T E M"
    };



    const uint8_t lines = sizeof(ascii_art) / sizeof(*ascii_art);
    
    const color4 title_color = YELLOW;
    const color4 label_color = LIGHT_CYAN;
    const color4 art_color = GREEN;
    const color4 art_bg_color = LIGHT_GREEN;

    for (uint8_t i = 0; i < lines; i++) {
        for (size_t j = 0; ascii_art[i][j]; j++) {
            unsigned char c = ascii_art[i][j];

            if ((c & 0xC0) != 0x80) {
                if ((unsigned char)ascii_art[i][j] == 0xE2 &&
                    ascii_art[i][j+1] != '\0' &&
                    (unsigned char)ascii_art[i][j+1] == 0x95) {
                    setColor(art_bg_color);
                } else
                    setColor(art_color);
            }

            putchar(ascii_art[i][j]);
        }
        putchar('\n');
    }

    printc("═══════════════════════════════════════════════════\n", title_color, WHITE);

    printc("SYSTEM\n", title_color, WHITE);

    printc("OS: ", label_color, 7);
#ifdef _WIN64
    puts(unameCmdWin(U_KERN_NAME | U_MACHINE));
#else
    puts(unameCmdLinux(U_KERN_NAME | U_MACHINE));
#endif


    printc("KERNEL-RELEASE: ", label_color, WHITE);
#ifdef _WIN64
    puts(unameCmdWin(U_KERN_RELEASE));
#else
    puts(unameCmdLinux(U_KERN_RELEASE));
#endif


    printc("───────────────────────────────────────────────────\n", title_color, WHITE);

    printc("INFO", title_color, WHITE);
    putchar('\n');

    printc("USER: ", label_color, WHITE);
    char *userName = get_user();

    puts(userName);


    printc("HOST: ", label_color, WHITE);
    char *hostName = get_hostname();

    puts(hostName);


    printc("DATE: ", label_color, WHITE);
    static char *today;
    if (!today) today = get_time(TIME_FMT);

    puts(today);


    printc("───────────────────────────────────────────────────\n", title_color, WHITE);

    printc("SHELL\n", title_color, WHITE);

    printc("HISTSIZE: ", label_color, WHITE);
    char *value = getKeyVal("HISTSIZE", lswrc_path);
    
    if (value) {
        printf("%g\n", atof(value));
        SAFE_FREE(value);
    } else
        printf("%d\n", DEFAULT_HISTSIZE);

    printc("LANGUAGES USED: ", label_color, WHITE);
    puts("C");


    printc("LINES OF CODE: ", label_color, WHITE);
    char *linesNum = linesNumber(); 

    puts(linesNum);


    printc("SIZE: ", label_color, WHITE);
    char *size = charNumber();

    puts(size);


    printc("VERSION: ", label_color, WHITE);
    puts("LSW - Gab-OS  "VERSION"");


    printc("CREATION DATE: ", label_color, WHITE);
    puts("10/18/2025");


    printc("AUTHOR: ", label_color, WHITE);
    printf("Gabriel Oliveira Miranda\n");

    printc("───────────────────────────────────────────────────\n", title_color, WHITE);

    printc("SPECS\n", title_color, WHITE);

    printc("CPU: ", label_color, WHITE);
    char *cpuName = get_cpu_model();

    puts(cpuName);


    printc("Memory: ", label_color, WHITE);
     uint64_t memTotal = get_total_ram_mb();

    printf("%" PRIu64"Mib\n", memTotal);

    printc("═══════════════════════════════════════════════════\n", title_color, WHITE);
}
