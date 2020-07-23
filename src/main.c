#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>

// System calls to create server:
//  getaddrinfo() CRAPS
//  socket()
//  bind()
//  listen()
//  accept()

bool create_server(const char *node, const char *service)
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
    fprintf(stderr, "getaddrinfo %s\n",
      gai_strerror(gai_return));
    exit(EXIT_FAILURE);
  }

  for (struct addrinfo *r = res; r != NULL;
  r = r->ai_next) {
    int socket_fd = socket(r->ai_family, r->ai_socktype,
      r->ai_protocol);
    puts("Continue here.");
  }

  return false;
}

int main(void)
{
  // create_server(NULL, "5000");
  create_server("127.0.0.1", "5000");

  return EXIT_SUCCESS;
}
