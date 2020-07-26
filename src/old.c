#include <arpa/inet.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

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
  struct header_field headers[];
};


_Bool is_crlf(char *target) {
  if (*target == '\r') {
    char *next = target + 1;
    if (*next != '\0' && *next == '\n') {
      return 1;
    }
  }

  return 0;
}

// V(isible)CHAR

_Bool is_vchar(char *target) {
  if (*target >= 33 && *target <= 126) {
    return 1;
  }

  return 0;
}

const char delimiters[] = {
  '"', '(', ')', ',', '/', ':', ';', '<', '=', '>', '?',
  '@', '[', '\\', ']', '{', '}', '\0' };

// T(oken)CHAR is any VCHAR, except delimiters

_Bool is_tchar(char *target) {
  for (const char *c = delimiters; *c != '\0'; c++) {
    if (!is_vchar(target) || *target == *c) {
      return 0;
    }
  }

  return 1;
}

// RFC Whitespace: SP HTAB

_Bool is_whitespace(char *character) {
  if (*character == ' ' || *character == '\t') {
    return 1;
  }

  return 0;
}

// TODO: Implement empty_line check with no overflow
_Bool is_empty_line(char *target) {
  size_t crlf_count = 0;
  for (char *c = target; *c != '\0'; *c += 2) {
    if (is_crlf(c)) {
      crlf_count++;
    }
  }

  return crlf_count;
}
// CRLFCRLF

_Bool has_empty_line(char *target, size_t size) {
  for (size_t i = 0; size - i >= 4; i++) {
    if (is_crlf(target + i)) {
      if (is_crlf(target + (i + 2))) {
        return 1;
      }
    }
  }

  return 0;
}

FILE *get_file_from_root(char *file_name) {
  size_t path_size = strlen(ROOT_PATH) +
    strlen(file_name) + 1;

  char *file_path = malloc(path_size);
  if (!file_path) {
    free(file_path);
    return NULL;
  }

  int characters_written = snprintf(file_path, path_size,
    "%s%s", ROOT_PATH, file_name);

  if (characters_written != (int) path_size - 1 ) {
    free(file_path);
    return NULL;
  }

  FILE *file = fopen(file_path, "r");

  free(file_path);

  return file;
}

size_t get_file_size(FILE *file) {
  int previous_location = ftell(file);
  if (previous_location == -1L) return 0;

  if (fseek(file, 0, SEEK_END) == 0) {
    int file_size = ftell(file);
    if (file_size == -1L) return 0;

    if(fseek(file, previous_location, SEEK_SET) == 0) {
      return file_size;
    }
  }

  return 0;
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

    if (received_bytes == -1) return -1;
    else if (received_bytes == 0) return 0;

    total_bytes += received_bytes;
    if (total_bytes == buffer_capacity) return -2;
    bytes_available -= received_bytes;
    
    // TODO Think of way to make use of this initial
    // scanning of the request.
    if (received_bytes >= 4) {
      if (has_empty_line(start_at, received_bytes))
        break;
    } else if (total_bytes >= 4) {
      size_t offset = 4 - received_bytes;
      if (has_empty_line(start_at - offset,
        received_bytes + offset)) break;
    }
  }

  buffer[total_bytes] = '\0';

  return total_bytes;
}

_Bool extract_request_line(char **buffer,
  struct request *request)
{
  // *result = malloc(sizeof(request_line));

  char *first_space = NULL;
  for (char *c = *buffer; *c != '\0'; c++) {
    if (!is_tchar(c)) {
      if (*c == ' ') {
        first_space = c;
      }
      break;
    }
  }

  if (!first_space) {
    // free(result);
    return 0;
  }
  // TODO: Figure out what the target whitelist is
  // Figure out what the HTTP version whitelist is.

  char *second_space = strchr(first_space + 1, ' ');
  if (!second_space) {
    // free(result);
    return 0;
  }

