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
void server_list_print(SERVER_LIST_NODE* server_list);
