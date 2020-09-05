#include <arpa/inet.h>
#include <assert.h>
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

// @@@
// Make recv_client and run_server the most important
// things in this file
// @@@
// Perhaps I can move all of the other code into another
// file.

struct client_connections {
  struct pollfd *pfds;
  struct address *address;
  size_t capacity;
  size_t length;
};

// @@@
// Use sockaddr form, more compact
// What this means is to use an array of sockaddr to store
// the addresses. This means I will just use a function
// that gives me a string if I do want the string.
struct address {
  sa_family_t family;
  char ip[INET6_ADDRSTRLEN];
  in_port_t port;
};

static bool create_server(const char *node,
  const char *service, int *fd, struct address *address);
static int attempt_to_listen(const struct addrinfo *ai);
static void accept_connections(int fd,
  struct address address);
static void accept_client(
  struct client_connections *client_connections,
  int server_fd);
static void recv_client(
  struct client_connections *client_connections,
  size_t index);
static void get_address(struct sockaddr *sa,
  struct address *address);

static struct client_connections create_client_connections(
  void);
static void add_to_client_connections(
  struct client_connections *client_connections, int fd,
  struct address address);
static void del_from_client_connections(
  struct client_connections *client_connections, int index);

void print_address(struct address address);

void run_server(const char *ip, const char *port)
{
  int fd;
  struct address address;

  if (!create_server(ip, port, &fd,
      &address)) {
    exit(EXIT_FAILURE);
  }

  printf(
    "%s():\n"
    "\tfd: %d\n", __func__, fd);

  printf("\thttp://");
  print_address(address);

  accept_connections(fd, address);
}

static bool create_server(const char *node,
  const char *service, int *fd, struct address *address)
{
  struct addrinfo hints = {
    .ai_family = AF_UNSPEC,
    .ai_flags = AI_PASSIVE,
    .ai_protocol = getprotobyname("TCP")->p_proto,
    .ai_socktype = SOCK_STREAM,
  };

  struct addrinfo *res;
  int gair = getaddrinfo(node, service, &hints, &res);
  if (gair != 0) {
    fprintf(stderr,
      "getaddrinfo error: %s\n", gai_strerror(gair));
    return false;
  }

  for (struct addrinfo *r = res; r != NULL;
       r = r->ai_next) {
    *fd = attempt_to_listen(r);

    if (*fd != -1) {
      get_address(r->ai_addr, address);

      break;
    }
  }

  freeaddrinfo(res);
  return (*fd != -1);
}

static int attempt_to_listen(const struct addrinfo *ai)
{
  int sock_fd = socket(ai->ai_family, ai->ai_socktype,
    ai->ai_protocol);
  if (sock_fd == -1) {
    perror("socket error");
    return -1;
  }

  int reuseaddr = 1;
  if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR,
      &reuseaddr, sizeof(reuseaddr)) == -1) {
    perror(
      "continuing without SO_REUSEADDR\n"
      "setsockopt error");
  }

  if (bind(sock_fd, ai->ai_addr, ai->ai_addrlen) == -1) {
    perror("bind error");
    goto cleanup;
  }

  if (listen(sock_fd, 5) == -1) {
    perror("listen error");
    goto cleanup;
  }
  
  return sock_fd;

  cleanup:
    close(sock_fd);
    return -1;
}

static void accept_connections(int server_fd,
  struct address address)
{
  struct client_connections client_connections =
    create_client_connections();

  add_to_client_connections(&client_connections, server_fd,
    address);

  printf("%s():\n", __func__);

  for (;;) {
    int poll_count = poll(client_connections.pfds,
      client_connections.length, -1);
    int poll_found = 0;

    if (poll_count == -1) {
      perror("poll error");
      exit(EXIT_FAILURE);
    }

    for (size_t i = 0; i < client_connections.length &&
         poll_found != poll_count; i++) {
      if (client_connections.pfds[i].revents & POLLIN) {
        poll_found++;
        client_connections.pfds[i].revents = 0;
        if (client_connections.pfds[i].fd == server_fd) {
          accept_client(&client_connections, server_fd);
        } else {
          recv_client(&client_connections, i);
        }
      }
    } // loop through client_connections.pfds[]
  } // infinite loop
}

