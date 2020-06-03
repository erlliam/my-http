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

enum { buffer_size = 1024 };

int get_request(int client_fd, char *method, char *target) {
  char request_buffer[buffer_size];
  int bytes_received = recv(
    client_fd, request_buffer, buffer_size-1, 0);

  if (bytes_received == -1) {
    perror("Recv");
    return -1;
  } else if (bytes_received == 0) {
    return -1;
  } else if (bytes_received == buffer_size - 1) {
    puts("Too much data");
    return -1;
  }
  request_buffer[bytes_received] = '\0';

  char *current_char = request_buffer;

  while (*current_char != ' ') current_char++;
  size_t method_size = current_char-request_buffer + 1;
  current_char++;

  *method = malloc(method_size);
  snprintf(method, method_size, "%s", request_buffer);

  while (*current_char != ' ') current_char++;
  size_t target_size = current_char-&(request_buffer[method_size]) + 1;
  *target = malloc(target_size);
  snprintf(target, target_size, "%s", &request_buffer[method_size]);
  
  return 0;
}

int send_response(int client_fd, char *method, char *target) {
  char status_line_headers[] =
    "HTTP/1.1 200 OK\n"
    "Content-Type: text/html\n"
    "\n";

  if (strcmp(method, "GET") == 0) {
    puts("Get request being handled");
  }
  puts(method);

  char message_body[1024] = {0};
  FILE *index_file = fopen("TRASH/index.html", "r");
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
  puts("Connection accepted");
  char *method;
  char *target;

  int request_result = get_request(
    client_fd, &method, &target);
  if (request_result == -1) {
    close(client_fd);
    return;
  }

  int response_result = send_response(
    client_fd, &method, &target);
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
  // I am thinking of making a struct that holds IP+PORT
  // and then calling set_ip and set_port once
  // and the switch statement updates the struct

  int server_fd = create_server(&server_address);
  puts("Server created at: xxx.xxx.xxx.xxx:xxxx");
  for (;;) {
    accept_connection(server_fd);
  }
  return 0;
}
