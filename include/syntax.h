#ifndef MY_HTTP_SYNTAX_H
#define MY_HTTP_SYNTAX_H

#include <stdbool.h>

struct request_line {
  char *method;
  char *request_target;
  char *http_version;
};

struct header_field {
  char *field_name;
  char *field_value;
};

struct header {
  size_t header_capacity;
  size_t header_length;
  struct header_field *header_fields;
};

struct request {
  struct request_line request_line;
  struct header headers;
};

struct request create_request(size_t header_capacity);

bool parse_request_line(char **string,
  struct request_line *request_line);

bool parse_method(char **string);
bool parse_request_target(char **string);
bool parse_http_version(char **string);

bool parse_header_field(char **string,
  struct header_field *header_field);

bool parse_headers(char **string, struct header *header);
bool parse_request(char **string,
  struct request_line *request_line,
  struct header *headers);

#endif
