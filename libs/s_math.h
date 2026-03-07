#ifndef S_MATH_H
#define S_MATH_H 

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

#define PI 3.14159265358979323846
#define E  2.71828182845904523536

#define KG_TO_LB_FACTOR 2.2046226218
#define LB_TO_KG_FACTOR 0.45359237

#define KM_TO_MI_FACTOR 0.62137
#define MI_TO_KM_FACTOR 1.609344

#define METERS_TO_FEET_FACTOR 3.28084
#define FEET_TO_METERS_FACTOR 0.3048

#ifdef _WIN32
    #undef RAND_MAX
    #define RAND_MAX 0x7FFFFFFF
#endif

bool isprime(int64_t n);
uint64_t fact(int64_t num, int32_t steps);
double h_atof(const char *str, bool mathlib);
double tetration(double base, int32_t height);
double s_ln(char *operation)__attribute__((nonnull));
char *s_chr(char *operation)__attribute__((nonnull));
char *s_oct(char *operation)__attribute__((nonnull));
char *s_hex(char *operation)__attribute__((nonnull));
char *s_bin(char *operation)__attribute__((nonnull));
double s_km(char *operation)__attribute__((nonnull));
double s_kg(char *operation)__attribute__((nonnull));
double s_fah(char *operation)__attribute__((nonnull));
double s_cel(char *operation)__attribute__((nonnull));
double s_log(char *operation)__attribute__((nonnull));
double s_bmi(char *operation)__attribute__((nonnull));
double s_tan(char *operation)__attribute__((nonnull));
double s_cos(char *operation)__attribute__((nonnull));
double s_sin(char *operation)__attribute__((nonnull));
double s_cot(char *operation)__attribute__((nonnull));
double s_sum(char *operation)__attribute__((nonnull));
double s_rad(char *operation)__attribute__((nonnull));
double s_abs(char *operation)__attribute__((nonnull));
double s_gon(char *operation)__attribute__((nonnull));
double s_deg(char *operation)__attribute__((nonnull));
double s_acos(char *operation)__attribute__((nonnull));
double s_asin(char *operation)__attribute__((nonnull));
double s_atan(char *operation)__attribute__((nonnull));
double s_acot(char *operation)__attribute__((nonnull));
double s_root(char *operation)__attribute__((nonnull));
double s_ceil(char *operation)__attribute__((nonnull));
double s_sign(char *operation)__attribute__((nonnull));
double s_log2(char *operation)__attribute__((nonnull));
double s_sqrt(char *operation)__attribute__((nonnull));
double s_fact(char *operation)__attribute__((nonnull));
double bc_len(char *operation)__attribute__((nonnull));
double s_feet(char *operation)__attribute__((nonnull));
double s_meter(char *operation)__attribute__((nonnull));
double s_miles(char *operation)__attribute__((nonnull));
double s_trunc(char *operation)__attribute__((nonnull));
double s_floor(char *operation)__attribute__((nonnull));
double s_round(char *operation)__attribute__((nonnull));
double s_log10(char *operation)__attribute__((nonnull));
double s_scale(char *operation)__attribute__((nonnull));
double bc_parse(char *operation)__attribute__((nonnull));
double s_pounds(char *operation)__attribute__((nonnull));
double s_randInt(char *operation)__attribute__((nonnull));
double s_isprime(char *operation)__attribute__((nonnull));
char *find_top_level_comma(char *s)__attribute__((nonnull));
char *bc_parse_str(char *operation)__attribute__((nonnull));
double s_randFloat(char *operation)__attribute__((nonnull));
int64_t parseBinToInt(const char *str)__attribute__((nonnull));
double parse_str_func(char *operation, const FuncEntry function);
uint16_t count_top_level_commas(const char *s)__attribute__((nonnull));

#define DEG_TO_RAD(x) ((x) * (PI / 180.0))
#define RAD_TO_DEG(x) ((x) * (180.0 / PI))
#define RAD_TO_GON(x) ((x) * (200.0 / PI))

#define KM_TO_MI(x) ((x) * KM_TO_MI_FACTOR)
#define MI_TO_KM(x) ((x) * MI_TO_KM_FACTOR)

#define LB_TO_KG(x) ((x) * LB_TO_KG_FACTOR)
#define KG_TO_LB(x) ((x) * KG_TO_LB_FACTOR)

#define C_TO_F(x) ((x) * 1.8 + (32.0))
#define F_TO_C(x) (((x) - 32.0) / 1.8)

#define M_TO_FT(meters) (meters * METERS_TO_FEET_FACTOR)
#define FT_TO_M(feet) (feet * FEET_TO_METERS_FACTOR)

#define BMI(weight, height) ((weight) / ((height) * (height)))

__attribute__((always_inline))
static inline uint32_t better_rand32(void) {
    return ((uint32_t)rand() << 16) ^ (uint32_t)rand();
}

static inline double gauss_range_double(double a, double b, double d) {
    if (d == 0.0) {
        puts("Error: step value cannot be zero");
        return NAN;
    }

    if ((d > 0.0 && a > b) || (d < 0.0 && a < b)) {
        puts("Error: step direction does not progress from X to Y");
        return NAN;
    }

    double raw_n = (b - a) / d;

    double steps = floor(raw_n);

    if (steps < 0.0) {
        puts("Error: calculated number of steps is negative");
        return NAN;
    }

    double n = steps + 1.0;

    double last = a + steps * d;

    double sum = n * (a + last) / 2.0;

    if (isnan(sum) || isinf(sum)) {
        puts("Error: Numeric overflow or invalid result during summation");
        return NAN;
    }
    return sum;
}

__attribute__((always_inline))
static inline double random_range_float(double min, double max) {
    uint32_t r = better_rand32();
    double normalized = (double)r / (double)UINT32_MAX;
    return min + normalized * (max - min);
}

__attribute__((always_inline))
static inline int32_t random_range_int(int32_t min, int32_t max) {
    uint32_t range = (uint32_t)(max - min + 1);
    uint32_t limit = UINT32_MAX - (UINT32_MAX % range);

    uint32_t r;
    do {
        r = better_rand32();
    } while (r >= limit);

    return min + (int32_t)(r % range);
}

#endif