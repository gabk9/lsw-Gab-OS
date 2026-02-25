#define _GNU_SOURCE
#include "utils.h"

#define PROJ_LINES_APPROX 8600
#define PROJ_SIZE_APPROX_BYTES 255000

#define RC_FILE "lswrc.txt"

#define PATH_MAIN_C "./main.c"
#define PATH_UTILS_C "./libs/utils.c"
#define PATH_UTILS_H "./libs/utils.h"
#define PATH_S_MATH_C "./libs/s_math.c"
#define PATH_S_MATH_H "./libs/s_math.h"
#define PATH_CHECKCMD_C "./libs/CheckCmd.c"
#define PATH_CHECKCMD_H "./libs/CheckCmd.h"
#define PATH_TERMINAL_C "./libs/terminal.c"
#define PATH_TERMINAL_H "./libs/terminal.h"


#ifdef _WIN32
    extern HANDLE hConsole;
#elif !defined(_WIN32) && !defined(__linux__) && !defined(__APPLE__) && !defined(__ANDROID__)
    #error "Operational system not recognized, terminating program!!"
#endif

static char *last_directory = NULL;

#ifdef _WIN32
LONG handler(EXCEPTION_POINTERS *e) {
    printf("Segmentation fault (core dumped)\n");
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

bool validStrBcFuncException(char *str, char *funcname) {
    char *test = strdup(str);
    if (!test) return false;

    charRm(test, ' ');

    size_t fn_len = strlen(funcname);

    if (strncmp(test, funcname, fn_len) != 0) {
        SAFE_FREE(test);
        return false;
    }

    if (test[fn_len] != '(') {
        SAFE_FREE(test);
        return false;
    }

    int32_t depth = 0;
    int32_t closing_index = -1;

    for (size_t i = fn_len; test[i]; i++) {
        if (test[i] == '(')
            depth++;
        else if (test[i] == ')') {
            depth--;
            if (depth == 0) {
                closing_index = (int32_t)i;
                break;
            }
        }

        if (depth < 0) {
            SAFE_FREE(test);
            return false;
        }
    }

    if (depth != 0) {
        SAFE_FREE(test);
        return false;
    }

    if (closing_index != (int32_t)strlen(test) - 1) {
        SAFE_FREE(test);
        return false;
    }

    SAFE_FREE(test);
    return true;
}

bool isBcVariable(const char *str) {

    if (isBetweenQuotes(str))
        return false;

    for (size_t i = 0; str[i]; i++) {
        if (str[i] == '$') {
            if (i > 0 && isdigit((unsigned char)str[i-1]))
                continue;
            if (isalpha((unsigned char)str[i+1]) || str[i+1] == '_')
                return true;
        }
    }
    return false;
}

bool isKeyRepeated(char *data_folder, const char *key_name) {
    char *path = buildLswRcPath(data_folder);

    bool foundKey = false;

    FILE *f = fopen(path, "r");

    if (!f)
        return false;

    char buffer[MAX_CHAR];
    while (fgets(buffer, sizeof(buffer), f)) {
        buffer[strcspn(buffer, "\n")] = '\0';
        removeComments(buffer);

        trim(buffer); trimEnd(buffer);

        if (!*buffer)
            continue;

        char *save;
        char *cmd = strtok_r(buffer, "=", &save);
        char *arg = strtok_r(NULL, "=", &save);

        trim(cmd); trimEnd(cmd);
        trim(arg); trimEnd(arg);

        bool isEqual = strcmp(cmd, key_name) == 0;

        if (isEqual && !foundKey)
            foundKey = true;
        else if (isEqual && foundKey) {
            SAFE_FCLOSE(f);
            SAFE_FREE(path);
            return true;
        }

    }

    SAFE_FCLOSE(f);
    SAFE_FREE(path);
    return false;
}

char *extractCommandOrKey(char *src, char **arg) {
    char *strCpy = strdup(src);
    
    char *cmd = extract_instruction(src, arg);
    if (strchr(cmd, '=')) {
        char *save;
        cmd = strtok_r(strCpy, "=", &save);
        *arg = strtok_r(NULL, "=", &save);
    }

    return cmd;
}

char *get_env_var(const char *name) {
    const char *val = getenv(name);
    if (!val) val = "Unknown";
    return strdup(val);
}

void saveHist(char *operation, char *history_path, char *data_folder) {
    char *path = buildLswRcPath(data_folder);
    uint16_t lines = getSavedHistSize(history_path);
    uint16_t MaxLines = getHistSizeConfig(path);
    MaxLines++;

    if (lines < MaxLines) {
        FILE *file = fopen(history_path, "a");
        if (!file) {
            printf("Error: could not open 'history.txt'\n");
            return;
        }

        fprintf(file, "%s\n", operation);
        SAFE_FCLOSE(file);
        return;
    }

    FILE *f = fopen(history_path, "r");
    if (!f) {
        printf("Error: could not open 'history.txt'\n");
        return;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size <= 0) {
        SAFE_FCLOSE(f);
        return;
    }

    char *buffer = malloc(size + 1);
    if (!buffer) {
        SAFE_FCLOSE(f);
        return;
    }

    size_t read = fread(buffer, 1, size, f);
    buffer[read] = '\0';
    SAFE_FCLOSE(f);

    uint16_t to_remove = lines - MaxLines + 1;
    char *content = buffer;

    while (to_remove > 0 && content) {
        content = strchr(content, '\n');
        if (content) {
            content++;
            to_remove--;
        }
    }

    if (!content)
        content = buffer + strlen(buffer);

    size_t contentLen = strlen(content);
    size_t opLen = strlen(operation);

    char *newBuffer = malloc(contentLen + opLen + 2);
    if (!newBuffer) {
        SAFE_FREE(buffer);
        return;
    }

    size_t pos = 0;

    memcpy(newBuffer + pos, content, contentLen);
    pos += contentLen;

    if (pos > 0 && newBuffer[pos - 1] != '\n')
        newBuffer[pos++] = '\n';

    memcpy(newBuffer + pos, operation, opLen);
    pos += opLen;

    newBuffer[pos] = '\0';

    FILE *file = fopen(history_path, "w");
    if (!file) {
        printf("Error: could not open 'history.txt'\n");
        SAFE_FREE(buffer);
        SAFE_FREE(newBuffer);
        return;
    }

    fprintf(file, "%s\n", newBuffer);

    SAFE_FCLOSE(file);
    SAFE_FREE(buffer);
    SAFE_FREE(newBuffer);
}

uint16_t getHistSizeConfig(char *lswrc_path) {
    uint16_t result = DEFAULT_HISTSIZE;

    FILE *f = fopen(lswrc_path, "r");
    if (!f) {
        SAFE_FREE(lswrc_path);
        return result;
    }

    char line[0x400];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = '\0';

        removeComments(line);
        trim(line);
        trimEnd(line);
        trimBetween(line);

        if (!*line)
            continue;
            
        char *lineCpy = strdup(line);
        if (!lineCpy)
            continue;

        char *save;
        char *key = strtok_r(lineCpy, "=", &save);
        char *val = strtok_r(NULL, "=", &save);

        if (key) {
            trim(key);
            trimEnd(key);
        }

        if (val) {
            trim(val);
            trimEnd(val);
            
            if (val[0] == '=')
                val[0] = ' ';
            trim(val);
        }

        if (key && val && strcmp(key, "HISTSIZE") == 0) {
            result = h_atof(val, false);
            SAFE_FREE(lineCpy);
            break;
        }

        SAFE_FREE(lineCpy);
    }

    SAFE_FCLOSE(f);
    SAFE_FREE(lswrc_path);
    return result;
}

uint16_t getSavedHistSize(char *path) {
    uint16_t lines = 1;

    FILE *f = fopen(path, "r");

    if (!f)
        return 0;

    char buff[0x400];
    while (fgets(buff, sizeof(buff), f))
        lines++;

    SAFE_FCLOSE(f);
    return lines;
}

char **extract_args(char *args, uint16_t *argc, char *firstArg) {
    char **argv = malloc(sizeof(char*) * MAX_ARGS);
    if (!argv) return NULL;

    *argc = 0;

    argv[*argc] = strdup(firstArg);
    if (!argv[*argc]) {
        SAFE_FREE(argv);
        return NULL;
    }
    (*argc)++;

    if (args) {
        uint16_t count = 0;
        char **list = parseData(args, &count);
        if (!list) {
            SAFE_FREE(argv[0]);
            SAFE_FREE(argv);
            return NULL;
        }

        for (uint16_t i = 0; i < count && *argc < MAX_ARGS; i++) {
            argv[*argc] = strdup(list[i]);
            if (!argv[*argc]) {
                for (uint16_t j = 0; j < *argc; j++)
                    SAFE_FREE(argv[j]);
                SAFE_FREE(argv);
                break;
            }
            (*argc)++;
        }

        for (uint16_t i = 0; i < count; i++)
            SAFE_FREE(list[i]);
        SAFE_FREE(list);
    }

    return argv;
}

