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
#define U32_NAN (uint32_t)-1
#define U64_NAN (uint64_t)-1
#define MAX_CHAR (1ULL << 10)
#define TIME_FMT "%a %d %b %Y %H:%M:%S %z"

#define RM_FORCE 0b00000001
#define RM_BIN   0b00000010

#define BC_QUIET   0b00000001
#define BC_MATHLIB 0b00000010

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
} while (0)

#define SAFE_FCLOSE(file) do { \
    if (file) { \
        fclose(file); \
        file = NULL; \
    } \
} while (0)

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
void echoHandler(char *str);
bool isValidFile(char *file);
void setColor(color4_t color);
void enableAnsiIfNeeded(void);
char *revStr(const char *str);
char *extractPath(char **str);
void removeComments(char *str);
bool isalldigit(const char *s);
int8_t isDir(const char *path);
uint64_t get_total_ram_mb(void);
char *get_default_address(void);
void safe_lower_inplace(char *s);
char *unameCmdWin(uint8_t flags);
char *buildAliasPath(char *path);
char *tolowerstr(const char *str);
int16_t move_to_trash(char *path);
char *unameCmdLinux(uint8_t flags);
void charRm(char *str, int8_t targ);
char *handle_cd_dash(char *address);
char *find_andand_outside_quotes(char *s);
void update_last_directory(char *address);
uint8_t sort(char **array, uint16_t count);
char *strrm(char *str, const char *substr);
char *findFirstEqualOutsideQuotes(char *s);
double eval(char *operation, bool mathlib);
uint8_t is_pi_or_e_expression(const char *s);
int16_t rm_delete(char *path, uint8_t flags);
int16_t strchar(const char *str, int8_t chr);
int16_t strrchar(const char *str, int8_t chr);
bool isValidBcCommand(char *str, char *command);
char *defaultAddressReplace(const char *address);
uint16_t countIndex(const char *str, int8_t chr);
void createShortcut(char *instruction, char *path);
char **parseData(const char *str, uint16_t *count);
void lsCmdWin(const char *dirPath, uint8_t showAll);
void lsCmdLinux(const char *dirPath, uint8_t showAll);
void charReplace(char *str, int8_t targ, int8_t repl);
uint8_t myStrcasestr(const char *str, const char *sub);
uint16_t CountSubStr(const char *str, const char *sub);
char *handle_normal_cd(const char *path, char *address);
double parse_hex_pi_e_bin(const char *str, int16_t *ok);
char **readHistory(const char *address, uint32_t *lineCount);
char **copyMat(char **dest, const char **src, uint16_t size);
void printInFileNTimes(FILE *stream, char *str, int64_t count);
const char *strcasestr_ptr(const char *haystack, const char *needle);
void split_instruction_args(char *line, char **out_cmd, char **out_args);
void printc(const char *str, color4_t initColor, color4_t resetColor, ...);
void printTarg(const char *str, const char *targ, color4_t markColor, int8_t ignoreCase);
int16_t find_main_operator_full(const char *s, const char **multiOps, const char *uniOps, char *foundOp);
void GetProjDir(char *program_root, uint16_t root_size, char *data_folder, uint16_t data_size, char *history_path, uint16_t hist_size);
bool isalias(char *operation, char *args, const char **cmds, uint16_t cmdCount, char **address, char *history_path, char *data_folder, uint16_t isInsideBash);

#endif