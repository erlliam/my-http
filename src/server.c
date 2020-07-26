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
  struct pollfd *pfds;
  size_t capacity;
  size_t length;
};

static void print_capacity_length(struct sockets sockets)
{
  printf(
    "sockets:\n"
    "\tcapacity:      %ld\n"
    "\tlength:        %ld\n",
    sockets.capacity,
    sockets.length
  );
}

static void print_sockets(struct sockets sockets)
{
  puts("pfds:");

  if (sockets.length == 0) {
    puts("\tpfds is empty.");
  } else {
    for (size_t i = 0; i < sockets.length; i++) {
      printf(
        "\tfd:          %d\n",
        sockets.pfds[i].fd
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

static struct sockets create_sockets() {
  size_t capacity = 5;
  return (struct sockets) {
    .pfds = malloc(sizeof(struct pollfd) * capacity),
    .capacity = capacity,
    .length = 0
  };
}

static void add_to_sockets(struct sockets *sockets, int fd)
{
  if (sockets->length == sockets->capacity) {
    sockets->capacity *= 2;
    // XXX error check realloc
    sockets->pfds = realloc(sockets->pfds,
      sizeof(struct pollfd) * sockets->capacity);
  }

  sockets->pfds[sockets->length].fd = fd;
  sockets->pfds[sockets->length].events = POLLIN;

  sockets->length++;
}

static void del_from_sockets(struct sockets *sockets,
  int index)
{
  sockets->pfds[index] = sockets->pfds[sockets->length - 1];

  sockets->length--;
}

static void accept_client(struct sockets sockets,
  size_t index);
static void recv_client(struct sockets sockets,
  size_t index);

void accept_connections(const int server_fd)
{
  struct sockets sockets = create_sockets();
  add_to_sockets(&sockets, server_fd);

  for (;;) {
    int poll_count = poll(sockets.pfds, sockets.length, -1);
    if (poll_count == -1) {
      perror("poll error");
      exit(EXIT_FAILURE);
      // Probably shouldn't exit
    }

    for (size_t i = 0; i < sockets.length; i++) {
      // XXX
      // Not sure how revents works.
      // Something involving bit masks?
      if (sockets.pfds[i].revents == POLLIN) {
        if (sockets.pfds[i].fd == server_fd) {
          accept_client(sockets, i);
        } else {
          recv_client(sockets, i);
        }
      } // revents == POLLIN
    } // loop through sockets.pfds[]
  } // infinite loop
}

static void accept_client(struct sockets sockets,
  size_t index)
{
  int fd = sockets.pfds[index].fd;

  // XXX store client information
  int client_fd = accept(fd, NULL, NULL);
  if (client_fd == -1) {
    perror("accept error");
  } else {
    add_to_sockets(&sockets, client_fd);
    puts("\tclient connected.");
  }
}

static void recv_client(struct sockets sockets,
  size_t index)
{
  int fd = sockets.pfds[index].fd;
  char buffer[8192];

  int rb = recv(fd, buffer, sizeof(buffer), 0);

  if (rb <= 0) {
    if (rb == 0) {
      puts("client closed connection.");
    } else {
      perror("recv error");
    }
    close(fd);
    del_from_sockets(&sockets, index);
  }

  buffer[rb] = '\0';
  fputs(buffer, stdout);
}
