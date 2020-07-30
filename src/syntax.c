#include <stdbool.h>
#include <stddef.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

bool is_sp(char c)
{
  return (c == 0x20);
}

bool is_crlf(char cr, char lf)
{
  return (cr == '\r' && lf == '\n');
}

bool is_vchar(char c)
{
  return (c >= 0x21 && c <= 0x7E);
}

char delimiters[] = {
  0x22, 0x28, 0x29, 0x2C, 0x2F, 0x3A, 0x3B, 0x3C, 0x3D,
  0x3E, 0x3F, 0x40, 0x5B, 0x5C, 0x5D, 0x7B, 0x7D
};

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

bool parse_method(char **string)
{
  char *c = *string;

  if (!is_tchar(*c)) return false;

  for (;;) {
    if (is_sp(*c)) break;
    if (!is_tchar(*c)) return false;

    c++;
  }

  *c = '\0';
  *string = c + 1;

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
      s[6] == u'.' && isdigit(s[7])  &&
      is_crlf(s[8], s[9])) {

    puts("if statement passed");
    s[8] = '\0';
    *string = s + 10;
    return true;
  }
  return false;
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
