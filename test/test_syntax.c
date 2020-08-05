// #define NDEBUG
#include <assert.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "test_syntax.h"
#include "syntax.h"

void test_parse_method(void);
void test_parse_request_target(void);
void test_parse_http_version(void);
void test_parse_header_field(void);

void run_test_syntax(void)
{
  test_parse_method();
  test_parse_request_target();
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
    assert(current_position == start);
  }
}


void test_parse_request_target(void)
{
    char request_target_string[] = "/ HTTP/1.1";
    char *current_position = request_target_string;

    assert(parse_request_target(&current_position));
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
    char header_field_string[] =
      "Server: Apache/2.2.22 (Debian)\xD\xA";
    struct header_field header_field;
    char *current_position = header_field_string;

    assert(parse_header_field(
      &current_position, &header_field));

    assert(strcmp("Server", header_field.field_name) == 0);
    assert(strcmp(
      "Apache/2.2.22 (Debian)",
      header_field.field_value) == 0);

    assert(current_position == (header_field_string +
      sizeof(header_field_string)) - 1);
  }
  {
    char header_field_string[] =
      "xdlol:        We are parsing   nicely     \xD\xA";
    struct header_field header_field;
    char *current_position = header_field_string;

    assert(parse_header_field(
      &current_position, &header_field));

    assert(strcmp("xdlol", header_field.field_name) == 0);
    assert(strcmp(
      "We are parsing   nicely",
      header_field.field_value) == 0);

    assert(current_position == (header_field_string +
      sizeof(header_field_string)) - 1);
  }
  {
    char header_field_string[] =
      "Not a token: Apache/2.2.22 (Debian)\xD\xA";
    struct header_field header_field;
    char *current_position = header_field_string;

    assert(!parse_header_field(
      &current_position, &header_field));
  }
  {
    char header_field_string[] =
      "Token:           \xD\xA";
    struct header_field header_field;
    char *current_position = header_field_string;

    assert(parse_header_field(
      &current_position, &header_field));
    assert(strcmp("Token", header_field.field_name) == 0);
    assert(strcmp("", header_field.field_value) == 0);
  }
  {
    char header_field_string[] =
      "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:79.0) Gecko/20100101 Firefox/79.0\xD\xA"
      "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\xD\xA"
      "Accept-Language: en-US,en;q=0.5\xD\xA"
      "Accept-Encoding: gzip, deflate, br\xD\xA";

    struct header_field user_agent;
    struct header_field accept;
    struct header_field accept_language;
    struct header_field accept_encoding;

    char *current_position = header_field_string;

    assert(parse_header_field(&current_position,
      &user_agent));
    assert(parse_header_field(&current_position,
      &accept));
    assert(parse_header_field(&current_position,
      &accept_language));
    assert(parse_header_field(&current_position,
      &accept_encoding));

    assert(strcmp("User-Agent", user_agent.field_name) == 0);
    assert(strcmp("Accept", accept.field_name) == 0);
    assert(strcmp("Accept-Language", accept_language.field_name) == 0);
    assert(strcmp("Accept-Encoding", accept_encoding.field_name) == 0);

    assert(strcmp("Mozilla/5.0 (X11; Linux x86_64; rv:79.0) Gecko/20100101 Firefox/79.0",
      user_agent.field_value) == 0);
    assert(strcmp("text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8",
      accept.field_value) == 0);
    assert(strcmp("en-US,en;q=0.5", accept_language.field_value) == 0);
    assert(strcmp("gzip, deflate, br", accept_encoding.field_value) == 0);
  }
}
