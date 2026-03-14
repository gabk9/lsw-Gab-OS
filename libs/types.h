#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdbool.h>

typedef enum types {
    BC_STR, BC_FLOAT, BC_INT,
    BC_BOOL, BC_CHAR, BC_NONE
} eval_types;

typedef double (*MathFunc)(char *operation);

typedef struct Functions {
    const char *name;
    MathFunc func;
    eval_types returnType;
} FuncEntry;

typedef struct {
    eval_types type;
    union {
        double num;
        char *str;
        bool boolean;
    };
} evalOut;

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