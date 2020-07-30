#include <stdlib.h>
#include <stdio.h>

#include "server.h"

int main(void)
{
  run_server("::", "5000");

  return EXIT_SUCCESS;
}
