#ifndef UTILS_H
#define UTILS_H

#include <math.h>
#include <time.h>
#include <ctype.h>
#include <wchar.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <float.h>
#include "s_math.h"
#include <stdbool.h>
#include <inttypes.h>
#include "terminal.h"
#include "CheckCmd.h"

#ifdef _WIN32
    #include <direct.h>
    #include <shlobj.h>
    #include <windows.h>
    
    #define cls(void) system("cls")
#else
    #include <pwd.h>
    #include <unistd.h>
    #include <libgen.h>
    #include <dirent.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <sys/ioctl.h>
    #include <sys/utsname.h>

    #define MAX_PATH 0x104
    #define cls(void) system("clear")
#endif

#define MAX_ARGS 0x20
#define MAX_CHAR (1ULL << 10)
#define TIME_FMT "%a %d %b %Y %H:%M:%S %z"
#define U32_NAN ((uint32_t)(UINT32_MAX - 1u))
#define MAX_SAFE_INT64_D  9223372036854775807.0
#define MIN_SAFE_INT64_D -9223372036854775808.0
#define U64_NAN ((uint64_t)(UINT64_MAX - 1ull))

#define RM_FORCE 0b00000001
#define RM_BIN   0b00000010

#define BC_QUIET   0b00000001
#define BC_MATHLIB 0b00000010

#define LS_ALL 0b00000001

#define DEFAULT_HISTSIZE 1000

#define BASH_VERSION 0b00000001
#define BASH_HELP    0b00000010
#define BASH_ALL (BASH_VERSION | BASH_HELP)

#define U_KERN_NAME        0b00000001
#define U_KERN_RELEASE     0b00000010
#define U_MACHINE          0b00000100
#define U_KERN_VERSION     0b00001000
#define U_HOST_NAME        0b00010000
#define U_OPERATING_SYSTEM 0b00100000
#define U_ALL (U_KERN_NAME | U_KERN_RELEASE | U_MACHINE | U_KERN_VERSION | U_HOST_NAME | U_OPERATING_SYSTEM)

#define SAFE_FREE(ptr) do { \
    if (ptr) { \
        free(ptr); \
        ptr = NULL; \
    } \
} while (false)

#define SAFE_FCLOSE(stream) do { \
    if (stream) { \
        fclose(stream); \
        stream = NULL; \
    } \
} while (false)

typedef enum color4_t {
    BLACK, BLUE, GREEN, CYAN, RED, MAGENTA, YELLOW,
    WHITE, GRAY, LIGHT_BLUE, LIGHT_GREEN, LIGHT_CYAN,
    LIGHT_RED, LIGHT_MAGENTA, LIGHT_YELLOW, BRIGHT_WHITE
} color4_t;

char randChr(void);
char *get_user(void);
void trim(char *str);
void initRandom(void);
char *charNumber(void);
char *linesNumber(void);
char *linesNumber(void);
void trimEnd(char *str);
char *getBasePath(void);
void setup_console(void);
char *get_hostname(void);
char *get_time(char *fmt);
double parse_len(char *s);
char *get_cpu_model(void);
void sleepF(double seconds);
bool isBin(const char *str);
bool isHex(const char *str);
bool isOct(const char *str);
void trimBetween(char *str);
char *myDirname(char *path);
char *echoHandler(char *str);
void setColor(color4_t color);
void enableAnsiIfNeeded(void);
char *revStr(const char *str);
char *extractPath(char **str);
int64_t hex_to_long(char *str);
void removeComments(char *str);
bool isalldigit(const char *s);
int8_t isDir(const char *path);
uint64_t get_total_ram_mb(void);
char *get_default_address(void);
int8_t isAppend(const char *str);
void safe_lower_inplace(char *s);
char *unameCmdWin(uint8_t flags);
bool isValidAction(char *action);
char *buildLswRcPath(char *path);
char *tolowerstr(const char *str);
int16_t move_to_trash(char *path);
char *unameCmdLinux(uint8_t flags);
char *get_env_var(const char *name);
void charRm(char *str, int8_t targ);
char *handle_cd_dash(char *address);
char* findStarOutsideQuotes(char *s);
uint16_t getSavedHistSize(char *path);
uint16_t getSavedHistSize(char *path);
char *find_andand_outside_quotes(char *s);
void update_last_directory(char *address);
char *strrm(char *str, const char *substr);
char *findFirstEqualOutsideQuotes(char *s);

#ifdef _WIN32
LONG WINAPI handler(EXCEPTION_POINTERS *e);
#endif

double eval(char *operation, bool mathlib);
uint8_t bsort(char **array, uint16_t count);
uint8_t is_pi_or_e_expression(const char *s);
int16_t rm_delete(char *path, uint8_t flags);
int16_t strchar(const char *str, int8_t chr);
uint16_t getHistSizeConfig(char *lswrc_path);
int16_t strrchar(const char *str, int8_t chr);
bool isValidFolderOrFileName(const char *name);
bool isValidBcCommand(char *str, char *command);
char *extractCommandOrKey(char *src, char **arg);
char *defaultAddressReplace(const char *address);
uint16_t countIndex(const char *str, int8_t chr);
char* findCharOutsideQuotes(char *s, char target);
char *extract_instruction(char *str, char **args);
void createShortcut(char *instruction, char *path);
char **parseData(const char *str, uint16_t *count);
void lsCmdWin(const char *dirPath, uint8_t showAll);
void lsCmdLinux(const char *dirPath, uint8_t showAll);
void charReplace(char *str, int8_t targ, int8_t repl);
uint8_t myStrcasestr(const char *str, const char *sub);
char *handle_normal_cd(const char *path, char *address);
double parse_hex_pi_e_bin(const char *str, int16_t *ok);
void int64_to_hex_min(int64_t v, char *out, size_t size);
bool isKeyRepeated(char *data_folder, const char *key_name);

__attribute__((unused))
char **readHistory(const char *address, uint32_t *lineCount);

char **copyMat(char **dest, const char **src, uint16_t size);
void printInFileNTimes(FILE *stream, char *str, int64_t count);
char **extract_args(char *args, uint16_t *argc, char *firstArg);
uint8_t echoNtimes(char *instruction, char *copy, uint16_t reps);
const char *strcasestr_ptr(const char *haystack, const char *needle);
void saveHist(char *operation, char *history_path, char *data_folder);
void split_instruction_args(char *line, char **out_cmd, char **out_args);
void printc(const char *str, color4_t initColor, color4_t resetColor, ...);
uint8_t echoFileNtimes(char *instruction, char *copy, uint16_t reps, uint16_t file);
bool has_top_level_operator(const char *s, const char *uniOps, const char **multiOps);
void printTarg(const char *str, const char *targ, color4_t markColor, int8_t ignoreCase);
int16_t find_main_operator_full(const char *s, const char **multiOps, const char *uniOps, char *foundOp);
void GetProjDir(char *program_root, uint16_t root_size, char *data_folder, uint16_t data_size, char *history_path, uint16_t hist_size);
bool isalias(char *operation, char *args, const char **cmds, uint16_t cmdCount, char **address, char *history_path, char *data_folder, uint16_t isInsideBash);

#endif