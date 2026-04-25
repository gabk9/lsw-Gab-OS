#ifndef UTILS_H
#define UTILS_H

#ifndef _CRT_SECURE_NO_WARNINGS
    #define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef _CRT_NONSTDC_NO_WARNINGS
    #define _CRT_NONSTDC_NO_WARNINGS
#endif

#include <time.h>
#include <stdio.h>
#include <float.h>
#include "types.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

#ifdef _WIN64
    #include <direct.h>
    #include <shlobj.h>
    #include <windows.h>
    #include <sys/stat.h>

    #define strdup _strdup
    #define strcasecmp _stricmp
    #define strncasecmp _strnicmp
    
    #define cls(void) system("cls")
#else
    #include <pwd.h>
    #include <unistd.h>
    #include <libgen.h>
    #include <dirent.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <sys/ioctl.h>

    #ifdef __APPLE__
        #include <sys/sysctl.h>
    #endif

    #include <sys/utsname.h>

    #define MAX_PATH 0x104
    #define cls(void) system("clear")
#endif

#define RC_FILE "lswrc.txt"

#define E_VAR "E"
#define PI_VAR "PI"
#define ANS_VAR "ans"
#define INT_VAR "int"
#define INF_VAR "inf"
#define STR_VAR "str"
#define CHR_VAR "chr"
#define NONE_VAR "none"
#define TRUE_VAR "true"
#define BOOL_VAR "bool"
#define FLOAT_VAR "float"
#define FALSE_VAR "false"
#define RAND_MAX_VAR "rand_max"

#define HEX_PREF "0x"
#define OCT_PREF "0o"
#define BIN_PREF "0b"

#define EPS 1e-9

#define BC_PROMPT_COLOR 0x0D
#define GET_BASE_COLOR(color) (((color) < (LIGHT_BLUE)) ? (color) : (color) - 0x08)

#define T_CMP(num1, num2) (fabs((num1) - (num2)) < (EPS))

#define MAX_ARGS 0x20
#define MAX_CHAR 0x400
#define TIME_FMT "%a %d %b %Y %H:%M:%S %z"
#define I64_NAN ((int64_t)(INT64_MAX - 1ULL))
#define U32_NAN ((uint32_t)(UINT32_MAX - 1U))
#define MAX_SAFE_INT64_D  9223372036854775807.0
#define MIN_SAFE_INT64_D -9223372036854775808.0
#define U64_NAN ((uint64_t)(UINT64_MAX - 1ULL))

#define RM_FORCE 0x0001
#define RM_BIN   0x0002

#define BC_QUIET   0x0001
#define BC_MATHLIB 0x0002

#define LS_ALL 0x0001

#define DEFAULT_HISTSIZE 1000
#define HISTSIZE_MAX     10000
#define HISTSIZE_MIN     10

#define BASH_VERSION 0x0001
#define BASH_HELP    0x0002
#define BASH_ALL (BASH_VERSION | BASH_HELP)

#define U_KERN_NAME        0x0001
#define U_KERN_RELEASE     0x0002
#define U_MACHINE          0x0004
#define U_KERN_VERSION     0x0008
#define U_HOST_NAME        0x0010
#define U_OPERATING_SYSTEM 0x0020
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

#ifdef _WIN32
    char *unameCmdWin(uint8_t flags);
    LONG WINAPI handler(EXCEPTION_POINTERS *e);
    void lsCmdWin(const char *dirPath, uint8_t showAll);
#else
    char *unameCmdLinux(uint8_t flags);
    void lsCmdLinux(const char *dirPath, uint8_t showAll);
#endif


