#include "utils.h"

#if !defined(_WIN32) && !defined(__linux__) && !defined(__APPLE__) && !defined(__ANDROID__)
    #error "Operational system not recognized, terminating program!!"
#endif

double parse_str_func(char *operation, const FuncEntry function) {    

    if (function.returnType != BC_STR && function.returnType != BC_CHAR) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("invalid function return type\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

        return NAN;
    }

    if (!isValidBcFuncName(function.name)) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("invalid function name: '%s()'\n", GetBaseColor(BC_PROMPT_COLOR), WHITE, function.name);

        return NAN;
    }

    bool isChr = function.returnType == BC_CHAR;

    char *buff = function.fn.s(operation);

    if (!buff)
        return NAN;

    if (!isChr) {
        size_t len = strlen(buff);

        if (len < 2) {
            SAFE_FREE(buff);
            return NAN;
        }

        if (buff[len-1] == '"') {
            buff[len-1] = '\0';
            len--;
        }
        if (*buff == '"') {
            memmove(buff, buff + 1, len+1);
            len--;
        }
    }    

    evalOut tmp = h_atof(buff, true);

    double val = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

    SAFE_FREE(buff);

    return val;
}

static uint8_t isnull(int32_t count, ...) {

    if (count < 1) {
        fprintf(stderr, "invalid count for <count>\n");
        exit(EXIT_FAILURE);
    }

    va_list args;
    va_start(args, count);

    uint8_t nullCount = 0;
    for (int8_t i = 0; i < count; i++) {
        void *ptr = va_arg(args, void *);

        if (!ptr)
            nullCount++;
    }

    va_end(args);
    return nullCount;
}

//! unused
char *find_top_level_comma(char *s) {
    int16_t level = 0;

    for (char *p = s; *p; p++) {
        if (*p == '(') level++;
        else if (*p == ')') level--;
        else if (*p == ',' && level == 0)
            return p;
    }           
    return NULL;
}

uint16_t count_top_level_commas(const char *s) {
    int32_t level = 0, count = 0;

    for (; *s; s++) {
        if (*s == '(') level++;
        else if (*s == ')') level--;
        else if (*s == ',' && level == 0)
            count++;
    }
    return count;
}

