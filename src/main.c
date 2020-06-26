#include <arpa/inet.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

#define ROOT_PATH "/home/altair/projects/my-http/TRASH/"

#define DEFAULT_IP "127.0.0.1"
#define DEFAULT_PORT "5000"

_Bool is_crlf(char *target) {
  if (*target == '\r') {
    char *next = target + 1;
    if (*next != '\0' && *next == '\n') {
      return 1;
    }
  }
  return 0;
}

int has_empty_line(char *string, size_t size) {
  for (size_t i = 0; size - i >= 4; i++) {
    if (is_crlf(string + i)) {
      if (is_crlf(string + (i + 2))) {
        return 1;
      }
    }
  }

  return 0;
}

FILE *get_file_from_root(char *file_name) {
  size_t path_size = strlen(ROOT_PATH) + strlen(file_name);
  char *file_path = malloc(path_size + 1);
  snprintf(file_path, path_size + 1,
    "%s%s", ROOT_PATH, file_name);

  FILE *file = fopen(file_path, "r");
  free(file_path);

  if (file == NULL) {
    return NULL;
  }

  return file;
}

int get_file_size(FILE *file) {
  int previous_location = ftell(file);

  fseek(file, 0, SEEK_END);
  int file_size = ftell(file);

  fseek(file, previous_location, SEEK_SET);

  return file_size;
}

int fill_buffer_with_request(int fd, char *buffer,
  size_t size)
{
  size_t total_bytes = 0;
  size_t buffer_capacity = size - 1;
  size_t bytes_available = buffer_capacity;

  for (;;) {
    char *start_at = buffer + total_bytes;

    int received_bytes = recv(
      fd, start_at, bytes_available, 0);

    if (received_bytes == -1) {
      perror("Recv");
      return -1;
    } else if (received_bytes == 0) {
      puts("Connection closed");
      return -1;
    }

    total_bytes += received_bytes;
    if (total_bytes == buffer_capacity) {
      puts("Buffer is not big enough");
      return -1;
    }

    bytes_available -= received_bytes;
    
    if (received_bytes >= 4) {
      if (has_empty_line(start_at, received_bytes)) {
        break;
      }
    } else if (total_bytes >= 4) {
      size_t offset = 4 - received_bytes;
      if (has_empty_line(start_at - offset,
        received_bytes + offset))
      {
        break;
      }
    }
  }
  buffer[total_bytes] = '\0';

  return 1;
}

int has_valid_request_line(char *string) {
  size_t space_count = 0;
  size_t crlf_count = 0;

  for (char *c = string; *c != '\0'; c++) {
    if (*c == ' ') {
      space_count++;
    } else if (is_crlf(c)) {
      crlf_count++;
      break;
    }
  }

  if (space_count == 2 && crlf_count == 1) {
    return 1;
  }

  return 0;
}

void malloc_and_memcpy(char **string, size_t size, 
  char *start)
{
  *string = malloc(size + 1);
  memcpy(*string, start, size);
  (*string)[size] = '\0';
}


enum { buffer_size = 8192 };


typedef struct request_line {
  char *method;
  char *target;
  char *version;
} request_line;

typedef struct header_field {
  char *field_name;
  char *field_value;
} header_field;

// Visible (printable char)
// hex: 21-7e
// decimal: 33-126

_Bool is_vchar(char *target) {
  if (*target >= 33 && *target <= 126) {
    return 1;
  }
  return 0;
}

// Delimiters DQUOTE and "(),/:;<=>?@[\]{}"
// tchar is any VCHAR, except delimiters

const char delimiters[] = {
  '"', '(', ')', ',', '/', ':', ';', '<', '=', '>', '?',
  '@', '[', '\\', ']', '{', '}', '\0' };


_Bool is_tchar(char *target) {
  for (const char *c = delimiters; *c != '\0'; c++) {
    if (!is_vchar(target) || *target == *c) {
      return 0;
    }
  }
  return 1;
}

void test_code() {
}

_Bool is_null(char *character) {
  if (*character == '\0') {
    return 1;
  }
  return 0;
}

typedef struct status_line {
  char *version;
  char *code;
  char *reason;
} status_line;

