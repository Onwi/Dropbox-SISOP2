#include "../include/user.h"

typedef struct infos {
	int socket;
	UserList *user_list;
}Infos;

int getCommandFromUser(int client_socket);
void *handle_client(void *arg);
