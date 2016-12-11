#ifndef FILE_TRANSMIT_SOCKET
#define FILE_TRANSMIT_SOCKET


int create_socket();
int create_server(int socket_fd, int port);
int create_client(int socket_fd, const char* host, int port);

#endif