  char *carriage_return = strchr(second_space + 1, '\r');
  if (!carriage_return && !is_crlf(carriage_return)) {
    // free(result);
    return 0;
  }

  request->method = *buffer;
  request->target = first_space + 1;
  request->version = second_space + 1;
  // (*result)->method = *request;
  // (*result)->target = first_space + 1;
  // (*result)->version = second_space + 1;

  *first_space = '\0';
  *second_space = '\0';
  *carriage_return ='\0';

  *buffer = carriage_return + 2;

  return 1;
}

void next_line(char **target) {
  char *carriage_return = strchr(*target, '\r');
  if (is_crlf(carriage_return)) {
    *target = carriage_return + 2;
  }
}

// TODO clean up crappy code

struct header_field *extract_header_field(char **buffer) {
  char *colon = NULL;
  for (char *c = *buffer; *c != '\0'; c++) {
    if (!is_tchar(c)) {
      if (*c == ':') {
        colon = c;
        break;
      } else if (is_crlf(c)) {
        next_line(buffer);
        return NULL;
      }

      break;
    }
  }

  if (!colon) return NULL;

  char *field_value = NULL;
  for (char *c = colon + 1; *c !='\0'; c++) {
    if (is_vchar(c)) {
      field_value = c;
      break;
    } else if (is_whitespace(c)) {
      // Skip whitespace.
    } else if (is_crlf(c)) {
      next_line(buffer);
      return NULL;
    }
  }

  if (!field_value) return NULL;

  char *last_vchar = field_value;
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

  if (!carriage_return) {
    // Really big error. Not sure if this ever happens.
    return 0;
  }

  *colon = '\0';
  *(last_vchar + 1) = '\0';

  struct header_field *result = malloc(sizeof(
    struct header_field));

  result->name = *buffer;
  result->value = field_value;
  
  *buffer = carriage_return + 2;

  return result;

}

// _Bool extract_headers(char **buffer,
//   struct request *request)
// {
//   size_t headers_size = 3;
//   size_t index = 0;
// 
//   *result = malloc(sizeof(headers));
//   (*result)->values = malloc(sizeof(header_field **) *
//     headers_size);
// 
//   while (!is_crlf(*buffer)) {
//     if (index == headers_size) {
//       headers_size *= 2;
// 
//       header_field **temp = realloc((*result)->values,
//         sizeof(header_field **) * headers_size);
// 
//       if (temp) (*result)->values = temp;
//       else {
//         free((*result)->values);
//         free(*result);
//         return 0;
//       }
//     }
// 
//     for (;;) {
//       struct header_field *extract_result =
//         extract_header_field( buffer);
// 
//       if (extract_result) {
//         (*result)->values[index] = extract_result;
//         break;
//       }
//     }
// 
//     index++;
//   }
// 
//   (*result)->size = index;
// 
//   return 1;
// }

_Bool read_from_client(int client_fd, char **buffer) {
  *buffer = malloc(BUFFER_SIZE);

  int result = fill_buffer_with_request(client_fd, *buffer,
    BUFFER_SIZE);

  if (result < 1) {
    if (result == -1) {
      puts("recv error when reading from client");
    } else if (result == 0) {
      puts("connected closed when reading from client");
    } else if (result == -2) {
      puts("buffer size reached when reading from client");
    }
    
    free(*buffer);
    return 0;
  }

  return 1;
}

_Bool parse_buffer(char **buffer, struct request *request)
{
  extract_request_line(buffer, request);
  // extract_headers(buffer, request);
  // if (!extract_request_line(request, client_request_line))
  //   return 0;

  // if (!extract_headers(request, request_headers)) return 0;

  return 1;
}