int8_t isAppend(const char *str) {
    uint16_t in_single = 0, in_double = 0;
    uint16_t found = 0;

    for (uint16_t i = 0; str[i]; i++) {

        if (str[i] == '\'' && !in_double) {
            in_single = !in_single;
            continue;
        }
        if (str[i] == '"' && !in_single) {
            in_double = !in_double;
            continue;
        }

        if (str[i] == '>') {

            if (in_single || in_double)
                return -1;

            if (str[i+1] == '>') {
                if (str[i+2] == '>')
                    return -2;
                if (found)
                    return -2;
                found = 2;
                i++;
            }
            else {
                if (found)
                    return -2;
                found = 1;
            }
        }
    }

    return found;
}

uint8_t echoNtimes(char *instruction, char *copy, uint16_t reps) {

    char *save;

    char *str = strtok_r(copy, "*", &save);
    char *num = strtok_r(NULL, "*", &save);

    if (!str || !num || (num[0] == '*' && num[1] == '\0')){
        puts("echo: invalid syntax");
        return 1;
    }

    trim(num); trimEnd(num);
    trim(str); trimEnd(str);
    bool QuoteAfterStar = (reps < strrchar(instruction, '\"') ||
                        reps < strrchar(instruction, '\''));

    double count;
    
    if (!QuoteAfterStar) {
        if (isBcVariable(num)) {
            printf("Warning: variables are currently unsupported\n");
            count = 0;
        } else {
            count = eval(num, true);
            count = (count == QUICK_EVAL_FIX) ? 0.0 : count;
        }

        if (isnan(count))
            return 0;

        if (count != (int64_t)count) {
            printf("echo: the multiplier must be an integer\n");
            return 0;
        }

        if (count <= 0) {
            printf("echo: the multiplier must be greater than 0\n");
            return 0;
        }
    }

    int32_t changed = 0;
    char *new = stringToVariable(str, &changed);

    if (!(changed && strcasecmp(str, "$path") == 0))
        new = echoHandler(new);


    if (QuoteAfterStar) {
        echoHandler(instruction);
        puts(instruction);
    } else 
        for (int32_t i = 0; i < count; i++)
            puts(new);

    SAFE_FREE(new);
    SAFE_FREE(copy);

    return 1;
}

char* findStarOutsideQuotes(char *s) {
    int32_t in_single = 0, in_double = 0;

    for (; *s; s++) {
        if (*s == '\'' && !in_double) in_single = !in_single;
        else if (*s == '"' && !in_single) in_double = !in_double;
        else if (*s == '*' && !in_single && !in_double)
            return s;
    }
    return NULL;
}

char* findCharOutsideQuotes(char *s, char target) {
    bool in_quotes = false;

    for (char *p = s; *p; ++p) {
        if (*p == '"') in_quotes = !in_quotes;
        else if (*p == target && !in_quotes)
            return p;
    }
    return NULL;
}

uint8_t echoFileNtimes(char *instruction, char *copy, uint16_t reps, uint16_t file) {

    char *work = strdup(instruction);
    if (!work) return 0;

    char *redir = findCharOutsideQuotes(work, '>');

    int32_t append = 0;
    char *filename = NULL;

    if (redir) {
        *redir = '\0';
        redir++;

        if (*redir == '>') {
            append = 1;
            redir++;
        }

        while (*redir == ' ') redir++;

        if (!*redir) {
            puts("echo: invalid syntax");
            goto fail;
        }

        filename = redir;

        trim(filename);
        trimEnd(filename);

        if (!isValidFolderOrFileName(filename)) {
            puts("echo: invalid file name");
            goto fail;
        }
    }

    char *star = findCharOutsideQuotes(work, '*');
    char *text = work;
    double count = 0;

    if (star) {
        *star = '\0';
        star++;

        trim(star);
        trim(text);
        trimEnd(text);

        if (isBcVariable(star)) {
            printf("Warning: variables are currently unsupported\n");
            count = 0;
        } else {
            count = eval(star, true);
            count = (count == QUICK_EVAL_FIX) ? 0.0 : count;
        }

        if (isnan(count))
            return 0;

        if (count != (int64_t)count) {
            puts("echo: the multiplier must be an integer");
            goto fail;
        }

        if (count <= 0) {
            puts("echo: the multiplier must be greater than 0");
            goto fail;
        }
    }
    else {
        trim(text);
        trimEnd(text);
    }

    int32_t changed = 0;
    char *new = stringToVariable(text, &changed);

    if (!(changed && strcasecmp(text, "$path") == 0))
        new = echoHandler(new);


    FILE *f = NULL;

    if (filename) {
        f = fopen(filename, append ? "a" : "w");
        if (!f) {
            perror("echo");
            goto fail;
        }
    }

    for (uint64_t i = 0; i < (uint64_t)count; i++) {
        if (f) {
            fputs(new, f);
            fputc('\n', f);
        }
        else {
            puts(new);
        }
    }

    if (f) SAFE_FCLOSE(f);

    SAFE_FREE(work);
    SAFE_FREE(copy);
    return 1;

fail:
    SAFE_FREE(work);
    SAFE_FREE(copy);
    return 0;
}

char *extract_instruction(char *str, char **args) {

    if (!str) {
        *args = NULL;
        return NULL;
    }


    char *p = str;

    while (*p == ' ') p++;

    char *instruction = p;

    if (*p == '"' || *p == '\'') {
        char quote = *p++;
        instruction = p;

        while (*p && *p != quote) p++;

        if (*p != quote) {
            printf("Syntax error: unmatched %c\n", quote);
            *args = NULL;
            return NULL;
        }

        *p = '\0';
        p++;
    }
    else {
        while (*p && *p != ' ') p++;

        if (*p) {
            *p = '\0';
            p++;
        }
    }

    while (*p == ' ') p++;

    *args = (*p) ? p : NULL;

    return instruction;
}

int16_t rm_delete(char *path, uint8_t flags) {
    if (flags & RM_BIN) {
        return move_to_trash(path) ? 0 : -1;
    }

#ifdef _WIN32
    if (RemoveDirectoryA(path))
        return 0;

    if (DeleteFileA(path))
        return 0;
#endif

    return remove(path);
}

int16_t move_to_trash(char *path) {
#ifdef _WIN32
    SHFILEOPSTRUCTA fileOp = {0};

    char from[MAX_PATH];
    snprintf(from, MAX_PATH, "%s%c%c", path, '\0', '\0');  

    fileOp.wFunc = FO_DELETE;
    fileOp.pFrom = from;
    fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT;

    return SHFileOperationA(&fileOp) == 0;
#else
    char dest[0x400];
    snprintf(dest, sizeof(dest),
            "%s/.local/share/Trash/files/%s",
            getenv("HOME"), path);

    return rename(path, dest) == 0;
#endif
}

void printInFileNTimes(FILE *stream, char *str, int64_t count) {
    if (count <= 0) {
        puts("Error: invalid value");
        return;
    }

    for (int32_t i = 0; i < count; i++) {
        if (i != count - 1)
            fprintf(stream, "%s\n", str);
        else
            fprintf(stream, "%s", str);
    }
}

//! unused
char **readHistory(const char *address, uint32_t *lineCount) {
    *lineCount = 0;

    FILE *f = fopen(address, "r");
    if (!f) return NULL;

    char buffer[0x400];

    while (fgets(buffer, sizeof(buffer), f))
        (*lineCount)++;

    if (*lineCount == 0) {
        SAFE_FCLOSE(f);
        return NULL;
    }

    char **mat = calloc(*lineCount, sizeof(char *));
    if (!mat) {
        SAFE_FCLOSE(f);
        return NULL;
    }

    for (uint32_t i = 0; i < *lineCount; i++) {
        mat[i] = calloc(MAX_CHAR, sizeof(char));
        if (!mat[i]) {
            for (uint32_t j = 0; j < i; j++)
                SAFE_FREE(mat[j]);
            SAFE_FREE(mat);
            SAFE_FCLOSE(f);
            return NULL;
        }
    }

    fseek(f, 0, SEEK_SET);

    for (uint32_t i = 0; i < *lineCount; i++)
        fgets(mat[i], MAX_CHAR, f);

    SAFE_FCLOSE(f);
    return mat;
}

void initRandom(void) {
#ifdef _WIN32
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    srand((unsigned)counter.QuadPart);
#else
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    srand((unsigned)(ts.tv_nsec ^ ts.tv_sec));
#endif
}

