#ifndef MY_HTTP_SYNTAX_H
#define MY_HTTP_SYNTAX_H

#include <stdbool.h>

struct header_field {
  char *field_name;
  char *field_value;
};

bool parse_method(char **string);
bool parse_http_version(char **string);
bool parse_header_field(char **string,
  struct header_field *header_field);

#endif
