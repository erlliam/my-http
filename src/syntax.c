#include <stdbool.h>
#include <stddef.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "syntax.h"

const char delimiters[] = "\"(),/:;<=>?@[\\]{}";
const char sub_delims[] = "!$&'()*+,;=";

// Rules can be found at:
//   RFC 5234 B.1.

static bool is_sp(char c)
{
  return c == 0x20;
}

static bool is_wsp(char c)
{
  return c == '\t' || is_sp(c);
}

static bool is_crlf(char first, char second)
{
  return first == '\r' && second == '\n';
}

static bool is_vchar(char c)
{
  return c >= 0x21 && c <= 0x7E;
}

static bool is_delimiter(char c)
{
  char *sr = strchr(delimiters, c);

  return c != '\0' && sr != NULL;
}

static bool is_tchar(char c)
{
  return is_vchar(c) && !is_delimiter(c);
}

static bool is_alpha(char c)
{
  return ('A' <= c && c <= 'Z')
    || ('a' <= c && c <= 'z');
}

static bool is_digit(char c)
{
  return '0' <= c && c <= '9';
}

static bool is_hex_digit(char c) {
  return (is_digit(c))
    || ('A' <= c && c <= 'F')
    || ('a' <= c && c <= 'f');
}

static bool is_unreserved(char c)
{
  return is_alpha(c) || is_digit(c)
    || c == '-' || c == '.' || c == '_' || c == '~';
}

static bool is_pct_encoded(char *s)
{
  return s[0] == '%'
    && is_hex_digit(s[1])
    && is_hex_digit(s[2]);
}

static bool is_sub_delim(char c)
{
  char *sr = strchr(sub_delims, c);

  return c != '\0' && sr != NULL;
}

static bool is_pchar(char *s)
{
  /*pchar: unreserved / pct-encoded / sub-delims / ":" / "@"
      unreserved: ALPHA / DIGIT / "-" / "." / "_" / "~"
      pct-encoded: "%" HEXDIG HEXDIG
      sub-delims: "!" / "$" / "&" / "'" / "(" / ")" / "*"
                      / "+" / "," / ";" / "=" */
  return is_unreserved(*s) || is_pct_encoded(s)
    || is_sub_delim(*s) || *s == ':' || *s == '@';
}

static bool is_query(char *s)
{
  return is_pchar(s) || *s == '/' || *s == '?';
}

static bool parse_char(char **string, char parse)
{
  if (**string != parse) return false;
  *string += 1;

  return true;
}

static bool parse_crlf(char **string)
{
  if (!is_crlf((*string)[0], (*string)[1])) return false;

  *string += 2;

  return true;
}

static void parse_ows(char **string)
{
  for (; is_wsp(**string); *string += 1);
}

static bool parse_token(char **string)
{
  // min: 1 tchar, max: inf tchar
  if (!is_tchar(**string)) return false;
  *string += 1;

  for (; is_tchar(**string); *string += 1);

  return true;
}
// origin-form: absolute-path [ "?" query ]
//   absolute-path: 1*( "/" segment )
//     absolute-path is a / and a segment.
//     there must always be a / and a segment.
//     a segment can be empty.
//     segment: *pchar

//   query: *( pchar / "/" / "?" )

// Check for /
// Start checking for segments.
// If you are not a segment:
//   you might be the start of the query.
//   if you are not the start of the query:
//     the request_target must be done
//     (you are a space)

bool parse_request_target(char **string)
{
  if (!parse_char(string, '/')) return false;

  do {
    for (; is_pchar(*string); *string += 1);
  } while (parse_char(string, '/'));

  if (parse_char(string, '?')) {
    for (; is_query(*string); *string += 1);
  }

  return true;
}

bool parse_http_version(char **string)
{
  // XXX Hard coded http_version
  char *s = *string;

  if (!(s[0] == 'H' && s[1] == 'T' && s[2] == 'T' &&
        s[3] == 'P' && s[4] == '/' && s[5] == '1' &&
        s[6] == '.' && s[7] == '1')) return false;

  *string += 8;

  return true;
}

bool parse_request_line(char **string,
  struct request_line *request_line)
{
  // char *method = parse_token(string);
  // if (method == NULL) return false;
  // char *first_space = parse_token(string);
  // if (first_space == NULL) return false;

  char *method = *string;
  if (!parse_token(string)) return false;

  char *method_end = *string;
  if (!parse_char(string, ' ')) return false;

  char *request_target = *string;
  if (!parse_request_target(string)) return false;

  char *request_target_end = *string;
  if (!parse_char(string, ' ')) return false;

  char *http_version = *string;
  if (!parse_http_version(string)) return false;

  char *http_version_end = *string;
  if (!parse_crlf(string)) return false;

  *method_end = '\0';
  *request_target_end = '\0';
  *http_version_end = '\0';

  request_line->method = method;
  request_line->request_target = request_target;
  request_line->http_version = http_version;

  return true;
}

bool parse_field_value(char **string)
{
  char *field_value_end = NULL;

  if (**string == '\r') field_value_end = *string - 1;

  for (; is_vchar(**string) || is_wsp(**string); *string += 1) {
    if (is_vchar(**string)) field_value_end = *string;
  }

  field_value_end++;

  if (!parse_crlf(string)) return false;


  *field_value_end = '\0';

  return true;
}

bool parse_header_field(char **string,
  struct header_field *header_field)
{
  char *field_name = *string;
  if (!parse_token(string)) return false;

  char *field_name_end = *string;
  if (!parse_char(string, ':')) return false;
  parse_ows(string);

  char *field_value = *string;
  if (!parse_field_value(string)) return false;
  // parse_field_value null terminates after last vchar + 1
  // also parses crlf

  *field_name_end = '\0';

  header_field->field_name = field_name;
  header_field->field_value = field_value;

  return true;
}

bool parse_headers(char **string, struct header *header)
{
  for (;; header->header_length++) {

    if (header->header_length == header->header_capacity) {
      header->header_capacity *= 2;

      struct header_field *r_a = realloc(
        header->header_fields,
        sizeof(struct header_field)
        * header->header_capacity);
      if (r_a == NULL) {
        fprintf(stderr, "out of memory");
        exit(EXIT_FAILURE);
      }

      puts("Resized");
      header->header_fields = r_a;
    }
    // Actually parse
    // XXX what happens if only one field is incorrect?
    // do we just declare that request invalid?
    // If this fails, mid parse, it should revert it's pointer location?
    if (!parse_header_field(string, &header->header_fields[header->header_length])) {
      break;
    }
  }
  // Let parse request handle this?
  if (!parse_crlf(string)) return false;

  return true;
}

bool parse_request()
{
  // XXX put it all together
  return false;
}
