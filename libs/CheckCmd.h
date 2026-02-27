#ifndef CHECKCMD_H
#define CHECKCMD_H

void checkLswrcSyntax(char *data_folder);
char *stringToVariable(const char *str, int32_t *changed);
double calc(double num1, char *operation, double num2, bool mathLib);
void manCmd(char *instruction, const char **cmds, uint8_t isInsideBash);
double CheckOperation(char *operation, FuncEntry *functions, size_t funcCount, const char *uniOps, const char **multiOps, bool mathlib);

__attribute__((hot))
void processCommand(char *input, const char **cmds, char **address, char *history_path, char *data_folder, bool isInsideBash, bool isFromAlias);

#endif