static double numericDebug(const char *buf) {
    if (strncasecmp(buf, BIN_PREF, strlen(BIN_PREF)) == 0) {
        size_t len = strlen(buf);

        const size_t pref_len = strlen(BIN_PREF);

        if (len <= pref_len) {
            printc("eval", BC_PROMPT_COLOR, WHITE);
            printf(": ");
            printc("invalid binary literal\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

            return NAN;
        }

        size_t end = len - 1;
        while (isIn(buf[end], "kmbtKMBT")) end --;

        for (size_t i = pref_len; i <= end; i++) {
            if (buf[i] != '0' && buf[i] != '1') {
                printc("eval", BC_PROMPT_COLOR, WHITE);
                printf(": ");
                printc("invalid binary digit: '%c\n", GetBaseColor(BC_PROMPT_COLOR), WHITE, buf[i]);

                break;
            }
        }

        return NAN;
    } else if (strncasecmp(buf, HEX_PREF, strlen(HEX_PREF)) == 0) {
        size_t len = strlen(buf);

        const size_t pref_len = strlen(HEX_PREF);

        if (len <= pref_len) {
            printc("eval", BC_PROMPT_COLOR, WHITE);
            printf(": ");
            printc("invalid hexadecimal literal\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

            return NAN;
        }

        for (size_t i = pref_len; buf[i]; i++) {

            if (isIn(buf[i], "kmbt") || isIn(buf[i], "KMBT")) {
                printc("eval", BC_PROMPT_COLOR, WHITE);
                printf(": ");
                printc("hexadecimal literal does not support suffixes\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

                return NAN;
            }

            if (!isxdigit(buf[i])) {
                printc("eval", BC_PROMPT_COLOR, WHITE);
                printf(": ");
                printc("invalid hexadecimal digit: '%c'\n", GetBaseColor(BC_PROMPT_COLOR), WHITE, buf[i]);

                break;
            }
        }

        return NAN;
    } else if (strncasecmp(buf, OCT_PREF, strlen(OCT_PREF)) == 0) {
        size_t len = strlen(buf);

        const size_t pref_len = strlen(OCT_PREF);

        if (len <= pref_len) {
            printc("eval", BC_PROMPT_COLOR, WHITE);
            printf(": ");
            printc("invalid octal literal\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

            return NAN;
        }

        size_t end = len - 1;
        while (isIn(buf[end], "kmbt") || isIn(buf[end], "KMBT")) end --;

        for (size_t i = pref_len; i <= end; i++) {
            if (buf[i] < '0' || buf[i] > '7') {
                printc("eval", BC_PROMPT_COLOR, WHITE);
                printf(": ");
                printc("invalid octal digit: '%c'\n", GetBaseColor(BC_PROMPT_COLOR), WHITE, buf[i]);

                break;
            }
        }

        return NAN;
    }

    printc("eval", BC_PROMPT_COLOR, WHITE);
    printf(": ");
    printc("invalid literal prefix: '%c'\n", GetBaseColor(BC_PROMPT_COLOR), WHITE, *buf);

    return NAN;
}

static double mathlibPart(char *buf, bool mathlib) {
    const struct {
        char suffix;
        double mult;
    } suffix[] = {
        {.suffix = 'k', .mult = 1e3},
        {.suffix = 'm', .mult = 1e6},
        {.suffix = 'b', .mult = 1e9},
        {.suffix = 't', .mult = 1e12},
    };

    bool has_exp = strncasecmp(buf, HEX_PREF, strlen(HEX_PREF)) == 0;
    bool allow_suffix = (!isHex(buf) && !has_exp);

    size_t len = strlen(buf);
    if (allow_suffix) {
        for (size_t mi = 0; mi < sizeof(suffix) / sizeof(*suffix); mi++) {
            if (len > 1 && (buf[len-1] == suffix[mi].suffix || buf[len-1] == toupper(suffix[mi].suffix))) {
                buf[len-1] = '\0';
                if (buf[len-2] == '!')
                    return 0.0;

                char *buff = eval(buf, mathlib);

                if (!buff)
                    return NAN;

                evalOut tmp = h_atof(buff, mathlib);
                double num = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

                SAFE_FREE(buff);
                return (isnan(num)) ? NAN : num * suffix[mi].mult;
            }
        }
    }

    if (strcmp(buf, PI_VAR) == 0) return PI;
    else if (strcmp(buf, E_VAR) == 0)  return E;

    uint16_t i = 0;
    while (buf[i] && (isdigit(buf[i]) || buf[i] == '.' || buf[i] == ',' || buf[i] == '-'))
        i++;

    if (i > 0 && strcmp(buf + i, E_VAR) == 0) {
        char temp[0x40];
        strncpy(temp, buf, i);
        temp[i] = '\0';
        char *buff = eval(temp, mathlib);

        if (!buff)
            return NAN;

        evalOut tmp = h_atof(buff, mathlib);
        double num = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

        SAFE_FREE(buff);
        return (isnan(num)) ? NAN : num * PI;
    }else if (i > 0 && strcmp(buf + i, E_VAR) == 0) {
        char temp[0x40];
        strncpy(temp, buf, i);
        temp[i] = '\0';
        char *buff = eval(temp, mathlib);

        if (!buff)
            return NAN;

        evalOut tmp = h_atof(buff, mathlib);
        double num = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

        SAFE_FREE(buff);
        return (isnan(num)) ? NAN : num * E;
    }

    return (double)U64_NAN;
}

evalOut h_atof(const char *str, bool mathlib) {

    if (!str || !*str) 
        return (evalOut){.type = BC_FLOAT, .num = NAN};

    char buf[0x80];
    strncpy(buf, str, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    trim(buf);
    trimEnd(buf);

    size_t len = strlen(buf);
    if (len == 0)
        return (evalOut){.type = BC_FLOAT, .num = NAN};

    if (len > 1 && mathlib && buf[len-1] == '!')
        return (evalOut){.type = BC_INT, .num = s_fact(buf)};

    bool isUnaryNeg = false;
    bool isUnaryNot = false;

    while (*buf == '-' || *buf == '~') {
        switch (*buf) {
            case '-':
                isUnaryNeg = !isUnaryNeg;
                break;
            case '~':
                isUnaryNot = !isUnaryNot;
                break;
        }

        memmove(buf, buf+1, strlen(buf)+1);
        trim(buf);
    }


    if (strcmp(buf, NONE_VAR) == 0) {
        if (isUnaryNeg) {
            printc("eval", BC_PROMPT_COLOR, WHITE);
            printf(": ");
            printc("bad operand type for unary negative(-): '"NONE_VAR"'\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);
        } else if (isUnaryNot) {
            printc("eval", BC_PROMPT_COLOR, WHITE);
            printf(": ");
            printc("bad operand type for unary not(~): '"NONE_VAR"'\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);
        }

        return (evalOut){.type = BC_FLOAT, .num = NAN};
    }

    bool isInf = strcasecmp(buf, INF_VAR) == 0;
    if (mathlib && isInf) {

        if (isInf && strcmp(buf, INF_VAR) != 0)
        return (evalOut){.type = BC_FLOAT, .num = 0.0};

        if (isUnaryNot) {
            printc("eval", BC_PROMPT_COLOR, WHITE);
            printf(": ");
            printc("bad operand type for unary not(~) '"INF_VAR"'\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);
            return (evalOut){.type = BC_FLOAT, .num = NAN};
        }

        return (evalOut){.type = BC_FLOAT, .num = isUnaryNeg ? -INFINITY : INFINITY};
    } 

    bool isAns = mathlib && strcmp(buf, OLD_ANSWER_STR) == 0;

    if (isAns && !Ans) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("'ans' is undefined\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);
        return (evalOut){.type = BC_FLOAT, .num = NAN};
    }

    if (isUnaryNeg) {
        if (!*buf) {
            printc("eval", BC_PROMPT_COLOR, WHITE);
            printf(": ");
            printc("missing value for unary negative(-)\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

            return (evalOut){.type = BC_FLOAT, .num = NAN};
        }

        double num;
        if (isAns) {
            evalOut tmp = h_atof(Ans, mathlib);
            num = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;
        } else {
            char *buff = eval(buf, mathlib);

            if (!buff)
                return (evalOut){.type = BC_FLOAT, .num = NAN};

            evalOut tmp = h_atof(buff, mathlib);
            num = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

            SAFE_FREE(buff);
        }

        if (isnan(num))
            return (evalOut){.type = BC_FLOAT, .num = NAN};

        if (num < MIN_SAFE_INT64_D || num > MAX_SAFE_INT64_D) {
            printc("eval", BC_PROMPT_COLOR, WHITE);
            printf(": ");
            printc("numeric overflow (too large)\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

            return (evalOut){.type = BC_FLOAT, .num = NAN};
        }

        return (evalOut){.type = BC_FLOAT, .num = -num};
    } else if (isUnaryNot) {
        if (!*buf) {
            printc("eval", BC_PROMPT_COLOR, WHITE);
            printf(": ");
            printc("missing value for unary not(~)\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

            return (evalOut){.type = BC_FLOAT, .num = NAN};
        }
        
        double num;

        if (isAns) {
            evalOut tmp = h_atof(Ans, mathlib);
            num = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;
        } else {
            char *buff = eval(buf, mathlib);

            if (!buff)
                return (evalOut){.type = BC_FLOAT, .num = NAN};

            evalOut tmp = h_atof(buff, mathlib);
            num = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

            SAFE_FREE(buff);
        }

        if (isnan(num))
            return (evalOut){.type = BC_FLOAT, .num = NAN};

        if (num < MIN_SAFE_INT64_D || num > MAX_SAFE_INT64_D) {
            printc("eval", BC_PROMPT_COLOR, WHITE);
            printf(": ");
            printc("numeric overflow (too large)\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

            return (evalOut){.type = BC_FLOAT, .num = NAN};
        }

        if (!CLOSE_ENOUGH(num, (int64_t)num)) {
            printc("eval", BC_PROMPT_COLOR, WHITE);
            printf(": ");
            printc("unary not(~) requires an integer\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

        return (evalOut){.type = BC_FLOAT, .num = NAN};
        }

        int64_t value = (int64_t)num;
        value = ~value;
        return (evalOut){.type = BC_INT, .num = (double)value};
    }

    if (isAns) {
        if (isBetweenQuotes(Ans, 1)) {
            printc("eval", BC_PROMPT_COLOR, WHITE);
            printf(": ");
            printc("cannot operate with strings\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

            return (evalOut){.type = BC_FLOAT, .num = NAN};
        }

        evalOut tmp = h_atof(Ans, mathlib);
        double val = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

        if (isnan(val))
            return (evalOut){.type = BC_FLOAT, .num = NAN};

        eval_types type = CLOSE_ENOUGH(val, (int64_t)val) ? BC_INT : BC_FLOAT;

        return (evalOut){.type = type, .num = val};
    }

    if (strcmp(buf, TRUE_VAR) == 0)
        return (evalOut){.type = BC_BOOL, .boolean = true};
    else if (strcmp(buf, FALSE_VAR) == 0)
        return (evalOut){.type = BC_BOOL, .boolean = false};

    len = strlen(buf);

    if (isBetweenQuotes(buf, 0)) {
        if (!injectEscape(buf, "eval"))
            return (evalOut){.type = BC_FLOAT, .num = NAN};

        size_t oldLen = len;
        len = strlen(buf);
        bool isNullChr = !buf[1] && oldLen != len;
        
        if (buf[len-1] == '\'') {
            buf[len-1] = '\0';
            len--;
        }
        if (*buf == '\'') {
            memmove(buf, buf+1, len+1);
            len--;
        }

        if (len > 1) {
            printc("eval", BC_PROMPT_COLOR, WHITE);
            printf(": ");
            printc("to use single quotes it must be a single character\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

            return (evalOut){.type = BC_FLOAT, .num = NAN};
        } else if (!isNullChr && len < 1) {
            printc("eval", BC_PROMPT_COLOR, WHITE);
            printf(": ");
            printc("missing the character inside quotes\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

            return (evalOut){.type = BC_FLOAT, .num = NAN};
        }

        return (evalOut){.type = BC_INT, .num = (unsigned char)*buf};
    }

    if (mathlib) {            
        int16_t ok = 0;
        double hex_pi_e = parse_bin_hex_oct_ans_e_pi(buf, &ok);

        if (isnan(hex_pi_e))
            return (evalOut){.type = BC_FLOAT, .num = NAN};

        if (ok)
            return (evalOut){.type = BC_FLOAT, .num = hex_pi_e};
    }

    bool is_hex = isHex(buf);

    bool is_octal = isOct(buf);
    
    bool is_bin = isBin(buf);

    if (*buf == '0' && buf[1] && buf[1] != '.'&& !is_bin && !is_octal && !is_hex)
        return (evalOut){.type = BC_FLOAT, .num = numericDebug(buf)};

    if (mathlib) {
        double tmp = mathlibPart(buf, mathlib);

        if (tmp != (double)U64_NAN) {
            eval_types type = CLOSE_ENOUGH(tmp, (int64_t)tmp) ? BC_INT : BC_FLOAT;
            return (evalOut){.type = type, .num = tmp};
        }
    }

    if (is_hex)
        return (evalOut){.type = BC_INT, .num = (double)hex_to_long(buf)};
    else if (is_octal)
        return (evalOut){.type = BC_INT, .num = (double)strtol(buf+strlen(OCT_PREF), NULL, 8)};
    else if (is_bin)
        return (evalOut){.type = BC_INT, .num = (double)parseBinToInt(buf)};

    if (*buf == '"' && buf[len-1] == '"') {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("cannot operate with string type values\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

        return (evalOut){.type = BC_FLOAT, .num = NAN};
    }

    if (!isalldigit(buf)) {
        bool shouldError;
        bool isValid = isBcVariable(buf, &shouldError);

        if (!isValid && shouldError) {
            for (size_t i = 0; buf[i]; i++) {
                if (isIn(buf[i], "()\"'!. _+-/*^%%&|<>"))
                    continue;

                if (!isalnum((unsigned char)buf[i])) {
                    printc("eval", BC_PROMPT_COLOR, WHITE);
                    printf(": ");
                    printc("illegal character: '%c'\n", GetBaseColor(BC_PROMPT_COLOR), WHITE, buf[i]);

                    return (evalOut){.type = BC_FLOAT, .num = NAN};
                }
            }

            printc("eval", BC_PROMPT_COLOR, WHITE);
            printf(": ");
            printc("invalid syntax\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

            return (evalOut){.type = BC_FLOAT, .num = NAN};
        }
    }

    double result = (!mathlib && isHex(buf)) ? 0.0 : atof(buf);

    eval_types type = CLOSE_ENOUGH(result, (int64_t)result) ? BC_INT: BC_FLOAT;

    return (evalOut){.type = type, .num = isnan(result) ? 0.0 : result};
}

int64_t parseBinToInt(const char *str) {
    int64_t n = 0;
    int32_t bits = 0;

    for (uint16_t i = 2; str[i]; i++) {
        n = (n << 1) | (str[i] - '0');
        bits++;
    }

    if (str[2] == '1') {
        n -= 1 << bits;
    }

    return n;
}

static uint8_t validPtrFuncArgs(char *arg, const char *error_str) {
    size_t len = strlen(arg);

    if (!len)
        return 1;

    if (!isBetweenQuotes(arg, 1)) {
        char *buff = eval(arg, true);

        if (!buff)
            return 0;

        if (!isBetweenQuotes(buff, 1)) {
            printc("eval", BC_PROMPT_COLOR, WHITE);
            printf(": ");
            printc("%s() requires an argument of type 'str'\n", GetBaseColor(BC_PROMPT_COLOR), WHITE, error_str);

            SAFE_FREE(buff);
            return 0;
        }

        SAFE_FREE(buff);
    }

    return 1;
}

static uint16_t countCommaOutsideQuotesAndParenthesis(const char *str, uint8_t quoteType) {
    uint16_t count = 0;
    bool insideQuotes = false;
    int32_t parenLevel = 0;

    if (!str) {
        return 0;
    }

    while (*str) {
        if (*str == (char)quoteType && parenLevel == 0) {
            insideQuotes = !insideQuotes;
        }
        else if (*str == '(' && !insideQuotes) {
            parenLevel++;
        }
        else if (*str == ')' && !insideQuotes) {
            if (parenLevel > 0)
                parenLevel--;
        }
        else if (*str == ',' && !insideQuotes && parenLevel == 0) {
            count++;
        }

        str++;
    }

    return count;
}

__attribute__((unused))
static uint16_t countCommaOutsideQuotes(const char *str, uint8_t quoteType) {
    uint16_t count = 0;
    bool insideQuotes = false;
    
    if (str == NULL) {
        return 0;
    }
    
    while (*str) {
        if (*str == (char)quoteType) {
            insideQuotes = !insideQuotes;
        } else if (*str == ',' && !insideQuotes) {
            count++;
        }
        str++;
    }
    
    return count;
}

__attribute__((unused))
static uint16_t countCommaOutsideParenthesis(const char *str) {
    uint16_t count = 0;
    int32_t parenLevel = 0;

    if (!str) {
        return 0;
    }

    while (*str) {
        if (*str == '(') {
            parenLevel++; 
        } else if (*str == ')') {
            if (parenLevel > 0)
                parenLevel--;
        } else if (*str == ',' && parenLevel == 0) {
            count++;
        }
        str++;
    }

    return count;
}

char *bc_parse_str(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NULL;
    operation = p;

    size_t len = strlen(operation);

    if (!len || countCommaOutsideQuotesAndParenthesis(operation, '"') != 0) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("str() requires exactly 1 argument\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

        return NULL;
    }

    char *buff = eval(operation, true);

    if (!buff)
        return NULL;

    len = strlen(buff);
    if (*buff != '"' && buff[len-1] != '"') {
        char *buff2 = malloc(len+3);
        buff2[len+3] = '\0';

        snprintf(buff2, len+3, "\"%s\"", buff);
        SAFE_FREE(buff);
        return buff2;
    }

    return buff;
}

double bc_parse(char *operation) {

    bool enablePrecision = strncmp(operation, "float", 5) == 0;

    char *p = strchr(operation, '(');
    if (!p)
        return NAN;

    operation = p;

    if (countCommaOutsideQuotesAndParenthesis(operation, '"') != 0) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("%s() requires exactly 1 argument\n", GetBaseColor(BC_PROMPT_COLOR), WHITE,
                enablePrecision ? "float" : "int");

        return NAN;
    }

    char *buff = eval(operation, true);
    if (!buff)
        return NAN;

    double num;

    if (isBetweenQuotes(buff, 2)) {
        size_t len = strlen(buff);
        memmove(buff, buff + 1, len - 2);
        buff[len-2] = '\0';

        evalOut tmp = h_atof(buff, true);
        num = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;
    } else {
        evalOut tmp = h_atof(buff, true);
        num = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;
    }

    SAFE_FREE(buff);

    if (isnan(num))
        return NAN;

    if (!enablePrecision && !CLOSE_ENOUGH(num, (int64_t)num))
        num = trunc(num);

    return num;
}

double bc_len(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p+1;

    size_t len = strlen(operation);
    operation[len-1] = '\0';
    len--;

    if (!len || countCommaOutsideQuotesAndParenthesis(operation, '"') != 0) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("len() requires exactly 1 argument\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

        return NAN;
    }

    char *buff = eval(operation, true);

    if (!buff)
        return NAN;

    if (!injectEscape(buff, "eval"))
        return NAN;

    if (!validPtrFuncArgs(buff, "len"))
        return NAN;

    len = strlen(buff);

    if (buff[len-1] == '"') {
        buff[len-1] = '\0';
        len--;
    } if (*buff == '"') {
        memmove(buff, buff+1, len+1);
        len--;
    }

    SAFE_FREE(buff);

    return (double)len;
}

double s_abs(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    char *buff = eval(operation, true);

    evalOut tmp = h_atof(buff, true);
    double value = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

    SAFE_FREE(buff);

    if (isnan(value))
        return NAN;

    return fabs(value);
}

double s_miles(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    char *buff = eval(operation, true);

    evalOut tmp = h_atof(buff, true);
    double km = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

    SAFE_FREE(buff);

    if (isnan(km))
        return NAN;

    return KM_TO_MI(km);
}

double s_km(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    char *buff = eval(operation, true);

    evalOut tmp = h_atof(buff, true);
    double miles = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

    SAFE_FREE(buff);

    if (isnan(miles))
        return NAN;

    return MI_TO_KM(miles);
}

double s_pounds(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    char *buff = eval(operation, true);

    evalOut tmp = h_atof(buff, true);
    double kg = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

    SAFE_FREE(buff);

    if (isnan(kg))
        return NAN;

    return KG_TO_LB(kg);
}

double s_kg(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    char *buff = eval(operation, true);

    evalOut tmp = h_atof(buff, true);
    double lbs = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

    SAFE_FREE(buff);

    if (isnan(lbs))
        return NAN;

    return LB_TO_KG(lbs);
}

double s_feet(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    char *buff = eval(operation, true);

    evalOut tmp = h_atof(buff, true);
    double meters = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

    SAFE_FREE(buff);

    if (isnan(meters))
        return NAN;

    return M_TO_FT(meters);
}

double s_meter(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    char *buff = eval(operation, true);

    evalOut tmp = h_atof(buff, true);
    double feet = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

    SAFE_FREE(buff);

    if (isnan(feet))
        return NAN;

    return FT_TO_M(feet);
}

double s_fah(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    char *buff = eval(operation, true);

    evalOut tmp = h_atof(buff, true);
    double cel = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

    SAFE_FREE(buff);

    if (isnan(cel))
        return NAN;

    return C_TO_F(cel);
}

double s_cel(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    char *buff = eval(operation, true);

    evalOut tmp = h_atof(buff, true);
    double fah = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

    SAFE_FREE(buff);

    if (isnan(fah))
        return NAN;

    return F_TO_C(fah);
}

char *s_oct(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NULL;
    operation = p;

    char *buff = eval(operation, true);

    evalOut tmp = h_atof(buff, true);
    double num = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

    SAFE_FREE(buff);

    if (isnan(num))
        return NULL;

    if (!CLOSE_ENOUGH(num, (int64_t)num)) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("oct() requires an argument of type 'int'\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

        return NULL;
    }

    char temp[0x80];

    bool isNeg = num < 0;

    snprintf(temp, sizeof(temp), "%.0lf", isNeg ? -num : num);

    int64_t value = strtol(temp, NULL, 0);

    char *buffer = malloc(0x40);
    if (!buffer)
        return NULL;

    snprintf(buffer, 64, isNeg ? "\"-"OCT_PREF"%"PRIo64"\"" : "\""OCT_PREF"%"PRIo64"\"", value);

    return buffer;
}

char *s_lower(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NULL;
    operation = p;

    char *buff = eval(operation, true);

    if (!buff || !*buff)
        return NULL;

    if (!validPtrFuncArgs(buff, "lower"))
        return NULL;

    for (size_t i = 0; buff[i]; i++) {
        char chr = buff[i];

        if (isupper(chr)) {
            buff[i] = tolower(chr);
            continue;
        }

        buff[i] = chr;
    }

    return buff;
}

char *s_upper(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NULL;
    operation = p;

    char *buff = eval(operation, true);

    if (!buff || !*buff)
        return NULL;

    if (!validPtrFuncArgs(buff, "upper"))
        return NULL;

    for (size_t i = 0; buff[i]; i++) {
        char chr = buff[i];

        int32_t backslashes = 0;
        size_t j = i;

        while (j > 0 && buff[j-1] == '\\') {
            backslashes++;
            j--;
        }

        bool escaped = backslashes & 1;

        if (escaped && isIn(chr, "ntbra'\"?fv0\\")) {
            buff[i] = chr;
            continue;
        }

        if (islower(chr))
            buff[i] = toupper(chr);
    }

    return buff;
}

char *s_chr(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NULL;
    operation = p;

    char *buff = eval(operation, true);

    if (!buff)
        return NULL;

    evalOut tmp = h_atof(buff, true);
    double num = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

    SAFE_FREE(buff);

    if (isnan(num))
        return NULL;

    if (!CLOSE_ENOUGH(num, (int64_t)num)) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("chr() requires an argument of type 'int'\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

        return NULL;
    }

    int64_t value = (int64_t)num;

    if (value < 0 || value > 127) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("chr() requires an integer between 0 <= x <= 127\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

        return NULL;
    }

    char *chr = malloc(5);
    if (!chr)
        return NULL;

    *chr = '\'';

    switch (value) {
        case 0:  strcpy(chr + 1, "\\0"); break;
        case 7:  strcpy(chr + 1, "\\a"); break;
        case 8:  strcpy(chr + 1, "\\b"); break;
        case 9:  strcpy(chr + 1, "\\t"); break;
        case 10: strcpy(chr + 1, "\\n"); break;
        case 11: strcpy(chr + 1, "\\v"); break;
        case 12: strcpy(chr + 1, "\\f"); break;
        case 13: strcpy(chr + 1, "\\r"); break;
        case 34: strcpy(chr + 1, "\\\""); break;
        case 39: strcpy(chr + 1, "\\'"); break;
        case 63: strcpy(chr + 1, "\\?"); break;
        case 92: strcpy(chr + 1, "\\\\"); break;
        default:
            if (value < 32 || value == 127) {
                printc("eval", BC_PROMPT_COLOR, WHITE);
                printf(": ");
                printc("chr() does not work with certain control and escape characters\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

                SAFE_FREE(chr);
                return NULL;
            }
            chr[1] = (char)value;
            chr[2] = '\0';
            break;
    }

    int32_t len = (chr[2] == '\0') ? 2 : 3;
    chr[len] = '\'';
    chr[len + 1] = '\0';

    return chr;
}

char *s_hex(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NULL;
    operation = p;

    char *buff = eval(operation, true);

    evalOut tmp = h_atof(buff, true);
    double val = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

    SAFE_FREE(buff);

    if (isnan(val))
        return NULL;

    if (!CLOSE_ENOUGH(val, (int64_t)val)) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("hex() requires an argument of type 'int'\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

        return NULL;
    }

    char temp[0x80];
    snprintf(temp, sizeof(temp), "%.0lf", val);
    
    int64_t value = strtoll(temp, NULL, 10);

    char *buffer = malloc(64);
    if (!buffer) {
        return NULL;
    }

    int64_to_hex_min(value, buffer, 0x40);

    for (uint16_t i = strlen(HEX_PREF) + 1; buffer[i]; i++)
        buffer[i] = toupper((unsigned char)buffer[i]);

    return buffer;
}

char *s_bin(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NULL;
    operation = p;

    char *buff = eval(operation, true);

    evalOut tmp = h_atof(buff, true);
    double val = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

    SAFE_FREE(buff);

    if (isnan(val))
        return NULL;

    if (!CLOSE_ENOUGH(val, (int64_t)val)) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("bin() requires an argument of type 'int'\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

        return NULL;
    }

    int64_t n = (int64_t)val;

    uint64_t u = (uint64_t)n;

    char buf[0x41];
    buf[64] = '\0';

    for (int32_t i = 63; i >= 0; i--) {
        buf[i] = (u & 1) ? '1' : '0';
        u >>= 1;
    }

    int16_t start = 0;
    while (start < 63 && buf[start] == buf[0] && buf[start + 1] == buf[0])
        start++;

    int16_t len = 64 - start;

    char *result = malloc(len + 5); 

    if (!result)
        return NULL;

    char *c = result;

    c[0] = '"';
    c[1] = '0';
    c[2] = 'b';

    memcpy(c + 3, buf + start, len);

    c[3+len] = '"';

    c[4+len] = '\0';

    return result;
}

double s_trunc(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    char *buff = eval(operation, true);

    evalOut tmp = h_atof(buff, true);
    double num = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

    SAFE_FREE(buff);

    if (isnan(num))
        return NAN;

    return trunc(num);
}

double s_rad(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    char *buff = eval(operation, true);

    evalOut tmp = h_atof(buff, true);
    double deg = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

    SAFE_FREE(buff);

    if (isnan(deg))
        return NAN;

    return DEG_TO_RAD(deg);
}

double s_gon(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    char *buff = eval(operation, true);

    evalOut tmp = h_atof(buff, true);
    double deg = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

    SAFE_FREE(buff);

    if (isnan(deg))
        return NAN;  

    return RAD_TO_GON(deg);
}

double s_deg(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    char *buff = eval(operation, true);

    evalOut tmp = h_atof(buff, true);
    double rad = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

    SAFE_FREE(buff);

    if (isnan(rad))
        return NAN;

    return RAD_TO_DEG(rad);
}

double s_sqrt(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    char *buff = eval(operation, true);

    evalOut tmp = h_atof(buff, true);
    double num = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

    SAFE_FREE(buff);

    if (isnan(num))
        return NAN;

    if (num < 0) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("sqrt() requires a non negative integer\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

        return NAN;
    }

    return sqrt(num);
}

double s_scale(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    char *buff = eval(operation, true);

    evalOut tmp = h_atof(buff, true);
    double value = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

    SAFE_FREE(buff);

    if (isnan(value))
        return NAN;

    if (isnan(value) || isinf(value)) {
        return 0;
    }

    char buf[0x20];
    snprintf(buf, sizeof(buf), "%g", value);

    char *exp = strchr(buf, 'e');
    if (exp) *exp = '\0';

    char *dot = strchr(buf, '.');
    if (!dot) {
        return 0;
    }

    char *end = buf + strlen(buf) - 1;
    while (end > dot && *end == '0')
        *end-- = '\0';

    return (double)strlen(dot + 1);
}

double s_sin(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    char *buff = eval(operation, true);

    evalOut tmp = h_atof(buff, true);
    double num = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

    SAFE_FREE(buff);

    if (isnan(num))
        return NAN;

    double result = sin(num);

    if (fabs(result) < 1e-6)
        result = 0.0;

    return result;
}

double s_asin(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    char *buff = eval(operation, true);

    evalOut tmp = h_atof(buff, true);
    double num = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

    SAFE_FREE(buff);

    if (isnan(num))
        return NAN;

    if (num < -1.0 || num > 1.0) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("asin() is defined only for -1 <= x <= 1\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

        return NAN;
    }
    
    return asin(num);
}

double s_cot(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    char *buff = eval(operation, true);

    evalOut tmp = h_atof(buff, true);
    double num = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

    SAFE_FREE(buff);

    if (isnan(num))
        return NAN;

    double t = tan(num);

    if (fabs(t) < 1e-12) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("cot() is undefined for %.10g rad\n", GetBaseColor(BC_PROMPT_COLOR), WHITE, num);

        return NAN;
    }

    return 1.0 / t;
}

double s_acot(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    char *buff = eval(operation, true);

    evalOut tmp = h_atof(buff, true);
    double num = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

    SAFE_FREE(buff);

    if (isnan(num))
        return NAN;

    return PI / 2.0 - atan(num);
}

double s_cos(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    char *buff = eval(operation, true);

    evalOut tmp = h_atof(buff, true);
    double num = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

    SAFE_FREE(buff);

    if (isnan(num))
        return NAN;

    double result = cos(num);

    if (fabs(result) < 1e-6)
        result = 0.0;

    return result;
}

double s_acos(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    char *buff = eval(operation, true);

    evalOut tmp = h_atof(buff, true);
    double num = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

    SAFE_FREE(buff);

    if (isnan(num))
        return NAN;

    if (num < -1.0 || num > 1.0) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("acos() is defined only for -1 <= x <= 1\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

        return NAN;
    }

    return acos(num);
}

double s_tan(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    char *buff = eval(operation, true);

    evalOut tmp = h_atof(buff, true);
    double angle = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

    SAFE_FREE(buff);

    if (isnan(angle))
        return NAN;

    double modPi = fmod(fabs(angle), PI);
    if (fabs(modPi - PI / 2.0) < 1e-8) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("tan() is undefined for %.10g rad\n", GetBaseColor(BC_PROMPT_COLOR), WHITE, angle);

        return NAN;
    }

    double result = tan(angle);

    if (fabs(result) < 1e-6)
        result = 0.0;

    return result;
}

double s_atan(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    char *buff = eval(operation, true);

    evalOut tmp = h_atof(buff, true);
    double angle = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

    SAFE_FREE(buff);

    if (isnan(angle))
        return NAN;

    return atan(angle);
}

double s_ln(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    char *buff = eval(operation, true);

    evalOut tmp = h_atof(buff, true);
    double num = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

    SAFE_FREE(buff);

    if (isnan(num))
        return NAN;

    return log(num);
}

double s_log10(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    char *buff = eval(operation, true);

    evalOut tmp = h_atof(buff, true);
    double num = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

    SAFE_FREE(buff);

    if (isnan(num))
        return NAN;

    return log10(num);
}

double s_log2(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    char *buff = eval(operation, true);

    evalOut tmp = h_atof(buff, true);
    double num = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

    SAFE_FREE(buff);

    if (isnan(num))
        return NAN;

    return log2(num);
}

double s_root(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p+1;
    operation[strlen(operation)-1] = '\0';

    char *comma = find_top_level_comma(operation);
    
    if (!comma) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("root requires exactly 2 arguments\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

        return NAN;
    }

    *comma = '\0';
    char *indexStr = operation;
    char *rootingStr = comma + 1;
    
    uint8_t nullCount = isnull(2, indexStr, rootingStr);
    if (nullCount) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("root() requires exactly 2 arguments (missing %"PRIu8")\n", GetBaseColor(BC_PROMPT_COLOR), WHITE, nullCount);

        return NAN;
    }

    trim(indexStr);
    trim(rootingStr);

    char *tmp1 = eval(indexStr, true);

    evalOut debug1 = h_atof(tmp1, true);
    double index = (debug1.type == BC_BOOL) ? (double)debug1.boolean : debug1.num;

    SAFE_FREE(tmp1);

    if (isnan(index))
        return NAN;

    char *tmp3 = eval(rootingStr, true);

    evalOut debug2 = h_atof(tmp3, true);
    double rooting = (debug2.type == BC_BOOL) ? (double)debug2.boolean : debug2.num;

    SAFE_FREE(tmp3);

    if (isnan(rooting))
        return NAN;

    bool invert = false;

    if (index == 0) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("root() requires an index that is not 0\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

        return NAN;
    }

    if (!CLOSE_ENOUGH(index, (int64_t)index)) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("root(0) requires an index of type 'int'\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

        return NAN;
    }

    if (index < 0) {
        invert = true;
        index = -index;
    }

    if (rooting < 0 && (((int64_t)index & 1) == 0)) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("root() requires an odd index when there is a negative number\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

        return NAN;
    }

    double result;

    if (rooting < 0) {
        result = -pow(-rooting, 1.0 / index);
    } else {
        result = pow(rooting, 1.0 / index);
    }

    if (invert)
        result = 1.0 / result;

    return result;
}

double s_bmi(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p+1;
    operation[strlen(operation)-1] = '\0';

    char *comma = find_top_level_comma(operation);

    if (!comma) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("bmi() requires exactly 2 arguments\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

        return NAN;
    }

    *comma = '\0';
    char *weightStr = operation;
    char *heightStr = comma + 1;

    uint8_t nullCount = isnull(2, weightStr, heightStr);
    if (nullCount) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("bmi() requires exactly 2 arguments (missing %"PRIu8")\n", GetBaseColor(BC_PROMPT_COLOR), WHITE, nullCount);

        return NAN;
    }

    trim(weightStr);
    trim(heightStr);

    char *tmp1 = eval(weightStr, true);

    evalOut debug1 = h_atof(tmp1, true);
    double weight = (debug1.type == BC_BOOL) ? (double)debug1.boolean : debug1.num;

    SAFE_FREE(tmp1);

    if (isnan(weight))
        return NAN;

    char *tmp2 = eval(heightStr, true);

    evalOut debug2 = h_atof(tmp2, true);
    double height = (debug2.type == BC_BOOL) ? (double)debug2.boolean : debug2.num;

    SAFE_FREE(tmp2);

    if (isnan(height))
        return NAN;

    return BMI(weight, height);
}

double s_log(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p+1;
    operation[strlen(operation)-1] = '\0';

    char *comma = find_top_level_comma(operation);

    if (!comma) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("log() requires exactly 2 arguments\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

        return NAN;
    }

    *comma = '\0';
    char *baseStr = operation;
    char *numStr = comma + 1;

    uint8_t nullCount = isnull(2, baseStr, numStr);
    if (nullCount) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("log() requires exactly 2 arguments (missing %"PRIu8")\n", GetBaseColor(BC_PROMPT_COLOR), WHITE, nullCount);

        return NAN;
    }

    trim(baseStr);
    trim(numStr);

    char *tmp1 = eval(baseStr, true);

    evalOut debug1 = h_atof(tmp1, true);
    double base = (debug1.type == BC_BOOL) ? (double)debug1.boolean : debug1.num;

    SAFE_FREE(tmp1);

    if (isnan(base))
        return NAN;

    char *tmp2 = eval(numStr, true);

    evalOut debug2 = h_atof(tmp2, true);
    double num = (debug2.type == BC_BOOL) ? (double)debug2.boolean : debug2.num;

    SAFE_FREE(tmp2);

    if (isnan(num))
        return NAN;

    if (base <= 1 || num <= 0) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("invalid values for log()\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

        return NAN;
    }

    return log(num) / log(base);
}

double s_randFloat(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p+1;
    operation[strlen(operation)-1] = '\0';

    char *comma = find_top_level_comma(operation);

    if (!comma) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("randf() requires exactly 2 arguments\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

        return NAN;
    }

    *comma = '\0';
    char *str_min = operation;
    char *str_max = comma + 1;

    uint8_t nullCount = isnull(2, str_min, str_max);
    if (nullCount) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("randf() requires exactly 2 arguments (missing %"PRIu8")\n", GetBaseColor(BC_PROMPT_COLOR), WHITE, nullCount);

        return NAN;
    }

    trim(str_max);
    trimEnd(str_max);
    trim(str_min);
    trimEnd(str_min);

    double maxLf = 0.0;
    double minLf = 0.0;

    if (strcasecmp(str_max, "rand_max") == 0)
        maxLf = (double)RAND_MAX;
    else {
        char *buff = eval(str_max, true);

        evalOut tmp = h_atof(buff, true);
        maxLf = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

        SAFE_FREE(buff);
    }

    if (isnan(maxLf)) {
        return NAN;
    }

    if (strcasecmp(str_min, "rand_max") == 0)
        minLf = (double)RAND_MAX;
    else {
        char *buff = eval(str_min, true);

        evalOut tmp = h_atof(buff, true);
        minLf = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

        SAFE_FREE(buff);
    }

    if (isnan(minLf)) {
        return NAN;
    }

    return random_range_float(minLf, maxLf);
}

double s_randInt(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p+1;
    operation[strlen(operation)-1] = '\0';

    char *comma = find_top_level_comma(operation);

    if (!comma) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("rand() requires exactly 2 arguments\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

        return NAN;
    }
    
    *comma = '\0';
    char *str_min = operation;
    char *str_max = comma + 1;
    
    uint8_t nullCount = isnull(2, str_min, str_max);
    if (nullCount) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("rand() requires exactly 2 arguments (missing %"PRIu8")\n", GetBaseColor(BC_PROMPT_COLOR), WHITE, nullCount);

        return NAN;
    }

    trim(str_max);
    trimEnd(str_max);
    trim(str_min);
    trimEnd(str_min);

    double maxInt = 0.0;
    double minInt = 0.0;

    if (strcasecmp(str_max, "rand_max") == 0)
        maxInt = (double)RAND_MAX;
    else {
        char *buff = eval(str_max, true);

        evalOut tmp = h_atof(buff, true);
        maxInt = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

        SAFE_FREE(buff);
    }

    if (isnan(maxInt))
        return NAN;

    if (strcasecmp(str_min, "rand_max") == 0)
        minInt = (double)RAND_MAX;
    else {
        char *buff = eval(str_min, true);

        evalOut tmp = h_atof(buff, true);
        minInt = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

        SAFE_FREE(buff);
    }

    if (isnan(minInt)) {
        return NAN;
    }

    if (!CLOSE_ENOUGH(minInt, (int64_t)minInt) || !CLOSE_ENOUGH(maxInt, (int64_t)maxInt)) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("rand() requires arguments of type 'int'\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

        return NAN;
    }

    return random_range_int((int32_t)minInt, (int32_t)maxInt);
}

double s_floor(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    char *buff = eval(operation, true);

    evalOut tmp = h_atof(buff, true);
    double num = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

    SAFE_FREE(buff);

    if (isnan(num))
        return NAN;

    return floor(num);
}

double s_ceil(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    char *buff = eval(operation, true);

    evalOut tmp = h_atof(buff, true);
    double num = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

    SAFE_FREE(buff);

    if (isnan(num))
        return NAN;

    return ceil(num);
}

double s_round(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    char *buff = eval(operation, true);

    evalOut tmp = h_atof(buff, true);
    double num = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

    SAFE_FREE(buff);

    if (isnan(num))
        return NAN;

    return round(num);
}

double tetration(double base, int32_t height) {
    if (height < 0) return NAN;
    if (height == 0) return 1.0;

    double result = base;

    for (int32_t i = 2; i <= height; i++) {
        if (result > log(DBL_MAX) / log(fabs(base)))
            return INFINITY;

        result = pow(base, result);
    }

    return result;
}

bool isprime(int64_t n) {
    if (n < 2) return false;
    
    for (int64_t i = 2; i * i <= n; i++)
        if (n % i == 0) return false;
    
    return true;
}

double s_isprime(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    char *buff = eval(operation, true);

    evalOut tmp = h_atof(buff, true);
    double num = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

    SAFE_FREE(buff);

    if (isnan(num))
        return NAN;

    if (num <= 1) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("isprime() requires a number grater than 1\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

        return NAN;
    }

    if (!CLOSE_ENOUGH(num, (int64_t)num)) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("isprime() requires an argument of type 'int'\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

        return NAN;
    }

    return isprime((int64_t)num);
}

uint64_t fact(int64_t num, int32_t steps) {

    if (num < 0)
        return U64_NAN;

    if (steps <= 0)
        return U64_NAN;

    if (num == 0)
        return 1;

    uint64_t result = 1;

    for (int64_t i = num; i >= 1; i -= steps)
        result *= i;

    return result;
}

double s_fact(char *operation) {
    char *test = strdup(operation);

    if (!test) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("strdup failed\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

        return NAN;
    }

    size_t len = strlen(test);
    if (len == 0) {
        SAFE_FREE(test);
        return NAN;
    }

    size_t stepsCount = 0;
    for (int32_t i = len-1; i >= 0; i--) {
        if (test[i] != '!')
            break;

        if (test[i] == '!')
            stepsCount++;
    }

    if (stepsCount == len)
        *test = '\0';
    else if (stepsCount == 0) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("to factor you need '!' as a suffix\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

        SAFE_FREE(test);
        return NAN;
    } else if (stepsCount > 0)
        test[len-stepsCount] = '\0';

    trimEnd(test);

    if (strlen(test) < 1) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("missing a value to factor\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

        SAFE_FREE(test);
        return NAN;
    }

    char *buff = eval(test, true);

    evalOut tmp = h_atof(buff, true);
    double num = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

    SAFE_FREE(buff);
    SAFE_FREE(test);

    if (isnan(num))
        return NAN;

    if (num < 0) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("cannot factor negative values\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

        return NAN;
    }

    if (!CLOSE_ENOUGH(num, (int64_t)num)) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("cannot factor a floating point number\n", GetBaseColor(BC_PROMPT_COLOR), WHITE);

        return NAN;
    }

    return (double)fact((int64_t)num, stepsCount);
}

double s_sign(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    char *buff = eval(operation, true);

    evalOut tmp = h_atof(buff, true);
    double num = (tmp.type == BC_BOOL) ? (double)tmp.boolean : tmp.num;

    SAFE_FREE(buff);

    if (isnan(num))
        return NAN;

    if (num > 0.0)
        return 1.0;

    if (num < 0.0)
        return -1.0;

    return signbit(num) ? -0.0 : 0.0;
}

double s_sum(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p+1;
    operation[strlen(operation)-1] = '\0';

    uint16_t commaCount = count_top_level_commas(operation);

    if (commaCount < 1 || commaCount > 2) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc(
            "sum() function requires at least 2 arguments and at most 3 arguments\n", 
            GetBaseColor(BC_PROMPT_COLOR), WHITE
        );

        return NAN;
    }

    char *comma1 = find_top_level_comma(operation);
    if (!comma1) {
        return NAN;
    }

    char *comma2 = find_top_level_comma(comma1 + 1);

    char *initStr = operation;
    char *endStr;
    char *diffStr = NULL;

    *comma1 = '\0';
    endStr = comma1 + 1;

    if (comma2) {
        *comma2 = '\0';
        diffStr = comma2 + 1;
    }

    uint8_t nullCount;
    if (diffStr)
        nullCount = isnull(3, initStr, endStr, diffStr);
    else
        nullCount = isnull(2, initStr, endStr);

    if (nullCount) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("sum() missing %"PRIu8" argument(s)\n", GetBaseColor(BC_PROMPT_COLOR), WHITE, nullCount);

        return NAN;
    }

    trim(initStr);
    trim(endStr);
    if (diffStr)
        trim(diffStr);

    char *tmp1 = eval(initStr, true);

    evalOut debug1 = h_atof(tmp1, true);
    double init = (debug1.type == BC_BOOL) ? (double)debug1.boolean : debug1.num;

    SAFE_FREE(tmp1);

    if (isnan(init))
        return NAN;

    char *tmp2 = eval(endStr, true);

    evalOut debug2 = h_atof(tmp2, true);
    double end = (debug2.type == BC_BOOL) ? (double)debug2.boolean : debug2.num;

    SAFE_FREE(tmp2);

    if (isnan(end)) 
        return NAN;

    char defaultDiff[] = "1";
    char *tmp3 = eval(diffStr ? diffStr : defaultDiff, true);

    evalOut debug3 = h_atof(tmp3, true);
    double diff = (debug3.type == BC_BOOL) ? (double)debug3.boolean : debug3.num;

    SAFE_FREE(tmp3);

    if (isnan(diff))
        return NAN;

    return gauss_range_double(init, end, diff);
}