#include <arpa/inet.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

#define ROOT_PATH "/home/altair/projects/my-http/static"

#define DEFAULT_IP "127.0.0.1"
#define DEFAULT_PORT "5000"

#define BUFFER_SIZE 8192

struct header_field {
  char *name;
  char *value;
};

struct request {
  char *method;
  char *target;
  char *version;
  size_t header_count;
  struct header_field *headers;
};

bool is_crlf(char *string)
{
  if (*string == 0xD && *(string + 1) == 0xA) {
    return true;
  }

  return false;
}

bool has_empty_line(char *string)
{
  for (char *c = string; *c != '\0'; c++) {
    if (is_crlf(c)) {
      if (is_crlf(c + 2)) {
        return true;
      }
    }
  }

  return false;
}

enum recv_from_client_return {
  RECV_ERROR,
  CONN_CLOSE,
  SIZE_LIMIT,
  RECV_SUCCESS
};

int fill_buffer(int fd, char *buffer,
  size_t buffer_size)
{
  size_t bytes_stored = 0;
  size_t size_limit = buffer_size - 1;
  size_t bytes_available = size_limit;

  // 8192 is a bad starting size. Perhaps start at 1024?

  for (;;) {
    char *insert_at = buffer + bytes_stored;

    ssize_t bytes_received = recv(fd, insert_at,
      bytes_available, 0);

    if (bytes_received == -1) return RECV_ERROR;
    else if (bytes_received == 0) return CONN_CLOSE;

    insert_at[bytes_received] = '\0';

    bytes_stored += bytes_received;
    bytes_available -= bytes_received;

    // TODO Handle a megabyte
    if (bytes_stored == size_limit) return SIZE_LIMIT;

    if (bytes_received >= 4)
      if (has_empty_line(insert_at))
        break;
    else if (bytes_stored >= 4)
      if (has_empty_line(insert_at - (4 - bytes_received)))
        break;
  }

  return RECV_SUCCESS;
}

bool recv_from_client(int fd, char **buffer)
{
  *buffer = malloc(BUFFER_SIZE);
  int result = fill_buffer(fd, *buffer, BUFFER_SIZE);

  switch (result) {
    case RECV_ERROR:
      return false;
    case CONN_CLOSE:
      return false;
    case SIZE_LIMIT:
      return false;
    case RECV_SUCCESS:
      return true;
  }

  puts("Read from client success");
  return true;
}

void set_headers(struct request *request)
{
  request->header_count = 1;
  request->headers = malloc(sizeof(struct header_field) *
    request->header_count);

  request->headers[0] = (struct header_field) {
    .name = "content-type",
    .value = "text/html",
  };

  puts("Set headers success");
}

// ! through ~
bool is_vchar(char *c) {
  if (*c > 0x20 && *c < 0x7F) {
    return true;
  }

  return false;
}

// "(),/:;<=>?@[\]{}
const char delimiters[] = {
  0x22, 0x28, 0x29, 0x2c, 0x2f, 0x3a, 0x3b, 0x3c, 0x3d,
  0x3e, 0x3f, 0x40, 0x5b, 0x5c, 0x5d, 0x7b, 0x7d
};

bool is_tchar(char *c)
{
  if (!is_vchar(c)) return false;
  for (const char *d = delimiters; *d != '\0'; d++) {
    if (*c == *d) {
      return false;
    }
  }

  return true;
}

bool extract_request_line(char **buffer,
  struct request *request)
{
  
  // all chars before space are tchars
  // validate_method_get_sapce
  // char *first_space = request_line_first_space(*buffer)
  // validating token before space

  request->method = "GET";
  request->target = "/";
  request->version = "HTTP/1.1";

  puts("Set request line success");
  return true;
}

bool parse_buffer(char **buffer,
  struct request *request)
{
  // No request line, fatal.
  // No headers, fatal? Not sure.
  // Parse a body?

  extract_request_line(buffer, request);
  // If fails, parse is failed.
  set_headers(request);

  puts("Parse buffer success");
  return true;
}

void accept_connection()
{
  int fd = 0;
  // Get fd

  char *buffer;
  char *buffer_start;
  struct request request;

  recv_from_client(fd, &buffer);
  // Complete
  buffer_start = buffer;

  parse_buffer(&buffer, &request);

  puts(request.method);
  puts(request.target);
  puts(request.version);
  puts(request.headers[0].name);

  free(request.headers);
}


int main()
{
  accept_connection();
  return 0;
}
