#ifndef CMD_H
#define CMD_H

#include "../thirdparty/dyarray.h"
#include <stdbool.h>
#include <stddef.h>

#define MAX_TOK_SIZE 64

#define PING_CMD_TYPE "PING"
#define SET_CMD_TYPE "SET"
#define GET_CMD_TYPE "GET"

typedef enum CmdTypes { UNKNOWN = 0, PING, SET, GET } CmdTypes;

typedef enum ArgTypes { NUMBER = 0, STRING } ArgTypes;
typedef struct Arg {
  ArgTypes type;
  char value[MAX_TOK_SIZE];
  size_t size;
} Arg;

typedef dyarray_type(Arg) Args;

typedef struct Cmd {
  CmdTypes type;
  Args args;
} Cmd;

bool cmd_parse(char *str, size_t str_len, Cmd *out_cmd);

#endif // CMD_H
