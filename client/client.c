#include <_string.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "../cmd/cmd.h"
#include <arpa/inet.h>

#define PORT "6378"
#define HOST "localhost"
#define CLRF "\r\n"
#define CLRF_LEN 2

#define MAXDATASIZE 1024

bool arg_to_resp(char *arg, size_t arg_len, char *dst, size_t dst_len) {
  char buf[MAXDATASIZE] = {0};
  bool is_numeric = true;

  for (size_t i = 0; i < arg_len; ++i) {
    if (isalpha(arg[i])) {
      is_numeric = false;
      break;
    }
  }

  int rv = 0;
  if (is_numeric) {
    rv = snprintf(buf, sizeof(buf), ":%s%s", arg, CLRF);
  } else {
    rv = snprintf(buf, sizeof(buf), "$%zu%s%s%s", arg_len, CLRF, arg, CLRF);
  }
  if (rv < 0) {
    return false;
  }

  if (strlcat(dst, buf, dst_len) >= dst_len) {
    return false;
  }

  return true;
}

bool args_to_resp(char **argv, int argc, char *dst, size_t dst_len) {

  char buf[MAXDATASIZE] = {0};
  snprintf(buf, sizeof(buf), "%s%d%s", "*", argc, CLRF);

  if (strlcat(dst, buf, dst_len) >= dst_len) {
    return false;
  }

  for (size_t i = 0; i < argc; ++i) {

    int rv = arg_to_resp(argv[i], strlen(argv[i]), dst, dst_len);
    if (rv <= 0) {
      return false;
    }
  }

  return true;
}

int main(int argc, char *argv[]) {
  int sockfd, numbytes;
  char buf[MAXDATASIZE] = {0};
  struct addrinfo hints = {0}, *servinfo, *p;
  int rv;
  char s[INET6_ADDRSTRLEN];

  if (argc < 2) {
    fprintf(stderr, "usage: client command ...arguments\n");
    exit(1);
  }

  if (!args_to_resp(&argv[1], argc - 1, buf, MAXDATASIZE - 1)) {
    fprintf(stderr, "failed to parse args\n");
    return 0;
  }

  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if ((rv = getaddrinfo(HOST, PORT, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  for (p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("client: socket");
      continue;
    }

    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      perror("client: connect");
      close(sockfd);
      continue;
    }

    break;
  }

  if (p == NULL) {
    fprintf(stderr, "client: failed to connect\n");
    return 2;
  }

  freeaddrinfo(servinfo);

  send(sockfd, buf, sizeof(buf), 0);

  memset(buf, 0, sizeof(buf));
  if ((numbytes = recv(sockfd, buf, MAXDATASIZE, 0)) == -1) {
    perror("recv");
    exit(1);
  }

  Cmd cmd = {0};
  cmd_parse(buf, numbytes, &cmd);

  memset(buf, 0, sizeof(buf));

  cmd_printable(&cmd, buf, sizeof(buf));

  printf("%s\n", buf);

  close(sockfd);

  return 0;
}
