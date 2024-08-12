#include "../../shared/include/communication.h"
#include "../../shared/include/definitions.h"


typedef struct server {
    int id;
    int sockfd;
    int is_coordinator;
    int port;
    char hostname[HOSTNAME_MAX_SIZE];
} SERVER;


typedef struct server_list_node {
    SERVER server;
    struct server_list_node* next;
} SERVER_LIST_NODE;


SERVER_LIST_NODE* server_list_init();
SERVER_LIST_NODE* server_list_insert(SERVER_LIST_NODE* server_list, SERVER server); // end of list
SERVER_LIST_NODE* server_list_remove(SERVER_LIST_NODE* server_list, int id);
SERVER server_list_get_server(SERVER_LIST_NODE* server_list, int id);

void server_list_print(SERVER_LIST_NODE* server_list);
void server_list_make_it_coordinator(SERVER_LIST_NODE* server_list, int id);
void server_list_make_it_backup(SERVER_LIST_NODE* server_list, int id);
void server_list_replicate_file(SERVER_LIST_NODE* server_list, int sockfd, FILE* fp, char file_name[FILE_NAME_MAX_SIZE + 1], unsigned int file_size, char username[USERNAME_MAX_SIZE + 1]);
