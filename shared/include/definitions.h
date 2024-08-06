#define _XOPEN_SOURCE 500
#define PORT 4000
#define SERVER_SYNC_PORT 4001
#define NAME_SERVER_PORT 4002
#define USERNAME_MAX_SIZE 32
#define USER_INPUT_MAX_SIZE 256
#define FILE_NAME_MAX_SIZE 64
#define FILE_PATH_MAX_SIZE 249
#define USERNAME_READ_ERROR 1001
#define SESSION_LIMIT_ERROR 1002
#define SESSION_FINISHED 1003
#define USER_EXIT 1004
#define INOTIFY_ERROR 1005

struct sync_dir_listener_struct
{
    char dir_path[9 + USERNAME_MAX_SIZE + 1];
    int sockfd;
};