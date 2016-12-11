#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>

#include <errno.h>

extern int errno;

#define FT_CONN 5

int create_socket()
{
    int socket_fd;
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        printf("Create socket error!\n");
        return 0;
    }
    return socket_fd;
}

int create_server(int socket_fd, int port)
{
    struct sockaddr_in   server_addr;
    int reuse_on = 1;

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htons(INADDR_ANY);
    server_addr.sin_port = htons(port);

    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse_on, sizeof(reuse_on));

    if (bind(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr))) {
        printf("Server Bind Port: %d Failed!\n", port);
        return 0;
    }

    if (listen(socket_fd, FT_CONN) < 0) {
        printf("Server Listen fail\n");
        return 0;
    }

    return 1;
}

int create_client(int socket_fd, const char* host, int port)
{
    unsigned long inaddr;
    struct sockaddr_in client_addr;
    struct hostent *hp;

    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;

    inaddr = inet_addr(host);
    if (inaddr != INADDR_NONE) {
        memcpy(&client_addr.sin_addr, &inaddr, sizeof(inaddr));
    } else {
        hp = gethostbyname(host);
        if (hp == NULL)
            return -1;
        memcpy(&client_addr.sin_addr, hp->h_addr, hp->h_length);
    }
    client_addr.sin_port = htons(port);

    errno = 0;
    if (connect(socket_fd, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0 && errno != EINPROGRESS) {
        printf("errno = %d\n", errno);
        return -1;
    }
    return socket_fd;
}