char randChr(void);
char *get_user(void);
void trim(char *str);
void initRandom(void);
char *charNumber(void);
char *linesNumber(void);
char *var2str(var buff);
char *linesNumber(void);
void trimEnd(char *str);
char *getBasePath(void);
void setup_console(void);
char *get_hostname(void);
char *get_time(char *fmt);
char *get_cpu_model(void);
float64 parse_len(char *s);
bool isBin(const char *str);
bool isHex(const char *str);
bool isOct(const char *str);
void trimBetween(char *str);
void setColor(color4 color);
char *myDirname(char *path);
char *echoHandler(char *str);
void sleepF(float64 seconds);
void enableAnsiIfNeeded(void);
char *revStr(const char *str);
char *extractPath(char **str);
void removeComments(char *str);
bool isalldigit(const char *s);
int8_t isDir(const char *path);
uint64_t get_total_ram_mb(void);
char *get_default_address(void);
int8_t isAppend(const char *str);
void safe_lower_inplace(char *s);
char *tolowerstr(const char *str);
int16_t move_to_trash(char *path);
char *get_env_var(const char *name);
void charRm(char *str, int8_t targ);
char *handle_cd_dash(char *address);
uint32_t getFileLength(FILE *stream);
char* findStarOutsideQuotes(char *s);
uint16_t getSavedHistSize(char *path);
uint16_t getSavedHistSize(char *path);
bool isIn(char needle, char *haystack);
char *buildLswRcPath(const char *path);
bool isValidBcFuncName(const char *str);
void shiftLeft_at(char *str, size_t pos);
int32_t bc_strcmp(char *str1, char *str2);
char *find_andand_outside_quotes(char *s);
void update_last_directory(char *address);
char *eval(char *operation, bool mathlib);
char *strrm(char *str, const char *substr);
char *findFirstEqualOutsideQuotes(char *s);
uint8_t bsort(char **array, uint16_t count);
int16_t rm_delete(char *path, uint8_t flags);
int16_t strchar(const char *str, int8_t chr);
int16_t strrchar(const char *str, int8_t chr);
paren_status parenthesis_check(const char *s);
bool is_wrapped_by_parentheses(const char *s);
bool isValidFolderOrFileName(const char *name);
bool isValidBcCommand(char *str, char *command);
char *extractCommandOrKey(char *src, char **arg);
char *defaultAddressReplace(const char *address);
uint16_t countIndex(const char *str, int8_t chr);
char* findCharOutsideQuotes(char *s, char target);
char *extract_instruction(char *str, char **args);
void createShortcut(char *instruction, char *path);
char *bc_strcat(const char *dest, const char *src);
char **parseData(const char *str, uint16_t *count);
bool isBcVariable(const char *str, bool *shouldError);
void charReplace(char *str, int8_t targ, int8_t repl);
int16_t injectEscape(char *str, const char *error_str);
void getItemTypeStr(char *buff, size_t size, var item);
uint8_t myStrcasestr(const char *str, const char *sub);
char *handle_normal_cd(const char *path, char *address);
char *getKeyVal(const char *key_name, const char *path);
void num_snprintf(char *buff, size_t size, float64 num);
float64 parse_base_fraction(const char *s, int8_t base);
void int64_to_hex_min(int64_t v, char *out, size_t size);
bool isBetweenQuotes(const char *action, int16_t quoteMode);
char **readHistory(const char *address, uint32_t *lineCount);
char **copyMat(char **dest, const char **src, uint16_t size);
void printInFileNTimes(FILE *stream, char *str, int64_t count);
char **extract_args(char *args, uint16_t *argc, char *firstArg);
int8_t getInvalidEscape(const char *str, const char *error_str);
float64 parse_bin_hex_oct_ans_e_pi(const char *str, int16_t *ok);
bool isKeyRepeated(const char *data_folder, const char *key_name);
const char *strcasestr_ptr(const char *haystack, const char *needle);
void saveHist(char *operation, char *history_path, char *data_folder);

__attribute__((format(printf, 1, 4)))
void printc(const char *str, color4 initColor, color4 resetColor, ...);

void split_instruction_args(char *line, char **out_cmd, char **out_args);
void printTarg(const char *str, const char *targ, color4 markColor, int8_t ignoreCase);
int16_t find_main_operator_full(const char *s, const char **multiOps, const char *uniOps, char *foundOp);
void GetProjDir(char *program_root, uint16_t root_size, char *data_folder, uint16_t data_size, char *history_path, uint16_t hist_size);
bool isalias(char *operation, char *args, const char **cmds, char **address, char *history_path, char *data_folder, uint16_t isInsideBash);

#endif
