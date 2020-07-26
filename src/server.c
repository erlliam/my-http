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

// XXX
void test_code();
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
  // fds is a misleading name!
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

void add_to_sockets(struct sockets *sockets, int fd)
{
  if (sockets->length == sockets->capacity) {
    sockets->capacity *= 2;
    sockets->fds = realloc(sockets->fds,
      sizeof(struct pollfd) * sockets->capacity);
  }

  sockets->fds[sockets->length].fd = fd;
  sockets->fds[sockets->length].events = POLLIN;

  sockets->length++;
}

void del_from_sockets(struct sockets *sockets, int index)
{
  sockets->fds[index] = sockets->fds[sockets->length - 1];

  sockets->length--;
}

void print_sockets(struct sockets sockets)
{
  puts("fds:");

  if (sockets.length == 0) {
    puts("\tfds is empty.");
  } else {
    for (size_t i = 0; i < sockets.length; i++) {
      printf(
        "\tfd:          %d\n",
        sockets.fds[i].fd
      );
    }
  }
  printf(
    "sockets:\n"
    "\tcapacity:   %ld\n"
    "\tlength:     %ld\n",
    sockets.capacity,
    sockets.length
  );
}

// XXX
void test_code()
{
  struct sockets sockets = create_sockets();
  print_sockets(sockets);
  
  add_to_sockets(&sockets, 5);
  print_sockets(sockets);
  del_from_sockets(&sockets, 0);
  print_sockets(sockets);
  add_to_sockets(&sockets, 100);
  add_to_sockets(&sockets, 34);
  add_to_sockets(&sockets, 12);
  // del_from_sockets(&sockets, 0);
  del_from_sockets(&sockets, 2);
  print_sockets(sockets);

  free(sockets.fds);
}

void accept_connections(const int server_fd)
{
  struct sockets sockets = create_sockets();
  add_to_sockets(&sockets, server_fd);

  for (;;) {
    int poll_count = poll(sockets.fds, sockets.length, -1);
    if (poll_count == -1) {
      perror("poll error");
      exit(EXIT_FAILURE);
      // Probably shouldn't exit
    }

    for (struct pollfd *pollfd = sockets.fds;
         pollfd < sockets.fds + sockets.length;
         pollfd++) {
      if (pollfd->revents == POLLIN) {
        if (pollfd->fd == server_fd) {
          int client_fd = accept(server_fd, NULL, NULL);
          if (client_fd == -1) {
            perror("accept error");
          } else {
            puts("\tclient connected.");
            add_to_sockets(&sockets, client_fd);
          }
        } else {
          char buffer[100];
          int nbytes = recv(pollfd->fd,
            buffer, sizeof(buffer), 0);

          if (nbytes <= 0) {
            if (nbytes == 0) {
              puts("Hoe hung up!");
            } else {
              perror("recv");
            }

            close(pollfd->fd);
            del_from_sockets(&sockets,
              (pollfd - sockets.fds));

          } else {
            buffer[99] = '\0';
            puts(buffer);
          }
        }
      }
    }
  }
}