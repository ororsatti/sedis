#include "cmd.h"
#include "../thirdparty/dyarray.h"
#include <_string.h>
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CLRF_LEN 2

int _cmd_get_next_token(char *str, size_t str_len, char **cursor, char *tok,
                        size_t tok_len) {
  size_t tok_idx = 0;
  char *p = *cursor;

  while (*p != '\r' && p - str <= str_len) {
    if (tok_len - 1 <= tok_idx) {
      return -1;
    }

    tok[tok_idx] = *p;
    tok_idx++;
    p++;
  }
  tok[tok_idx] = '\0';

  *cursor = p + CLRF_LEN;

  return tok_idx;
}

bool _cmd_read_bulk_str(char **cursor, char *tok, size_t tok_len,
                        size_t bulk_str_size) {

  if (bulk_str_size >= tok_len) {
    return false;
  }

  strlcpy(tok, *cursor, bulk_str_size + 1);
  *cursor += bulk_str_size + CLRF_LEN;
  return true;
}

int _cmd_parse_int(char *str, size_t str_len) {
  char *p = str;
  int val = 0;
  while (p - str < str_len) {
    val = (val * 10) + (*p++ - '0');
  }

  return val;
}

CmdTypes _cmd_get_type(char *cmd, size_t cmd_len) {
  if (strncmp(cmd, PING_CMD_TYPE, cmd_len) == 0) {
    return PING;
  }

  return UNKNOWN;
}

bool cmd_parse(char *src, size_t src_len, Cmd *out_cmd) {
  char tok[MAX_TOK_SIZE] = {0};
  char *cursor = src;
  int rv = _cmd_get_next_token(src, strlen(src), &cursor, tok, sizeof(tok));

  if (tok[0] != '*') {
    return false;
  }

  // subtract 1 to account for cmd name
  int argc = _cmd_parse_int(&tok[1], rv - 1) - 1;
  printf("argcp: %s, argc:%d\n", tok, argc);

  rv = _cmd_get_next_token(src, strlen(src), &cursor, tok, sizeof(tok));
  if (rv <= 0) {
    return false;
  }

  int cmd_tok_size = _cmd_parse_int(&tok[1], rv - 1);
  char cmd[MAX_TOK_SIZE] = {0};
  rv = _cmd_read_bulk_str(&cursor, cmd, sizeof(cmd), cmd_tok_size);
  if (rv <= 0) {
    return false;
  }

  out_cmd->type = _cmd_get_type(cmd, rv);
  if (out_cmd->type == UNKNOWN) {
    return false;
  }

  for (int i = 0; i < argc; ++i) {
    rv = _cmd_get_next_token(src, strlen(src), &cursor, tok, sizeof(tok));

    if (rv < 0) {
      return false;
    }

    Arg arg = {0};
    switch (tok[0]) {
    case ':': {
      arg.type = NUMBER;
      long val = strtol(&tok[1], NULL, 10);
      memcpy(arg.value, &val, sizeof(long));
      arg.size = sizeof(long);

    } break;
    case '$': {
      int str_size = _cmd_parse_int(&tok[1], rv - 1);
      if (!_cmd_read_bulk_str(&cursor, tok, sizeof(tok), str_size)) {
        return false;
      }

      strlcpy(arg.value, tok, str_size + 1);
      arg.size = str_size;
      arg.type = STRING;
    } break;
    case '+': {
      strlcpy(arg.value, &tok[1], sizeof(arg.value));
      arg.size = strlen(arg.value);
      arg.type = STRING;
    } break;
    default: {
      return false;
    }
    }

    dyarray_push(&out_cmd->args, arg);
  }

  return true;
}

#define MSG "*4\r\n$4\r\nPING\r\n:123\r\n$5\r\nABCDE\r\n+TEST\r\n"

const char *cmd_type_to_string(CmdTypes type) {
  switch (type) {
  case UNKNOWN:
    return "UNKNOWN";
  case PING:
    return "PING";
  default:
    return "INVALID";
  }
}
long _arg_number_value(Arg *arg) {
  long val = -1;
  memcpy(&val, arg->value, arg->size);

  return val;
}

char *_arg_string_value(Arg *arg) { return arg->value; }

int main(void) {
  Cmd out_cmd = {0};
  cmd_parse(MSG, sizeof(MSG), &out_cmd);
  printf("CMD: %s\n", cmd_type_to_string(out_cmd.type));

  for (int i = 0; i < out_cmd.args.size; ++i) {
    Arg arg = out_cmd.args.items[i];

    if (arg.type == NUMBER) {
      printf("ARG[%d]: %ld\n", i, _arg_number_value(&arg));
    }

    if (arg.type == STRING) {
      printf("ARG[%d]: %s\n", i, _arg_string_value(&arg));
    }
  }

  dyarray_free(out_cmd.args);
}
