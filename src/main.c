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
  // Bad to use no curly brace if statements?
  if (*start == '\n' && *(start - 1) == '\r') return 1;

  return 0;
}

int contains_empty_line(char *data, size_t last_index) {
  size_t current_index = 0;
  for (;;) {
    if (current_index == last_index) {
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
    
    if (contains_empty_line(buffer, total_bytes)) break;

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
  char *request_target;
  char *http_version;
};

void fill_request_line(
  struct request_line *request_line,
  char *data)
{
  // TODO
  printf("%p, %p ", (void *)request_line, data);
  request_line->method = "GET";
}

int get_request(int client_fd) {
  char *buffer = malloc(buffer_size);

  if (fill_empty_buffer(
    client_fd,
    buffer,
    buffer_size) == -1) return -1;

  struct request_line *request_line;
  request_line = malloc(sizeof(*request_line));

  fill_request_line(request_line, buffer);
  puts(request_line->method);

  return 0;
}

int send_response(int client_fd) {
  char status_line_headers[] =
    "HTTP/1.1 200 OK\n"
    "Content-Type: text/html\n"
    "\n";

  char message_body[1024] = {0};
  FILE *index_file = fopen(
    "/home/altair/projects/my-http"
    "/TRASH/index.html", "r");
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

  int request_result = get_request(client_fd);
  if (request_result == -1) {
    close(client_fd);
    return;
  }

  int response_result = send_response(client_fd);
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
  puts("Server created at: xxx.xxx.xxx.xxx:xxxx");
  for (;;) {
    accept_connection(server_fd);
  }
  return 0;
}
