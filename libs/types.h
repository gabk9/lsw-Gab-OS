#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdbool.h>

typedef enum types {
    BC_STR, BC_FLOAT, BC_INT,
    BC_BOOL, BC_CHR, BC_NONE
} eval_ty;

typedef char *(*S_Func)(char *operation);
typedef double (*F_Func)(char *operation);

typedef struct Functions {
    eval_ty returnType;
    const char *name;
    union func {
        F_Func f;
        S_Func s;
    } fn;
} FuncEntry;

typedef struct eval_var {
    eval_ty type;
    union {
        double num;
        // int64_t integer;
        char *str;
        bool boolean;
    };
} var;

extern var Ans;

typedef enum color {
    BLACK, BLUE, GREEN, CYAN, RED, MAGENTA, YELLOW,
    WHITE, GRAY, LIGHT_BLUE, LIGHT_GREEN, LIGHT_CYAN,
    LIGHT_RED, LIGHT_MAGENTA, LIGHT_YELLOW, BRIGHT_WHITE
} color4;

typedef enum paren_result {
    PAREN_OK,
    PAREN_MISSING_CLOSE,
    PAREN_MISSING_OPEN,
    PAREN_UNCLOSED_QUOTE
} paren_status;

#endif
