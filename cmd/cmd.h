#ifndef CMD_H
#define CMD_H

#include <stdbool.h>
#include <stddef.h>

#define MAX_TOK_SIZE 64

#define PING_CMD_TYPE "PING"
#define SET_CMD_TYPE "SET"
#define GET_CMD_TYPE "GET"

typedef enum ArgTypes { NUMBER = 0, STRING } ArgTypes;
typedef struct Arg {
  ArgTypes type;
  char value[MAX_TOK_SIZE];
  size_t size;
} Arg;

typedef struct Cmd {
  Arg *args;
} Cmd;

bool cmd_parse(char *str, size_t str_len, Cmd *out_cmd);
bool cmd_serialize(Cmd *src, char *dst, size_t dst_len);
bool cmd_printable(Cmd *cmd, char *buf, size_t buf_len);

#endif // CMD_H
