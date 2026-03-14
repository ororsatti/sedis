#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/_types/_socklen_t.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT "6378"
#define BACKLOG 5

int MAX = 1024;

const char *inet_ntop2(void *addr, char *buf, size_t size) {
  struct sockaddr_storage *sas = addr;
  struct sockaddr_in *sa4;
  struct sockaddr_in6 *sa6;
  void *src;

  switch (sas->ss_family) {
  case AF_INET:
    sa4 = addr;
    src = &(sa4->sin_addr);
    break;
  case AF_INET6:
    sa6 = addr;
    src = &(sa6->sin6_addr);
    break;
  default:
    return NULL;
  }

  return inet_ntop(sas->ss_family, src, buf, size);
}

void handle_existing_client(int cli_fd, fd_set *p_master) {
  printf("Handling client: %d\n", cli_fd);

  char buf[MAX];
  int nbytes;
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

  send(cli_fd, buf, nbytes, 0);
}

int get_server_sock() {
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

void handle_new_client(int server_fd, fd_set *p_master, int *p_max_fd) {
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

  char remoteIP[INET6_ADDRSTRLEN];
  printf("sodis: new connection from %s on "
         "socket %d\n",
         inet_ntop2(&addr, remoteIP, sizeof remoteIP), conn_fd);
}

int main() {

  fd_set master, read_fds;

  FD_ZERO(&master);
  FD_ZERO(&read_fds);

  int serverfd = get_server_sock();
  FD_SET(serverfd, &master);

  int max_fd = serverfd;

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
        handle_new_client(serverfd, &master, &max_fd);
      } else {
        handle_existing_client(i, &master);
      }
    }
  }
}