void sleepF(double seconds) {
#ifdef _WIN32
    LARGE_INTEGER freq, start, now;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);

    double target = seconds;

    if (seconds > 0.002) {
        DWORD coarse = (DWORD)((seconds - 0.001) * 1000.0);
        Sleep(coarse);
    }

    do {
        QueryPerformanceCounter(&now);
    } while ((double)(now.QuadPart - start.QuadPart) / freq.QuadPart < target);
#else 
    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);

    double target = seconds;

    if (seconds > 0.002) {
        struct timespec ts;
        ts.tv_sec  = (time_t)(seconds - 0.001);
        ts.tv_nsec = (long)(((seconds - 0.001) - ts.tv_sec) * 1e9);
        nanosleep(&ts, NULL);
    }

    do {
        clock_gettime(CLOCK_MONOTONIC, &now);
        double elapsed =
            (now.tv_sec - start.tv_sec) +
            (now.tv_nsec - start.tv_nsec) / 1e9;
        if (elapsed >= target) break;
    } while (true);
#endif
}

bool isValidFolderOrFileName(const char *name) {
    if (!name || !*name) return false;

#ifdef _WIN32
    const char *invalid = "<>:\"/\\|?*";
    size_t len = strlen(name);

    if (len > 255) return false;
    if (name[len-1] == ' ' || name[len-1] == '.') return false;

    for (size_t i = 0; i < len; ++i) {
        unsigned char c = name[i];
        if (c < 0x20) return false;

        for (size_t j = 0; invalid[j]; ++j)
            if (c == invalid[j]) return false;
    }

    const char *reserved[] = {
        "CON","PRN","AUX","NUL",
        "COM1","COM2","COM3","COM4","COM5","COM6","COM7","COM8","COM9",
        "LPT1","LPT2","LPT3","LPT4","LPT5","LPT6","LPT7","LPT8","LPT9"
    };

    char base[0x100];
    strncpy(base, name, sizeof(base));
    base[sizeof(base)-1] = 0;
    char *dot = strchr(base, '.');
    if (dot) *dot = 0;

    for (size_t i = 0; i < sizeof(reserved)/sizeof(reserved[0]); ++i)
        if (_stricmp(base, reserved[i]) == 0)
            return false;

#else
    if (strlen(name) > 255) return false;
    for (size_t i = 0; name[i]; ++i)
        if (name[i] == '/') return false;
#endif

    if (!strcmp(name, ".") || !strcmp(name, ".."))
        return false;

    return true;
}


char *revStr(const char *str) {
    int32_t len = strlen(str);
    char *new = calloc(len + 1, 1);
    if (!new) return NULL;

    for (int32_t i = 0; i < len; i++)
        new[i] = str[len - 1 - i];

    return new;
}

double parse_len(char *s) {
    while (*s && isspace((unsigned char)*s))
        s++;

    if (*s == '=') {
        s++;
        while (*s && isspace((unsigned char)*s))
            s++;
    }

    double result = eval(s, true);
    return (result == QUICK_EVAL_FIX) ? 0.0 : result;
}

char randChr(void) {
    if (rand() % 4) {
        char c = 'a' + rand() % 26;
        if (rand() % 2) {
            c = toupper(c);
        }
        return c;
    } else {
        return '0' + rand() % 10;
    }
}

char *defaultAddressReplace(const char *address) {
    char *copy = strdup(address);
    char *Default = get_default_address();
    size_t len = strlen(Default);

#ifdef _WIN32
    charReplace(copy, '\\', '/');
    charReplace(Default, '\\', '/');
#endif

    if (
        strcmp(copy, Default) == 0 ||
        (strncmp(copy, Default, len) == 0 && copy[len] == '/')
    ) {
        char *tmp = strrm(copy, Default);
        SAFE_FREE(copy);
        copy = tmp;
    } else {
        SAFE_FREE(copy);
        SAFE_FREE(Default);
        return strdup(address);
    }

    size_t extra = strlen(copy) + 2;
    char *buffer = malloc(extra);
    *buffer = '\0';
    snprintf(buffer, extra, "~%s", copy);

    SAFE_FREE(copy);
    SAFE_FREE(Default);

    return buffer;
}

void split_instruction_args(char *line, char **cmd, char **args) {
    char *p = line;

    while (*p == ' ') p++;
    *cmd = p;

    while (*p && *p != ' ') p++;

    if (*p) {
        *p = '\0';
        p++;
        while (*p == ' ') p++;
        *args = *p ? p : NULL;
    } else {
        *args = NULL;
    }
}

char *find_andand_outside_quotes(char *s) {
    char quote = 0;

    for (char *p = s; *p && *(p + 1); p++) {
        if ((*p == '"' || *p == '\'') && !quote) {
            quote = *p;
        }
        else if (*p == quote) {
            quote = 0;
        }

        if (!quote && *p == '&' && *(p + 1) == '&')
            return p;
    }
    return NULL;
}

char **parseData(const char *str, uint16_t *count) {
    char **list = malloc(MAX_ARGS * sizeof(char*));
    if (!list) return NULL;

    *count = 0;
    uint16_t i = 0;

    while (str[i]) {
        while (isspace((unsigned char)str[i])) i++;
        if (!str[i]) break;

        if (*count >= MAX_ARGS)
            break;

        if (str[i] == '"') {
            i++;
            const char *start = &str[i];
            while (str[i] && str[i] != '"') i++;

            size_t len = &str[i] - start;
            list[*count] = malloc(len + 1);
            memcpy(list[*count], start, len);
            list[*count][len] = '\0';
            (*count)++;

            if (str[i] == '"') i++;
        } else {
            const char *start = &str[i];
            while (str[i] && !isspace((unsigned char)str[i])) i++;

            size_t len = &str[i] - start;
            list[*count] = malloc(len + 1);
            memcpy(list[*count], start, len);
            list[*count][len] = '\0';
            (*count)++;
        }
    }

    return list;
}

void enableAnsiIfNeeded(void) {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;
    DWORD mode = 0;
    if (!GetConsoleMode(hOut, &mode)) return;
    if (!(mode & 0x4)) {
        SetConsoleMode(hOut, mode | 0x4);
    }
#endif
}

int8_t isDir(const char *path) {
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(path);
    if (attr == INVALID_FILE_ATTRIBUTES)
        return -1;
    return (attr & FILE_ATTRIBUTE_DIRECTORY) ? 1 : 0;
#else
    struct stat st;
    if (stat(path, &st) != 0)
        return -1;
    return S_ISDIR(st.st_mode) ? 1 : 0;
#endif
}

void int64_to_hex_min(int64_t v, char *out, size_t size) {
    uint64_t u = (uint64_t)v;

    int32_t bits;
    for (bits = 8; bits < 64; bits++) {
        int64_t sign_bit = 1LL << (bits - 1);
        int64_t min = -sign_bit;
        int64_t max = sign_bit - 1;

        if (v >= min && v <= max)
            break;
    }

    int32_t hex_digits = (bits + 3) / 4;
    uint64_t mask = (1ULL << (hex_digits * 4)) - 1;
    u &= mask;

    snprintf(out, size, "0x%0*"PRIX64, hex_digits, u);
}

int64_t hex_to_long(char *str) {
    char *end;
    int64_t v = strtoll(str, &end, 16);

    if (*end == '\0') {

        const char *p = str;
        if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X'))
            p += 2;

        size_t digits = 0;
        for (; *p; ++p)
            if (isxdigit(*p)) digits++;

        size_t bits = digits * 4;

        int64_t sign_bit = 1LL << (bits - 1);
        int64_t mask     = (1LL << bits) - 1;

        v &= mask;
        if (v & sign_bit)
            v -= (1LL << bits);

        return v;
    }

    return U64_NAN;
}

bool isBin(const char *str) {
    if (!str) return false;

    char *suffixes = "kmbt";

    uint16_t len = strlen(str);

    if (strncasecmp(str, "0b", 2) != 0)
        return false;

    bool isValidSuffix = false;
    for (uint16_t i = 0; i < strlen(suffixes); i++) {
        if (str[len-1] == suffixes[i] && !isdigit(str[len-1])) {
            isValidSuffix = true;
            break;
        }
    }

    if (!isValidSuffix && !isdigit(str[len-1]))
        return false;

    for (uint16_t i = 2; i < len - 1; i++) {
        if (str[i] != '1' && str[i] != '0')
            return false;
    }

    return strlen(str) > 2;
}

bool isHex(const char *str) {
    if (!str) return false;

    if (strncasecmp(str, "0x", 2) != 0)
        return false;

    for (uint16_t i = 2; str[i]; i++) {
        if (!isxdigit((unsigned char) str[i]))
            return false;
    }

    return strlen(str) > 2;
}