void send_404_response(int fd) {
  FILE *html = get_file_from_root("/404.html");
  if (!html) {
    puts("404 file is missing");
    return;
  }

  char status_line[] = "HTTP/1.1 404 Not Found\r\n";

  char headers_format[] =
    "Content-Type: text/html\r\n"
    "Content-Length: %ld\r\n";

  char empty_line[] = "\r\n";

  size_t html_size = get_file_size(html);
  char *body = malloc(html_size);
  size_t content_length = fread(body, 1, html_size, html);
  if (content_length != html_size) {
    puts("content_length != html_size");
  }

  size_t header_size = snprintf(NULL, 0,
    headers_format, content_length) + 1;
  char *headers = malloc(header_size);

  snprintf(headers, header_size,
    headers_format, html_size);

  send(fd, status_line, strlen(status_line), 0);
  send(fd, headers, strlen(headers), 0);
  send(fd, empty_line, strlen(empty_line), 0);
  send(fd, body, content_length, 0);

  free(headers);
  free(body);
  fclose(html);
  puts(status_line);
}

// void handle_get_request(int client_fd,
//   struct request *request)
// {
//   // TODO make things happen based on the headers we got
//   char index[] = "index.html";
// 
//   char last_character = client_request_line.target[
//     strlen(client_request_line.target) - 1];
// 
//   if (last_character == '/') {
//     size_t new_size = strlen(index) +
//       strlen(client_request_line.target) + 1;
// 
//     char *new_target = malloc(new_size);
//     // TODO error check snprintf...
//     snprintf(new_target, new_size, "%s%s",
//       client_request_line.target, index);
//     
//     // TODO check if we don't have to free this.
//     // free(client_request_line.target);
//     client_request_line.target = new_target;
//   }
// 
//   size_t path_size = strlen(ROOT_PATH) +
//     strlen(client_request_line.target) + 1;
// 
//   char *file_path = malloc(path_size);
// 
//   // TODO error check snprintf
//   snprintf(file_path, path_size, "%s%s",
//     ROOT_PATH, client_request_line.target);
// 
//   puts(client_request_line.target);
// 
//   printf("%30s\n", "START OF HEADERS");
//   for (size_t i = 0; i < request_headers.size; i++) {
//     printf("%s: %s\n",
//       request_headers.values[i]->name,
//       request_headers.values[i]->value);
//   }
//   printf("%30s\n", "END OF HEADERS");
// 
//   FILE *target_file = fopen(file_path, "r");
// 
//   if (target_file == NULL) {
//     send_404_response(client_fd);
//     return;
//   }
// 
//   // TODO generate response based on request headers
//   char status_line_headers[] =
//     "HTTP/1.1 200 OK\r\n"
//     "Content-Type: text/html\r\n"
//     "\r\n";
// 
//   size_t file_size = get_file_size(target_file);
//   char *body = malloc(file_size);
// 
//   size_t body_size = fread(body, 1, file_size, target_file);
// 
//   send(client_fd, status_line_headers,
//     strlen(status_line_headers), 0);
// 
//   send(client_fd, body, body_size, 0);
// }
void respond_to_head();

// int send_response(int client_fd,
//   request_line client_request_line,
//   headers request_headers)
// {
//   if (strcmp(client_request_line.version, "HTTP/1.1") != 0)
//   {
//     puts("Only HTTP/1.1 is supported.");
//     return -1;
//   }
// 
//   if (strcmp(client_request_line.method, "GET") == 0) {
//     handle_get_request(client_fd, client_request_line,
//       request_headers);
//   }
// /*
//   if (strcmp(request_line.method, "HEAD") == 0) {
//     respond_to_head();
//   }
// */
//   return 0;
// }

// void free_headers(headers **target) {
//   for (size_t i = 0; i < (*target)->size; i++) {
//     free((*target)->values[i]);
//   }
// 
//   free((*target)->values);
//   free(*target);
// }

void accept_connection(int server_fd) {
  int client_fd = accept(server_fd, NULL, NULL);

  char *buffer = NULL;
  if (!read_from_client(client_fd, &buffer)) return;
  puts("read from client success");
  char *buffer_start = buffer;

  struct request request;
  
  parse_buffer(&buffer, &request);
  puts("parse buffer success");
  puts(request->method);

  free(buffer_start);
  puts("freed buffer");

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