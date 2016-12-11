#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "file_transfer.h"
#include "socket_wrapper.h"


int main(int argc, char **argv)
{
    int server_fd = create_socket();
    if (!create_server(server_fd, FT_SERVER_PORT)) {
        printf("Create server fail\n");
        return 0;
    }

    //create folder
    struct stat dir_stat = {0};
    if (stat(FT_SAVE_PATH, &dir_stat) != 0 || !S_ISDIR(dir_stat.st_mode)) {
        mkdir(FT_SAVE_PATH, 0700);
    }
    while(1) {
        struct sockaddr_in client_addr;
        socklen_t length = sizeof(client_addr);

        int conn_fd = accept(server_fd, (struct sockaddr*)&client_addr, &length);
        if (conn_fd < 0) {
            printf("Accpet fail\n");
            break;
        }
        get_file_from_client(conn_fd);
        close(conn_fd);
    }
}
