#include <stdbool.h>

bool is_vchar(char c);
bool is_tchar(char c);
bool is_http_version(const char *s, char major, char minor);
bool parse_request_line(const char **string);
