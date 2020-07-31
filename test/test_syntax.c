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
    char request_line[] = "GET / HTTP/1.1";
    char *start = request_line;
    char *current_position = request_line;

    assert(parse_method(&current_position));
    assert(strcmp("GET", start) == 0);
    assert(current_position == start + 4);
  }
  {
    char request_line[] = " GET / HTTP/1.1";
    char *start = request_line;
    char *current_position = request_line;

    assert(!parse_method(&current_position));
    assert(current_position == start);
  }
  {
    char request_line[] = "G;ET / HTTP/1.1";
    char *start = request_line;
    char *current_position = request_line;

    assert(!parse_method(&current_position));
    assert(current_position == start + 1);
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
