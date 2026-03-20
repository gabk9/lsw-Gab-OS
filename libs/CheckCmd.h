#ifndef CHECKCMD_H
#define CHECKCMD_H

#include "types.h"
#include <stdint.h>

void checkLswrcSyntax(const char *data_folder);
char *stringToVariable(const char *str, int32_t *changed);
var calc(var left, const char *operation, var right, bool mathLib);
void manCmd(char *instruction, const char **cmds, uint8_t isInsideBash);
var parse_operation(char *operation, const FuncEntry *functions, size_t funcCount, const char *uniOps, const char **multiOps, bool mathlib);

__attribute__((hot))
void processCommand(char *input, const char **cmds, char **address, char *history_path, char *data_folder, bool isInsideBash, bool isFromAlias);

#endif