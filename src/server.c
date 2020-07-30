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
  char ip[INET6_ADDRSTRLEN];
  in_port_t port;
};

bool create_server(const char *node, const char *service, int *fd, struct sockaddr_storage *addr);
void accept_connections(int server_fd, struct sockaddr_storage server_addr);
static int attempt_to_listen(struct addrinfo *addrinfo);
static struct address get_address(struct sockaddr_storage addr);
static void accept_client(struct sockets *sockets, size_t index);
static void recv_client(struct sockets *sockets, size_t index);
static void *get_in_addr(struct sockaddr *sa);

static struct sockets create_sockets();
static void add_to_sockets(struct sockets *sockets, int fd, struct sockaddr_storage addr);
static void del_from_sockets(struct sockets *sockets, int index);

void run_server(char *ip, char *port)
{
  int server_fd;
  struct sockaddr_storage server_addr;

  if (create_server(ip, port, &server_fd,
      &server_addr)) {

    printf("run_server():\n"
      "\tint server_fd: %d\n", server_fd);
    accept_connections(server_fd, server_addr);
  } else {
    exit(EXIT_FAILURE);
  }
}

bool create_server(const char *node, const char *service,
  int *fd, struct sockaddr_storage *addr)
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
    int sock_fd = attempt_to_listen(r);

    if (sock_fd != -1) {
      *fd = sock_fd;
      int port;

      char ip[INET6_ADDRSTRLEN];

      struct sockaddr *sa = r->ai_addr;
      if (sa->sa_family == AF_INET) {
        struct sockaddr_in *in = (struct sockaddr_in *)sa;

        inet_ntop(AF_INET, &(in->sin_addr),
          ip, sizeof(ip));
        port = ntohs(in->sin_port);
      } else if (sa->sa_family == AF_INET6) {
        struct sockaddr_in6 *in6 = (struct sockaddr_in6 *)sa;

        inet_ntop(AF_INET6, &(in6->sin6_addr),
          ip, sizeof(ip));
        port = ntohs(in6->sin6_port);
      }

      // XXX turn :: to [::]
      printf("http://%s:%d\n", ip, port);

      freeaddrinfo(res);
      return true;
    }
  }

  freeaddrinfo(res);
  return false;
}


void accept_connections(int server_fd,
  struct sockaddr_storage server_addr)
{
  struct sockets sockets = create_sockets();
  add_to_sockets(&sockets, server_fd, server_addr);

  puts("accepting connections");

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
