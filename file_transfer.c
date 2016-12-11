#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <limits.h>
#include <errno.h>
extern int errno;

#include "file_transfer.h"
#include "md5.h"
#define HEADER_LENGTH sizeof(struct ft_header)

struct ft_header {
    //自定义协议
    char const_title[4];
    long long file_size;
    char file_name[FT_NAME_MAX];
    char file_name_len;
};

static int is_big_endian()
{
    unsigned short test = 0x1234;
    return (*(unsigned char *)&test) == 0x12;
}

//翻转大端或小端
static long long revert_endian(long long input)
{
    return ((input&0x00000000000000ff)<<56) |
        ((input&0x000000000000ff00)<<40) |
        ((input&0x0000000000ff0000)<<24) |
        ((input&0x00000000ff000000)<<8) |
        ((input&0x000000ff00000000)>>8) |
        ((input&0x0000ff0000000000)>>24) |
        ((input&0x00ff000000000000)>>40) |
        ((input&0xff00000000000000)>>56);
}

static void print_md5_result(unsigned char decrypt[16])
{
    printf("md5 result:");
    for(int i = 0; i < 16; i++) {
        printf("%x",decrypt[i]);
    }
    printf("\n");
    fflush(stdout);
}

static char* get_file_name(const char* path)
{
    int len = strlen(path);
    int index = len - 1;
    while((*(path+index) != '\\' && *(path+index) != '/') && index > 0) {
        index--;
    }
    if (index == (len - 1)) {
        return NULL;
    } else if (index > 0) {
        index += 1;
    }
    len -= index;
    char *result = (char *)malloc(sizeof(char) * len);
    memset(result, 0, len);
    memcpy(result, path+index, len);
    return result;
}

static int recv_header(int conn_fd, struct ft_header* header)
{
    int len = 0, recv_len;
    char * recv_buf = (char *) header;
    do {
        recv_len = recv(conn_fd, recv_buf+len, HEADER_LENGTH - len, 0);
        len += recv_len;
    } while (len < HEADER_LENGTH && recv_len > 0);
    //传输统一使用大端序
    if (!is_big_endian()) {
        header->file_size = revert_endian(header->file_size);
    }

    printf("Get a file, name:%s,size:%lld\n", header->file_name, header->file_size);

    return (!strncmp(header->const_title, FT_CHAT_KEY, 4)) && header->file_size > 0;
}

int get_file_from_client(int conn_fd)
{
    struct ft_header header;
    memset(&header, 0, sizeof(header));

    if (!recv_header(conn_fd, &header)) {
        printf("Error request\n");
        return 0;
    }

    char path[FT_PATH_MAX] = FT_SAVE_PATH;
    int path_len = strlen(path);
    if (path_len + header.file_name_len > FT_PATH_MAX) {
        printf("Error name len\n");
        return 0;
    }
    memcpy(path+path_len, header.file_name, header.file_name_len);
    path[path_len + header.file_name_len] = '\0';
    if (access(path, F_OK) != -1) {
        printf("File exist!\n");
        return 0;
    }

    FILE *fp = fopen(path, "w");
    if (fp == NULL) {
        printf("File open fail");
        return 0;
    }

    unsigned char buffer[FT_BUFFER_SIZE];
    memset(&buffer, 0, FT_BUFFER_SIZE);

    MD5_CTX md5_context;
    memset(&md5_context, 0, sizeof(MD5_CTX));
    MD5Init(&md5_context);

    //recv长度,剩余文件大小
    int len, file_left_len = header.file_size;
    while ((len = recv(
                    conn_fd,
                    buffer,
                    FT_BUFFER_SIZE > file_left_len ? file_left_len : FT_BUFFER_SIZE,
                    0)
                ) && file_left_len > 0
            ) {
        if (len < 0) {
            printf("Recieve Data From Client Failed!\n");
            break;
        }

        MD5Update(&md5_context, buffer, len);

        int write_length = fwrite(buffer, sizeof(char), len, fp);
        if (write_length < len) {
            printf("File:\t%s Write Failed!\n", path);
            break;
        }

        file_left_len -= len;
        memset(&buffer, 0, FT_BUFFER_SIZE);
    }
    fclose(fp);
    if (file_left_len) {
        printf("Receive fail!\n");
        return 0;
    }

    unsigned char md5_result[16] = "\0";
    MD5Final(&md5_context, md5_result);
    print_md5_result(md5_result);
    if ((len = recv(conn_fd, buffer, 16, 0)) < 16) {
        printf("Receive MD5 result fail\n");
        return 0;
    }

    if (strncmp((char *)buffer, (char *)md5_result, 16)) {
        printf("MD5 Check sum fail!\n");
    } else {
        printf("MD5 Check sum success!\n");
    }

    if (send(conn_fd, &md5_result, 16, 0) < 0) {
        printf("Send MD5 fail\n");
        return 0;
    }
    return 1;
}

int send_file_to_server(int conn_fd, const char* file_path)
{
    struct ft_header header;
    memset(&header, 0, sizeof(header));
    strncpy(header.const_title, FT_CHAT_KEY, 4);

    char* real_file_name = get_file_name(file_path);
    strcpy(header.file_name, real_file_name);
    free(real_file_name);
    header.file_name_len = strlen(header.file_name);

    struct stat file_stat;
    memset(&file_stat, 0, sizeof(struct stat));
    if (0 != stat(file_path, &file_stat)) {
        printf("Get file stat fail\n");
        return 0;
    }

    header.file_size = file_stat.st_size;
    if (!is_big_endian()) {
        header.file_size = revert_endian(header.file_size);
    }
    if (send(conn_fd, &header, sizeof(struct ft_header), 0) < 0) {
        printf("error: %s\n", strerror(errno));
        printf("Send header fail\n");
        return 0;
    }

    FILE *fp = fopen(file_path, "r");
    if (NULL == fp) {
        printf("Open file fail\n");
        return 0;
    }

    unsigned char buffer[FT_BUFFER_SIZE];
    memset(&buffer, 0, FT_BUFFER_SIZE);

    MD5_CTX md5_context;
    memset(&md5_context, 0, sizeof(MD5_CTX));
    MD5Init(&md5_context);

    int len = 0;
    while((len = fread(buffer, sizeof(char), FT_BUFFER_SIZE, fp)) > 0) {
        if (send(conn_fd, buffer, len, 0) < 0) {
            printf("Send File:\t%s Failed!\n", file_path);
            fclose(fp);
            return 0;
        }
        MD5Update(&md5_context, buffer, len);
        memset(&buffer, 0, FT_BUFFER_SIZE);
    }
    fclose(fp);

    unsigned char md5_result[16] = "\0";
    MD5Final(&md5_context, md5_result);
    print_md5_result(md5_result);

    if (send(conn_fd, &md5_result, 16, 0) < 0) {
        printf("Send MD5 fail\n");
        return 0;
    }

    if ((len = recv(conn_fd, buffer, 16, 0)) < 16) {
        printf("Receive MD5 result fail\n");
        return 0;
    }

    if (strncmp((char *)buffer, (char *)md5_result, 16)) {
        printf("MD5 Check sum fail\n");
    } else {
        printf("MD5 Check sum success!\n");
    }

    return 1;
}
