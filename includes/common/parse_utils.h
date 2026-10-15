#ifndef PARSE_UTILS_H
#define PARSE_UTILS_H

#include <stdint.h>
#include <stdbool.h>

extern int8_t last_exit_code;
extern char *last_argument;

char **parseData(const char *str, uint16_t *count);
char **extract_args(char *args, uint16_t *argc, char *firstArg);
int16_t processCommand(char **argv,uint32_t argc,const char **cmds, char **address,
        char *history_path, char *data_folder, bool isInsideBash, bool isFromAlias);

#endif