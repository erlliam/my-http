// #define NDEBUG
#include <assert.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "test_syntax.h"
#include "syntax.h"

void test_parse_request_target(void);
void test_parse_http_version(void);

void test_parse_request_line(void);

void test_parse_header_field(void);

void test_parse_headers(void);

void run_test_syntax(void)
{
  // test_parse_request_target();
  // test_parse_http_version();

  test_parse_request_line();

  test_parse_header_field();
  test_parse_headers();
}

void test_parse_request_target(void)
{
  {
    char request_target_string[] = "/hi/hi/?omghiHTTP/1.1";
    char *current_position = request_target_string;

    assert(!parse_request_target(&current_position));
  }
  {
    char request_target_string[] = "/ HTTP/1.1";
    char *current_position = request_target_string;

    assert(parse_request_target(&current_position));
  }
  {
    char request_target_string[] = "/hi/hi/%";
    char *current_position = request_target_string;

    assert(!parse_request_target(&current_position));
  }
  {
    char request_target_string[] = "/hi/hi/%AF HTTP/1.1";
    char *current_position = request_target_string;

    assert(parse_request_target(&current_position));
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

void test_parse_request_line(void)
{
  {
    char request_line_string[] =
      "GET / HTTP/1.1\r\n"
      "uuu";
    // char request_line_string[] =
    //   "GET / HTTP/1.1\r\n"
    //   "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:79.0) Gecko/20100101 Firefox/79.0\r\n"
    //   "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"
    //   "Accept-Language: en-US,en;q=0.5\r\n"
    //   "Accept-Encoding: gzip, deflate, br\r\n";
    struct request_line request_line;
    char *current_position = request_line_string;

    assert(parse_request_line(
      &current_position, &request_line));

    assert(strcmp(request_line.method, "GET") == 0);
    assert(strcmp(request_line.request_target, "/") == 0);
    assert(strcmp(request_line.http_version, "HTTP/1.1") == 0);
    assert(strcmp(current_position, "uuu") == 0);
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

    assert(strcmp(header_field.field_name, "Server") == 0);
    assert(strcmp(
      header_field.field_value,
      "Apache/2.2.22 (Debian)") == 0);

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

bool strcmp_header_field(
  struct header_field header_field,
  char *field_name, char *field_value)
{
  return strcmp(header_field.field_name, field_name) == 0
    && strcmp(header_field.field_value, field_value) == 0;

}

void test_parse_headers(void)
{
  {
    char header_field_string[] =
      "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:79.0) Gecko/20100101 Firefox/79.0\r\n"
      "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"
      "Accept-Language: en-US,en;q=0.5\r\n"
      "Accept-Encoding: gzip, deflate, br\r\n"
      "\r\n";

    char *current_position = header_field_string;

    size_t header_capacity = 1;
    size_t header_length = 0;
    struct header_field *header_fields = malloc(sizeof(
      struct header_field) * header_capacity);

    struct header headers = {
      .header_capacity = header_capacity,
      .header_length = header_length,
      .header_fields = header_fields
    };

    assert(parse_headers(&current_position, &headers));
    assert(headers.header_length == 4);
    assert(headers.header_capacity ==
      (((1 * 2) * 2) * 2));
    assert(strcmp_header_field(headers.header_fields[0],
      "User-Agent", "Mozilla/5.0 (X11; Linux x86_64; rv:79.0) Gecko/20100101 Firefox/79.0"));
    assert(strcmp_header_field(headers.header_fields[1],
      "Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8"));
    assert(strcmp_header_field(headers.header_fields[2],
      "Accept-Language", "en-US,en;q=0.5"));
    assert(strcmp_header_field(headers.header_fields[3],
      "Accept-Encoding", "gzip, deflate, br"));


    free(headers.header_fields);
  }
}
