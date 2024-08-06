typedef struct server_list_node {
    int sockfd;
    struct server_list_node* next;
} SERVER_LIST_NODE;

void server_list_init();