bool isOct(const char *str) {
    if (!str || !*str)
        return false;

    if (strncasecmp(str, "0o", 2) != 0)
        return false;

    for (uint16_t i = 2; str[i]; i++) {
        if (str[i] < '0' || str[i] > '7')
            return false;
    }

    return true;
}

bool isValidBcCommand(char *str, char *command) {

    uint16_t len = strlen(command);

    if (strncmp(str, command, strlen(command)) == 0 && (str[len] == '\0' || str[len] == ' '))
        return true;

    return false;
}

char *myDirname(char *path) {
#ifdef _WIN32
    static char buffer[MAX_PATH];
    strcpy(buffer, path);
    for (uint16_t i = strlen(buffer) - 1; i >= 0; i--) {
        if (buffer[i] == '\\' || buffer[i] == '/') {
            buffer[i] = '\0';
            break;
        }
    }
    return buffer;
#else
    return dirname(path);
#endif
}

char *getBasePath(void) {
    static char path[MAX_PATH];

#ifdef _WIN32
    GetModuleFileNameA(NULL, path, sizeof(path));
    strcpy(path, myDirname(path));
#else
    int16_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len != -1) {
        path[len] = '\0';
        strcpy(path, dirname(path));
    } else {
        strcpy(path, ".");
    }
#endif

    return path;
}

char *buildPath(const char *relative) {
    static char full[1024];
    snprintf(full, sizeof(full), "%s/%s", getBasePath(), relative);
    return full;
}

char *linesNumber(void) {
    uint32_t lines = 0;
    char buffer[0x400];

    const char *files[] = {
        PATH_MAIN_C,
        PATH_UTILS_C,
        PATH_UTILS_H,
        PATH_TERMINAL_C,
        PATH_TERMINAL_H,
        PATH_S_MATH_C,
        PATH_S_MATH_H,
        PATH_CHECKCMD_C,
        PATH_CHECKCMD_H
    };

    const uint8_t fileCount = sizeof(files) / sizeof(files[0]);
    static char result[0x40];

    for (uint8_t i = 0; i < fileCount; i++) {
        FILE *f = fopen(buildPath(files[i]), "r");

        if (!f) {
            snprintf(result, sizeof(result), "%u", PROJ_LINES_APPROX);
            return result;
        }

        while (fgets(buffer, sizeof(buffer), f)) {
            lines++;
        }

        SAFE_FCLOSE(f);
    }

    snprintf(result, sizeof(result), "%u", lines);
    return result;
}

char *charNumber(void) {
    uint32_t totalSize = 0;

    const char *files[] = {
        PATH_MAIN_C,
        PATH_UTILS_C,
        PATH_UTILS_H,
        PATH_TERMINAL_C,
        PATH_TERMINAL_H,
        PATH_S_MATH_C,
        PATH_S_MATH_H,
        PATH_CHECKCMD_C,
        PATH_CHECKCMD_H
    };

    uint16_t fileCount = sizeof(files) / sizeof(files[0]);
    
    static char result[0x40];
    for (uint16_t i = 0; i < fileCount; i++) {
        FILE *f = fopen(buildPath(files[i]), "rb");
        if (!f) {
            snprintf(result, sizeof(result), "%d B / %.2lf KiB / %.2lf Mib", PROJ_SIZE_APPROX_BYTES, (double)PROJ_SIZE_APPROX_BYTES / 0x400, (double)PROJ_SIZE_APPROX_BYTES / 0x100000);
            return result;
        }

        fseek(f, 0, SEEK_END);
        totalSize += ftell(f);
        SAFE_FCLOSE(f);
    }

    snprintf(result, sizeof(result), "%"PRIu32" B / %.2lf KiB / %.2lf Mib", totalSize, (double)totalSize / 0x400, (double)totalSize / 0x100000);
    return result;
}

char *get_user(void) {
#ifdef _WIN32
    char *user = getenv("USERNAME");
#else
    #ifndef __ANDROID__
        char *user = getenv("USER");
    #else
        uid_t uid = getuid();
        struct passwd *pw = getpwuid(uid);

        char *user = pw->pw_name;
    #endif
#endif

    return (user) ? user : "Unknown";
}

int16_t strchar(const char *str, int8_t chr) {
    if (strlen(str) == 0) return -1;

    for (uint16_t i = 0; str[i]; i++) 
        if (str[i] == (char)chr) return i;

    return -1;   
}

int16_t strrchar(const char *str, int8_t chr) {
    uint16_t len = strlen(str);
    
    if (len == 0) return -1; 
    
    for (uint16_t i = len - 1; ; i--) {
        if (str[i] == (char)chr) return (ssize_t)i;
        if (i == 0) break;
    }
    return -1;
}

void lsCmdLinux(const char *dirPath, uint8_t showAll) {
#ifndef _WIN32
    DIR *dir = opendir(dirPath);
    if (!dir) {
        perror("erro");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {

        if (!showAll) {
            if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
                continue;
            if (entry->d_name[0] == '.')
                continue;
        }

        char fullPath[0x1000];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", dirPath, entry->d_name);

        int8_t t = isDir(fullPath);

        if (t == 1)
            printf("\033[94m%s\033[0m\n", entry->d_name);
        else
            printf("\033[37m%s\033[0m\n", entry->d_name);
    }

    closedir(dir);
#endif
}

void lsCmdWin(const char *dirPath, uint8_t showAll) {
#ifdef _WIN32
    char searchPath[0x1000];

    size_t len = strlen(dirPath);
    if (len + 3 >= sizeof(searchPath)) {
        fprintf(stderr, "path too long\n");
        return;
    }

    strcpy(searchPath, dirPath);
    if (len > 0 && (searchPath[len-1] == '/' || searchPath[len-1] == '\\'))
        searchPath[len-1] = '\0';

    strcat(searchPath, "\\*");

    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(searchPath, &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "erro: FindFirstFile failed (%lu)\n", GetLastError());
        return;
    }

    do {
        const char *name = fd.cFileName;

        if (!showAll) {
            if (!strcmp(name, ".") || !strcmp(name, ".."))
                continue;
            if (name[0] == '.')
                continue;
        }

        char fullPath[0x1000];
        snprintf(fullPath, sizeof(fullPath), "%s\\%s", dirPath, name);

        int8_t t = isDir(fullPath);

        if (t == 1)
            printf("\033[94m%s\033[0m\n", name);
        else
            printf("\033[37m%s\033[0m\n", name);

    } while (FindNextFileA(hFind, &fd));

    FindClose(hFind);
#endif
}

char *unameCmdWin(uint8_t flags) {
#ifdef _WIN32
    static char result[0x400];
    char buffer[0x100];
    result[0] = '\0';

    OSVERSIONINFOEX ver;
    SYSTEM_INFO sysInfo;

    ZeroMemory(&ver, sizeof(ver));
    ver.dwOSVersionInfoSize = sizeof(ver);

    if (!GetVersionEx((OSVERSIONINFO*)&ver)) {
        strcpy(result, "Windows");
        return result;
    }

    GetSystemInfo(&sysInfo);

    if (flags & U_KERN_NAME)
        strcat(result, "Windows ");

    if (flags & U_HOST_NAME) {
        char* hostname = get_hostname();
        if (hostname) {
            sprintf(buffer, "%s ", hostname);
            strcat(result, buffer);
        }
    }

    if (flags & U_KERN_RELEASE) {
        sprintf(buffer, "%lu.%lu ", ver.dwMajorVersion, ver.dwMinorVersion);
        strcat(result, buffer);
    }

    if (flags & U_KERN_VERSION) {
        sprintf(buffer, "build %lu ", ver.dwBuildNumber);
        strcat(result, buffer);
    }

    if (flags & U_MACHINE) {
        switch (sysInfo.wProcessorArchitecture) {
            case PROCESSOR_ARCHITECTURE_AMD64:  strcat(result, "x86_64 "); break;
            case PROCESSOR_ARCHITECTURE_INTEL:  strcat(result, "x86 ");    break;
            case PROCESSOR_ARCHITECTURE_ARM64:  strcat(result, "ARM64 ");  break;
            case PROCESSOR_ARCHITECTURE_ARM:    strcat(result, "ARM ");    break;
            default: strcat(result, "unknown "); break;
        }
    }

    if (flags & U_OPERATING_SYSTEM) {
        const char *os_name = "Windows";
        
        if (ver.dwMajorVersion == 10) {
            if (ver.dwBuildNumber >= 22000)
                os_name = "Windows 11";
            else if (ver.dwBuildNumber >= 20348)
                os_name = "Windows Server 2022";
            else if (ver.dwBuildNumber >= 19045)
                os_name = "Windows 10 (22H2)";
            else if (ver.dwBuildNumber >= 19044)
                os_name = "Windows 10 (21H2)";
            else if (ver.dwBuildNumber >= 19043)
                os_name = "Windows 10 (21H1)";
            else if (ver.dwBuildNumber >= 19042)
                os_name = "Windows 10 (20H2)";
            else if (ver.dwBuildNumber >= 19041)
                os_name = "Windows 10 (2004)";
            else
                os_name = "Windows 10";
        }
        else if (ver.dwMajorVersion == 6) {
            switch (ver.dwMinorVersion) {
                case 3: os_name = "Windows 8.1"; break;
                case 2: os_name = "Windows 8"; break;
                case 1: os_name = "Windows 7"; break;
                case 0: os_name = "Windows Vista"; break;
            }
        }
        else if (ver.dwMajorVersion == 5) {
            switch (ver.dwMinorVersion) {
                case 2: 
                    if (GetSystemMetrics(SM_SERVERR2))
                        os_name = "Windows Server 2003 R2";
                    else if (ver.wSuiteMask & VER_SUITE_WH_SERVER)
                        os_name = "Windows Home Server";
                    else if (ver.wProductType == VER_NT_WORKSTATION)
                        os_name = "Windows XP x64";
                    else
                        os_name = "Windows Server 2003";
                    break;
                case 1: os_name = "Windows XP"; break;
                case 0: os_name = "Windows 2000"; break;
            }
        }

        sprintf(buffer, "%s ", os_name);
        strcat(result, buffer);
    }

    if (!*result) {
        strcpy(result, "Windows");
    } else {
        trimEnd(result);
    }

    return result;
#else
    return NULL;
#endif
}

