#define _GNU_SOURCE
#include "utils.h"

#define VERSION "r1.2.48"

#ifdef _WIN32
    #define SYSTEM "Windows"
#elif defined(__linux__) || defined(__APPLE__)
    #ifdef __linux__
        #define SYSTEM "Linux"
    #else
        #define SYSTEM "Mac OS"
    #endif
#else
    #error "Operational system not recognized, terminating program!!"
#endif

void revCmd(char *instruction) {
    
    if (*instruction == '\0') {
        char *string = calloc(MAX_CHAR, sizeof(char));

        if (!string) {
            printf("Error: memory allocation Error!!\n");
            return;
        }

        uint8_t appear = 1;

        while (true) {
            if (appear) {
                printf("Reading from the input, type 'stop' or 'quit' to exit ");
                puts("and use 'clear' or 'cls' to clear the screen and the scrollback buffer\n");
            }

            appear = 0;

            fgets(string, MAX_CHAR, stdin);
            
            if (*string == '\0') {
                printf("Error: insert a string!\n\n");
                continue;
            }
            
            string[strcspn(string, "\n")] = '\0';
            removeComments(string);
            
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
                SAFE_FREE(string);
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
                printf("Error: currently it does not support paths\n");
                return;
            }

            if (!isValidFolderOrFileName(sourceFile) ||
                !isValidFolderOrFileName(destFile)) {
                printf("Error: invalid file name\n");
                return;
            }

            FILE *source = fopen(sourceFile, "r");

            if (!source) {
                printf("Error: '%s' does not exist!\n", sourceFile);
                return;
            }


            char *line = calloc(MAX_CHAR, sizeof(char));

            FILE *dest = fopen(destFile, "w");
            if (!dest) {
                printf("Error: could not create '%s'\n", destFile);
                SAFE_FCLOSE(source);
                SAFE_FREE(line);
                return;
            }

            while (fgets(line, MAX_CHAR, source)) {
                trim(line);
                trimEnd(line);

                size_t len = strlen(line);
                int has_nl = (len > 0 && line[len - 1] == '\n');

                if (has_nl)
                    line[len - 1] = '\0';

                char *rev = revStr(line);
                if (!rev) break;

                fputs(rev, dest);
                if (has_nl)
                    fputc('\n', dest);

                SAFE_FREE(rev);
            }

            SAFE_FREE(line);
            SAFE_FCLOSE(source);
            SAFE_FCLOSE(dest);
        } else {

            uint8_t isPath = (strchar(instruction, '/') != -1 || strchar(instruction, '/') != -1);

            if (isPath) {
                printf("Error: currently it does not support paths\n");
                return;
            }

            if (!isValidFolderOrFileName(instruction)) {
                printf("Error: invalid file name\n");
                return;
            }

            FILE *source = fopen(instruction, "r");

            if (!source) {
                printf("Error: '%s' does not exist\n", instruction);
                return;
            }

            char *line = calloc(MAX_CHAR, sizeof(char));

            if (!line) {
                printf("Error: memory allocation error!!\n");
                return;
            }

            while (fgets(line, MAX_CHAR, source)) {
                trim(line);
                trimEnd(line);

                size_t len = strlen(line);
                int has_nl = (len > 0 && line[len - 1] == '\n');

                if (has_nl)
                    line[len - 1] = '\0';

                char *rev = revStr(line);
                if (!rev) break;

                puts(rev);

                SAFE_FREE(rev);
            }

        }

    }
}

