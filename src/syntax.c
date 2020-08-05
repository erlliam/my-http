#include <stdbool.h>
#include <stddef.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "syntax.h"
/*
  TODO:
    Rewrite parsing code:
      Do not modify the data until it succeeds
      Think about what should happen when it fails
*/
const char delimiters[] = "\"(),/:;<=>?@[\\]{}";
const char sub_delims[] = "!$&'()*+,;=";

// Rules can be found at:
//   RFC 5234 B.1.

static bool is_sp(char c)
{
  return (c == 0x20);
}

static bool is_wsp(char c)
{
  return (c == '\t' || is_sp(c));
}

static bool is_crlf(char first, char second)
{
  return (first == '\r' && second == '\n');
}

static bool is_vchar(char c)
{
  return (c >= 0x21 && c <= 0x7E);
}

static bool is_delimiter(char c)
{
  for (size_t i = 0; i < strlen(delimiters); i++) {
    if (c == delimiters[i]) {
      return true;
    }
  }

  return false;
}

static bool is_tchar(char c)
{
  return (is_vchar(c) && !is_delimiter(c));
}

static bool is_alpha(char c)
{
  // A-Z, a-z
  return ((c >= 0x41 && c <= 0x5A)
       || (c >= 0x61 && c <= 0x7A));

}

static bool is_digit(char c)
{
  // 0-9
  return (c >= 0x30 && c <= 0x39);
}

static bool is_hex_digit(char c) {
  return ((is_digit(c)) || (c >= 0x41 && c <= 0x46));
}

static bool is_unreserved(char c)
{
  return (is_alpha(c) || is_digit(c) || c == '-'
          || c == '.' || c == '_' || c == '~');
}

static bool is_pct_encoded(char *s)
{
  return (*s == '%' && is_hex_digit(*(s + 1))
          && is_hex_digit(*(s + 2)));
}


static bool is_sub_delim(char c)
{
  for (size_t i = 0; i < strlen(sub_delims); i++) {
    if (c == sub_delims[i]) {
      return true;
    }
  }

  return false;
}

static bool is_pchar(char *s)
{
  return (is_unreserved(*s) || is_pct_encoded(s)
          || is_sub_delim(*s) || *s == ':' || *s == '@');
}

static bool is_query(char *s)
{
  return (is_pchar(s) || *s == '/' || *s == '?');
}

static bool parse_char(char **string, char parse)
{
  if (**string != parse) return false;
  *string += 1;

  return true;
}

static bool parse_char_and_null_terminate(char **string,
  char parse)
{
  if (**string != parse) return false;

  **string = '\0';
  *string += 1;

  return true;
}

static bool parse_crlf(char **string)
{
  char *cr = *string;
  char *lf = cr + 1;
  if (!is_crlf(*cr, *lf)) return false;

  *string = lf + 1;

  return true;
}

static void parse_ows(char **string)
{
  char *c = *string;
  for (; is_wsp(*c); c++);

  *string = c;
}

static bool parse_token(char **string)
{
  // min: 1 tchar, max: inf tchar
  char *c = *string;
  if (!is_tchar(*c)) return false;
  c++;

  for (; is_tchar(*c); c++);

  *string = c;

  return true;
}

// XXX it parses method and null terminates...
// I don't like the name parse_method
bool parse_method(char **string)
{
  char *start_position = *string;

  if (!parse_token(string)) goto fail;

  char *sp = *string;
  if (!parse_char(string, ' ')) goto fail;

  *sp = '\0';

  return true;

  fail:
    *string = start_position;
    return false;
}

bool parse_request_target(char **string)
{
  
  /*
request-target:
  [_] origin-form:
    absolute-path [ "?" query ]
      absolute-path: 1*( "/" segment )
        segment: *pchar
          pchar: unreserved / pct-encoded / sub-delims / ":" / "@"
            unreserved: ALPHA / DIGIT / "-" / "." / "_" / "~"
            pct-encoded: "%" HEXDIG HEXDIG
            sub-delims: "!" / "$" / "&" / "'" / "(" / ")" / "*" / "+" / "," / ";" / "="
      query: *( pchar / "/" / "?" )
  [_] absolute-form
  [_] authority-form
  [_] asterik-form
  */
  return false;
}

bool parse_http_version(char **string)
{
  char *s = *string;

  if (s[0] == 'H' && s[1] == 'T'   && s[2] == 'T'  &&
      s[3] == 'P' && s[4] == '/'   && isdigit(s[5]) &&
      s[6] == '.' && isdigit(s[7])) {
    return true;
  }
  return false;
}

bool parse_request_line(char **string,
  struct request_line *request_line)
{
  // XXX
  (void)string;
  (void)request_line;
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

bool parse_field_value(char **string)
{
  char *c = *string;
  char *last_vchar = NULL;

  // Empty field-value
  // handle empty field-value
  if (*c == '\r') last_vchar = c - 1;

  for (; is_vchar(*c) || is_wsp(*c); c++) {
    if (is_vchar(*c)) last_vchar = c;
  }

  *string = c;
  if (!parse_crlf(string)) return false;
  *(last_vchar + 1) = '\0';

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
