#ifndef CHECKCMD_H
#define CHECKCMD_H

void checkLswrcSyntax(char *data_folder);
double calc(double num1, char *operation, double num2);
void manCmd(char *instruction, const char **cmds, uint8_t isInsideBash);
double CheckOperation(char *operation, char **functions, const char *uniOps, const char **multiOps, bool mathlib);

__attribute__((hot))
void processCommand(char *input, char *args, const char **cmds, uint16_t cmdCount, char **address, char *history_path, char *data_folder, uint8_t isInsideBash);

#endif