#include <arpa/inet.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

#define ROOT_PATH "/home/altair/projects/my-http/TRASH/"

int is_crlf(char *string) {
  if (*string == '\r') {
    if (*(string + 1) == '\n') {
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

struct request_line {
  char *method;
  char *target;
  char *version;
};

struct request_line *get_request_line(char *data) {
  char *first_space = strchr(data, ' ');
  char *second_space = strchr(first_space + 1, ' ');
  char *first_cr = strchr(second_space + 1, '\r');

  size_t method_size = first_space - data;
  size_t target_size = second_space - (first_space + 1);
  size_t version_size = first_cr - (second_space + 1);

  struct request_line *request_line = malloc(
    sizeof(request_line));

  malloc_and_memcpy(&(request_line->method),
    method_size, data);

  malloc_and_memcpy(&(request_line->target),
    target_size, first_space + 1);

  malloc_and_memcpy(&(request_line->version),
    version_size, second_space + 1);

  return request_line;
}

enum { buffer_size = 8192 };

int get_request(int client_fd,
  struct request_line **request_line)
{
  char *buffer = malloc(buffer_size);
  int return_code = 0;

  if (fill_buffer_with_request(client_fd, buffer,
    buffer_size) == -1)
  {
    return_code = -1;
  }

  if (has_valid_request_line(buffer)) {
    *request_line = get_request_line(buffer);
  } else {
    return_code = -1;
  }

  free(buffer);
  return return_code;
}

struct status_line {
  char *version;
  char *code;
  char *reason;
};

struct headers {
  char *content_type;
};

struct status_line *make_status_line(char *version,
  char *code, char *reason);

void actual_send_response(int fd, struct status_line,
  struct headers, char *body);

FILE *get_file(char *file_name) {
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

void send_404_response(int fd) {
  char headers_format[] =
    "Content-Type: text/html\r\n"
    "Content-Length: %ld\r\n";

  size_t content_length = 69;
  size_t header_size = snprintf(NULL, 0,
    headers_format, content_length);

  char *headers = malloc(header_size + 1);
  snprintf(headers, header_size + 1,
    headers_format, content_length);

  char status_line[] = "HTTP/1.1 404 Not Found\r\n";
  char empty_line[] = "\r\n";

  FILE *html = get_file("404.html");
  int html_size = get_file_size(html);

  send(fd, status_line, strlen(status_line), 0);
  send(fd, headers, strlen(headers), 0);
  send(fd, empty_line, strlen(empty_line), 0);
  send(fd, "<p>Hi</p>", strlen("<p>Hi</p>"), 0);
}

int send_response(int client_fd,
  struct request_line request_line)
{
  
  // Check if HTTP version is supported.
  // Check if HTTP method is supported.
  // Check if target exists.

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

  puts(request_line.target);

  char message_body[1024] = {0};
  FILE *target_file = fopen(target, "r");
  if (target_file == NULL) {
    puts("File not found");
    // Send 404 not found.
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

  // validate request_line method, target, and version

  int response_result = send_response(client_fd,
    *request_line);

  free(request_line->method);
  free(request_line->target);
  free(request_line->version);
  free(request_line);

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
  struct sockaddr_in server_address;
  server_address.sin_family = AF_INET;

  switch(argc) {
    case 1:
      set_ip("127.0.0.1", &server_address);
      set_port("5000", &server_address);
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
