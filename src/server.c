#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <poll.h>

#include "server.h"

static int attempt_to_listen(struct addrinfo *addrinfo);

int create_server(const char *node, const char *service)
{
  struct addrinfo hints = {
    .ai_family = AF_UNSPEC,
    .ai_flags = AI_PASSIVE,
    .ai_protocol = getprotobyname("tcp")->p_proto,
    .ai_socktype = SOCK_STREAM,
  };

  struct addrinfo *res;

  int gair = getaddrinfo(node, service, &hints, &res);
  if (gair != 0) {
    fprintf(stderr, "getaddrinfo error: %s\n",
      gai_strerror(gair));
    exit(EXIT_FAILURE);
  }

  int socket_fd;

  for (struct addrinfo *r = res; r != NULL;
       r = r->ai_next) {
    socket_fd = attempt_to_listen(r);
    if (socket_fd != -1) {
      break;
    }
  }

  freeaddrinfo(res);

  return socket_fd;
}

static int attempt_to_listen(
  struct addrinfo *addrinfo)
{
  int reuseaddr = 1;

  int socket_fd = socket(addrinfo->ai_family,
    addrinfo->ai_socktype, addrinfo->ai_protocol);

  if (socket_fd == -1) {
    perror("socket error");
    return -1;
  }

  if (setsockopt(socket_fd, SOL_SOCKET,
      SO_REUSEADDR,
      &reuseaddr, sizeof(reuseaddr)) == -1) {
    perror(
      "continuing without SO_REUSEADDR\n"
      "setsockopt error");
    // puts("continuing without SO_REUSEADDR"); 
  }

  if (bind(socket_fd, addrinfo->ai_addr,
      addrinfo->ai_addrlen)) {
    perror("bind error");
    goto error;
  }

  if (listen(socket_fd, 5) == -1) {
    perror("listen error");
    goto error;
  }
  
  return socket_fd;

  error:
    close(socket_fd);
    return -1;
}

struct sockets {
  struct pollfd *fds;
  size_t capacity;
  size_t length;
};

struct sockets create_sockets() {
  size_t capacity = 5;
  return (struct sockets) {
    .fds = malloc(sizeof(struct pollfd) * capacity),
    .capacity = capacity,
    .length = 0
  };
}

void add_to_sockets(struct sockets sockets, int fd)
{
  if (sockets.length == sockets.capacity) {
    sockets.capacity *= 2;
    sockets.fds = realloc(sockets.fds,
      sizeof(struct pollfd) * sockets.capacity);
  }

  sockets.fds[sockets.length].fd = fd;
  sockets.fds[sockets.length].events = POLLIN;

  sockets.length++;
}

void accept_connections(const int server_fd)
{
  int fd_count = 0;
  int fd_size = 5;
  struct pollfd *pfds = malloc(sizeof *pfds * fd_size);

  pfds[0].fd = server_fd;
  pfds[0].events = POLLIN;

  fd_count++;

  for (;;) {
    int poll_count = poll(pfds, fd_count, -1);
    if (poll_count == -1) {
      perror("poll error");
      exit(EXIT_FAILURE);
      // Probably shouldn't exit
    }

    for (int i = 0; i < fd_count; i++) {
      if (pfds[i].revents == 1) {
        if (pfds[i].fd == server_fd) {
          int client_fd = accept(server_fd, NULL, NULL);
          puts("\tclient connected");

          if (client_fd == -1) {
            perror("accept error");
          } else {
            // Add to pollfd array
          }
        }
      }
    }
    // struct sockaddr client_addr;
    // socklen_t client_addr_size = sizeof(client_addr);
    // accept(server_fd, &client_addr, &client_addr_size);
  }

  free(pfds);
}
