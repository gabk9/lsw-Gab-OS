#include "utils.h"

#if !defined(_WIN32) && !defined(__linux__) && !defined(__APPLE__) && !defined(__ANDROID__)
    #error "Operational system not recognized, terminating program!!"
#endif

double parse_str_func(char *operation, FuncEntry function) {    

    if (function.returnType != RET_STRING && function.returnType != RET_CHAR) {
        printf("eval: invalid function return type\n");
        return NAN;
    }

    if (!isValidBcFuncName(function.name)) {
        printf("eval: invalid function name: '%s()'\n", function.name);
        return NAN;
    }

    bool isChr = function.returnType == RET_CHAR;

    char *buff = NULL;

    if (strcmp(function.name, "chr") == 0)
        buff = s_chr(operation);
    else if (strcmp(function.name, "bin") == 0)
        buff = s_bin(operation);
    else if (strcmp(function.name, "hex") == 0)
        buff = s_hex(operation);
    else if (strcmp(function.name, "oct") == 0)
        buff = s_oct(operation);
    else {
        printf("eval: undefined function: '%s()'\n", function.name);
        return NAN;
    }

    if (!isChr) {
        size_t len = strlen(buff);
        memmove(buff, buff+1, len+1);
        buff[len-2] = '\0';
    }    

    double num = h_atof(buff, true);

    SAFE_FREE(buff);

    return num;
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

double h_atof(const char *str, bool mathlib) {
    char buf[0x80];
    strncpy(buf, str, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    trim(buf);
    trimEnd(buf);

    if (!injectEscape(buf, "eval"))
        return NAN;

    bool isUnaryNot = false;
    bool isUnaryNeg = false;

    const int32_t end = (int32_t)strlen(buf) - 1;
    if (mathlib && buf[end] == '!')
        return s_fact(buf);

    if (*buf == '~' || *buf == '-') {
        if (*buf == '~')
            isUnaryNot = true;
        else
            isUnaryNeg = true;

        *buf = ' ';
        trim(buf);
    }

    if (mathlib && strcasecmp(buf, "inf") == 0) {

        if (isUnaryNot) {
            printf("eval: to use the not(~) operator the number must be integer\n");
            return NAN;
        }

        return isUnaryNeg ? -INFINITY : INFINITY;
    }

    bool isAns = mathlib && strcasecmp(buf, OLD_ANSWER_STR) == 0;

    if (isAns && isnan(Ans)) {
        puts("Warning: Ans is undefined");
        return NAN;
    }

    if (isUnaryNeg) {
        if (!*buf) {
            printf("eval: missing value for unary negative(-)\n");
            return NAN;
        }

        double num;
        if (isAns)
            num = Ans;
        else
            num = eval(buf, mathlib);

        if (isnan(num))
            return NAN;

        if (num < MIN_SAFE_INT64_D || num > MAX_SAFE_INT64_D) {
            printf("eval: numeric overflow (too large)\n");
            return NAN;
        }

        return -num;
    }

    if (isUnaryNot) {
        if (!*buf) {
            printf("eval: missing value for unary not(~)\n");
            return NAN;
        }
        
        double num;

        if (isAns)
            num = Ans;
        else
            num = eval(buf, mathlib);

        if (isnan(num))
            return NAN;

        if (num < MIN_SAFE_INT64_D || num > MAX_SAFE_INT64_D) {
            printf("eval: numeric overflow (too large)\n");
            return NAN;
        }

        if (num != (int64_t)num) {
            printf("eval: to use the not(~) operator the number must be integer\n");
            return NAN;
        }

        int64_t value = (int64_t)num;
        value = ~value;
        return (double)value;
    }

    if (isAns)
        return Ans;

    size_t len = strlen(buf);

    if (len > 1 && *buf == '\'' && buf[len-1] == '\'') {
        if (len-2 > 1) {
            printf("eval: to use sigle quotes it must be a single character\n");
            return NAN;
        } else if (len-2 < 1) {
            printf("eval: missing the character inside quotes\n");
            return NAN;
        }
        return (double)buf[1];
    }

    if (mathlib) {            
        int16_t ok = 0;
        double hex_pi_e = parse_bin_hex_oct_ans_e_pi(buf, &ok);
        if (ok)
            return hex_pi_e;
    }

    if (mathlib) {
        bool is_hex = isHex(buf);

        bool is_octal = isOct(buf);
        
        bool is_bin = isBin(buf);

        const struct {
            char suffix;
            double mult;
        } suffix[] = {
            {.suffix = 'k', .mult = 1e3},
            {.suffix = 'm', .mult = 1e6},
            {.suffix = 'b', .mult = 1e9},
            {.suffix = 't', .mult = 1e12},
        };

        bool has_exp = strncasecmp(buf, "0x", 2) == 0;
        bool allow_suffix = (!is_hex && !has_exp);

        if (allow_suffix) {
            for (size_t mi = 0; mi < sizeof(suffix) / sizeof(*suffix); mi++) {
                if (len > 1 && (buf[len-1] == suffix[mi].suffix || buf[len-1] == toupper(suffix[mi].suffix))) {
                    buf[len-1] = '\0';
                    if (buf[len-2] == '!')
                        return 0.0;

                    double num = eval(buf, mathlib);
                    return (isnan(num)) ? NAN : num * suffix[mi].mult;
                }
            }
        }

        if (*buf == '"' && buf[len-1] == '"') {
            printf("eval: cannot operate with string type values\n");
            return NAN;
        }

        if (strcasecmp(buf, "pi") == 0) return PI;
        else if (strcasecmp(buf, "e") == 0)  return E;

        uint16_t i = 0;
        while (buf[i] && (isdigit(buf[i]) || buf[i] == '.' || buf[i] == ',' || buf[i] == '-'))
            i++;

        if (i > 0 && strcasecmp(buf + i, "pi") == 0) {
            char temp[0x40];
            strncpy(temp, buf, i);
            temp[i] = '\0';
            double num = eval(temp, mathlib);
            return (isnan(num)) ? NAN : num * PI;
        }

        if (i > 0 && strcasecmp(buf + i, "e") == 0) {
            char temp[0x40];
            strncpy(temp, buf, i);
            temp[i] = '\0';
            double num = eval(temp, mathlib);
            return (isnan(num)) ? NAN : num * E;
        }

        if (is_hex)
            return (double)hex_to_long(buf);

        if (is_octal) {
            return (double)strtol(buf+2, NULL, 8);
        }

        if (is_bin)
            return parseBinToInt(buf);
    }

    if (!isalldigit(buf)) {
        bool shouldError;
        bool isValid = isBcVariable(buf, &shouldError);

        if (!isValid && shouldError) {
            for (size_t i = 0; buf[i]; i++) {
                if (buf[i] == ' ' || buf[i] == '(' || buf[i] == ')' ||
                    buf[i] == '"' || buf[i] == '\'' || buf[i] == '!'
                )
                    continue;

                if (!isIn(buf[i], "+-/*^%%&|<>") && !isalnum(buf[i])) {
                    printf("eval: illegal character: '%c'\n", buf[i]);
                    return NAN;
                }
            }

            printf("eval: invalid syntax\n");
            return NAN;
        }
    }

    double result = (!mathlib && isHex(buf)) ? 0.0 : atof(buf);
    return isnan(result) ? 0.0 : result;
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

static uint8_t validPtrFuncArgs(char *arg) {
    size_t len = strlen(arg);

    if (!len)
        return 1;

    if (arg[0] != '"' || arg[len-1] != '"') {

        if (arg[len-1] == '\'' && arg[0] == '\'') {
            printf("eval: must be a string\n");
            return 0;
        }

        if (arg[len-1] != '"' && arg[0] == '"') {
            printf("eval: missing closing quote\n");
            return 0;
        }

        if (arg[len-1] == '"' && arg[0] != '"') {
            char *tmp = strdup(arg);
            tmp[len-1] = '\0';
            printf("eval: invalid argument: '%s'\n", tmp);
            SAFE_FREE(tmp);
            return 0;
        }

        if (isdigit((uint8_t)arg[0])) {

            if (isOct(arg) || isHex(arg) || isBin(arg)) {
                printf("eval: must be a string\n");
                return 0;
            }
        }

        printf("eval: invalid argument: '%s'\n", arg);
        return 0;
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

double bc_len(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p+1;

    size_t len = strlen(operation);
    operation[len-1] = '\0';
    len--;

    if (!len || countCommaOutsideQuotesAndParenthesis(operation, '"') != 0) {
        printf("eval: strlen() requires exactly 1 argument\n");
        return NAN;
    }

    if (!validPtrFuncArgs(operation))
        return NAN;

    return (double)len - 2.0;
}

double s_fabs_or_abs(char *operation) {
    bool enable_single_point = *operation == 'f';
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    double value = eval(operation, true);

    if (isnan(value))
        return NAN;

    if (value == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }

    if (!enable_single_point) {
        if (value != (int64_t)value) {
            printf("eval: abs() requires an integer\n");
            return NAN;
        }
    }

    return enable_single_point ? fabs(value) : (double)llabs((int64_t)value);
}

double s_miles(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    double km = eval(operation, true);

    if (isnan(km))
        return NAN;

    if (km == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }    

    return KM_TO_MI(km);
}

double s_km(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    double miles = eval(operation, true);

    if (isnan(miles))
        return NAN;

    if (miles == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }

    return MI_TO_KM(miles);
}

double s_pounds(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    double kg = eval(operation, true);

    if (isnan(kg))
        return NAN;

    if (kg == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }

    return KG_TO_LB(kg);
}

double s_kg(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    double lbs = eval(operation, true);

    if (isnan(lbs))
        return NAN;

    if (lbs == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }

    return LB_TO_KG(lbs);
}

double s_feet(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    double meters = eval(operation, true);

    if (isnan(meters))
        return NAN;

    if (meters == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }

    return M_TO_FT(meters);
}

double s_meter(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    double feet = eval(operation, true);

    if (isnan(feet))
        return NAN;

    if (feet == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }

    return FT_TO_M(feet);
}

double s_fah(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    double cel = eval(operation, true);

    if (isnan(cel))
        return NAN;

    if (cel == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }

    return C_TO_F(cel);
}

double s_cel(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    double fah = eval(operation, true);

    if (isnan(fah))
        return NAN;

    if (fah == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }

    return F_TO_C(fah);
}

char *s_oct(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NULL;
    operation = p;

    double num = eval(operation, true);

    if (isnan(num))
        return NULL;

    if (num == (double)U64_NAN) {
        putchar('\n');
        return NULL;
    }

    if (num != (int64_t)num) {
        printf("eval: oct() requires an integer!\n");
        return NULL;
    }

    char temp[0x80];

    bool isNeg = num < 0;

    snprintf(temp, sizeof(temp), "%.0lf", isNeg ? -num : num);

    int64_t value = strtol(temp, NULL, 0);

    char *buffer = malloc(0x40);
    if (!buffer)
        return NULL;

    snprintf(buffer, 64, isNeg ? "\"-0o%"PRIo64"\"" : "\"0o%"PRIo64"\"", value);

    return buffer;
}

char *s_chr(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NULL;
    operation = p;

    double num = eval(operation, true);
    if (isnan(num))
        return NULL;

    if (num != (int64_t)num) {
        printf("eval: chr() requires an integer\n");
        return NULL;
    }

    int64_t value = (int64_t)num;

    if (value < 0 || value > 127) {
        printf("eval: chr() requires an integer between 0 and 127 (inclusive)\n");
        return NULL;
    }

    char *buff = malloc(5);
    if (!buff)
        return NULL;

    *buff = '\'';

    switch (value) {
        case 7:  strcpy(buff + 1, "\\a"); break;
        case 8:  strcpy(buff + 1, "\\b"); break;
        case 9:  strcpy(buff + 1, "\\t"); break;
        case 10: strcpy(buff + 1, "\\n"); break;
        case 11: strcpy(buff + 1, "\\v"); break;
        case 12: strcpy(buff + 1, "\\f"); break;
        case 13: strcpy(buff + 1, "\\r"); break;
        case 34: strcpy(buff + 1, "\\\""); break;
        case 39: strcpy(buff + 1, "\\'"); break;
        case 63: strcpy(buff + 1, "\\?"); break;
        case 92: strcpy(buff + 1, "\\\\"); break;
        default:
            if (value < 32 || value == 127 || value == 1) {
                printf("eval: chr() does not work with certain control and escape characters\n");
                SAFE_FREE(buff);
                return NULL;
            }
            buff[1] = (char)value;
            buff[2] = '\0';
            break;
    }

    int32_t len = (buff[2] == '\0') ? 2 : 3;
    buff[len] = '\'';
    buff[len + 1] = '\0';

    return buff;
}

char *s_hex(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NULL;
    operation = p;

    double val = eval(operation, true);

    if (isnan(val))
        return NULL;

    if (val == (double)U64_NAN) {
        putchar('\n');
        return NULL;
    }

    if (val != (int64_t)val) {
        printf("eval: hex() requires an integer!\n");
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

    for (uint16_t i = 3; buffer[i]; i++)
        buffer[i] = toupper((unsigned char)buffer[i]);

    return buffer;
}

char *s_bin(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NULL;
    operation = p;

    double val = eval(operation, true);

    if (isnan(val))
        return NULL;

    if (val == (double)U64_NAN) {
        putchar('\n');
        return NULL;
    }

    if (val != (int64_t)val) {
        printf("Bin: bin() requires an integer!\n");
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

    double num = eval(operation, true);

    if (isnan(num))
        return NAN;

    if (num == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }    

    return trunc(num);
}

double s_rad(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    double deg = eval(operation, true);

    if (isnan(deg))
        return NAN;

    if (deg == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }    

    return DEG_TO_RAD(deg);
}

double s_gon(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    double deg = eval(operation, true);

    if (isnan(deg))
        return NAN;

    if (deg == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }    

    return RAD_TO_GON(deg);
}

double s_deg(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    double rad = eval(operation, true);

    if (isnan(rad))
        return NAN;

    if (rad == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }

    return RAD_TO_DEG(rad);
}

double s_sqrt(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    double num = eval(operation, true);

    if (isnan(num))
        return NAN;

    if (num == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }

    if (num < 0) {
        puts("eval: sqrt() requires a non negative!");
        return NAN;
    }

    return sqrt(num);
}

double s_scale(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    double value = eval(operation, true);

    if (isnan(value))
        return NAN;

    if (value == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }    

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

    double num = eval(operation, true);

    if (isnan(num))
        return NAN;

    if (num == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }

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

    double num = eval(operation, true);

    if (isnan(num))
        return NAN;

    if (num == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }    
    
    if (num < -1.0 || num > 1.0) {
        puts("eval: asin() is defined only for -1 <= x <= 1");
        return NAN;
    }
    
    return asin(num);
}

double s_cot(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    double num = eval(operation, true);

    if (isnan(num))
        return NAN;

    if (num == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }

    double t = tan(num);

    if (fabs(t) < 1e-12) {
        printf("eval: cot() undefined for %.10g rad\n", num);
        return NAN;
    }

    return 1.0 / t;
}

double s_acot(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    double num = eval(operation, true);

    if (isnan(num))
        return NAN;

    if (num == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }

    return PI / 2.0 - atan(num);
}

double s_cos(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    double num = eval(operation, true);

    if (isnan(num))
        return NAN;

    if (num == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }    

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

    double num = eval(operation, true);

    if (isnan(num))
        return NAN;

    if (num == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }    

    if (num < -1.0 || num > 1.0) {
        puts("eval: acos() is defined only for -1 <= x <= 1");
        return NAN;
    }

    return acos(num);
}

double s_tan(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    double angle = eval(operation, true);

    if (isnan(angle))
        return NAN;

    if (angle == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }    

    double modPi = fmod(fabs(angle), PI);
    if (fabs(modPi - PI / 2.0) < 1e-8) {
        printf("eval: tan() undefined for %.10g rad\n", angle);
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

    double angle = eval(operation, true);

    if (isnan(angle))
        return NAN;

    if (angle == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }

    return atan(angle);
}

double s_ln(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    double num = eval(operation, true);

    if (isnan(num))
        return NAN;

    if (num == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }    

    return log(num);
}

double s_log10(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    double num = eval(operation, true);

    if (isnan(num))
        return NAN;

    if (num == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }    

    return log10(num);
}

double s_log2(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    double num = eval(operation, true);

    if (isnan(num))
        return NAN;

    if (num == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }    

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
        printf("eval: root() requires exactly 2 arguments\n");
        return NAN;
    }

    *comma = '\0';
    char *indexStr = operation;
    char *rootingStr = comma + 1;
    
    uint8_t nullCount = isnull(2, indexStr, rootingStr);
    if (nullCount) {
        printf("eval: root() requires exactly 2 arguments (missing %"PRIu8")\n", nullCount);
        return NAN;
    }

    trim(indexStr);
    trim(rootingStr);

    double index = eval(indexStr, true);

    if (isnan(index))
        return NAN;

    if (index == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }  
    
    double rooting = eval(rootingStr, true);

    if (isnan(rooting))
        return NAN;

    if (rooting == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }

    bool invert = false;

    if (index == 0) {
        printf("eval: root() requires an index that is not 0\n");
        return NAN;
    }

    if (index != (int64_t)index) {
        printf("eval: troot() requires an integer index\n");
        return NAN;
    }

    if (index < 0) {
        invert = true;
        index = -index;
    }

    if (rooting < 0 && (((int64_t)index & 1) == 0)) {
        printf("eval: root() requires an odd index when there is a negative number\n");
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
        printf("eval: bmi() requires exactly 2 arguments\n");
        return NAN;
    }

    *comma = '\0';
    char *weightStr = operation;
    char *heightStr = comma + 1;

    uint8_t nullCount = isnull(2, weightStr, heightStr);
    if (nullCount) {
        printf("eval: bmi() requires exactly 2 arguments (missing %"PRIu8")\n", nullCount);
        return NAN;
    }

    trim(weightStr);
    trim(heightStr);

    double weight = eval(weightStr, true);

    if (isnan(weight))
        return NAN;

    if (weight == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }  
    
    double height = eval(heightStr, true);

    if (isnan(height))
        return NAN;

    if (height == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }

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
        printf("eval: log() requires exactly 2 arguments\n");
        return NAN;
    }

    *comma = '\0';
    char *baseStr = operation;
    char *numStr = comma + 1;

    uint8_t nullCount = isnull(2, baseStr, numStr);
    if (nullCount) {
        printf("eval: log() requires exactly 2 arguments (missing %"PRIu8")\n", nullCount);
        return NAN;
    }

    trim(baseStr);
    trim(numStr);

    double base = eval(baseStr, true);

    if (isnan(base))
        return NAN;

    if (base == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }  

    double num = eval(numStr, true);

    if (isnan(num))
        return NAN;

    if (num == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }  

    if (base <= 1 || num <= 0) {
        printf("eval: invalid values for log()\n");
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
        printf("eval: randf() requires exactly 2 arguments\n");
        return NAN;
    }

    *comma = '\0';
    char *str_min = operation;
    char *str_max = comma + 1;

    uint8_t nullCount = isnull(2, str_min, str_max);
    if (nullCount) {
        printf("eval: randf() requires exactly 2 arguments (missing %"PRIu8")\n", nullCount);
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
    else 
        maxLf = eval(str_max, true);

    if (isnan(maxLf)) {
        return NAN;
    }

    if (maxLf == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }  

    if (strcasecmp(str_min, "rand_max") == 0)
        minLf = (double)RAND_MAX;
    else 
        minLf = eval(str_min, true);

    if (isnan(minLf)) {
        return NAN;
    }

    if (minLf == (double)U64_NAN) {
        putchar('\n');
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
        printf("eval: rand() requires exactly 2 arguments\n");
        return NAN;
    }
    
    *comma = '\0';
    char *str_min = operation;
    char *str_max = comma + 1;
    
    uint8_t nullCount = isnull(2, str_min, str_max);
    if (nullCount) {
        printf("eval: rand() requires exactly 2 arguments (missing %"PRIu8")\n", nullCount);
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
    else 
        maxInt = eval(str_max, true);

    if (isnan(maxInt))
        return NAN;

    if (maxInt == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }  

    if (strcasecmp(str_min, "rand_max") == 0)
        minInt = (double)RAND_MAX;
    else 
        minInt = eval(str_min, true);

    if (isnan(minInt)) {
        return NAN;
    }

    if (minInt == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }  

    if (minInt != (int64_t)minInt || maxInt != (int64_t)maxInt) {
        printf("eval: rand() requires an integer!\n");
        return NAN;
    }

    return random_range_int((int32_t)minInt, (int32_t)maxInt);
}

double s_floor(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    double num = eval(operation, true);

    if (isnan(num))
        return NAN;

    if (num == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }   

    double result = floor(num);
    return result;
}

double s_ceil(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    double num = eval(operation, true);

    if (isnan(num))
        return NAN;

    if (num == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }   

    return ceil(num);
}

double s_round(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    double num = eval(operation, true);

    if (isnan(num))
        return NAN;

    if (num == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }   

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

    double num = eval(operation, true);

    if (isnan(num))
        return NAN;

    if (num == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }   

    if (num <= 1) {
        printf("eval: isprime() requires a number greater than 1\n");
        return NAN;
    }

    if (num != (int64_t)num) {
        printf("eval: isprime() requires an integer!\n");
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
        printf("eval: strdup failed\n");
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
        printf("eval: to factor you need '!' as a suffix\n");
        SAFE_FREE(test);
        return NAN;
    } else if (stepsCount > 0)
        test[len-stepsCount] = '\0';

    trimEnd(test);

    if (strlen(test) < 1) {
        printf("eval: missing a value to factor\n");
        SAFE_FREE(test);
        return NAN;
    }

    double num = eval(test, true);

    SAFE_FREE(test);

    if (isnan(num))
        return NAN;

    if (num == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }   

    if (num < 0) {
        printf("eval: cannot factor negative values\n");
        return NAN;
    }

    if (num != (int64_t)num) {
        printf("eval: cannot factor a floating point number\n");
        return NAN;
    }

    return (double)fact((int64_t)num, stepsCount);
}

double s_sign(char *operation) {
    char *p = strchr(operation, '(');
    if (!p)
        return NAN;
    operation = p;

    double num = eval(operation, true);

    if (isnan(num))
        return NAN;

    if (num == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }   

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
        printf(
            "eval: sum() function requires at least 2 arguments and at most 3 arguments\n"
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
        printf("eval: sum() missing %"PRIu8" argument(s)\n", nullCount);
        return NAN;
    }

    trim(initStr);
    trim(endStr);
    if (diffStr)
        trim(diffStr);

    double init = eval(initStr, true);

    if (isnan(init))
        return NAN;

    if (init == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }   

    double end  = eval(endStr, true);

    if (isnan(end)) 
        return NAN;

    if (end == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }   

    char defaultDiff[] = "1";
    double diff = eval(diffStr ? diffStr : defaultDiff, true);

    if (isnan(diff))
        return NAN;


    if (diff == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }

    double result = gauss_range_double(init, end, diff);

    return result;
}