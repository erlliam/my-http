#include <stdlib.h>
#include <stdio.h>

#include "server.h"

void test(void) {
  struct the_test {
    char the_string[3];
  };

  struct the_test test;

  puts(test.the_string);

  snprintf(test.the_string, 10, "%s", "he");
  printf("%d\n", EXIT_FAILURE);
  printf("%d\n", EXIT_SUCCESS);

  puts(test.the_string);
}

int main(void)
{
  int server_fd = create_server(NULL, "5000");
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
