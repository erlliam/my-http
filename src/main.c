#include <stdlib.h>
#include <stdio.h>

#include "server.h"
#include "syntax.h"

void test(void) {
  test_cases();
}

void real_main(void)
{
  int server_fd = create_server(NULL, "5000");
  if (server_fd == -1) {
    return;
  }

  printf(
    "created server:\n"
    "\tHardcoded.\n"
    "\thttp://127.0.0.1:5000\n"
  );

  puts("accepting connections:");
  accept_connections(server_fd);
}

int main(void)
{
  test();
  return EXIT_SUCCESS;
}

