#ifndef CHECKCMD_H
#define CHECKCMD_H

#include "types.h"
#include <stdint.h>

void checkLswrcSyntax(char *data_folder);
char *stringToVariable(const char *str, int32_t *changed);
void manCmd(char *instruction, const char **cmds, uint8_t isInsideBash);
evalOut calc(evalOut left, char *operation, evalOut right, bool mathLib);
evalOut parse_operation(char *operation, FuncEntry *functions, size_t funcCount, const char *uniOps, const char **multiOps, bool mathlib);

__attribute__((hot))
void processCommand(char *input, const char **cmds, char **address, char *history_path, char *data_folder, bool isInsideBash, bool isFromAlias);

#endif