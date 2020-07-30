// #define NDEBUG
#include <assert.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "syntax.h"

void test_parse_method();

void run_test_syntax()
{
  // test_is_checks();
  test_parse_method();
  // test_parse_request_target();
  // test_parse_http_version();
  // test_parse_request_line();
}

void test_parse_method()
{
  {
    char *request_line = malloc(256);
    snprintf(request_line, 256, "%s",
        "GET / HTTP/1.1");

    char *method = request_line;

    assert(parse_method(&request_line));
    assert(request_line == method + 4);
    assert(strcmp("GET", method) == 0);
    assert(*(method + 3) == '\0');
    // assert(*(method + 3) == '\0');

    free(method);
  }
  {
    char *request_line = malloc(256);
    snprintf(request_line, 256, "%s",
        "G(T/ / HTTP/1.1");
    // "(),/:;<=>?@[\]{}

    char *method = request_line;

    assert(!parse_method(&request_line));

    free(method);
  }
}


// 
// void test_is_checks()
// {
//   assert(is_sp(' ') == true);
//   assert(is_sp('a') == false);
// 
//   assert(is_vchar(' ') == false);
//   assert(is_vchar(';') == true);
// 
//   assert(is_delimiter(';') == true);
//   assert(is_delimiter('a') == false);
//   
//   assert(is_tchar('z') == true);
//   assert(is_tchar(';') == false);
// }
// 
// void test_parse_http_version() {
//   char *request_line = malloc(256);
//   snprintf(request_line, 256, "%s",
//       "HTTP/1.1\r\n");
// 
//   char *http_version = request_line;
// 
//   assert(parse_http_version(&request_line));
// 
//   puts(http_version);
//   free(http_version);
// }