char *unameCmdLinux(uint8_t flags) {
#ifndef _WIN32
    static char result[0x400];
    char buffer[0x100];
    result[0] = '\0';

    struct utsname pc;

    if (uname(&pc) == -1) {
        strcpy(result, "Linux");
        return result;
    }

    if (flags & U_KERN_NAME) {
        sprintf(buffer, "%s ", pc.sysname);
        strcat(result, buffer);
    }

    if (flags & U_HOST_NAME) {
        sprintf(buffer, "%s ", pc.nodename);
        strcat(result, buffer);
    }

    if (flags & U_KERN_RELEASE) {
        sprintf(buffer, "%s ", pc.release);
        strcat(result, buffer);
    }

    if (flags & U_KERN_VERSION) {
        sprintf(buffer, "%s ", pc.version);
        strcat(result, buffer);
    }

    if (flags & U_MACHINE) {
        sprintf(buffer, "%s ", pc.machine);
        strcat(result, buffer);
    }

    if (flags & U_OPERATING_SYSTEM) {

    #if defined(__ANDROID__)
        strcat(result, "Android ");

    #elif defined(__linux__)

        FILE *fp = fopen("/proc/version", "r");

        if (fp) {
            char version[0x100];

            if (fgets(version, sizeof(version), fp)) {
                if (strstr(version, "GNU"))
                    strcat(result, "GNU/Linux ");
                else
                    strcat(result, "Linux ");
            } else {
                strcat(result, "Linux ");
            }

            SAFE_FCLOSE(fp);
        } else {
            strcat(result, "Linux ");
        }

    #elif defined(__APPLE__)
        strcat(result, "Darwin ");
    #else
        sprintf(buffer, "%s ", pc.sysname);
        strcat(result, buffer);
    #endif
    }

    if (!*result) {
        strcpy(result, pc.sysname);
    } else {
        trimEnd(result);
    }

    return result;
#else
    return NULL;
#endif
}

char *get_hostname(void) {
    static char hostname[0x100];

#ifdef _WIN32
    DWORD size = sizeof(hostname);
    if (!GetComputerNameA(hostname, &size))
        strcpy(hostname, "Unknown");
#else
    if (gethostname(hostname, sizeof(hostname)) != 0)
        strcpy(hostname, "Unknown");
#endif

    return hostname;
}

char *get_time(char *fmt) {
    static char buffer[0x50];
    time_t now;
    struct tm *time_info;

    time(&now);
    time_info = localtime(&now);

    strftime(buffer, sizeof(buffer), fmt, time_info);
    return buffer;
}

void GetProjDir(char *program_root, uint16_t root_size, char *data_folder, uint16_t data_size, char *history_path, uint16_t hist_size) {
#ifdef _WIN32
    if (GetModuleFileNameA(NULL, program_root, (DWORD)root_size) == 0)
        strcpy(program_root, ".");
    else {
        char *last_slash = strrchr(program_root, '\\');
        if (last_slash) *last_slash = '\0';
    }

    snprintf(data_folder, data_size, "%s\\data", program_root);

    DWORD ftyp = GetFileAttributesA(data_folder);
    if (ftyp == INVALID_FILE_ATTRIBUTES || !(ftyp & FILE_ATTRIBUTE_DIRECTORY))
        CreateDirectoryA(data_folder, NULL);

    snprintf(history_path, hist_size, "%s\\data\\history.txt", program_root);

#else
    int16_t len = readlink("/proc/self/exe", program_root, root_size - 1);
    if (len != -1) {
        program_root[len] = '\0';
        char *last_slash = strrchr(program_root, '/');
        if (last_slash) *last_slash = '\0';
    } else {
        strcpy(program_root, ".");
    }

    snprintf(data_folder, data_size, "%s/data", program_root);

    struct stat st = {0};
    if (stat(data_folder, &st) == -1)
        mkdir(data_folder, 0755);

    snprintf(history_path, hist_size, "%s/data/history.txt", program_root);
#endif
}

void setColor(enum color4 color) {
#ifdef _WIN32
    SetConsoleTextAttribute(hConsole, color);
#else
    switch(color) {
        case BLACK:           printf("\033[30m"); break;
        case BLUE:            printf("\033[34m"); break;
        case GREEN:           printf("\033[32m"); break;
        case CYAN:            printf("\033[36m"); break;
        case RED:             printf("\033[31m"); break;
        case MAGENTA:         printf("\033[35m"); break;
        case YELLOW:          printf("\033[33m"); break;
        case WHITE:           printf("\033[37m"); break;
        case GRAY:            printf("\033[90m"); break;
        case LIGHT_BLUE:      printf("\033[94m"); break;
        case LIGHT_GREEN:     printf("\033[92m"); break;
        case LIGHT_CYAN:      printf("\033[96m"); break;
        case LIGHT_RED:       printf("\033[91m"); break;
        case LIGHT_MAGENTA:   printf("\033[95m"); break;
        case LIGHT_YELLOW:    printf("\033[93m"); break;
        case BRIGHT_WHITE:    printf("\033[97m"); break;
        default:              printf("\033[0m");  break;
    }
#endif
} 

void trimBetween(char *str) {
    uint16_t r = 0;
    uint16_t w = 0;
    uint16_t last_space = 1;

    while (str[r]) {
        unsigned char c = str[r++];

        if (c == ' ') {
            if (!last_space) {
                str[w++] = ' ';
                last_space = 1;
            }
        } else {
            str[w++] = c;
            last_space = 0;
        }
    }

    if (w > 0 && str[w - 1] == ' ')
        w--;

    str[w] = '\0';
}

void trimEnd(char *str) {
    uint16_t len = strlen(str);

    for (int16_t i = len - 1; i >= 0; i--)
        if (str[i] != ' ')  {
            str[i + 1] = '\0';
            break;
        }
}

void trim(char *str) {
    if (!str) return;
    
    uint16_t spaces = 0;
    while (str[spaces] == ' ') {
        spaces++;
    }
    
    if (spaces == 0) return;
    
    uint16_t i = 0;
    while (str[spaces + i] != '\0') {
        str[i] = str[spaces+i];
        i++;
    }
    str[i] = '\0';
}

char *extractPath(char **str) {
    trim(*str);
    trimEnd(*str);

    char *s = *str;

    if (*s == '\"') {
        s++;
        char *end = strchr(s, '\"');

        if (end) {
            *end = '\0';
            *str = end + 1;

            trim(*str);
            trimEnd(*str);
            return s;
        }
    }

    char *end = strchr(s, ' ');
    if (end) {
        *end = '\0';
        *str = end + 1;

        trim(*str);
        trimEnd(*str);
        return s;
    }

    *str = s + strlen(s);
    return s;
}

char *strrm(char *str, const char *substr) {
    uint16_t len = strlen(str);
    uint16_t sublen = strlen(substr);
    char *copy = strdup(str);


    if (sublen == 0 || len < sublen)
        return copy;

    for (uint16_t i = 0; i <= len - sublen; i++) {
        int8_t match = 1;
        for (uint16_t j = 0; j < sublen; j++) {
            if (copy[i + j] != substr[j]) {
                match = 0;
                break;
            }
        }
        if (match) {
            for (uint16_t k = i; k <= len - sublen; k++) 
                copy[k] = copy[k + sublen];
            break;
        }
    }

    return copy;
}

