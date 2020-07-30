#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "test_syntax.h"

void ASSERT(char *message, bool check)
{
  if (check) {
    printf("__FILE__: %s\n", __FILE__);
    printf("__LINE__: %d\n", __LINE__);
    puts(message);
  }
}


int main(void)
{
  ASSERT("1 + 1 is two", 1 + 1 == 2);
  run_test_syntax();
  
  return EXIT_SUCCESS;
}
