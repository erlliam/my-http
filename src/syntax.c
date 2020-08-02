#include <stdbool.h>
#include <stddef.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

const char delimiters[] = {
  0x22, 0x28, 0x29, 0x2C, 0x2F, 0x3A, 0x3B, 0x3C, 0x3D,
  0x3E, 0x3F, 0x40, 0x5B, 0x5C, 0x5D, 0x7B, 0x7D
};

bool is_sp(char c)
{
  return (c == 0x20);
}

bool is_crlf(char first, char second)
{
  return (first == 0xD && second == 0xA);
}

bool is_vchar(char c)
{
  return (c >= 0x21 && c <= 0x7E);
}

bool is_delimiter(char c)
{
  // "(),/:;<=>?@[\]{}
  for (size_t i = 0; i < sizeof(delimiters); i++) {
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
    s[0] == u'H' && s[1] == u'T' && s[2] == u'T' &&
    s[3] == u'P' && s[4] == u'/' && s[5] == major &&
    s[6] == u'.' && s[7] == minor
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
  if (!parse_char_and_null_terminate(string, u' '))
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

  if (s[0] == u'H' && s[1] == u'T'   && s[2] == u'T'  &&
      s[3] == u'P' && s[4] == u'/'   && isdigit(s[5]) &&
      s[6] == u'.' && isdigit(s[7])) {
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
  if (!parse_char_and_null_terminate(string, u':'))
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
  for (; is_vchar(*c) || isblank(*c); c++) {
    if (is_vchar(*c)) last_vchar = c;
  }

  if (!is_crlf(*c, *(c + 1))) return false;

  *(last_vchar + 1) = '\0';
  // LF + 1
  *string = (c + 1) + 1;

  return true;
}

bool parse_header_field(char **string)
{
  char *field_name = *string;
  (void)field_name;
  if (!parse_field_name(string)) return false;

  char *field_value = *string;
  if (!parse_field_value(string)) return false;
  (void)field_value;
  
  return true;
}

struct request_line {
  char *method;
  char *request_target;
  char *http_version;
};

bool parse_request_line(char **string,
  struct request_line *request_line)
{
  char *method = *string;
  if (!parse_method(string)) return false;

  char *request_target = *string;
  if (!parse_request_target(string)) return false;

  char *http_version = *string;
  if (!parse_http_version(string)) return false;

  *request_line = (struct request_line) {
    .method = method,
    .request_target = request_target,
    .http_version = http_version
  };

  return true;

}