void charReplace(char *str, int8_t targ, int8_t repl) {
    for (uint16_t i = 0; str[i]; i++) 
        if (str[i] == (char)targ) str[i] = (char)repl;
}

uint16_t countIndex(const char *str, int8_t chr) {
    uint16_t count = 0;

    for (uint16_t i = 0; str[i]; i++) {
        if (str[i] == (char)chr)
            count++;
    }

    return count;
}

char *echoHandler(char *str) {
    size_t len = strlen(str);
    char *out = malloc(len*2);
    if (!out) return str;

    size_t o = 0;
    int32_t inQuotes = 0;
    char quoteChar = 0;
    int32_t wroteSomething = 0;

    for (size_t i = 0; i < len; i++) {
        char c = str[i];

        if ((c == '"' || c == '\'') && !inQuotes) {
            inQuotes = 1;
            quoteChar = c;
            continue;
        }

        if (inQuotes && c == quoteChar) {
            inQuotes = 0;
            continue;
        }

        if (!inQuotes && c == ' ') {
            if (wroteSomething && out[o-1] != '\n') {
                out[o++] = '\n';
            }
            continue;
        }

        out[o++] = c;
        wroteSomething = 1;
    }

    if (o > 0 && out[o-1] == '\n')
        o--;

    out[o] = '\0';

    SAFE_FREE(str);
    return out;
}

char *buildLswRcPath(char *path) {
    uint16_t extra = strlen(path) + 1 + strlen(RC_FILE);
    char *buffer = calloc(extra, sizeof(char));
    strcpy(buffer, path);

#ifdef _WIN32
    strcat(buffer, "\\");
#else
    strcat(buffer, "/");    
#endif

    strcat(buffer, RC_FILE);

    return buffer;
}

bool aliasExists(const char *filePath, const char *shortcutName) {
    FILE *f = fopen(filePath, "r");
    if (!f) return false;

    char *line = calloc(MAX_CHAR, sizeof(char));

    while (fgets(line, MAX_CHAR, f)) {
        line[strcspn(line, "\n")] = '\0';

        char *original = line;
        char *clean = strrm(line, "alias");
        line = clean;
        SAFE_FREE(original);

        char *aliasName = strtok(line, "=");
        if (!aliasName) {
            SAFE_FREE(line);
            SAFE_FCLOSE(f);
            return false;
        }

        trim(aliasName);

        if (strcmp(shortcutName, aliasName) == 0) {
            SAFE_FREE(line);
            SAFE_FCLOSE(f);
            return true;
        }

        SAFE_FREE(line);
        line = calloc(MAX_CHAR, sizeof(char));
    }

    SAFE_FREE(line);
    SAFE_FCLOSE(f);
    return false;
}
char *findFirstEqualOutsideQuotes(char *s) {
    bool insideQuotes = false;

    for (; *s; s++) {
        if (*s == '\'' || *s == '\"')
            insideQuotes = !insideQuotes;
        else if (*s == '=' && !insideQuotes)
            return s;
    }
    return NULL;
}

bool isBetweenQuotes(const char *action) {
    size_t len = strlen(action);
    if ((action[0] != '\'' || action[len-1] != '\'') &&
        (action[0] != '\"' || action[len-1] != '\"'))
        return 0;

    return 1;
}

void createShortcut(char *instruction, char *path) {    
    
    if (!*instruction) {
        puts("alias: missing operand\nUse \"man alias\" to check the manual");
        return;
    }

    char *alias = strdup(instruction);
    
    trim(alias);
    trimEnd(alias);

    char *eq = findFirstEqualOutsideQuotes(alias);

    if (!eq) {
        puts("Error: syntax error for 'alias', use \"man alias\" to check the manual");
        SAFE_FREE(alias);
        return;
    }

    *eq = '\0';
    char *shortcutName = alias;
    char *action = eq + 1;

    if (!shortcutName) {
        puts("Error: missing shortcut name, use \"man alias\" to check the manual");
        SAFE_FREE(alias);
        return;
    }

    if (!action) {
        puts("Error: missing action, use \"man alias\" to check the manual");
        SAFE_FREE(alias);
        return;
    }

    trimBetween(shortcutName);
    trimEnd(shortcutName);
    trim(shortcutName);
    trim(action);

    size_t len = strlen(action);
    
    if (!isBetweenQuotes(action)) {
        puts("Error: the action should be between quotes, and it must be equal, use \"man alias\" to check the manual");
        SAFE_FREE(alias);
        return;
    }

    char *buffer = buildLswRcPath(path);

    if (aliasExists(buffer, shortcutName)) {
        puts("Warning: duplicated alias found, note that only the first "
            "occurrence of this shortcut will work!");
    }
    
    char quote = action[0];

    action[0] = ' ';
    action[len-1] = ' ';

    trim(action);
    trimEnd(action);
    trimBetween(action);

    FILE *f = fopen(buffer, "a");

    fprintf(f, "alias %s=%c%s%c\n", shortcutName, quote, action, quote);
    SAFE_FCLOSE(f);

    SAFE_FREE(buffer);
    SAFE_FREE(alias);
}

void removeComments(char *str) {
    if (!str) return;

    bool in_double = false;
    bool in_single = false;

    for (size_t i = 0; str[i]; i++) {

        if (str[i] == '"' && !in_single) {
            in_double = !in_double;
        }
        else if (str[i] == '\'' && !in_double) {
            in_single = !in_single;
        }
        else if (str[i] == '#' && !in_double && !in_single) {
            str[i] = '\0';
            return;
        }
    }

    if (in_double || in_single) {
        char *p = strchr(str, '#');
        if (p) *p = '\0';
    }
}

bool isalias(char *operation, char *args, const char **cmds, char **address, char *history_path, char *data_folder, uint16_t isInsideBash) {
    char *aliasPath = buildLswRcPath(data_folder);
    FILE *f = fopen(aliasPath, "r");
    SAFE_FREE(aliasPath);

    if (!f) return false;

    char *line = calloc(MAX_CHAR, sizeof(char));

    while (fgets(line, MAX_CHAR, f)) {
        line[strcspn(line, "\n")] = '\0';
        trim(line);

        if (strncmp(line, "alias", 4) != 0)
            continue;

        char *eq = findFirstEqualOutsideQuotes(line);

        if (!eq) {
            puts("Error: syntax error for 'alias', use \"man alias\" to check the manual");
            SAFE_FREE(line);
            return false;
        }

        *eq = '\0';
        char *shortcutName = line;
        char *action = eq + 1;

        shortcutName = strchr(shortcutName, ' ');

        if (!shortcutName || !action) {
            puts("Error: syntax error for 'alias', use \"man alias\" to check the manual");
            SAFE_FREE(line);
            return false;
        }

        removeComments(action);
        trimEnd(action);

        trimBetween(shortcutName);
        trim(shortcutName);
        trim(action);

        if (action) {
            action[0] = ' ';
            action[strlen(action)-1] = ' ';
    
            trim(action);
            trimEnd(action);
        }

        if (isInsideBash && strncmp(action, "bash", 4) == 0 && strcmp(shortcutName, operation) == 0) { //* Just a simple fix
            char *option;

            uint16_t len = strlen(action);

            if (len == 4)
                option = args;
                
            else {
                if (action[4] != ' ' && action[4] != '\0')
                    return false;
                else {
                    option = strrchr(action, ' ');
                }

            }
            trim(option);

            uint16_t argc_bash;
            char **argv_bash = extract_args(args, &argc_bash, "bash");

            bashCmd(argc_bash, argv_bash, cmds, true);
            
            if (args) {
                for (uint16_t i = 1; i < argc_bash; i++)
                    SAFE_FREE(argv_bash[i]);
                SAFE_FREE(argv_bash);
            }

            return true;
        }

        if (strcmp(operation, shortcutName) == 0) {
            char *fullAction;

            if (args && *args) {
                size_t len = strlen(action) + strlen(args) + 3;
                fullAction = malloc(len);
                snprintf(fullAction, len, "%s %s", action, args);
            } else {
                fullAction = strdup(action);
            }

            char *save;
            strtok_r(action, " ", &save); 

            if (!*args)
                args = strtok_r(NULL, " ", &save);

            processCommand(fullAction, cmds, address, history_path, data_folder, isInsideBash, true);
            SAFE_FREE(fullAction);
            SAFE_FREE(line);
            SAFE_FCLOSE(f);
            return true;
        }

    }

    SAFE_FREE(line);
    SAFE_FCLOSE(f);
    return false;
}

