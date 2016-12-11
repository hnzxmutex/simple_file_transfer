#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "file_transfer.h"
#include "socket_wrapper.h"


int main(int argc, char **argv)
{
    int client_fd = create_socket();
    if (argc < 2) {
        printf("Usage: ./file_client ip\n");
        return 0;
    }
    if (!create_client(client_fd, argv[1], FT_SERVER_PORT)) {
        printf("Connect server fail\n");
        return 0;
    }
    char file_name[FT_NAME_MAX];
    printf("File Path:\t");
    scanf("%s", file_name);
    send_file_to_server(client_fd, file_name);
    return 0;
}