static void accept_client(
  struct client_connections *client_connections,
  int server_fd)
{
  struct sockaddr_storage sa_s;
  socklen_t addr_len = sizeof(sa_s);

  int client_fd = accept(server_fd, (struct sockaddr *)&sa_s,
    &addr_len);

  if (client_fd == -1) {
    perror("accept error");
    return;
  }

  struct address address;
  get_address((struct sockaddr *)&sa_s, &address);

  add_to_client_connections(client_connections, client_fd, address);

  printf("\tConnected: ");
  print_address(address);
}

static void recv_client(
  struct client_connections *client_connections,
  size_t index)
{
  int fd = client_connections->pfds[index].fd;
  char buffer[8192];

  int rb = recv(fd, buffer, sizeof(buffer) - 1, 0);

  if (rb <= 0) {
    printf("\tDisconnected: ");
    print_address(client_connections->address[index]);

    del_from_client_connections(client_connections, index);
    close(fd);
    return;
  }

  buffer[rb] = '\0';

  printf(
    "received from %s:%d:\n%s\n",
    client_connections->address[index].ip,
    client_connections->address[index].port,
    buffer);
}

static struct client_connections create_client_connections(
  void)
{
  // XXX
  // Not sure how I feel about the default capacity being
  // inside of the function that creates it
  size_t capacity = 5;

  struct pollfd *pfds = malloc(sizeof(struct pollfd) *
    capacity);
  struct address *address = malloc(sizeof(struct address) *
    capacity);

  if (!(pfds && address)) {
    fprintf(stderr, "out of memory");
    exit(EXIT_FAILURE);
  }

  return (struct client_connections) {
    .pfds = pfds,
    .address = address,
    .capacity = capacity,
    .length = 0
  };
}

static void add_to_client_connections(
  struct client_connections *client_connections, int fd,
  struct address address)
{
  if (client_connections->length ==
      client_connections->capacity) {
    {
      size_t old_capacity = client_connections->capacity;
      client_connections->capacity *= 2;
      assert(client_connections->capacity > old_capacity);
    }
    struct pollfd *cc_pfds = realloc(
      client_connections->pfds,
      sizeof(struct pollfd) * client_connections->capacity);

    struct address *cc_address = realloc(
      client_connections->address, sizeof(struct address) *
      client_connections->capacity);

    if (cc_pfds && cc_address) {
      client_connections->pfds = cc_pfds;
      client_connections->address = cc_address;
    } else {
      fprintf(stderr, "out of memory");
      exit(EXIT_FAILURE);
    }
  }

  
  client_connections->
    pfds[client_connections->length].fd = fd;
  client_connections->
    pfds[client_connections->length].events = POLLIN;

  // @@@ use pointer?
  // I have forgotten what this means
  client_connections->
    address[client_connections->length] = address;

  client_connections->length++;
}

static void del_from_client_connections(
  struct client_connections *client_connections, int index)
{
  client_connections->pfds[index] = client_connections->
    pfds[client_connections->length - 1];

  client_connections->length--;
}

static void get_address(struct sockaddr *sa,
  struct address *address)
{
  if (sa->sa_family == AF_INET) {
    struct sockaddr_in *in = (struct sockaddr_in *)sa;

    inet_ntop(AF_INET, &(in->sin_addr),
      address->ip, sizeof(address->ip));

    address->port = ntohs(in->sin_port);
  } else if (sa->sa_family == AF_INET6) {
    struct sockaddr_in6 *in6 = (struct sockaddr_in6 *)sa;

    inet_ntop(AF_INET6, &(in6->sin6_addr),
      address->ip, sizeof(address->ip));

    address->port = ntohs(in6->sin6_port);
  }

  address->family = sa->sa_family;
}

void print_address(struct address address)
{
  if (address.family == AF_INET) {
    printf("%s:%d\n", address.ip, address.port);
  } else {
    printf("[%s]:%d\n", address.ip, address.port);
  }
}