void safe_lower_inplace(char *s) {
    if (!s) return;
    for (size_t i = 0; s[i]; i++)
        s[i] = (char)tolower((unsigned char)s[i]);
}

char *tolowerstr(const char *str) {
    uint16_t len = strlen(str);
    char *cpy = malloc(len + 1);
    if (!cpy) return NULL;

    for (uint16_t i = 0; i < len; i++)
        cpy[i] = (unsigned char)tolower((unsigned char)str[i]);
    cpy[len] = '\0';
    return cpy;
}

uint8_t myStrcasestr(const char *str, const char *sub) {
    if (!*sub) return 1;
    uint8_t lenSub = strlen(sub);

    for (; *str; str++) {
        if (strncasecmp(str, sub, lenSub) == 0)
            return 1;
    }
    return 0;
}

const char *strcasestr_ptr(const char *haystack, const char *needle) {
    if (!haystack || !needle) return NULL;
    
    size_t needle_len = strlen(needle);
    if (needle_len == 0) return haystack;
    
    size_t haystack_len = strlen(haystack);
    
    for (size_t i = 0; i <= haystack_len - needle_len; i++) {
        if (strncasecmp(haystack + i, needle, needle_len) == 0) {
            return haystack + i;
        }
    }

    return NULL;
}

void printTarg(const char *str, const char *targ, enum color4 markColor, int8_t ignoreCase) {
    const char *p = str;
    uint16_t targLen = strlen(targ);

    while (*p) {
        const char *found = NULL;

        if (ignoreCase) {
            found = strcasestr_ptr(p, targ);
        } else {
            found = strstr(p, targ);
        }

        if (!found) {
            printf("%s", p);
            break;
        }

        printf("%.*s", (int16_t)(found - p), p);
        setColor(markColor);
        printf("%.*s", (int16_t)targLen, found);
        setColor(7);

        p = found + targLen;
    }
    putchar('\n');
}

void charRm(char *str, int8_t targ) {
    int8_t i = 0, j = 0;

    while (str[i] != '\0') {
        if (str[i] != (char)targ) {
            str[j++] = str[i];
        }
        i++;
    }

    str[j] = '\0';
}

double parse_base_fraction(const char *s, int8_t base) {
    double result = 0.0;
    double frac = 0.0;
    double div = base;
    int8_t seen_dot = 0;

    for (; *s; s++) {
        if (*s == '.') {
            if (seen_dot) break;
            seen_dot = 1;
            continue;
        }

        int32_t digit;
        if (*s >= '0' && *s <= '9') digit = *s - '0';
        else if (*s >= 'a' && *s <= 'f') digit = *s - 'a' + 10;
        else if (*s >= 'A' && *s <= 'F') digit = *s - 'A' + 10;
        else break;

        if (digit >= base) break;

        if (!seen_dot) {
            result = result * base + digit;
        } else {
            frac += digit / div;
            div *= base;
        }
    }

    return result + frac;
}

double parse_bin_hex_oct_ans_e_pi(const char *str, int16_t *ok) {
    *ok = 0;

    if (!str || !*str)
        return 0.0;

    char *cpy = strdup(str);
    charRm(cpy, ' ');

    int16_t sign = 1;
    int16_t pos = 0;
    int32_t base = 0;

    if (cpy[pos] == '-') {
        sign = -1;
        pos++;
    } else if (cpy[pos] == '+') {
        pos++;
    }

    bool isBinary = false; 
    if (strncasecmp(cpy + pos, "0x", 2) == 0) {
        base = 16;
        pos += 2;
    } else if (strncasecmp(cpy + pos, "0b", 2) == 0) {
        base = 2;
        pos += 2;
    } else if (strncasecmp(cpy + pos, "0o", 2) == 0) {
        base = 8;
        pos += 2;
    } else
        base = 10;

    int16_t num_start = pos;

    bool dot_seen = false;

    while (cpy[pos]) {
        if (cpy[pos] == '.') {
            if (dot_seen) break;
            dot_seen = true;
            pos++;
            continue;
        }

        int32_t digit;
        if (cpy[pos] >= '0' && cpy[pos] <= '9')
            digit = cpy[pos] - '0';
        else if (cpy[pos] >= 'a' && cpy[pos] <= 'f')
            digit = cpy[pos] - 'a' + 10;
        else if (cpy[pos] >= 'A' && cpy[pos] <= 'F')
            digit = cpy[pos] - 'A' + 10;
        else
            break;

        if (digit >= base)
            break;

        pos++;
    }

    if (pos == num_start) {
        SAFE_FREE(cpy);
        return 0.0;
    }

    double mult = 0.0;

    if (strcasecmp(cpy + pos, "pi") == 0)
        mult = PI;
    else if (strcasecmp(cpy + pos, "e") == 0)
        mult = E;
    else if (strcasecmp(cpy + pos, "ans") == 0) {
        if (isnan(Ans))
            puts("Warning: Ans is undefined");
        mult = Ans;
    } else {
        SAFE_FREE(cpy);
        return 0.0;
    }

    char buf[0x40];
    size_t len = pos - num_start;

    if (len >= sizeof(buf)) {   
        SAFE_FREE(cpy);
        return 0.0;
    }

    strncpy(buf, cpy + num_start, len);
    buf[len] = '\0';

    double value;

    if (!isBinary)
        value = parse_base_fraction(buf, base);
    else
        value = parse_base_fraction(buf, 2);

    *ok = 1;
    
    SAFE_FREE(cpy);
    return sign * value * mult;
}

bool has_top_level_operator(const char *s, const char *uniOps, const char **multiOps) {
    int32_t depth = 0;

    for (int32_t i = 0; s[i]; i++) {
        if (s[i] == '(') depth++;
        else if (s[i] == ')') depth--;

        if (depth == 0) {
            for (int32_t j = 0; multiOps[j]; j++) {
                int32_t len = strlen(multiOps[j]);
                if (strncmp(&s[i], multiOps[j], len) == 0)
                    return true;
            }

            if (strchr(uniOps, s[i]))
                return true;
        }
    }

    return false;
}

int16_t find_main_operator_full(const char *s, const char **multiOps, const char *uniOps, char *foundOp) {
    int32_t len = strlen(s);

    for (int32_t i = len - 1; i >= 0; i--) {

        int32_t depth = 0;
        bool in_quotes = false;

        for (int32_t k = 0; k <= i; k++) {
            if (s[k] == '\'')
                in_quotes = !in_quotes;

            if (!in_quotes) {
                if (s[k] == '(')
                    depth++;
                else if (s[k] == ')')
                    depth--;
            }
        }

        if (depth != 0 || in_quotes)
            continue;

        for (int32_t j = 0; multiOps[j]; j++) {

            int32_t oplen = strlen(multiOps[j]);
            int32_t start = i - oplen + 1;

            if (start < 0)
                continue;

            if (strncmp(&s[start], multiOps[j], oplen) == 0) {

                strncpy(foundOp, multiOps[j], oplen);
                foundOp[oplen] = '\0';
                return start;
            }
        }
    }

    for (int32_t i = len - 1; i >= 0; i--) {

        int32_t depth = 0;
        bool in_quotes = false;

        for (int32_t k = 0; k <= i; k++) {
            if (s[k] == '\'')
                in_quotes = !in_quotes;

            if (!in_quotes) {
                if (s[k] == '(')
                    depth++;
                else if (s[k] == ')')
                    depth--;
            }
        }

        if (depth != 0 || in_quotes)
            continue;

        if (strchr(uniOps, s[i])) {

            int32_t k = i - 1;
            while (k >= 0 && isspace((unsigned char)s[k]))
                k--;

            if (k < 0)
                continue;

            if (strchr(uniOps, s[k]) || s[k] == '(')
                continue;

            foundOp[0] = s[i];
            foundOp[1] = '\0';
            return i;
        }
    }

    return -1;
}

double eval(char *operation, bool mathlib) {
    char *functions[] = {
        "scale", "sqrt", "sin", "cos", "tan", "ln",
        "log10", "log2", "log", "floor", "ceil", "round",
        "fact", "sign", "sum", "rad", "deg", "trunc", "randf",
        "fah", "cel", "root", "rand", "mi", "km", "lb", "kg",
        "oct", "hex", "bin", "abs", "fabs", "len", "bmi", "feet",
        "meter", "cot", "gon", "chr", "asin", "acos", "atan", "acot",
        "isprime"
    };

    const char uniOps[] = "+-/*^%%&|<>";
    const char *multiOps[] = {
        "**", "&&", "||", "<<", ">>",
        "<=", "==", ">=", "!=", "^^", 
        NULL
    };

    enum paren_result result = parenthesis_check(operation);

    if (result != PAREN_OK) {

        switch (result) {
            case PAREN_MISSING_CLOSE:
                printf("eval: expected ')'\n");
                break;

            case PAREN_MISSING_OPEN:
                printf("eval: unexpected ')'\n");
                break;

            case PAREN_UNCLOSED_QUOTE:
                printf("eval: unclosed quote\n");
                break;

            default:
                break;
        }

        return NAN;
    }

    return CheckOperation(operation, functions, uniOps, multiOps, mathlib);
}

