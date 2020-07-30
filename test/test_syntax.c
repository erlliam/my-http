// #define NDEBUG
#include <assert.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "test_syntax.h"
#include "syntax.h"

void test_parse_method(void);
void test_parse_http_version(void);

void run_test_syntax(void)
{
  // test_is_checks();
  test_parse_method();
  // test_parse_request_target();
  test_parse_http_version();
  // test_parse_request_line();
}

void test_parse_method(void)
{
  {
    char string[] = "GET / HTTP/1.1";

    char *method = string;
    char *p_string = string;

    assert(parse_method(&p_string));
    assert(*(method + 3) == '\0');
    assert(strcmp("GET", method) == 0);
    assert(p_string == method + 4);
  }
  {
    char string[] = " GET / HTTP/1.1";
    char *p_string = string;
    assert(!parse_method(&p_string));
  }
  {
    char string[] = ";ET / HTTP/1.1";
    char *p_string = string;
    assert(!parse_method(&p_string));
  }
}

void test_parse_http_version(void)
{
  {
    char string[] = "HTTP/1.1";
    char *p_string = string;
    assert(parse_http_version(&p_string));
  }
  {
    char string[] = "hTTP/1.1";
    char *p_string = string;
    assert(!parse_http_version(&p_string));
  }
  {
    char string[] = "HTTP/a.1";
    char *p_string = string;
    assert(!parse_http_version(&p_string));
  }

}

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
