#include "utils.h"

#if !defined(_WIN32) && !defined(__linux__) && !defined(__APPLE__) && !defined(__ANDROID__)
    #error "Operational system not recognized, terminating program!!"
#endif

double parse_double(char *str, char *funcName) {    
    char *test;
    bool heap = false;
    if (funcName) {
        test = functionHandler(str, funcName);
        if (strcmp(test, BC_ERROR) == 0) return (double)U64_NAN;
        heap = true;
    } else 
        test = str;

    double num = eval(test, true);
    num = (num == QUICK_EVAL_FIX) ? 0.0 : num;

    if (heap)
        SAFE_FREE(test);

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

enum paren_result parenthesis_check(const char *s) {

    int32_t level = 0;
    bool in_double_quotes = false;
    bool in_single_quotes = false;

    for (; *s; s++) {

        if (*s == '"' && !in_single_quotes) {
            in_double_quotes = !in_double_quotes;
            continue;
        }

        if (*s == '\'' && !in_double_quotes) {
            in_single_quotes = !in_single_quotes;
            continue;
        }

        if (in_double_quotes || in_single_quotes)
            continue;

        if (*s == '(') {
            level++;
        }
        else if (*s == ')') {
            level--;
            if (level < 0)
                return PAREN_MISSING_OPEN;
        }
    }

    if (in_double_quotes || in_single_quotes)
        return PAREN_UNCLOSED_QUOTE;

    if (level > 0)
        return PAREN_MISSING_CLOSE;

    return PAREN_OK;
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

char *functionHandler(char *operation, const char *function) {
    operation = strrm(operation, function);    
    trim(operation);

    enum paren_result result = parenthesis_check(operation);

    if (result != PAREN_OK) {

        switch (result) {
            case PAREN_MISSING_CLOSE:
                printf("Error: expected ')'\n\n");
                break;

            case PAREN_MISSING_OPEN:
                printf("Error: unexpected ')'\n\n");
                break;

            case PAREN_UNCLOSED_QUOTE:
                printf("Error: unclosed quote\n\n");
                break;

            default:
                break;
        }

        return BC_ERROR;
    }

    char *copy = strdup(operation);
    if (!copy) {
        return BC_ERROR;
    }

    copy[0] = ' ';
    copy[strlen(copy) - 1] = '\0';

    trim(copy);
    trimEnd(copy);

    if (isBcVariable(copy)) {
        printf("Warning: variables are currently unsupported\n\n");
        SAFE_FREE(copy);
        return BC_ERROR;
    }

    return copy;
}

double h_atof(const char *str, bool mathlib) {
    char buf[0x80];
    strncpy(buf, str, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    trim(buf);
    trimEnd(buf);

    if (mathlib && isBcVariable(buf)) {
        printf("Warning: variables are currently unsupported\n\n");
        return NAN;
    }
    
    bool isUnaryNot = false;
    bool isUnaryNeg = false;

    if (*buf == '~' || *buf == '-') {
        if (*buf == '~')
            isUnaryNot = true;
        else
            isUnaryNeg = true;

        memmove(buf, buf+1, strlen(buf)+1);
        trim(buf);
    }

    if (strcasecmp(buf, "inf") == 0) {
        if (!mathlib)
            return 0.0;

        return isUnaryNeg ? -INFINITY : INFINITY;
    }

    if (strcasecmp(buf, "nan") == 0)
        return 0.0;

    bool isAns = mathlib && strcasecmp(buf, OLD_ANSWER_STR) == 0;

    if (isUnaryNeg) {

        if (isAns && isnan(Ans)) {
            puts("Warning: Ans is undefined\n");
            return (double)U64_NAN;
        }

        double num;
        if (isAns)
            num = Ans;
        else {
            num = eval(buf, mathlib);
            if (num == QUICK_EVAL_FIX)
                return QUICK_EVAL_FIX;
        }

        if (num < MIN_SAFE_INT64_D || num > MAX_SAFE_INT64_D) {
            printf("Error: numeric overflow (too large)\n\n");
            return NAN;
        }

        return -num;
    }

    if (isUnaryNot) {
        double num;

        if (isAns)
            num = Ans;
        else {
            num = eval(buf, mathlib);
            if (num == QUICK_EVAL_FIX)
                return QUICK_EVAL_FIX;
        }

        if (num < MIN_SAFE_INT64_D || num > MAX_SAFE_INT64_D) {
            printf("Error: numeric overflow (too large)\n\n");
            return NAN;
        }

        if (num != (int64_t)num) {
            printf("Error: to use the not(~) operator the number must be integer\n\n");
            return NAN;
        }

        int64_t value = (int64_t)num;
        value = ~value;
        return (double)value;
    }

    if (*buf == '\'' && buf[2] == '\'' && buf[3] == '\0')
        return (double)buf[1];

    if (mathlib) {            
        int16_t ok = 0;
        double hex_pi_e = parse_bin_hex_oct_ans_e_pi(buf, &ok);
        if (ok)
            return hex_pi_e;
    }

    uint16_t len = strlen(buf);

    if (mathlib) {

        bool is_hex = isHex(buf);
    
        bool has_exp = (strchr(buf, 'e') != NULL || strchr(buf, 'E') != NULL);
    
        bool is_octal = isOct(buf);
    
        bool is_bin = isBin(buf);
    
        bool allow_suffix = (!is_hex && !has_exp);
    
        const struct {
            char suffix;
            double mult;
        } suffix[] = {
            {'k', 1e3},
            {'m', 1e6},
            {'b', 1e9},
            {'t', 1e12},
        };
    
        if (allow_suffix || (*buf == 'e' || *buf == 'E')) {
            for (size_t mi = 0; mi < sizeof(suffix) / sizeof(suffix[0]); mi++) {
                if (len > 1 && (buf[len-1] == suffix[mi].suffix || buf[len-1] == toupper(suffix[mi].suffix))) {
                    buf[len - 1] = '\0';
                    double num = eval(buf, mathlib);
                    return (num == QUICK_EVAL_FIX) ? 0.0 : num * suffix[mi].mult;
                }
            }
        }
    
        if (strcasecmp(buf, "pi") == 0) return PI;
        if (strcasecmp(buf, "-pi") == 0) return -PI;
        if (strcasecmp(buf, "e") == 0)  return E;
        if (strcasecmp(buf, "-e") == 0) return -E;
    
        uint16_t i = 0;
        while (buf[i] && (isdigit(buf[i]) || buf[i] == '.' || buf[i] == ',' || buf[i] == '-'))
            i++;
    
        if (i > 0 && strcasecmp(buf + i, "pi") == 0) {
            char temp[0x40];
            strncpy(temp, buf, i);
            temp[i] = '\0';
            return eval(temp, mathlib) * PI;
        }
    
        if (i > 0 && strcasecmp(buf + i, "e") == 0) {
            char temp[0x40];
            strncpy(temp, buf, i);
            temp[i] = '\0';
            double num = eval(temp, mathlib);
            return (num == QUICK_EVAL_FIX) ? 0.0 : num * E;
        }
    
        if (is_hex)
            return (double)hex_to_long(buf);
    
        if (is_octal) {
            return (double)strtol(buf+2, NULL, 8);
        }
    
        if (is_bin)
            return parseBinToInt(buf);
    }

    if (!isalldigit(buf))
        return QUICK_EVAL_FIX;

    return (!mathlib && isHex(buf)) ? 0.0 : atof(buf);
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
            printf("Error: must be a string\n\n");
            return 0;
        }

        if (arg[len-1] != '"' && arg[0] == '"') {
            printf("Error: missing closing quote\n\n");
            return 0;
        }

        if (arg[len-1] == '"' && arg[0] != '"') {
            char *tmp = strdup(arg);
            tmp[len-1] = '\0';
            printf("Error: invalid argument: '%s'\n\n", tmp);
            SAFE_FREE(tmp);
            return 0;
        }

        if (isdigit((uint8_t)arg[0])) {

            if (isOct(arg) || isHex(arg) || isBin(arg)) {
                printf("Error: must be a string\n\n");
                return 0;
            }
        }

        printf("Error: invalid argument: '%s'\n\n", arg);
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
    char *test = functionHandler(operation, "len");
    if (strcmp(test, BC_ERROR) == 0) return NAN;

    trim(test);
    trimEnd(test);

    double len = strlen(test);

    if (!len || countCommaOutsideQuotesAndParenthesis(test, '"') != 0) {
        printf("Error: strlen() requires exactly 1 argument\n\n");
        SAFE_FREE(test);
        return NAN;
    }

    if (!validPtrFuncArgs(test)) {
        SAFE_FREE(test);
        return NAN;
    }

    test[(size_t)len-1] = '\0';
    len--;
    memmove(test, test + 1, (size_t)len);
    len--;

    SAFE_FREE(test);

    return len;
}

double s_fabs_or_abs(char *operation, bool enable_single_point) {
    char *function = enable_single_point ? "fabs" : "abs"; 
    char *test = functionHandler(operation, function);
    if (strcmp(test, BC_ERROR) == 0) return NAN;

    double value = eval(test, true);

    SAFE_FREE(test);

    if (value == QUICK_EVAL_FIX)
        return 0.0;

    if (value == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }

    if (!enable_single_point) {
        if (value != (int64_t)value) {
            printf("Error: abs() requires an integer\n\n");
            return NAN;
        }
    }

    return enable_single_point ? fabs(value) : (double)llabs((int64_t)value);
}

double s_miles(char *operation) {
    char *test = functionHandler(operation, "mi");
    if (strcmp(test, BC_ERROR) == 0) return NAN;

    double km = eval(test, true);

    SAFE_FREE(test);

    if (km == QUICK_EVAL_FIX)
        return 0.0;

    if (km == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }    

    return KM_TO_MI(km);
}

double s_km(char *operation) {
    char *test = functionHandler(operation, "km");
    if (strcmp(test, BC_ERROR) == 0) return NAN;

    double miles = eval(test, true);

    SAFE_FREE(test);

    if (miles == QUICK_EVAL_FIX)
        return 0.0;

    if (miles == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }

    return MI_TO_KM(miles);
}

double s_pounds(char *operation) {
    char *test = functionHandler(operation, "lb");
    if (strcmp(test, BC_ERROR) == 0) return NAN;

    double kg = eval(test, true);

    SAFE_FREE(test);

    if (kg == QUICK_EVAL_FIX)
        return 0.0;

    if (kg == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }

    return KG_TO_LB(kg);
}

double s_kg(char *operation) {
    char *test = functionHandler(operation, "kg");
    if (strcmp(test, BC_ERROR) == 0) return NAN;

    double lbs = eval(test, true);

    SAFE_FREE(test);

    if (lbs == QUICK_EVAL_FIX)
        return 0.0;

    if (lbs == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }

    return LB_TO_KG(lbs);
}

double s_feet(char *operation) {
    char *test = functionHandler(operation, "feet");
    if (strcmp(test, BC_ERROR) == 0) return NAN;

    double meters = eval(test, true);

    SAFE_FREE(test);

    if (meters == QUICK_EVAL_FIX)
        return 0.0;

    if (meters == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }

    return M_TO_FT(meters);
}

double s_meter(char *operation) {
    char *test = functionHandler(operation, "meter");
    if (strcmp(test, BC_ERROR) == 0) return NAN;

    double feet = eval(test, true);

    SAFE_FREE(test);

    if (feet == QUICK_EVAL_FIX)
        return 0.0;

    if (feet == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }

    return FT_TO_M(feet);
}

double s_fah(char *operation) {
    char *test = functionHandler(operation, "fah");
    if (strcmp(test, BC_ERROR) == 0) return NAN;

    double cel = eval(test, true);

    SAFE_FREE(test);

    if (cel == QUICK_EVAL_FIX)
        return 0.0;

    if (cel == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }

    return C_TO_F(cel);
}

double s_cel(char *operation) {
    char *test = functionHandler(operation, "cel");
    if (strcmp(test, BC_ERROR) == 0) return NAN;

    double fah = eval(test, true);

    SAFE_FREE(test);

    if (fah == QUICK_EVAL_FIX)
        return 0.0;

    if (fah == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }

    return F_TO_C(fah);
}

char *s_oct(char *operation) {
    char *test = functionHandler(operation, "oct");
    if (strcmp(test, BC_ERROR) == 0) return NULL;

    double num = eval(test, true);

    SAFE_FREE(test);

    if (num == QUICK_EVAL_FIX)
        return NULL;

    if (num == (double)U64_NAN) {
        putchar('\n');
        return NULL;
    }

    if (num != (int64_t)num) {
        printf("Error: oct() requires an integer!\n\n");
        return NULL;
    }

    if (num < 0) {
        printf("Error: oct() requires non negative numbers\n\n");
        return NULL;
    }

    char temp[0x80];
    snprintf(temp, sizeof(temp), "%.0lf", num);

    int64_t value = strtol(temp, NULL, 0);

    char *buffer = malloc(64);
    if (!buffer)
        return NULL;


    snprintf(buffer, 64, "0o%" PRIo64, value);

    return buffer;
}

char *s_chr(char *operation) {
    char *test = functionHandler(operation, "chr");
    if (strcmp(test, BC_ERROR) == 0)
        return NULL;

    double num = eval(test, true);
    SAFE_FREE(test);

    if (num == QUICK_EVAL_FIX)
        return NULL;

    if (num != (int64_t)num) {
        printf("Error: chr() requires an integer\n\n");
        return NULL;
    }

    int64_t value = (int64_t)num;

    if (value < 0 || value > 127) {
        printf("Error: chr() requires an integer between 0 and 127 (inclusive)\n\n");
        return NULL;
    }

    if (value < 32 || value == 127) {
        printf("Error: chr() does not allow control characters\n\n");
        return NULL;
    }

    char *buff = malloc(4);
    if (!buff)
        return NULL;

    buff[0] = '\'';
    buff[1] = (char)value;
    buff[2] = '\'';
    buff[3] = '\0';

    return buff;
}

char *s_hex(char *operation) {
    char *test = functionHandler(operation, "hex");
    if (strcmp(test, BC_ERROR) == 0) return NULL;

    double val = eval(test, true);

    SAFE_FREE(test);

    if (val == QUICK_EVAL_FIX)
        return NULL;

    if (val == (double)U64_NAN) {
        putchar('\n');
        return NULL;
    }

    if (val != (int64_t)val) {
        printf("Error: hex() requires an integer!\n\n");
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

    for (uint16_t i = 2; buffer[i]; i++)
        buffer[i] = toupper((unsigned char)buffer[i]);

    return buffer;
}

char *s_bin(char *operation) {
    char *test = functionHandler(operation, "bin");
    if (strcmp(test, BC_ERROR) == 0)
        return NULL;

    double val = eval(test, true);
    SAFE_FREE(test);

    if (val == QUICK_EVAL_FIX)
        return NULL;

    if (val == (double)U64_NAN) {
        putchar('\n');
        return NULL;
    }

    if (val != (int64_t)val) {
        printf("Bin: bin() requires an integer!\n\n");
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

    char *result = malloc(len + 3);
    char *p = result;

    *p++ = '0';
    *p++ = 'b';
    memcpy(p, buf + start, len + 1);

    return result;
}

double s_trunc(char *operation) {
    char *test = functionHandler(operation, "trunc");
    if (strcmp(test, BC_ERROR) == 0) return NAN;

    double num = eval(test, true);

    SAFE_FREE(test);

    if (num == QUICK_EVAL_FIX)
        return 0.0;

    if (num == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }    

    return trunc(num);
}

double s_rad(char *operation) {
    char *test = functionHandler(operation, "rad");
    if (strcmp(test, BC_ERROR) == 0) return NAN;

    double deg = eval(test, true);

    SAFE_FREE(test);

    if (deg == QUICK_EVAL_FIX)
        return 0.0;

    if (deg == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }    

    return DEG_TO_RAD(deg);
}

double s_gon(char *operation) {
    char *test = functionHandler(operation, "gon");
    if (strcmp(test, BC_ERROR) == 0) return NAN;

    double deg = eval(test, true);

    SAFE_FREE(test);

    if (deg == QUICK_EVAL_FIX)
        return 0.0;

    if (deg == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }    

    return RAD_TO_GON(deg);
}

double s_deg(char *operation) {
    char *test = functionHandler(operation, "deg");
    if (strcmp(test, BC_ERROR) == 0) return NAN;

    double rad = eval(test, true);

    SAFE_FREE(test);

    if (rad == QUICK_EVAL_FIX)
        return 0.0;

    if (rad == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }

    return RAD_TO_DEG(rad);
}

double s_sqrt(char *operation) {
    char *test = functionHandler(operation, "sqrt");
    if (strcmp(test, BC_ERROR) == 0) return NAN;

    double num = eval(test, true);

    SAFE_FREE(test);

    if (num == QUICK_EVAL_FIX)
        return 0.0;

    if (num == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }

    if (num < 0) {
        puts("Error: sqrt() requires a non negative!\n");
        return NAN;
    }

    return sqrt(num);
}

double s_scale(char *operation) {
    char *test = functionHandler(operation, "scale");
    if (strcmp(test, BC_ERROR) == 0)
        return NAN;

    double value = eval(test, true);

    SAFE_FREE(test);

    if (value == QUICK_EVAL_FIX)
        return 0.0;

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
        SAFE_FREE(test);
        return 0;
    }

    char *end = buf + strlen(buf) - 1;
    while (end > dot && *end == '0')
        *end-- = '\0';

    return (double)strlen(dot + 1);
}

double s_sin(char *operation) {
    char *test = functionHandler(operation, "sin");
    if (strcmp(test, BC_ERROR) == 0) return NAN;

    double num = eval(test, true);

    SAFE_FREE(test);

    if (num == QUICK_EVAL_FIX)
        return 0.0;

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
    char *test = functionHandler(operation, "asin");
    if (strcmp(test, BC_ERROR) == 0) return NAN;

    double num = eval(test, true);

    SAFE_FREE(test);

    if (num == QUICK_EVAL_FIX)
        return 0.0;

    if (num == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }    
    
    if (num < -1.0 || num > 1.0) {
        puts("Error: asin() is defined only for -1 <= x <= 1\n");
        return NAN;
    }
    
    return asin(num);
}

double s_cot(char *operation) {
    char *test = functionHandler(operation, "cot");
    if (strcmp(test, BC_ERROR) == 0) return NAN;

    double num = eval(test, true);

    SAFE_FREE(test);

    if (num == QUICK_EVAL_FIX)
        return 0.0;

    if (num == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }

    double t = tan(num);

    if (fabs(t) < 1e-12) {
        printf("Error: cot() undefined for %.10g rad\n\n", num);
        return NAN;
    }
    
    return 1.0 / t;
}

double s_acot(char *operation) {
    char *test = functionHandler(operation, "acot");
    if (strcmp(test, BC_ERROR) == 0) return NAN;

    double num = eval(test, true);

    SAFE_FREE(test);

    if (num == QUICK_EVAL_FIX)
        return 0.0;

    if (num == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }

    return PI / 2.0 - atan(num);
}

double s_cos(char *operation) {
    char *test = functionHandler(operation, "cos");
    if (strcmp(test, BC_ERROR) == 0) return NAN;

    double num = eval(test, true);

    SAFE_FREE(test);

    if (num == QUICK_EVAL_FIX)
        return 0.0;

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
    char *test = functionHandler(operation, "acos");
    if (strcmp(test, BC_ERROR) == 0) return NAN;

    double num = eval(test, true);

    SAFE_FREE(test);

    if (num == QUICK_EVAL_FIX)
        return 0.0;

    if (num == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }    

    if (num < -1.0 || num > 1.0) {
        puts("Error: acos() is defined only for -1 <= x <= 1\n");
        return NAN;
    }

    return acos(num);
}

double s_tan(char *operation) {
    char *test = functionHandler(operation, "tan");
    if (strcmp(test, BC_ERROR) == 0)
        return NAN;

    double angle = eval(test, true);

    SAFE_FREE(test);

    if (angle == QUICK_EVAL_FIX)
        return 0.0;

    if (angle == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }    

    double modPi = fmod(fabs(angle), PI);
    if (fabs(modPi - PI / 2.0) < 1e-8) {
        printf("Error: tan() undefined for %.10g rad\n\n", angle);
        return NAN;
    }

    double result = tan(angle);

    if (fabs(result) < 1e-6)
        result = 0.0;

    return result;
}

double s_atan(char *operation) {
    char *test = functionHandler(operation, "atan");
    if (strcmp(test, BC_ERROR) == 0)
        return NAN;

    double angle = eval(test, true);

    SAFE_FREE(test);

    if (angle == QUICK_EVAL_FIX)
        return 0.0;

    if (angle == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }

    return atan(angle);
}

double s_ln(char *operation) {
    char *test = functionHandler(operation, "ln");
    if (strcmp(test, BC_ERROR) == 0) return NAN;

    double num = eval(test, true);

    SAFE_FREE(test);

    if (num == QUICK_EVAL_FIX)
        return 0.0;

    if (num == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }    

    return log(num);
}

double s_log10(char *operation) {
    char *test = functionHandler(operation, "log10");
    if (strcmp(test, BC_ERROR) == 0) return NAN;

    double num = eval(test, true);

    SAFE_FREE(test);

    if (num == QUICK_EVAL_FIX)
        return 0.0;

    if (num == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }    

    return log10(num);
}

double s_log2(char *operation) {
    char *test = functionHandler(operation, "log2");
    if (strcmp(test, BC_ERROR) == 0) return NAN;

    double num = eval(test, true);

    SAFE_FREE(test);

    if (num == QUICK_EVAL_FIX)
        return 0.0;

    if (num == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }    

    return log2(num);
}

double s_root(char *operation) {
    char *test = functionHandler(operation, "root");
    if (strcmp(test, BC_ERROR) == 0) return NAN;

    char *comma = find_top_level_comma(test);
    
    if (!comma) {
        printf("Error: root() requires exactly 2 arguments\n\n");
        return NAN;
    }

    *comma = '\0';
    char *indexStr = test;
    char *rootingStr = comma + 1;
    
    uint8_t nullCount = isnull(2, indexStr, rootingStr);
    if (nullCount) {
        printf("Error: root() requires exactly 2 arguments (missing %"PRIu8")\n\n", nullCount);
        SAFE_FREE(test);
        return NAN;
    }

    trim(indexStr);
    trim(rootingStr);

    double index = eval(indexStr, true);

    if (index == QUICK_EVAL_FIX) {
        SAFE_FREE(test);
        return 0.0;
    }

    if (index == (double)U64_NAN) {
        putchar('\n');
        SAFE_FREE(test);
        return NAN;
    }  
    
    double rooting = eval(rootingStr, true);

    if (rooting == QUICK_EVAL_FIX) {
        SAFE_FREE(test);
        return 0.0;
    }

    if (rooting == (double)U64_NAN) {
        putchar('\n');
        SAFE_FREE(test);
        return NAN;
    }

    SAFE_FREE(test);

    bool invert = false;

    if (index == 0) {
        printf("Error: root() requires an index that is not 0\n\n");
        return NAN;
    }

    if (floor(index) != index) {
        printf("Error: troot() requires an integer index\n\n");
        return NAN;
    }

    if (index < 0) {
        invert = true;
        index = -index;
    }

    if (rooting < 0 && ((int32_t)index % 2 == 0)) {
        printf("Error: root() requires an odd index when there is a negative number\n\n");
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
    char *test = functionHandler(operation, "bmi");
    if (strcmp(test, BC_ERROR) == 0) return NAN;

    char *comma = find_top_level_comma(test);
    
    if (!comma) {
        printf("Error: bmi() requires exactly 2 arguments\n\n");
        return NAN;
    }

    *comma = '\0';
    char *weightStr = test;
    char *heightStr = comma + 1;
    
    uint8_t nullCount = isnull(2, weightStr, heightStr);
    if (nullCount) {
        printf("Error: bmi() requires exactly 2 arguments (missing %"PRIu8")\n\n", nullCount);
        SAFE_FREE(test);
        return NAN;
    }

    trim(weightStr);
    trim(heightStr);

    double weight = eval(weightStr, true);

    if (weight == QUICK_EVAL_FIX) {
        SAFE_FREE(test);
        return 0.0;
    }

    if (weight == (double)U64_NAN) {
        putchar('\n');
        SAFE_FREE(test);
        return NAN;
    }  
    
    double height = eval(heightStr, true);

    if (height == QUICK_EVAL_FIX) {
        SAFE_FREE(test);
        return 0.0;
    }

    if (height == (double)U64_NAN) {
        putchar('\n');
        SAFE_FREE(test);
        return NAN;
    }

    SAFE_FREE(test);

    return BMI(weight, height);
}

double s_log(char *operation) {
    char *test = functionHandler(operation, "log");
    if (strcmp(test, BC_ERROR) == 0) return NAN;

    char *comma = find_top_level_comma(test);

    if (!comma) {
        printf("Error: log() requires exactly 2 arguments\n\n");
        return NAN;
    }
    
    *comma = '\0';
    char *baseStr = test;
    char *numStr = comma + 1;

    uint8_t nullCount = isnull(2, baseStr, numStr);
    if (nullCount) {
        printf("Error: log() requires exactly 2 arguments (missing %"PRIu8")\n\n", nullCount);
        SAFE_FREE(test);
        return NAN;
    }

    trim(baseStr);
    trim(numStr);

    double base = eval(baseStr, true);

    if (base == QUICK_EVAL_FIX) {
        SAFE_FREE(test);
        return 0.0;
    }

    if (base == (double)U64_NAN) {
        putchar('\n');
        SAFE_FREE(test);
        return NAN;
    }  

    double num = eval(numStr, true);

    if (num == QUICK_EVAL_FIX) {
        SAFE_FREE(test);
        return 0.0;
    }

    if (num == (double)U64_NAN) {
        putchar('\n');
        SAFE_FREE(test);
        return NAN;
    }  

    SAFE_FREE(test);

    if (base <= 1 || num <= 0) {
        printf("Error: invalid values for log()\n\n");
        return NAN;
    }

    return log(num) / log(base);
}

double s_randFloat(char *operation) {
    char *test = functionHandler(operation, "randf");
    if (strcmp(test, BC_ERROR) == 0) return NAN;

    char *comma = find_top_level_comma(test);

    if (!comma) {
        printf("Error: randf() requires exactly 2 arguments\n\n");
        return NAN;
    }
    
    *comma = '\0';
    char *str_min = test;
    char *str_max = comma + 1;

    
    uint8_t nullCount = isnull(2, str_min, str_max);
    if (nullCount) {
        printf("Error: randf() requires exactly 2 arguments (missing %"PRIu8")\n\n", nullCount);
        SAFE_FREE(test);
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

    if (maxLf == QUICK_EVAL_FIX) {
        SAFE_FREE(test);
        return 0.0;
    }

    if (maxLf == (double)U64_NAN) {
        putchar('\n');
        SAFE_FREE(test);
        return NAN;
    }  
    
    if (strcasecmp(str_min, "rand_max") == 0)
        minLf = (double)RAND_MAX;
    else 
        minLf = eval(str_min, true);
    
    if (minLf == QUICK_EVAL_FIX) {
        SAFE_FREE(test);
        return 0.0;
    }

    if (minLf == (double)U64_NAN) {
        putchar('\n');
        SAFE_FREE(test);
        return NAN;
    }  

    SAFE_FREE(test);

    return random_range_float(minLf, maxLf);
}

double s_randInt(char *operation) {
    char *test = functionHandler(operation, "rand");
    if (strcmp(test, BC_ERROR) == 0) return NAN;

    char *comma = find_top_level_comma(test);

    if (!comma) {
        printf("Error: rand() requires exactly 2 arguments\n\n");
        return NAN;
    }
    
    *comma = '\0';
    char *str_min = test;
    char *str_max = comma + 1;
    
    uint8_t nullCount = isnull(2, str_min, str_max);
    if (nullCount) {
        printf("Error: rand() requires exactly 2 arguments (missing %"PRIu8")\n\n", nullCount);
        SAFE_FREE(test);
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

    if (maxInt == QUICK_EVAL_FIX) {
        SAFE_FREE(test);
        return 0.0;
    }

    if (maxInt == (double)U64_NAN) {
        putchar('\n');
        SAFE_FREE(test);
        return NAN;
    }  

    if (strcasecmp(str_min, "rand_max") == 0)
        minInt = (double)RAND_MAX;
    else 
        minInt = eval(str_min, true);

    if (minInt == QUICK_EVAL_FIX) {
        SAFE_FREE(test);
        return 0.0;
    }

    if (minInt == (double)U64_NAN) {
        putchar('\n');
        SAFE_FREE(test);
        return NAN;
    }  

    SAFE_FREE(test);

    if (ceil(minInt) != minInt || ceil(maxInt) != maxInt) {
        printf("Error: rand() requires an integer!\n\n");
        return NAN;
    }

    return random_range_int((int32_t)minInt, (int32_t)maxInt);
}

double s_floor(char *operation) {
    char *test = functionHandler(operation, "floor");
    if (strcmp(test, BC_ERROR) == 0) return NAN;

    double num = eval(test, true);

    SAFE_FREE(test);

    if (num == QUICK_EVAL_FIX)
        return 0.0;

    if (num == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }   

    double result = floor(num);
    return result;
}

double s_ceil(char *operation) {
    char *test = functionHandler(operation, "ceil");
    if (strcmp(test, BC_ERROR) == 0) return NAN;

    double num = eval(test, true);

    SAFE_FREE(test);

    if (num == QUICK_EVAL_FIX)
        return 0.0;

    if (num == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }   

    return ceil(num);
}

double s_round(char *operation) {
    char *test = functionHandler(operation, "round");
    if (strcmp(test, BC_ERROR) == 0) return NAN;

    double num = eval(test, true);

    SAFE_FREE(test);

    if (num == QUICK_EVAL_FIX)
        return 0.0;

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

uint64_t fact(int64_t num) {
    if (num < 0) {
        printf("Error: cant factorial negative numbers with fact()\n\n");
        return U64_NAN;
    } else if (num == 0)
        return 1;

    for (uint16_t i = num - 1; i >= 1; i--)
        num *= i;

    return num;
}

double s_fact(char *operation) {
    char *test = functionHandler(operation, "fact");
    if (strcmp(test, BC_ERROR) == 0) return NAN;

    double num = eval(test, true);

    SAFE_FREE(test);

    if (num == QUICK_EVAL_FIX)
        return 0.0;


    if (num == (double)U64_NAN) {
        putchar('\n');
        return NAN;
    }   

    if (ceil(num) != num) {
        printf("Error: fact() requires an integer!\n\n");
        return NAN;
    }

    return fact((int64_t)num);
}

double s_sign(char *operation) {
    char *test = functionHandler(operation, "sign");
    if (strcmp(test, BC_ERROR) == 0) return NAN;

    double num = eval(test, true);

    SAFE_FREE(test);

    if (num == QUICK_EVAL_FIX)
        return 0.0;

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
    char *raw = functionHandler(operation, "sum");
    if (strcmp(raw, BC_ERROR) == 0) return NAN;

    char *test = strdup(raw);
    SAFE_FREE(raw);
    if (!test)
        return NAN;

    uint16_t commaCount = count_top_level_commas(test);

    if (commaCount < 1 || commaCount > 2) {
        printf(
            "Error: sum() function requires at least 2 arguments and at most 3 arguments\n\n"
        );
        SAFE_FREE(test);
        return NAN;
    }

    char *comma1 = find_top_level_comma(test);
    if (!comma1) {
        SAFE_FREE(test);
        return NAN;
    }

    char *comma2 = find_top_level_comma(comma1 + 1);

    char *initStr = test;
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
        printf("Error: sum() missing %"PRIu8" argument(s)\n\n", nullCount);
        SAFE_FREE(test);
        return NAN;
    }

    trim(initStr);
    trim(endStr);
    if (diffStr)
        trim(diffStr);

    double init = eval(initStr, true);

    if (init == QUICK_EVAL_FIX) {
        SAFE_FREE(test);
        return 0.0;
    }

    if (init == (double)U64_NAN) {
        putchar('\n');
        SAFE_FREE(test);
        return NAN;
    }   

    double end  = eval(endStr, true);

    if (end == QUICK_EVAL_FIX) {
        SAFE_FREE(test);
        return 0.0;
    }

    if (end == (double)U64_NAN) {
        putchar('\n');
        SAFE_FREE(test);
        return NAN;
    }   

    char defaultDiff[] = "1";
    double diff = eval(diffStr ? diffStr : defaultDiff, true);

    if (diff == QUICK_EVAL_FIX) {
        SAFE_FREE(test);
        return 0.0;
    }

    if (diff == (double)U64_NAN) {
        putchar('\n');
        SAFE_FREE(test);
        return NAN;
    }

    double result = gauss_range_double(init, end, diff);

    SAFE_FREE(test);
    return result;
}