char *randstrCmd(char *instruction) {
    double len = 1;

    if (*instruction != '\0') {

        if (instruction[0] != '-') {
            printf("randstr: invalid argument: '%s'\n", instruction);
            return NULL;
        }

        if (instruction[1] == '-') {
            if (strncmp(instruction, "--len", 5) == 0)
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

    if (len == U64_NAN) {
        errno = EINVAL;
        perror("Error");
        return NULL;
    }

    if (len <= 0 || len >= 65536) {
        printf("Error: length must be > 0 and < 65536\n");
        return NULL;
    }

    if (ceil(len) != len) {
        printf("Error: must be integer!\n");
        return NULL;
    }

    char *str = malloc(len + 1);
    if (!str) {
        puts("Error: memory allocation error!!");
        return NULL;
    }

    for (int i = 0; i < len; i++)
        str[i] = randChr();

    str[(uint16_t)len] = '\0';
    return str;
}

void sleepCmd(char *instruction) {

    if (*instruction == '\0') {
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

    double time = eval(instruction, true);

    if (isnan(time) || time == U64_NAN)
        return;

    time *= unit;
        
    if (time < 0) {
        puts("Error: must be greater than 0");
        return;
    }

    sleepF(time);
}

int32_t lcCmd(char *instruction) {
    char buffer[0x400];
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

void bashCmd(uint16_t argc, char **argv, bool insideBash) {
    if (argc < 2) 
        return;

    uint8_t flags = 0;

    for (uint16_t i = 1; i < argc; i ++) {

        if (argv[i][0] != '-') {
            printf("bash: invalid argument: '%s'\n", argv[i]);
            return;
        }

        if (argv[i][1] == '-') {
            if (strcmp(argv[i], "--version") == 0)
                flags |= BASH_VERSION;
            else if (strcmp(argv[i], "--help") == 0)
                flags |= BASH_HELP;
            else if (strcmp(argv[i], "--all") == 0)
                flags |= BASH_ALL;
            else
                printf("bash: invalid option: '%s'\n", argv[i]);
        } else {
            for (uint16_t j = 1; argv[i][j]; j++) {
                char opt = tolower((unsigned char)argv[i][j]);
                switch (opt) {
                    case 'a': flags |= BASH_ALL; break;
                    case 'h': flags |= BASH_HELP; break;
                    case 'v': flags |= BASH_VERSION; break;
                    default:
                        printf("bash: invalid option: '-%c'\n", argv[i][j]);
                        return;
                }
            }
        }
    }

    if (flags & BASH_VERSION) {
        printf("lsw - Gab-OS  %s\n", VERSION);
    }

    if ((flags & BASH_HELP) && insideBash) {
        printf("'bash' shows the shell information\n\nbash [OPTION...]\n\nOptions:\n");
        printf("\t'-v', '--version'   show version information\n"
               "\t'-h', '--help'      display manual\n"
               "\t'-a', '--all'       displays everything\n");
    } else if ((flags & BASH_HELP) && !insideBash) {
        printf("You can run commands using 'lsw [COMMAND...]', or you can use options 'lsw [OPTION...]', lsw is just an exemple, ");
        printf("it may differ if you choose a different name to save on the path, you can use quotes and spaces to separate arguments\n");
        printf("\nOptions:\n");
        printf("\t'-v', '--version'   show version information\n"
               "\t'-h', '--help'      display manual\n"
               "\t'-a', '--all'       displays everything\n");
    }

}

void renameCmd(char *instruction) {
    if (*instruction == '\0') {
        puts("rename: missing operand\nUse \"man rename\" to check the manual");
        return;
    }

    trim(instruction);
    trimEnd(instruction);
    
    char *oldName = extractPath(&instruction);
    if (!oldName || *oldName == '\0') {
        puts("rename: missing operand\nUse \"man rename\" to check the manual");
        return;
    }

    trim(instruction);
    trimEnd(instruction);

    char *newName = extractPath(&instruction);
    if (!newName || *newName == '\0') {
        puts("rename: missing operand\nUse \"man rename\" to check the manual");
        return;
    }

    if (strchr(oldName, '/') || strchr(newName, '/')) {
        puts("rename: renaming across directories is not allowed");
        return;
    }

    if (rename(oldName, newName) != 0)
        perror("Error");

}

void manCmdMulti(char *instruction, const char **cmds, uint8_t isInsideBash) {
    char buffer[0x400];
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

    for (int i = 0; i < cmdCount; i++) {
        manCmd(commands[i], cmds, isInsideBash);
        
        if (i < cmdCount - 1) {
            puts("\n────────────────────────────────────────────────────────────────────────\n");
        }
    }

}

void clearHistoryCmd(const char *path) {
    char answer[0x20];
    
    printf("Are you sure you want to delete 'history.txt'? (y/n): ");
    if (!fgets(answer, sizeof(answer), stdin)) {
        puts("Error reading input");
        return;
    }
    answer[strcspn(answer, "\n")] = '\0';

    answer[strcspn(answer, "\n")] = '\0';
    safe_lower_inplace(answer);
    removeComments(answer);

    if (answer[0] == 'y' && (answer[1] == '\0' || answer[1] == ' ')) {

        if (remove(path) != 0) {
            perror("Error");
        } else {
            puts("'history.txt' deleted successfully");
        }

    } else {
        puts("Deletion cancelled");
    }
}

void bcCmd(uint16_t argc, char **argv, const char **cmds) {
    setup_console();
    double result;
    uint8_t appear = 0;

    uint8_t flags = 0;

    for (uint16_t i = 1; i < argc; i++) {
        char *opt = argv[i];

        if (opt[0] != '-') {
            printf("bc: invalid argument: '%s'\n", opt);
            return;
        }

        if (opt[1] == '-') {
            if (strcmp(opt, "--quiet") == 0)
                flags |= BC_QUIET;
            else if (strcmp(opt, "--mathlib") == 0)
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

    char *operation = calloc(MAX_CHAR, sizeof(char));
    
    if (!operation) {
        puts("Error: Allocation error!!");
        return;
    }


    while (true) {
        if (!appear && !quiet) {
            printf("A simple calculator command, so far it only works with 2 numbers, type 'quit' or 'exit' to exit\n");
            printf("type 'man' to check the manual inside the calculator, otherwise use 'man bc'\n");
            printf("it no longer supports comma instead of dots and type 'clear' or 'cls' to clear the screen and scrollback buffer");
            printf("\nPS: mathlib is off by default, type 'mathlib' to turn it on/off "
                   "if you're inside the terminal, otherwise use 'bc -l' or 'bc --mathlib', it enables functions and "
                   "binary, hexadecimal and octal numbers\n");
            printf("Mathlib status: ");
            if (mathlib)
                printc("on\n\n", GREEN, WHITE);
            else 
                printc("off\n\n", RED, WHITE);
        }


        appear = 1;
        result = NAN;
        
        fgets(operation, MAX_CHAR, stdin);
        operation[strcspn(operation, "\n")] = '\0';

        if (*operation == '\0') {
            puts("Error: insert an operation\n");
            continue;
        }

        removeComments(operation);

        trim(operation);
        trimEnd(operation);

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

        } else if (isValidBcCommand(operation, "man")) {
            puts("\n────────────────────────────────────────────────────────────────────────────────\n");
        
            manCmdMulti("bc", cmds, true);
            
            puts("\n────────────────────────────────────────────────────────────────────────────────\n");
        
            continue;
        }
        else if (strncmp(operation, "hex", 3) == 0 && mathlib) {
            char *value = s_hex(operation);
            if (value) {
                printf("%s\n\n", value);
                fflush(stdout);
            }

            SAFE_FREE(value);
            continue;
        }
        else if (strncmp(operation, "oct", 3) == 0 && mathlib) {
            char *value = s_oct(operation);
            if (value) {
                printf("%s\n\n", value);
                fflush(stdout);
            }

            SAFE_FREE(value);
            continue;
        }
        else if (strncmp(operation, "bin", 3) == 0 && mathlib) {
            char *value = s_bin(operation);
            if (value) {
                printf("%s\n\n", value);
                fflush(stdout);
            }

            SAFE_FREE(value);
            continue;
        }

        result = eval(operation, mathlib);

        if (!isnan(result) && result != U64_NAN) {
            printf("%g\n\n", result);
            fflush(stdout);
        } else if (result == U64_NAN)
            puts("");
    }
    SAFE_FREE(operation);
}

void grepCmd(char *instruction) {

    char buffer[0x400];
    strncpy(buffer, instruction, sizeof(buffer));
    buffer[sizeof(buffer)-1] = '\0';

    
    int8_t ignoreCase = 0;
    char *option;
    char *rest = buffer;

    if (buffer[0] == '-') {
        option = strtok(buffer, " ");
        rest = strtok(NULL, "");

        if (option && option[1] == '-') {
            if (strcmp(option, "--ignore-case") == 0)
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

    if (!file || *file == '\0') {
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
        printf("pattern '%s' not found in '%s'\n", pattern, file);

    SAFE_FCLOSE(f);
}

void historyCmd(const char *path) {
    FILE *f = fopen(path, "r");

    if (!f) {
        perror("Error");
        return;
    }

    uint32_t lineCount = 0;

    //* temporary, ig
    // char **lines = readHistory(path, &lineCount);

    // if (!lines) {
    //     puts("Error: failed to read history file");
    //     return;
    // }

    // for (uint32_t i = 0; i < lineCount; i++) {
    //     charReplace(lines[i], '\n', '\0');
    //     printf("%05u  %s\n", i + 1, lines[i]);
    // }

    // for (uint32_t i = 0; i < lineCount; i++)
    //     SAFE_FREE(lines[i]);

    // SAFE_FREE(lines);

    char buffer[0x400];
    while (fgets(buffer, sizeof(buffer), f)) {
        buffer[strcspn(buffer, "\n")] = '\0';

        printf("%5u  %s\n", lineCount, buffer);
        lineCount++;
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

        if (arg[0] == '-') {
            if (!strcmp(arg, "-f") || !strcmp(arg, "--force")) {
                flags |= RM_FORCE;
            }
            else if (!strcmp(arg, "-i") || !strcmp(arg, "--interactive")) {
                flags &= ~RM_FORCE;
            }
            else if (!strcmp(arg, "-b") || !strcmp(arg, "--recycle-bin")) {
                flags |= RM_BIN;
            }
            else {
                printf("rm: invalid option '%s'\n", arg);
                return;
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
            puts("Error reading input");
            return;
        }

        answer[strcspn(answer, "\n")] = '\0';
        safe_lower_inplace(answer);

        if (answer[0] != 'y') {
            puts("Deletion cancelled");
            return;
        }
    }

    for (uint16_t i = 0; i < fileCount; i++) {
        if (rm_delete(argv[i], flags) != 0) {
            perror(argv[i]);
        }
        else {
            if (flags & RM_BIN)
                printf("'%s' moved to recycle bin\n", argv[i]);
            else
                printf("'%s' deleted successfully\n", argv[i]);
        }
    }
}

void touchCmd(char *instruction) {

    if (*instruction == '\0') {
        puts("touch: missing operand\nUse \"man touch\" to check the manual");
        return;
    }
    
    trim(instruction);

    int16_t string = strrchar(instruction, '<');
    int16_t reps = strrchar(instruction, '*');


    char *copy = strdup(instruction);
    char *save;


    if (string == -1) {

        if (!isValidFolderOrFileName(instruction)) {
            printf("Error: invalid file name\n");
            return;
        }

        FILE *f = fopen(instruction, "w");
        SAFE_FCLOSE(f);
    } else if (string != -1 && reps ==  -1){
        char *filename = strtok_r(instruction, "<", &save);
        char *inFile = strtok_r(NULL, ">", &save);
    
        trim(inFile);
        trimEnd(inFile);
    
        trim(filename);
        trimEnd(filename);

        if (!isValidFolderOrFileName(filename)) {
            printf("Error: invalid file name\n");
            SAFE_FREE(copy);
            return;
        }

        echoHandler(inFile);

        FILE *f = fopen(filename, "w");
    
        fprintf(f, "%s", inFile);

        SAFE_FCLOSE(f);
    } else {
        char *filename = strtok_r(copy, "<", &save);
        char *inFile = strtok_r(NULL, "<", &save);

        char *test = strdup(inFile);
        trim(test);

        char *str = strtok_r(inFile, "*", &save);
        char *num = strchr(instruction, '*');

        num = strtok(num, "<");

        char *cpy = strdup(filename);

        filename = strdup(cpy);

        trim(str); trimEnd(str);    
        trim(filename); trimEnd(filename);

        if (!isValidFolderOrFileName(filename)) {
            printf("Error: invalid file name\n");
            SAFE_FREE(copy);
            SAFE_FREE(test);
            return;
        }

        printf("Instruction: '%s'\n", instruction);

        bool QuoteAfterStar = (reps < strrchar(instruction, '\"') ||
                               reps < strrchar(instruction, '\''));

        double count;

        if (!QuoteAfterStar) {
            count = eval(num+1, true);
    
            if (count == U64_NAN) {
                SAFE_FREE(copy);
                SAFE_FREE(test);
                return;
            }
    
            if (count <= 0) {
                errno = EINVAL;
                perror("Error");
                SAFE_FREE(copy);
                SAFE_FREE(test);
                return;
            }
    
            
            if (ceil(count) != count) {
                printf("Error: must be integer\n");
                SAFE_FREE(copy);
                SAFE_FREE(test);
                return;
            }
        }

        echoHandler(str);

        FILE *f = fopen(filename, "w");

        if (QuoteAfterStar) {
            echoHandler(test);
            fputs(test, f);
        } else
            printInFileNTimes(f, str, count);

        SAFE_FCLOSE(f);

        SAFE_FREE(test);
    }

    SAFE_FREE(copy);
}

void catCmd(char *instruction, uint32_t max_lines, const char *cmdName) {
    if (*instruction == '\0') {
        printf("%s: missing operand\nUse \"man %s\" to check the manual\n", cmdName, cmdName);
        return;
    }

    trim(instruction);

    FILE *f = fopen(instruction, "rb");
    if (!f) {
        fprintf(stderr, "Error: couldn't open %s\n", instruction);
        return;
    }

    char line[4096];
    uint32_t lines = 0;

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

void tailCmd(char *instruction, uint32_t max_lines) {

    if (!instruction || *instruction == '\0') {
        puts("tail: missing operand\nUse \"man tail\" to check the manual");
        return;
    }

    char path[0x400];
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

void rmdirCmd(char *instruction) {

    char *option;
    uint8_t flags = 0;

    char *args = instruction;

    if (args[0] == '-') {
        char *end = args;

        while (*end && *end != ' ')
            end++;

        size_t len = end - args;

        char option[0x40];
        if (len >= sizeof(option)) len = sizeof(option) - 1;
        memcpy(option, args, len);
        option[len] = '\0';

        if (option[1] == '-') {
            if (strcmp(option, "recycle-bin") == 0)
                flags |= RM_BIN;
            else {
                printf("rmdir: invalid option: '%s'\n", option);
                return;
            }
        } else {
            for (uint16_t i = 1; i < option[i]; i++) {
                char opt = tolower((unsigned char)option[i]);
                switch (opt) {
                    case 'b': flags |= RM_BIN; break;
                    default:
                        printf("rmdir: invalid option: '-%c'\n", option[i]);
                        return;
                }
            }
        }

        args = end;
        while (*args == ' ') args++;
    }

    char buffer[0x400];
    strncpy(buffer, args, sizeof(buffer));
    buffer[sizeof(buffer)-1] = '\0';

    char *rest = buffer;

    if (*rest == '\0') {
        puts("rmdir: missing operand\nUse \"man rmdir\" to check the manual");
        return;
    }

    uint16_t fileCount = 0;
    char **files = parseData(rest, &fileCount);

    if (fileCount == 0) {
        puts("rmdir: missing operand\nUse \"man rmdir\" to check the manual");
        return;
    }

    if (flags & RM_BIN)
        for (uint16_t i = 0; i < fileCount; i++) {
            if (!move_to_trash(files[i])) {
                printf("Error: cannot move '%s' to the recycle bin: operation failed\n", files[i]);
            } else {
                printf("'%s' moved to recycle bin\n", files[i]);
            }
            SAFE_FREE(files[i]);
        }
    else
        for (uint16_t i = 0; i < fileCount; i++) {
            if (rmdir(files[i]) != 0) {
                perror(files[i]);
            } else {
                printf("'%s' deleted successfully\n", files[i]);
            }
            SAFE_FREE(files[i]);
        }

    SAFE_FREE(files);
}

void mkdirCmd(char *command) {
    
    char buffer[0x400];
    strncpy(buffer, command, sizeof(buffer));
    buffer[sizeof(buffer)-1] = '\0';

    char *rest = buffer;

    if (rest[0] == '\0') {
        puts("mkdir: missing operand\nUse \"man mkdir\" to check the manual");
        return;
    }

    uint16_t fileCount = 0;
    char **files = parseData(rest, &fileCount);

    if (fileCount == 0) {
        puts("mkdir: missing operand\nUse \"man mkdir\" to check the manual");
        return;
    }

#ifdef _WIN32
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
    
#ifdef _WIN32
    charReplace(path, '\\', '/');
#endif
    
    if (strcmp(path, "-") == 0) {
        SAFE_FREE(raw);
        return handle_cd_dash(address);
    }

    update_last_directory(address);

    char *buffer = NULL;

    if (path[0] == '~') {
        char *Default = get_default_address();

        path[0] = ' ';
        trim(path);

        buffer = malloc(strlen(Default) + strlen(path) + 1);
        buffer[0] = '\0';
        strcat(buffer, Default);
        strcat(buffer, path);

        SAFE_FREE(Default);
    }

    char *final = buffer ? buffer : path;
    char *result = handle_normal_cd(final, address);

    SAFE_FREE(raw);
    if (buffer) SAFE_FREE(buffer);

    return result;
}

void listDrives(void) {
#ifdef _WIN32
    DWORD drives = GetLogicalDrives();
    if (drives == 0) {
        puts("Error: could not get logical drives");
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
        perror("Error opening /media directory");
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
        puts("  (no mounted drives found)");

    closedir(dir);
#endif
}

void cmdsCommand(const char **cmds, uint16_t count, uint8_t isInsideBash) {
    static char **copy = NULL;
    static int8_t initialized = 0;

    if (!initialized) {
        copy = malloc(count * sizeof(char *));
        for (uint16_t i = 0; i < count; i++)
            copy[i] = strdup(cmds[i]);

        bsort(copy, count);
        initialized = 1;
    }

    for (uint16_t i = 0; i < count; i++) {
        if (strcmp(copy[i], "bash") == 0 && !isInsideBash)
            continue;

        puts(copy[i]);
    }

    count = (isInsideBash) ? count : count - 1;

    printf("\n\nTotal commands: %"PRIu16"\n", count);
}

void updatehistory(void) {
    const char *logs[] = {
        "a0.0.4 - terminal creation\n\tAdded: clear, echo and exit command\n",
        "a0.0.55 - minor changes\n\tAdded: neofetch cmd\n",
        "a0.0.75 - minor changes\n\tAdded: history command\n",
        "a0.0.8 - minor changes\n\tEdited: optimized the sort algorithm\n",
        "a0.0.95 - minor changes\n\tEdited: now the neofetch menu displays the cpu and mem\n",
        "a0.0.975 - minor changes\n\tRemoved: system instruction\n",
        "a0.1.35 - big changes\n\tAdded: cd instruction\n",
        "b0.1.55 - big changes\n\tAdded: ls instruction\n",
        "b0.1.6 - minor changes\n\tAdded: 'initguid.h' and 'knownfolders.h' windows libs \n",
        "b0.1.65 - minor changes\n\tEdited: neofetch function\n",
        "b0.1.82 - big changes\n\tAdded: man command\n\tEdited: now neofetch displays the author of this terminal\n",
        "b0.2.0 - big changes\n\tEdited: get_time function and neofetch command\n",
        "b0.2.1 - big changes\n\tAdded: date command\n",
        "b0.2.23 - big changes\n\tAdded: pwd command\n",
        "b0.2.34 - big changes\n\tAdded: mkdir and rmdir command\n",
        "b0.2.36 - minor changes\n\tEdited: now neofetch displays the host\n",
        "b0.2.71 - big changes\n\tAdded: cat, touch and rm command\n",
        "b0.2.73 - minor changes\n\tEdited: echo cmd, changed how it works\n",
        "b0.2.78 - minor changes\n\tEdited: now you can write in files with the echo cmd\n",
        "b0.2.8 - minor changes\n\tEdited: now the touch command can also write in the file at the same time it's being created\n",
        "b0.2.91 - minor changes\n\tAdded: history command\n",
        "b0.2.912 - minor changes\n\tAdded: uname command\n",
        "b0.3.14 - big changes\n\tFixed: history command\n\tAdded: grep command\n",
        "b0.3.1415 (pi) - small changes\n\tEdited: optimized the command and manual identifier\n",
        "b0.3.33 - minor changes\n\tEdited: now you can multiply the text to how many times you want with touch and echo command\n",
        "b0.3.66 - big changes\n\tAdded: bc command\n",
        "b0.3.92 - big changes\n\tEdited: now it handles a lot more of operations\n",
        "b0.4.0 - minor changes\n\tEdited: the neofetch command now displays the line number of the code\n",
        "b0.4.1 - minor changes\n\tEdited: now the uname command has more options\n",
        "b0.4.37 - big changes\n\tAdded: a lot of new operators and functions to the bc command\n",
        "b0.4.5 - small changes\n\tAdded: floor(), ceil(), round() and fact() function to the bc command\n",
        "b0.4.59 - small changes\n\tAdded: drives command\n",
        "b0.4.62 - small changes\n\tEdited: added -i to grep command\n",
        "b0.4.7 - small changes\n\tEdited: added clear, deg and rad to bc command\n",
        "b0.4.89 - big changes\n\tAdded: terminal command\n",
        "b0.4.97 - small changes\n\tAdded: clearhistory and rand command\n",
        "b0.5.01 - minor changes\n\tEdited: optimized most of the 's_math.c' code\n",
        "b0.5.15 - small changes\n\tAdded: now you can run more than one command using '&&'\n",
        "b0.5.17 - minor changes\n\tEdited: now the terminal shows the total amount of commands and logs\n",
        "b0.5.2 - minor changes\n\tEdited: created 'CheckCmd.c' and 'CheckCmd.h' to handle the commands and eval functions\n",
        "b0.5.3 - small changes\n\tAdded: rand() function to the calculator\n",
        "b0.5.53 - big changes\n\tAdded: alias command\n",
        "b0.5.57 - minor changes\n\tFixed:  now the cd aliases works properly on windows\n",
        "b0.5.6 - minor changes\n\tEdited: now you cand check the manual of multiple commands at once\n",
        "b0.5.65 - minor changes\n\tEdited: now echo and touch breaks lines at each space if the string doesn't have any quotations marks at the beginning and in the end\n",
        "b0.5.7 - big changes\n\tAdded: rename command\n",
        "b0.5.72 - minor changes\n\tEdited: echoHandler() now works properly\n",
        "b0.5.75 - big changes\n\tEdited: improved alias to work more like on linux\n\tFixed: echo command\n",
        "b0.5.76 - minor changes\n\tEdited: changed echoHandler()\n",
        "b0.5.79 - minor changes\n\tEdited: changed int variables to optimize the code, using 'inttypes.h'\n",
        "b0.5.85 - small changes\n\tEdited: now on windows you can generate 32bit numbers with rand command, or rand() in the calculator\n",
        "b0.5.9 - small changes\n\tEdited: now the calculator supports hex and octal numbers\n",
        "b0.6.0 - small changes\n\tAdded: hex(), int() and oct() to the calculator\n",
        "b0.6.07 - minor changes\n\tEdited: now echo and touch supports hex and oct numbers and the calculator was improved\n\n"
        "b0.6.15 - big changes\n\tEdited: improved ls command\n",
        "b0.6.18 - small changes\n\tEdited: improved functionHandler()\n",
        "b0.6.21 - minor changes\n\tEdited: improved echoCmd() and touchCmd()\n",
        "b0.6.24 - small changes\n\tAdded: euler constant\n",
        "b0.6.26 - big changes\n\tAdded: bash command (available if you're inside the terminal)\n",
        "b0.6.29 - minor changes\n\tEdited: a few tweaks to the calculator, now you can use -PI or -E, still not case sensitive\n",
        "b0.6.35 - small changes\n\tEdited: now to convert octal to integer or hex to integer, you just type it in\n\tRemoved: int() calculator function\n",
        "b0.6.39 - minor changes\n\tEdited: a few more tweaks to the calculator\n",
        "b0.6.45 - small changes\n\tEdited: improved sin(), cos() and tan() suffix identifier\n",
        "b0.6.5 - small changes\n\tEdited: now you can type things like '3pi' or '-pi' and it will work, including hex and oct numbers\n",
        "b0.6.52 - minor changes\n\tEdited: replaced some of the strtok to strtok_r, which is thread safe\n",
        "b0.6.58 - small changes\n\tFixed: now you can echo with another command using '&&' without any bugs\n",
        "b0.6.65 - small changes\n\tFixed: an annoying asf calculator bug\n",
        "b0.6.75 - big changes\n\tAdded: now you can choose the difference with the sum() function\n\tRemoved: that stupid thing of printing in columns of the ls command\n",
        "b0.6.78 - small changes\n\tFixed: now RAND_MAX keyword works properly\n",
        "b0.6.89 - changes\n\tAdded: more options to grep, uname and rm command\n\tEdited: now you can remove multiple files or empty folders using the rm and rmdir command\n",
        "b0.6.94 - small changes\n\tAdded: K, M, B and T suffixes for the numbers on the calculator (not case sensitive) and also 'exit' is now a valid calculator command\n",
        "b0.7.0 - small changes\n\tEdited: now you can create multiple folders with mkdir command\n",
        "b0.7.1 - big changes\n\tFixed: thought that only echo was affected by the '&&' bug, but it was every single command, it's now fixed\n",
        "b0.7.22 - big changes\n\tEdited: now bc and uname can combine options\n\tAdded: options to bc command\n",
        "b0.7.3 - small changes\n\tAdded: head and tail command\n",
        "b0.7.33 - small changes\n\tEdited: improved the neofetch ascii art\n",
        "b0.7.37 - small changes\n\tFixed: now '&&' should work properly...\n",
        "b0.7.4 - minor changes\n\tEdited: now you use multiple commands and change disk at the same time on windows\n",
        "b0.7.46 - minor changes\n\tAdded: comment support\n",
        "b0.7.53 - big changes\n\tAdded: lc command\n",
        "b0.7.57 - small changes\n\tAdded: fah() and cel() functions to the calculator\n",
        "b0.7.65 - big changes\n\tEdited: now lc works with multiple files, adding their line count\n",
        "b0.7.74 - big changes\n\tEdited: now the prompt looks just like linux\n",
        "b0.7.8 - small changes\n\tEdited: now the neofetch displays kernel release and architecture\n",
        "b0.7.83 - small changes\n\tAdded: bin() function to the calculator, it support binary numbers as well\n",
        "b0.7.89 - big changes\n\tEdited: changed prompt to avoid confusion\n",
        "b0.7.92 - minor changes\n\tRemoved: now the calculator no longer supports comma instead of dot\n",
        "b0.8.0 - big changes\n\tEdited: Now mathmatical expressions and another functions work inside some functions\n",
        "b0.8.1 - small changes\n\tEdited: now sin(), cos(), tan() and scale() works with another functions and operations\n",
        "b0.8.13 - minor changes\n\tEdited: now you make operations with the result of functions, i think everything else works as usual, at least i hope so\n",
        "b0.8.17 - small changes\n\tAdded: root() function to the calculator\n",
        "b0.8.20 - small changes\n\tRemoved: terminal command\n\tEdited: now to write in the files using touch, instead of '>' it is '<' now, and also changed the **argv thingy\n",
        "b0.8.25 - big changes\n\tAdded: randf() function\n\tEdited: rand() now generates integers\n",
        "b0.8.37 - big changes\n\tAdded: mi(), km(), lb() and kg() functions to the calculator\n",
        "b0.8.4 - minor changes\n\tEdited: edited echoHandler() with the new function 'trimBetween()', and now createShortcut() now has an implementation of that same function\n",
        "b0.8.45 - minor changes\n\tAdded: more options to uname command\n\tEdited: neofetch now shows the size in megabytes\n",
        "b0.8.54 - big changes\n\tEdited: now functions should work properly with another functions inside\n",
        "b0.8.59 - minor changes\n\tAdded: yes command\n",
        "b0.8.71 - big changes\n\tAdded: sleep command\n",
        "b0.8.82 - big changes\n\tAdded: randstr command\n",
        "b0.9.0 - big changes\n\tAdded: rev command\n\tEdited: now the manual for a few commands gives more explanations\n",
        "b0.9.07 - minor changes\n\tAdded: file name verification\n\tEdited: now randChr() is able to randomize numbers\n",
        "b0.9.12 - minor changes\n\tEdited: optimized the code\n",
        "b0.9.17 - small changes\n\tAdded: '-o' option to uname\n",
        "b0.9.28 - big changes\n\tEdited: now instead of unknown, the size and lines show a approximated number\n",
        "b0.9.35 - small changes\n\tEdited: improved man\n",
        "b0.9.39 - minor changes\n\tEdited: now neofetch displays the creation date\n",
        "b0.9.51 - big changes\n\tEdited: the calculator now works properly when dealing with the wrong data type\n",
        "b0.9.57 - small changes\n\tEdited: oct() function now works properly\n",
        "b0.9.63 - minor changes\n\tRemoved: some useless functions from the source code\n",
        "b0.9.7 - big changes\n\tEdited: improved the sleep() function, now it has more precision and works with sigle point precision numbers\n",
        "b0.9.76 - small changes\n\tEdited: in neofetch KERNEL -> KERNEL-RELEASE + KERNEL-VERSION\n",
        "b0.9.85 - big changes\n\tFixed: now commands that randomizes values works properly outside the terminal\n",
        "b0.9.89 - minor changes\n\tEdited: now the source code is a little more safe\n",
        "b0.9.95 - small changes\n\tFixed: now echo and touch works a lot better when multiplying strings\n",
        "r1.0.4 - big changes\n\tEdited: edited the calculator initial message\n",
        "r1.0.5 - big changes\n\tEdited: file headers organization\n",
        "r1.0.54 - minor changes\n\tEdited: now the source code is safer\n",
        "r1.0.6 - big changes\n\tEdited: made some preparations for the future update\n",
        "r1.0.64 - minor changes\n\tFixed: echo and touch behavior when multiplying strings with quotes\n",
        "r1.0.69 - small changes\n\tAdded: help message when initializing the program\n",
        "r1.0.8 - big changes\n\tEdited: improved echo behavior once again\n\tFixed: freed some pointers that I had forgotten to and also the sleep suffix identifier\n",
        "r1.0.85 - small changes\n\tRemoved: Kernel version from neofetch\n",
        "r1.1.0 - big changes\n\tAdded: now rm can move to the recycle bin\n",
        "r1.1.1 - minor changes\n\tEdited: time format\n",
        "r1.1.13 - minor changes\n\tEdited: stop and clean in rev\n",
        "r1.1.18 - small changes\n\tFixed: now you can use hex(), oct() or bin() as parameters\n",
        "r1.1.23 - minor changes\n\tEdited: factored the math code\n",
        "r1.1.3 - big changes\n\tAdded: rmdir can now also move to the recycle bin\n",
        "r1.1.34 - small changes\n\tFixed: early freed pointers\n",
        "r1.1.36 - minor changes\n\tEdited: bc manual\n",
        "r1.1.45 - big changes\n\tEdited: improved the option identifier for all commands\n",
        "r1.1.51 - small changes\n\tEdited: optimized the history command since that 'future update' isn't coming any time soon\n",
        "r1.1.6 - big changes\n\tAdded: now the terminal works with commands with spaces, using quotes, e.g: '[COMMAND WITH SPACES]' [ARGS...]\n",
        "r1.1.63 - minor changes\n\tEdited: linesNumber() refactor\n",
        "r1.1.7 - big changes\n\tFixed: bc, rm and uname seg-fault\n",
        "r1.1.79 - big changes\n\tAdded: seg-fault message for windows\n",
        "r1.1.83 - small changes\n\tEdited: uname and randstr option identifier\n",
        "r1.1.92 - big changes\n\tEdited: now single characters options are no longer case sensitive, and also upgraded the file/folder name verification\n",
        "r1.2.0 - minor changes\n\tEdited: fact() function\n",
        "r1.2.1 - small changes\n\tFixed: man seg-fault\n",
        "r1.2.16 - small changes\n\tEdite: echo function refactor\n",
        "r1.2.27 - big changes\n\tAdded: append in echo command\n",
        "r1.2.34 - small changes\n\tEdited: echoHandler()\n",
        "r1.2.43 - big changes\n\tAdded: help option to bash command\n\tEdited: bash now uses argc and argv\n",
        "r1.2.48 - small changes\n\tAdded: all option to bash command\n"
    };

    uint16_t logCount = sizeof(logs) / sizeof(logs[0]);

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

            if (opt[0] != '-') {
                printf("uname: invalid argument: '%s'\n", opt);
                return NULL;
            }

            if (opt[1] == '-') {
                if (strcmp(opt, "--all") == 0) {
                    flags = U_ALL;
                }
                else if (strcmp(opt, "--kernel-name") == 0) {
                    flags |= U_KERN_NAME;
                }
                else if (strcmp(opt, "--kernel-release") == 0) {
                    flags |= U_KERN_RELEASE;
                }
                else if (strcmp(opt, "--machine") == 0) {
                    flags |= U_MACHINE;
                }
                else if (strcmp(opt, "--kernel-version") == 0) {
                    flags |= U_KERN_VERSION;
                }
                else if (strcmp(opt, "--nodename") == 0) {
                    flags |= U_HOST_NAME;
                }
                else if (strcmp(opt, "--operating-system") == 0) {
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

#ifdef _WIN32
    return unameCmdWin(flags);
#else
    return unameCmdLinux(flags);
#endif
}

void echoCmd(char *instruction) {

    if (*instruction == '\0') {
        putchar('\n');
        return;
    }

    int8_t file = strrchar(instruction, '>');
    int8_t reps = strrchar(instruction, '*');

    char *copy = strdup(instruction);
    char *save;

    if (!copy) { perror("strdup"); return; }


    if (reps != -1 && file == -1) {
        if (!echoNtimes(instruction, copy, reps)) {
            return;
        }
    } else if (file == -1 && reps == -1) {

        echoHandler(copy);
        puts(copy);

        SAFE_FREE(copy);
        return;

    } else if (reps == -1 && file != -1) {

        char *inFile = strtok_r(copy, ">", &save);
        char *filename = strtok_r(NULL, ">", &save);

        if (!inFile || !filename) {
            puts("Error: invalid syntax");
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
                puts("Error: invalid redirection syntax");
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

        echoHandler(inFile);

        if (!isValidFolderOrFileName(filename)) {
            printf("Error: invalid file name\n");
            return;
        }

        printf("File name: '%s'\n", filename);
        printf("Mode: '%s'\n", mode);
        FILE *f = fopen(filename, mode);
        if (!f) {
            perror("fopen");
            SAFE_FREE(copy);
            return;
        }

        fprintf(f, "%s\n", inFile);
        SAFE_FCLOSE(f);

        SAFE_FREE(copy);
        return;
        
    } else if (reps != -1 && file != -1) {
        if (!echoFileNtimes(instruction, copy, reps, file)) {
            return;
        }
    }
}

void lsCmd(const char *option, const char *address) {
    const char *dirPath = (address && address[0]) ? address : ".";

    uint8_t flags = 0;

    if (option[0] == '-') {

        if (option[1] == '-') {
            if (strcmp(option, "--all") == 0)
                flags |= LS_ALL;
            else {
                printf("ls: invalid option: '%s'\n", option);
                return;
            }
        } else {
            for (uint16_t i = 1; option[i]; i++) {
                char opt = tolower((unsigned char)option[i]);
                switch (opt) {
                    case 'a': flags |= LS_ALL; break;
                    default:
                        printf("ls: invalid option: '-%c'\n", option[i]);
                        return;
                }
            }
        }

    } else if (*option != '\0' && option[0] != '-') {
        printf("Error: invalid argument: '%s'\n", option);
        return;
    }

    enableAnsiIfNeeded();

#ifdef _WIN32
    lsCmdWin(dirPath, flags);
#else
    lsCmdLinux(dirPath, flags);
#endif
}

void neofetchCmd(void) {
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



    uint8_t lines = sizeof(ascii_art) / sizeof(ascii_art[0]);
    
    color4_t title_color = YELLOW;
    color4_t label_color = LIGHT_CYAN;
    color4_t art_color = GREEN;
    color4_t art_bg_color = LIGHT_GREEN;

    for (uint8_t i = 0; i < lines; i++) {
        for (size_t j = 0; ascii_art[i][j]; j++) {
            unsigned char c = ascii_art[i][j];

            if ((c & 0xC0) != 0x80) {
                if ((unsigned char)ascii_art[i][j] == 0xE2 &&
                    (unsigned char)ascii_art[i][j+1] == 0x95) {
                    setColor(art_bg_color);
                } else {
                    setColor(art_color);
                }
            }

            putchar(ascii_art[i][j]);
        }
        putchar('\n');
    }
    
    printc("═══════════════════════════════════════════════════\n", title_color, WHITE);
    
    printc("SYSTEM\n", title_color, WHITE);
    
    printc("OS: ", label_color, 7);
    printf("%s ", SYSTEM);

#ifdef _WIN32
    puts(unameCmdWin(0b100));
#else
    puts(unameCmdLinux(0b100));
#endif


    printc("KERNEL-RELEASE: ", label_color, WHITE);
#ifdef _WIN32
    puts(unameCmdWin(0b10));
#else
    puts(unameCmdLinux(0b10));
#endif
    

    printc("───────────────────────────────────────────────────\n", title_color, WHITE);
    
    printc("INFO", title_color, WHITE);
    putchar('\n');
    
    printc("USER: ", label_color, WHITE);
    static char *userName;
    userName = get_user();

    puts(userName);


    printc("HOST: ", label_color, WHITE);
    static char *hostName;
    hostName = get_hostname();

    puts(hostName);


    printc("DATE: ", label_color, WHITE);
    static char *today;
    today = get_time(TIME_FMT);
    
    puts(today);


    printc("───────────────────────────────────────────────────\n", title_color, WHITE);

    printc("TERMINAL\n", title_color, WHITE);

    printc("LANGUAGES USED: ", label_color, WHITE);
    puts("C");


    printc("LINES OF CODE: ", label_color, WHITE);
    static char *linesNum; 
    linesNum =  linesNumber();

    puts(linesNum);
    

    printc("SIZE: ", label_color, WHITE);
    static char *size;
    size = charNumber();
    
    puts(size);

    
    printc("VERSION: ", label_color, WHITE);
    printf("lsw - Gab-OS  %s\n", VERSION);


    printc("CREATION DATE: ", label_color, WHITE);
    puts("10/18/2025");
    

    printc("AUTHOR: ", label_color, WHITE);
    printf("Gabriel Oliveira Miranda\n");

    printc("───────────────────────────────────────────────────\n", title_color, WHITE);

    printc("SPECS\n", title_color, WHITE);

    printc("CPU: ", label_color, WHITE);
    static char *cpuName;
    cpuName = get_cpu_model();

    puts(cpuName);


    printc("Memory: ", label_color, WHITE);
    static uint64_t memTotal;
    memTotal = get_total_ram_mb();
    
    printf("%"PRIu64"Mib\n", memTotal);

    printc("═══════════════════════════════════════════════════\n", title_color, WHITE);
}