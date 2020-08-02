#include <stdlib.h>
#include <stdio.h>

#include "server.h"

int main(void)
{
  const char test[] = "Hey!";

  test[0] = 'f';

  run_server("0", "5000");

  return EXIT_SUCCESS;
}
