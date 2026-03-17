#include "cmd.h"
#define STB_DS_IMPLEMENTATION
#include "../thirdparty/stb_ds.h"
#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXDATASIZE 1024
#define CLRF "\r\n"
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

long arg_number_value(Arg *arg) {
  long val = -1;
  memcpy(&val, arg->value, arg->size);

  return val;
}

char *arg_string_value(Arg *arg) { return arg->value; }

bool cmd_parse(char *src, size_t src_len, Cmd *out_cmd) {
  char tok[MAX_TOK_SIZE] = {0};
  char *cursor = src;
  int rv = _cmd_get_next_token(src, strlen(src), &cursor, tok, sizeof(tok));

  if (tok[0] != '*') {
    return false;
  }

  int argc = _cmd_parse_int(&tok[1], rv - 1);

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

    arrput(out_cmd->args, arg);
  }

  return true;
}

bool _arg_serialize(Arg *arg, char *dst, size_t dst_len) {
  char buf[MAXDATASIZE] = {0};

  int rv = 0;
  if (arg->type == NUMBER) {
    rv = snprintf(buf, sizeof(buf), ":%ld%s", arg_number_value(arg), CLRF);
  } else {
    rv = snprintf(buf, sizeof(buf), "$%zu%s%s%s", arg->size, CLRF, arg->value,
                  CLRF);
  }
  if (rv < 0) {
    return false;
  }

  if (strlcat(dst, buf, dst_len) >= dst_len) {
    return false;
  }

  return true;
}

bool cmd_serialize(Cmd *src, char *dst, size_t dst_len) {

  char buf[MAXDATASIZE] = {0};
  snprintf(buf, sizeof(buf), "%s%zu%s", "*", arrlen(src->args), CLRF);

  if (strlcat(dst, buf, dst_len) >= dst_len) {
    return false;
  }

  for (size_t i = 0; i < arrlen(src->args); ++i) {
    if (!_arg_serialize(&src->args[i], dst, dst_len)) {
      return false;
    }
  }

  return true;
}

bool cmd_printable(Cmd *cmd, char *buf, size_t buf_len) {
  for (size_t i = 0; i < arrlen(cmd->args); ++i) {
    Arg arg = cmd->args[i];

    if (arg.type == STRING) {
      if (strlcat(buf, arg_string_value(&arg), buf_len) >= buf_len) {
        return false;
      }
    } else if (arg.type == NUMBER) {
      snprintf(buf, buf_len, "%s%ld", buf, arg_number_value(&arg));
    }
    strlcat(buf, " ", buf_len);
  }

  return true;
}

// int main(void) {
//   Cmd cmd = {0};
//   Arg arg = {0};
//
//   arg.size = strlcpy(arg.value, "SET", MAX_TOK_SIZE);
//   arg.type = STRING;
//   dyarray_push(&cmd.args, arg);
//
//   arg.size = strlcpy(arg.value, "goofy", MAX_TOK_SIZE);
//   arg.type = STRING;
//   dyarray_push(&cmd.args, arg);
//
//   long val = 123;
//   memcpy(arg.value, &val, sizeof(long));
//   arg.size = sizeof(long);
//   arg.type = NUMBER;
//   dyarray_push(&cmd.args, arg);
//
//   char buf[MAXDATASIZE] = {0};
//   cmd_printable(&cmd, buf, MAXDATASIZE);
//
//   printf("%s\n", buf);
//
//   return 0;
// }
