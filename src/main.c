#include <arpa/inet.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

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


int is_crlf(char *start) {
  if (*start == '\n' && *(start - 1) == '\r') return 1;

  return 0;
}

int contains_empty_line(char *data, size_t data_size) {
  if (data_size < 4) {
    return 0;
  }

  size_t current_index = 0;
  for (;;) {
    if (current_index == data_size) {
      return 0;
    }
    if (is_crlf(data + current_index) &&
        is_crlf(data + current_index + 2)) break;

    current_index++;
  }
  return 1;
}

// Really bad function name...
int fill_empty_buffer(int fd, char *buffer, size_t size) {
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
    bytes_available -= received_bytes;

    if (received_bytes < 4) {
      // TODO Fix here
      puts("We can't properly check for CRLF");
    }

    if (contains_empty_line(
      start_at, received_bytes)) break;

    if (total_bytes == buffer_capacity) {
      puts("Buffer is not big enough");
      return -1;
    }
  }

  buffer[total_bytes] = '\0';

  return 1;
}

enum { buffer_size = 8192 };

struct request_line {
  char *method;
  char *target;
  char *version;
};

void malloc_and_memcpy(char **string, size_t size, 
  char *start)
{
  *string = malloc(size + 1);
  memcpy(*string, start, size);
  (*string)[size] = '\0';
}

struct request_line *get_request_line(char *data) {
  // Validate the format of the request_line first...
  // There is no error checking going on here...
  // What could possibly go wrong?
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

int get_request(int client_fd,
  struct request_line **request_line)
{
  char *buffer = malloc(buffer_size);

  if (fill_empty_buffer(
    client_fd,
    buffer,
    buffer_size) == -1) return -1;

  *request_line = get_request_line(buffer);

  return 0;
}

int send_response(int client_fd,
  struct request_line request_line)
{
  if (strcmp(request_line.version, "HTTP/1.1")) {
    puts("We do not support this HTTP method");
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

  snprintf(target, le_size, "%s%s", root, request_line.target);
  puts(target);

  char http_200[] = "HTTP/1.1 200 OK\n"
  char status_line_headers[] =
    "HTTP/1.1 200 OK\n"
    "\n";

  puts(request_line.target);

  char message_body[1024] = {0};
  FILE *index_file = fopen(target, "r");
  if (index_file == NULL) puts("File not found.");

  fseek(index_file, 0, SEEK_END);
  int file_size = ftell(index_file);
  fseek(index_file, 0, SEEK_SET);

  fread(message_body, 1, 1024, index_file);

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