request_line *extract_request_line(char **request)
{
  // TODO: Figure out what the target whitelist is
  // Figure out what the HTTP version whitelist is.
  request_line *result = malloc(sizeof(
    request_line));

  char *first_space = NULL;
  for (char *c = *request; *c != '\0'; c++) {
    if (!is_tchar(c)) {
      if (*c == ' ') {
        first_space = c;
      }
      break;
    }
  }

  if (!first_space) return NULL;

  char *second_space = strchr(first_space + 1, ' ');
  if (!second_space) return NULL;

  char *carriage_return = strchr(second_space + 1, '\r');
  if (!carriage_return && !is_crlf(carriage_return))
    return 0;

  result->method = *request;
  result->target = first_space + 1;
  result->version = second_space + 1;

  *first_space = '\0';
  *second_space = '\0';
  *carriage_return ='\0';

  *request = carriage_return + 2;

  return result;
}

// Whitespace SP HTAB

_Bool is_whitespace(char *character) {
  if (*character == ' ' || *character == '\t') {
    return 1;
  }
  return 0;
}

char *find_whitespace(char *start) {
  for (char *c = start; *c != '\0'; c++) {
    if (is_whitespace(c)) {
      return c;
    }
  }
  return 0;
}

char *find_non_whitespace(char *string) {
  for (char *c = string; *c != '\0'; c++) {
    if (!is_whitespace(c)) {
      return c;
    }
  }
  return 0;
}

void next_line(char **request) {
  puts("Implement me.");
}

// By this point the request_line is extracted.
// request is the char after the first CRLF
// next_line function is not yet written.
// It will move the pointer request to the next line.

int extract_headers(char **request) {
  char *colon = NULL;
  for (char *c = *request; *c != '\0'; c++) {
    if (!is_tchar(c)) {
      if (*c == ':') {
        colon = c;
        break;
      }
      break;
    }
  }

  if (!colon) {
    next_line(request);
    return 0;
  }

  char *field_value = NULL;
  for (char *c = colon + 1; *c !='\0'; c++) {
    if (is_vchar(c)) {
      field_value = c;
      break;
    } else if (is_whitespace(c)) {
      // Skip whitespace.
    } else {
      break;
    }
  }

  if (!field_value) {
    next_line(request);
    return 0;
  }

  char *last_vchar = NULL;
  char *carriage_return = NULL;
  for (char *c = field_value + 1; *c !='\0'; c++) {
    if (is_vchar(c)) {
      last_vchar = c;
    } else if (is_whitespace(c)) {
      // Skip whitespace.
    } else if (is_crlf(c)) {
      carriage_return = c;
      break;
    } else {
      last_vchar = NULL;
      break;
    }
  }

  if (!last_vchar) {
    next_line(request);
    return 0;
  }

  if (!carriage_return) {
    // Really big error. Not sure if this ever happens.
    return 0;
  }

  *colon = '\0';
  *(last_vchar + 1) = '\0';

  // Field name
  puts(*request);
  // Field value
  puts(field_value);
  
  // Should I do a linked list?
  // RFC recommends hash table but I don't know how to
  // make one

  *request = carriage_return + 2;
  return 1;
}


_Bool parse_request(char *request_buffer,
  request_line **client_request_line)
{
  *client_request_line = extract_request_line(
    &request_buffer);

  if (!client_request_line) {
    puts("Invalid request line format");
    return 0;
  }

  extract_headers(&request_buffer);

  return 1;
}

int get_request(int client_fd,
  struct request_line **request_line)
{
  char *buffer = malloc(buffer_size);
  if (fill_buffer_with_request(client_fd, buffer,
    buffer_size) == -1) return -1;

  if (!parse_request(buffer, request_line)) return -1;

  return 1;
}

void send_404_response(int fd) {
  time_t now = time(NULL);
  printf("Seconds since 00:00, Jan 1 1970 UTC:\n"
    "\t%ld\n", now);

  char headers_format[] =
    "Content-Type: text/html\r\n"
    "Content-Length: %ld\r\n";

  FILE *html = get_file_from_root("404.html");
  if (html == NULL) {
    puts("404 file is missing");
    return;
  }

  int html_size = get_file_size(html);

  size_t header_size = snprintf(NULL, 0,
    headers_format, html_size) + 1;
  char *headers = malloc(header_size);
  snprintf(headers, header_size,
    headers_format, html_size);

  char *body = malloc(html_size);
  size_t body_size = fread(body, 1, html_size, html);

  char status_line[] = "HTTP/1.1 404 Not Found\r\n";
  char empty_line[] = "\r\n";

  send(fd, status_line, strlen(status_line), 0);
  send(fd, headers, strlen(headers), 0);
  send(fd, empty_line, strlen(empty_line), 0);
  send(fd, body, body_size, 0);

  free(headers);
  free(body);
  fclose(html);
  puts(status_line);
}

