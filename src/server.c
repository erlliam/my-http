#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "server.h"

// XXX update struct to hold ip and port information
struct sockets {
  struct pollfd *pfds;
  struct address *address;
  size_t capacity;
  size_t length;
};

struct address {
  char ip[INET6_ADDRSTRLEN];
  // XXX use in_port_t (uint16_t) in_port_t
  unsigned short port;
};

static int attempt_to_listen(struct addrinfo *addrinfo);
static struct address get_address(struct sockaddr_storage addr);
static void accept_client(struct sockets *sockets,
  size_t index);
static void recv_client(struct sockets *sockets,
  size_t index);
static void *get_in_addr(struct sockaddr *sa);

int create_server(const char *node, const char *service)
{
  struct addrinfo hints = {
    .ai_family = AF_UNSPEC,
    .ai_flags = AI_PASSIVE,
    .ai_protocol = getprotobyname("tcp")->p_proto,
    .ai_socktype = SOCK_STREAM,
  };

  // XXX make use of this addrinfffffo
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

static struct sockets create_sockets() {
  size_t capacity = 5;
  return (struct sockets) {
    .pfds = malloc(sizeof(struct pollfd) * capacity),
    .address = malloc(sizeof(struct address) * capacity),
    .capacity = capacity,
    .length = 0
  };
}

static void add_to_sockets(struct sockets *sockets, int fd,
  struct sockaddr_storage addr)
{
  if (sockets->length == sockets->capacity) {
    sockets->capacity *= 2;

    struct pollfd *pfds = realloc(sockets->pfds,
      sizeof(struct pollfd) * sockets->capacity);

    if (pfds) {
      sockets->pfds = pfds;
    } else {
      // XXX what to do, realloc fails
      puts("pfds realloc failed.");
    }

    struct address *address = realloc(sockets->address,
      sizeof(struct address) * sockets->capacity);

    if (address) {
      sockets->address = address;
    } else {
      // XXX what to do, realloc fails
      puts("address realloc failed.");
    }
  }

  sockets->pfds[sockets->length].fd = fd;
  sockets->pfds[sockets->length].events = POLLIN;
  sockets->address[sockets->length] = get_address(addr);

  sockets->length++;
}

static void del_from_sockets(struct sockets *sockets,
  int index)
{
  sockets->pfds[index].revents = 0;
  sockets->pfds[index] = sockets->pfds[sockets->length - 1];

  sockets->length--;
}

void accept_connections(const int server_fd)
{
  struct sockets sockets = create_sockets();
  // XXX fill with server sockaddr
  struct sockaddr_storage empty;
  add_to_sockets(&sockets, server_fd, empty);

  for (;;) {
    int poll_count = poll(sockets.pfds, sockets.length, -1);
    if (poll_count == -1) {
      perror("poll error");
      puts("exiting.");
      // XXX Probably shouldn't exit
      exit(EXIT_FAILURE);
    }

    for (size_t i = 0; i < sockets.length; i++) {
      // XXX Not sure how revents works.
      // Something involving bit masks?
      if (sockets.pfds[i].revents == POLLIN) {
        if (sockets.pfds[i].fd == server_fd) {
          accept_client(&sockets, i);
        } else {
          recv_client(&sockets, i);
        }
      }
    } // loop through sockets.pfds[]
  } // infinite loop
}

static void accept_client(struct sockets *sockets,
  size_t index)
{
  int fd = sockets->pfds[index].fd;

  struct sockaddr_storage addr;
  socklen_t addr_len = sizeof(addr);
  int client_fd = accept(fd, (struct sockaddr *)&addr,
    &addr_len);

  if (client_fd == -1) {
    perror("accept error");
    return;
  }

  add_to_sockets(sockets, client_fd, addr);
}

static void recv_client(struct sockets *sockets,
  size_t index)
{
  int fd = sockets->pfds[index].fd;
  char buffer[8192];

  int rb = recv(fd, buffer, sizeof(buffer), 0);

  if (rb <= 0) {
    if (rb == 0) {
      printf(
        "client disconnected:\n"
        "\t%s:%d\n",
        sockets->address[index].ip,
        sockets->address[index].port);
    } else {
      perror("recv error");
    }
    del_from_sockets(sockets, index);
    close(fd);
  }

  buffer[rb] = '\0';
  printf("%d: %s\n", fd, buffer);
}


static struct address get_address(struct sockaddr_storage addr)
{
  char client_ip[INET6_ADDRSTRLEN];
  const char *rc = inet_ntop(addr.ss_family,
    get_in_addr((struct sockaddr *)&addr),
    client_ip, sizeof(client_ip));

  if (rc == NULL) {
    // XXX should we set ip to null pointer?
    return (struct address) {
      .ip = "Hi",
      .port = 0
    };
  }

  struct address address;
  // XXX error check
  snprintf(address.ip, INET6_ADDRSTRLEN, "%s", client_ip);

  if (addr.ss_family == AF_INET) {
    address.port = ntohs(
      ((struct sockaddr_in *)&addr)->sin_port);
  } else if (addr.ss_family == AF_INET6) {
    address.port = ntohs(
      ((struct sockaddr_in6 *)&addr)->sin6_port);
  } else {
    address.port = 0;
  }
  
  // XXX
  printf(
    "client connected:\n"
    "\t%s:%d\n",
    address.ip,
    address.port);
  // printf("client connected:\n"
  //   "\tip:    %s\n"
  //   "\tport:  %d\n",
  //   address.ip,
  //   address.port);
  
  return address;
}

// function from: https://beej.us/guide/bgnet/examples/pollserver.c

static void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}
