#include <stdlib.h>
#include <stdio.h>

#include "server.h"

int main(void)
{
  int server_fd = create_server("127.0.0.1", "5000");
  if (server_fd == -1) {
    return EXIT_FAILURE;
  }

  printf(
    "created server:\n"
    "\tHardcoded.\n"
    "\thttp://127.0.0.1:5000\n"
  );

  puts("accepting connections:");
  accept_connections(server_fd);

  return EXIT_SUCCESS;
}
