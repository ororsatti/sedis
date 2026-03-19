#include "../thirdparty/stb_ds.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../cmd/cmd.h"

#define PORT "6378"
#define BACKLOG 5
#define MAXDATASIZE 1024

typedef struct Num_Map {
  char *key;
  Arg value;
} Num_Map;

typedef struct Context {
  Num_Map *num_map;
} Context;

int get_server_sock(void) {
  struct addrinfo hints = {
      .ai_flags = AI_PASSIVE,
      .ai_socktype = SOCK_STREAM,
      .ai_family = AF_UNSPEC,
  };
  struct addrinfo *ai, *p;

  int rv, listenerfd, yes = 1;

  if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
    fprintf(stderr, "sedis: failed get address info: %s\n", gai_strerror(rv));
    exit(1);
  }

  for (p = ai; p != NULL; p = p->ai_next) {
    if ((listenerfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) <
        0) {
      continue;
    }

    setsockopt(listenerfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    if (bind(listenerfd, p->ai_addr, p->ai_addrlen) != 0) {
      close(listenerfd);
      continue;
    }

    break;
  }

  if (p == NULL) {
    fprintf(stderr, "sedis: failed to bind\n");
    exit(2);
  }

  freeaddrinfo(ai);

  if (listen(listenerfd, BACKLOG) != 0) {
    perror("sedis: listen");
    exit(3);
  }

  return listenerfd;
}

void handle_existing_client(int cli_fd, fd_set *p_master, Context ctx) {
  char buf[MAXDATASIZE];
  int nbytes, rv;
  bzero(buf, sizeof(buf));

  if ((nbytes = recv(cli_fd, buf, sizeof(buf), 0)) <= 0) {
    if (nbytes == 0) {
      printf("sedis: connection closed: %d\n", cli_fd);
    } else {
      perror("recv");
    }

    FD_CLR(cli_fd, p_master);
    close(cli_fd);
    return;
  }

  Cmd cmd = {0};
  if (!cmd_parse(buf, nbytes, &cmd)) {
    arrfree(cmd.args);
    perror("parse cmd");
    return;
  }

  if (strncmp(cmd.args[0].value, "PING", cmd.args[0].size) == 0) {
    char *pong = "*1\r\n+PONG\r\n";
    send(cli_fd, pong, strlen(pong), 0);
  }

  if (strncmp(cmd.args[0].value, "SET", cmd.args[0].size) == 0) {
    if (arrlen(cmd.args) != 3) {
      char *err = "*1\r\n-ERR wrong amount of args\r\n";
      send(cli_fd, err, strlen(err), 0);
    } else {
      shput(ctx.num_map, cmd.args[1].value, cmd.args[2]);
      char *resp = "*1\r\n+OK\r\n";
      send(cli_fd, resp, strlen(resp), 0);
    }
  }

  if (strncmp(cmd.args[0].value, "GET", cmd.args[0].size) == 0) {
    if (arrlen(cmd.args) != 2) {
      char *err = "*1\r\n-ERR wrong amount of args\r\n";
      send(cli_fd, err, strlen(err), 0);
    } else {
      Cmd resp_cmd = {0};
      char resp_buf[MAXDATASIZE] = {0};
      Arg val = shget(ctx.num_map, cmd.args[1].value);

      arrput(resp_cmd.args, val);
      cmd_serialize(&resp_cmd, resp_buf, MAXDATASIZE);

      send(cli_fd, resp_buf, strlen(resp_buf), 0);
    }
  }

  arrfree(cmd.args);
}

void handle_new_client(int server_fd, fd_set *p_master, int *p_max_fd,
                       Context ctx) {
  struct sockaddr_storage addr;
  socklen_t addr_len = sizeof(addr);

  int conn_fd;
  if ((conn_fd = accept(server_fd, (struct sockaddr *)&addr, &addr_len)) < 0) {
    perror("accept");
    return;
  }

  FD_SET(conn_fd, p_master);
  if (conn_fd > *p_max_fd) {
    *p_max_fd = conn_fd;
  }

  handle_existing_client(conn_fd, p_master, ctx);
}

int main(void) {
  fd_set master, read_fds;

  FD_ZERO(&master);
  FD_ZERO(&read_fds);

  int serverfd = get_server_sock();
  FD_SET(serverfd, &master);

  int max_fd = serverfd;

  Context ctx = {0};
  sh_new_strdup(ctx.num_map);

  for (;;) {
    FD_COPY(&master, &read_fds);

    if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) == -1) {
      perror("select");
      exit(4);
    }

    for (int i = 0; i <= max_fd; ++i) {
      if (!FD_ISSET(i, &read_fds)) {
        continue;
      }

      if (i == serverfd) {
        handle_new_client(serverfd, &master, &max_fd, ctx);
      } else {
        handle_existing_client(i, &master, ctx);
      }
    }
  }
}
