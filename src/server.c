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

struct sockets {
  struct pollfd *pfds;
  struct address *address;
  size_t capacity;
  size_t length;
};

struct address {
  sa_family_t family;
  char ip[INET6_ADDRSTRLEN];
  in_port_t port;
};

bool create_server(const char *node, const char *service,
  int *fd, struct address *address);
void accept_connections(int fd, struct address address);
static int attempt_to_listen(struct addrinfo *addrinfo);
void get_address2(struct sockaddr *sa, struct address *address);
static void accept_client(struct sockets *sockets, size_t index);
static void recv_client(struct sockets *sockets, size_t index);

static struct sockets create_sockets();
static void add_to_sockets(struct sockets *sockets,
  int fd, struct address address);
static void del_from_sockets(struct sockets *sockets,
  int index);

void run_server(char *ip, char *port)
{
  int fd;
  struct address address;

  if (create_server(ip, port, &fd,
      &address)) {

    printf(
      "%s():\n"
      "\tfd: %d\n", __func__, fd);

    address.family == AF_INET ?
      printf("\thttp://%s:%d\n",
        address.ip, address.port):
      printf("\thttp://[%s]:%d\n",
        address.ip, address.port);

    accept_connections(fd, address);

  } else {
    exit(EXIT_FAILURE);
  }
}

bool create_server(const char *node, const char *service,
  int *fd, struct address *address)
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
      get_address2(r->ai_addr, address);

      break;
    }
  }

  freeaddrinfo(res);
  return (*fd != -1);
}

void accept_connections(int server_fd,
  struct address address)
{
  struct sockets sockets = create_sockets();
  add_to_sockets(&sockets, server_fd, address);

  printf("%s()\n", __func__);

  for (;;) {
    int poll_count = poll(sockets.pfds, sockets.length, -1);
    if (poll_count == -1) {
      perror("poll error");
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

static int attempt_to_listen(struct addrinfo *ai)
{
  int sock_fd = socket(ai->ai_family, ai->ai_socktype,
    ai->ai_protocol);
  if (sock_fd == -1) {
    perror("socket error");
    return -1;
  }

  int REUSEADDR = 1;
  if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR,
      &REUSEADDR, sizeof(REUSEADDR)) == -1) {
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

static struct sockets create_sockets()
{
  // XXX SMELL?
  size_t capacity = 5;
  return (struct sockets) {
    .pfds = malloc(sizeof(struct pollfd) * capacity),
    .address = malloc(sizeof(struct address) * capacity),
    .capacity = capacity,
    .length = 0
  };
}

static void add_to_sockets(struct sockets *sockets, int fd,
  struct address address)
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
  sockets->address[sockets->length] = address;

  sockets->length++;
}

static void del_from_sockets(struct sockets *sockets,
  int index)
{
  // XXX should previous data be deleted or is it ok to
  // just overwrite as needed

  sockets->pfds[index].revents = 0;
  // set revents to 0 to prevent blocking on recv
  sockets->pfds[index] = sockets->pfds[sockets->length - 1];

  sockets->length--;
}

static void accept_client(struct sockets *sockets,
  size_t index)
{
  int fd = sockets->pfds[index].fd;

  struct sockaddr_storage sa_s;
  socklen_t addr_len = sizeof(sa_s);

  int client_fd = accept(fd, (struct sockaddr *)&sa_s,
    &addr_len);

  if (client_fd == -1) {
    perror("accept error");
    return;
  }

  struct address address;
  get_address2((struct sockaddr *)&sa_s, &address);

  add_to_sockets(sockets, client_fd, address);
}

static void recv_client(struct sockets *sockets,
  size_t index)
{
  int fd = sockets->pfds[index].fd;
  char buffer[8192];

  int rb = recv(fd, buffer, sizeof(buffer), 0);

  if (rb <= 0) {
    if (rb == 0) {
      // XXX Print [] for IPv6?
      printf("client disconnected:\n"
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
  // XXX remove this "chat" crap
  if (rb > 0) printf("%d: %s\n", fd, buffer);
}

void get_address2(struct sockaddr *sa,
  struct address *address)
{
  address->family = sa->sa_family;

  if (sa->sa_family == AF_INET) {
    struct sockaddr_in *in = (struct sockaddr_in *)sa;

    // XXX error check
    inet_ntop(AF_INET, &(in->sin_addr),
      address->ip, sizeof(address->ip));

    address->port = ntohs(in->sin_port);
  } else if (sa->sa_family == AF_INET6) {
    struct sockaddr_in6 *in6 = (struct sockaddr_in6 *)sa;

    // XXX error check
    inet_ntop(AF_INET6, &(in6->sin6_addr),
      address->ip, sizeof(address->ip));

    address->port = ntohs(in6->sin6_port);
  }
}
