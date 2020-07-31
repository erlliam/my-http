#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "test_syntax.h"

void sdfafs(char *message, bool check)
{
  if (check) {
    printf("__FILE__: %s\n", __FILE__);
    printf("__LINE__: %d\n", __LINE__);
    puts(message);
  }

  puts("hi""__func__");
  printf("\x1B[1;92m");
  puts("[ PASSED ] parse_method");
  printf("\x1B[0m");

  printf("\x1B[1;91m");
  puts("[ FAILED ] parse_request_target");
  printf("\x1B[0m");
}

int main(void)
{
  run_test_syntax();
  
  return EXIT_SUCCESS;
}
