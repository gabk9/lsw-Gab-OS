#ifndef TERMINAL_H
#define TERMINAL_H

#include <inttypes.h>

void listDrives(void);
void updatehistory(void);
void mkdirCmd(char *command);
void revCmd(char *instruction);
void grepCmd(char *instruction);
void echoCmd(char *instruction);
void rmdirCmd(char *instruction);
void touchCmd(char *instruction);
int32_t lcCmd(char *instruction);
void sleepCmd(char *instruction);
void renameCmd(char *instruction);
void neofetchCmd(char *lswrc_path);
char *randstrCmd(char *instruction);
void clearHistoryCmd(const char *path);
void rmCmd(uint16_t argc, char **argv);
char *unameCmd(uint16_t argc, char **argv);
void historyCmd(char *operation, const char *path);
void tailCmd(char *instruction, uint32_t max_lines);
void lsCmd(const char *option ,const char *address);
char *cdCmd(const char *instruction, char *address);
void bcCmd(uint16_t argc, char **argv, const char **cmds);
void catCmd(char *instruction, uint32_t max_lines, const char *cmdName);
void cmdsCommand(const char **cmds, uint16_t count, uint8_t isInsideBash);
void manCmdMulti(char *instruction, const char **cmds, uint8_t isInsideBash);
void bashCmd(uint16_t argc, char **argv, const char **cmds, uint16_t cmdCount, bool insideBash);

#endif