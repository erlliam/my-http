#ifndef MY_HTTP_SYNTAX_H
#define MY_HTTP_SYNTAX_H

#include <stdbool.h>

bool parse_method(char **string);
bool parse_http_version(char **string);

#endif
