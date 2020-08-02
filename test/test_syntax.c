// #define NDEBUG
#include <assert.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "test_syntax.h"
#include "syntax.h"

void test_parse_method(void);
void test_parse_http_version(void);
void test_parse_header_field(void);

void run_test_syntax(void)
{
  // test_is_checks();
  test_parse_method();
  // test_parse_request_target();
  test_parse_http_version();
  // test_parse_request_line();
  test_parse_header_field();
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

void test_parse_header_field(void)
{
  {
    char header_field[] = 
      "Server: Apache/2.2.22 (Debian)\xD\xA";
    char *current_position = header_field;

    char *field_name = header_field;
    char *field_value = header_field + 8;
    assert(parse_header_field(&current_position));
    assert(strcmp("Server", field_name) == 0);
    assert(strcmp(
      "Apache/2.2.22 (Debian)", field_value) == 0);
    assert(current_position == (header_field +
      sizeof(header_field)) - 1);

    puts(field_name);
    puts(field_value);
  }
}