int send_response(int client_fd,
  struct request_line request_line)
{
  if (strcmp(request_line.version, "HTTP/1.1") != 0) {
    puts("Only HTTP/1.1 is supported.");
    return -1;
  }

  if (strcmp(request_line.method, "GET") != 0) {
    puts("Only GET is supported.");
    return -1;
  }

  char last_character = request_line.target[
    strlen(request_line.target) - 1];

  char index_html[] = "/index.html";

  if (last_character == '/') {
    size_t le_size = sizeof(index_html);
    request_line.target = malloc(le_size);

    snprintf(request_line.target, le_size,
      "%s", index_html);
  }

  char root[] = "/home/altair/projects/my-http/TRASH";

  size_t le_size = strlen(root) +
    strlen(request_line.target) + 1;

  char *target = malloc(le_size);

  snprintf(target, le_size, "%s%s",
    root, request_line.target);

  char status_line_headers[] =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "\r\n";

  char message_body[1024] = {0};
  FILE *target_file = fopen(target, "r");
  if (target_file == NULL) {
    send_404_response(client_fd);
    return -1;
  }

  fseek(target_file, 0, SEEK_END);
  int file_size = ftell(target_file);
  fseek(target_file, 0, SEEK_SET);

  fread(message_body, 1, 1024, target_file);

  send(
    client_fd, status_line_headers,
    strlen(status_line_headers), 0);

  send(
    client_fd, message_body,
    file_size, 0);

  return 0;
}

void accept_connection(int server_fd) {
  int client_fd = accept(server_fd, NULL, NULL);
  struct request_line *request_line;

  int request_result = get_request(client_fd,
    &request_line);

  if (request_result == -1) {
    close(client_fd);
    return;
  }

  int response_result = send_response(client_fd,
    *request_line);

  if (response_result == -1) {
    close(client_fd);
    return;
  }

  close(client_fd);
}

void print_and_exit(char *string, int status) {
  puts(string);
  exit(status);
}

void set_ip(char *ip, struct sockaddr_in *server_address) {
  int result = inet_pton(
    AF_INET, ip, &(server_address->sin_addr));
  if (!result) {
    print_and_exit("Invalid IP", 1);
  }
}

void set_port(
  char *port, struct sockaddr_in *server_address) {
  if (strlen(port) == 0) {
    print_and_exit("Invalid Port: empty", 1);
  }

  char *endptr;
  long port_check = strtol(port, &endptr, 10);
  if (!(*endptr == '\0')) {
    print_and_exit(
      "Invalid Port: must only contain numbers", 1);
  }

  if (!(port_check >= 0 && port_check <= 65535)) {
    print_and_exit("Invalid Port: must be 0-65535", 1);
  }

  server_address->sin_port = htons(port_check);
}

int create_server(struct sockaddr_in *server_address) {
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) perror("Socket");
  int optval = 1;
  setsockopt(
    server_fd, SOL_SOCKET, SO_REUSEADDR,
    &optval, sizeof(optval));

  int bind_result = bind(
    server_fd, (struct sockaddr *)server_address,
    sizeof(*server_address));
  if (bind_result == -1) perror("Bind");

  if (listen(server_fd, 10) == -1) perror("Listen");
  return server_fd;
}

int main(int argc, char **argv) {
  test_code();
  struct sockaddr_in server_address;
  server_address.sin_family = AF_INET;

  switch(argc) {
    case 1:
      set_ip(DEFAULT_IP, &server_address);
      set_port(DEFAULT_PORT, &server_address);
      break;
    case 2:
      set_ip(argv[1], &server_address);
      set_port("5000", &server_address);
      break;
    case 3:
    default:
      set_ip(argv[1], &server_address);
      set_port(argv[2], &server_address);
      break;
  }

  int server_fd = create_server(&server_address);

  printf(
    "Server created at: http://%s:%d\n",
    inet_ntoa(server_address.sin_addr),
    ntohs(server_address.sin_port));

  for (;;) {
    accept_connection(server_fd);
  }
  return 0;
}
