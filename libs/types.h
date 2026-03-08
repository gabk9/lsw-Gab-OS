#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

typedef enum returns {
    BC_STR, BC_FLOAT, BC_INT,
    BC_BOOL, BC_CHAR, BC_NONE
} evalRet;

typedef double (*MathFunc)(char *operation);

typedef struct Functions {
    const char *name;
    MathFunc func;
    evalRet returnType;
} FuncEntry;

typedef struct {
    evalRet type;
    union {
        double num;
        char *str;
        int32_t boolean;
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