#ifndef file_transfer
#define file_transfer

#define FT_SERVER_PORT 4433
#define FT_BUFFER_SIZE 1024
#define FILE_NAME_MAX_SIZE 512

#define FT_NAME_MAX 256
#define FT_PATH_MAX 512
#define FT_SAVE_PATH "./save_path/"

//4 char
#define FT_CHAT_KEY "shn"

int get_file_from_client(int conn_fd);
int send_file_to_server(int conn_fd, const char* file_name);
#endif