char *handle_cd_dash(char *address) {
    if (!last_directory) {
        printf("cd: no previous directory\n");
        return strdup(address);
    }

    printf("%s\n", last_directory);

    if (chdir(last_directory) != 0) {
        perror("cd");
        return "";
    }

    char new_cwd[0x400];
    if (getcwd(new_cwd, sizeof(new_cwd)) != NULL) {
        char *temp = last_directory;
        last_directory = strdup(address);
        SAFE_FREE(temp);
        return strdup(new_cwd);
    }

    return strdup(address);
}

void update_last_directory(char *address) {
    if (last_directory != NULL)
        SAFE_FREE(last_directory);

    last_directory = strdup(address);
}

char *handle_normal_cd(const char *path, char *address) {
    char old_cwd[0x400];
    if (!getcwd(old_cwd, sizeof(old_cwd))) {
        strcpy(old_cwd, ".");
    }

    if (strlen(path) == 0) {
    #ifdef _WIN32
        const char *home_path = getenv("USERPROFILE");
    #else
        const char *home_path = getenv("HOME");
    #endif
        if (!home_path) return strdup(address);

        if (chdir(home_path) == 0) {
            char new_cwd[0x400];
            if (getcwd(new_cwd, sizeof(new_cwd)) != NULL && strcmp(old_cwd, new_cwd) != 0)
                return strdup(new_cwd);
        }
    }

#ifdef _WIN32
    if (isalpha((unsigned char)path[0]) && path[1] == ':') {
        if (chdir(path) == 0) {
            char new_cwd[0x400];
            if (getcwd(new_cwd, sizeof(new_cwd)) != NULL && strcmp(old_cwd, new_cwd) != 0)
                return strdup(new_cwd);
        }
        return strdup(address);
    }
#endif

    if (strcmp(path, "..") == 0) {
        char new_path[0x400];
        strncpy(new_path, address, sizeof(new_path) - 1);
        new_path[sizeof(new_path) - 1] = '\0';

        char *last_slash = strrchr(new_path, '/');
    #ifdef _WIN32
        char *last_backslash = strrchr(new_path, '\\');
        if (!last_slash || (last_backslash && last_backslash > last_slash))
            last_slash = last_backslash;
    #endif

        if (last_slash != NULL && last_slash != new_path) {
            *last_slash = '\0';
        } else {
            new_path[1] = '\0';
        }

        if (chdir(new_path) == 0) {
            char new_cwd[0x400];
            if (getcwd(new_cwd, sizeof(new_cwd)) != NULL && strcmp(old_cwd, new_cwd) != 0)
                return strdup(new_cwd);
        }
    }

    if (strcmp(path, ".") == 0) {
        return strdup(address);
    }

    if (path[0] == '/') {
        if (chdir(path) == 0) {
            char new_cwd[0x400];
            if (getcwd(new_cwd, sizeof(new_cwd)) != NULL && strcmp(old_cwd, new_cwd) != 0)
                return strdup(new_cwd);
        }
    }

    char full_path[0x800];
    snprintf(full_path, sizeof(full_path), "%s/%s", address, path);

    for (char *p = full_path; *p; p++) {
        if (p[0] == '/' && p[1] == '/') {
            memmove(p, p + 1, strlen(p));
        }
    }

    if (chdir(full_path) == 0) {
        char new_cwd[0x400];
        if (getcwd(new_cwd, sizeof(new_cwd)) != NULL && strcmp(old_cwd, new_cwd) != 0)
            return strdup(new_cwd);
    }

    perror("cd");
    return strdup(address);
}

char **copyMat(char **dest, const char **src, uint16_t size) {
    dest = malloc(size * sizeof(char *));
    if (!dest) return NULL;

    for (uint16_t i = 0; i < size; i++) {
        dest[i] = strdup(src[i]);
    }

    return dest;
}

bool isalldigit(const char *s) {
    if (!s || !*s) return false;

    if (*s == '-')
        s++;

    bool hex = isHex(s);
    bool oct = isOct(s);
    bool bin = isBin(s);

    if (hex)
        return true;

    if (oct)
        return true;
    
    if (bin)
        return true;


    uint16_t dotCount = 0;
    const char *p = s;

    for (; *p; p++) {
        if (*p == '.') {
            if (++dotCount > 1)
                return false;
        }
        else if (*p == 'e' || *p == 'E') {
            //* i'll keep it that way
            // p++;
            // if (*p == '+' || *p == '-')
            //     p++;
            // if (!isdigit((unsigned char)*p))
            //     return false;

            // while (isdigit((unsigned char)*p))
            //     p++;

            // return *p == '\0';
            return false;
        }
        else if (!isdigit((unsigned char)*p)) {
            break;
        }
    }

    if (!*p)
        return true;

    return false;
}

uint8_t bsort(char **array, uint16_t count) {
    char *aux;
    uint8_t switches = 0;

    for(uint16_t i = 0; i < count - 1; i++) {
        for(uint16_t j = 0; j < count - i - 1; j++) {
            if(strcasecmp(array[j], array[j + 1]) > 0) {
                aux = array[j];
                array[j] = array[j + 1];
                array[j + 1] = aux;

                switches = 1;
            }
        }
        if(!switches)
            break;
    }
    return 1;
}

char *get_default_address(void) {
#ifdef _WIN32
    const char *home = getenv("USERPROFILE");
    return home ? strdup(home) : strdup("C:\\");
#else
    char *home = getenv("HOME");
    if (home) return strdup(home);
    struct passwd *pw = getpwuid(getuid());
    if (pw) return strdup(pw->pw_dir);
#endif
    return strdup(".");
}

void printc(const char *str, enum color4 initColor, enum color4 resetColor, ...) {
    setColor(initColor);

    va_list args;
    va_start(args, resetColor);
    vprintf(str, args);
    va_end(args);

    setColor(resetColor);
}

char* get_cpu_model(void) {
#ifdef _WIN32
    static char cpu[0x80];
    HKEY hKey;
    DWORD size = sizeof(cpu);

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                    "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                    0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return "Unknown";

    if (RegGetValueA(hKey, NULL, "ProcessorNameString", RRF_RT_REG_SZ, NULL, cpu, &size) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return "Unknown";
    }

    RegCloseKey(hKey);
    cpu[strcspn(cpu, "\n")] = '\0';
    return cpu;
#elif __linux__
    static char cpu[0x80];

    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (fp) {
        while (fgets(cpu, sizeof(cpu), fp)) {
            if (strncmp(cpu, "model name", 10) == 0) {
                fclose(fp);
                cpu[strcspn(cpu, "\n")] = '\0';
                char *colon = strchr(cpu, ':');
                return colon ? colon + 2 : "Unknown";
            }
        }
        fclose(fp);
    }

    fp = fopen("/sys/firmware/devicetree/base/model", "r");
    if (fp) {
        fgets(cpu, sizeof(cpu), fp);
        fclose(fp);
        cpu[strcspn(cpu, "\n")] = '\0';
        return cpu;
    }

    fp = fopen("/sys/devices/system/cpu/cpu0/uevent", "r");
    if (fp) {
        while (fgets(cpu, sizeof(cpu), fp)) {
            if (strncmp(cpu, "OF_COMPATIBLE_", 14) == 0) {
                fclose(fp);
                cpu[strcspn(cpu, "\n")] = '\0';
                return cpu;
            }
        }
        fclose(fp);
    }

    return "Unknown";
#elif __APPLE__
    static char cpu[0x80];
    uint16_t size = sizeof(cpu);
    if (sysctlbyname("machdep.cpu.brand_string", cpu, &size, NULL, 0) == 0) {
        cpu[strcspn(cpu, "\n")] = '\0';
        return cpu;
    }
    return "Unknown";
#endif
}

uint64_t get_total_ram_mb(void) {
#ifdef _WIN32
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    return status.ullTotalPhys / (1024 * 1024);
#elif __linux__
    uint32_t mem_kb = 0;
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) return -1;
    fscanf(fp, "MemTotal: %"PRIu32" kB", &mem_kb);
    SAFE_FCLOSE(fp);
    return mem_kb / 1024;
#elif __APPLE__
    int64_t mem;
    uint32_t len = sizeof(mem);
    if (sysctlbyname("hw.memsize", &mem, &len, NULL, 0) == 0)
        return (long)(mem / (1024 * 1024));
    return -1;
#endif
}

void setup_console(void) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}