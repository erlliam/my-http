#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>

#define SO_REUSEADDR_VALUE true

// System calls to create server:
//  [x] getaddrinfo() CRAPS
//  [x] socket()
//  [x] bind()
//  [_] listen()
//  [_] accept()


int create_server(const char *node, const char *service)
{
  struct addrinfo hints = {
    .ai_family = AF_UNSPEC,
    .ai_flags = AI_PASSIVE,
    .ai_protocol = getprotobyname("tcp")->p_proto,
    .ai_socktype = SOCK_STREAM,
  };

  struct addrinfo *res;

  int gai_return = getaddrinfo(node, service, &hints, &res);

  if (gai_return != 0) {
    perror("getaddrinfo error");
    exit(EXIT_FAILURE);
  }

  int socket_fd;

  for (struct addrinfo *r = res; r != NULL;
  r = r->ai_next) {

    socket_fd = socket(r->ai_family, r->ai_socktype,
      r->ai_protocol);

    if (socket_fd == -1) {
      perror("socket error");
      continue;
    }

    int sso_return = setsockopt(socket_fd, SOL_SOCKET,
      SO_REUSEADDR,
      &(int){SO_REUSEADDR_VALUE}, sizeof(int));

    if (sso_return == -1) {
      perror("setsockopt error");
      continue;
    }

    int bind_return = bind(socket_fd, r->ai_addr,
      r->ai_addrlen);

    if (bind_return == -1) {
      perror("bind error");
      continue;
    }

    break;

  }

  freeaddrinfo(res);

  if (socket_fd == -1) {
    fprintf(stderr, "failed to create socket\n");
    exit(EXIT_FAILURE);
  }

  if (listen(socket_fd, 5) == -1) {
    perror("listen error");
    exit(EXIT_FAILURE);
  }

  return socket_fd;
}

int main(void)
{
  // create_server(NULL, "5000");
  int server_fd = create_server("127.0.0.1", "5000");
  puts("Created server.");
  printf("Server fd: %d\n", server_fd);

  return EXIT_SUCCESS;
}
