#include <stdbool.h>
#include <stddef.h>
#include <ctype.h>
#include <assert.h>

bool is_sp(char c) { return (c == 0x20); }
bool is_vchar(char c) { return (c >= 0x21 && c <= 0x7E); }

// "(),/:;<=>?@[\]{}

char delimiters[] = {
  0x22, 0x28, 0x29, 0x2C, 0x2F, 0x3A, 0x3B, 0x3C, 0x3D,
  0x3E, 0x3F, 0x40, 0x5B, 0x5C, 0x5D, 0x7B, 0x7D
};

bool is_delimiter(char c)
{
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

// HTTP/x.x

bool is_http_version(const char *s, char major, char minor)
{
  return (bool) (
    s[0] == u'H' && s[1] == u'T' && s[2] == u'T' &&
    s[3] == u'P' && s[4] == u'/' && s[5] == major &&
    s[6] == u'.' && s[7] == minor
  );
}

void parse_method(const char **string)
{
  
}

void parse_request_target(const char **string)
{

}

void parse_http_version(const char **string)
{

}

bool parse_request_line(const char **string)
{
  // make spaces nul terminators
  // make cr nul terminator
  // G must be tchar
  // E must be tchar
  // T must be tchar
  // if not tchar we return DATA THAT ISTSNOT THCAR
  // BAD STNRAX

  // XXX stttart
  #include <stdio.h>
  // char *method = string
  // int rc = parse_method(&string);
  // if (!rc) exit
  // char *request_line = string;
  // parse_request_line(&string);

  parse_method(string);
  parse_request_target(string);
  parse_http_version(string);

  return true;
}

void test_cases()
{
  assert(is_sp(' ') == true);
  assert(is_sp('a') == false);

  assert(is_vchar(' ') == false);
  assert(is_vchar(';') == true);

  assert(is_delimiter(';') == true);
  assert(is_delimiter('a') == false);
  
  assert(is_tchar('z') == true);
  assert(is_tchar(';') == false);

  {
    char request_line[] = "GET / HTTP/1.1";
    char request_line2[] = "G;t / HtTP/1.1";

    // :O Test driven development??
    assert(parse_method(true) == true);
    assert(parse_method(false) == false);
  }
}
