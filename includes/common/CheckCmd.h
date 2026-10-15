#ifndef CHECKCMD_H
#define CHECKCMD_H

#include "types.h"
#include <stdint.h>

#ifndef _WIN64
    #include <stddef.h>
#endif

void checkLswrcSyntax(const char *data_folder);
var calc(var left, const char *operation, var right, bool mathLib);
void manCmd(char *instruction, const char **cmds, uint8_t isInsideBash);
var parse_operation(char *operation, const FuncEntry *functions, size_t funcCount, const char *uniOps, const char **multiOps, bool mathlib);

#endif
