#ifndef MY_HTTP_SERVER_H
#define MY_HTTP_SERVER_H
int create_server(const char *node, const char *service);
void accept_connections(const int server_fd);
#endif