#include <stdbool.h>
#include <stddef.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "syntax.h"

// Use static, use const for struct, write my own helper
// functions

// Make functions not modify unless fully suceeds
// Think about what happens if the parsing fails when it's
// allowed to fail (header fields)

const char delimiters[] = "\"(),/:;<=>?@[\\]{}";

static bool is_sp(char c)
{
  return (c == ' ');
}

bool is_crlf(char first, char second)
{
  return (first == '\r' && second == '\n');
}

bool is_vchar(char c)
{
  // return (c >= 0x21 && c <= 0x7E);
  return (c >= '!' && c <= '~');
}

bool is_delimiter(char c)
{
  // "(),/:;<=>?@[\]{}
  // XXX strchr?
  for (size_t i = 0; i < strlen(delimiters); i++) {
    if (c == delimiters[i]) {
      return true;
    }
  }

  return false;
}

bool is_tchar(char c) {
  return (is_vchar(c) && !is_delimiter(c));
}

bool is_http_version(const char *s, char major, char minor)
{
  // HTTP/x.x
  return (bool) (
    s[0] == 'H' && s[1] == 'T' && s[2] == 'T' &&
    s[3] == 'P' && s[4] == '/' && s[5] == major &&
    s[6] == '.' && s[7] == minor
  );
}

bool parse_char_and_null_terminate(char **string,
  char parse)
{
  if (**string != parse) return false;

  **string = '\0';
  *string += 1;

  return true;
}

bool parse_crlf(char **string)
{
  char *cr = *string;
  char *lf = cr + 1;
  if (!is_crlf(*cr, *lf)) return false;

  *string = lf + 1;

  return true;
}

void parse_ows(char **string)
{
  char *c = *string;
  for (; isblank(*c); c++);

  *string = c;
}

// min: 1 tchar, max: inf tchar
bool parse_token(char **string)
{
  char *c = *string;
  if (!is_tchar(*c)) return false;
  c++;

  for (; is_tchar(*c); c++);

  *string = c;

  return true;
}

bool parse_method(char **string)
{
  if (!parse_token(string)) return false;

  // XXX maybe parse_sp function?
  if (!parse_char_and_null_terminate(string, ' '))
    return false;

  return true;
}

bool parse_request_target(char **string)
{
  (void)string;
  return false;
}

bool parse_http_version(char **string)
{
  char *s = *string;

  if (s[0] == 'H' && s[1] == 'T'   && s[2] == 'T'  &&
      s[3] == 'P' && s[4] == '/'   && isdigit(s[5]) &&
      s[6] == '.' && isdigit(s[7])) {
  //   puts("if statement passed");
  //   s[8] = '\0';
  //   *string = s + 10;
    return true;
  }
  return false;
}

bool parse_field_name(char **string)
{
  if (!parse_token(string)) return false;
  if (!parse_char_and_null_terminate(string, ':'))
    return false;

  parse_ows(string);

  return true;
}

// Whitelist.
// Store last vchar.
// Once you hit CRLF, null terminate at last vchar + 1
// Deals with OWS
bool parse_field_value(char **string)
{
  char *c = *string;
  char *last_vchar = NULL;

  // Empty field-value
  // handle empty field-value
  if (*c == '\r') last_vchar = c - 1;

  for (; is_vchar(*c) || isblank(*c); c++) {
    if (is_vchar(*c)) last_vchar = c;
  }

  *string = c;
  if (!parse_crlf(string)) return false;
  *(last_vchar + 1) = '\0';

  // XXX "old" code, not sure if it's more clear?
  // char *lf = c + 1;
  // if (!is_crlf(*c, *lf)) return false;
  // *(last_vchar + 1) = '\0';
  // *string = lf + 1;

  return true;
}

bool parse_header_field(char **string,
  struct header_field *header_field)
{
  char *field_name = *string;
  if (!parse_field_name(string)) return false;

  char *field_value = *string;
  if (!parse_field_value(string)) return false;

  header_field->field_name = field_name;
  header_field->field_value = field_value;

  return true;
}
