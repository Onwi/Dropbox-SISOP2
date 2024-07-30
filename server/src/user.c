#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "../include/user.h"

// linked list operations
UserList *init() {
	return NULL;
}

// insert node to end of list
UserList *insert_user(UserList *list, User user) {
	UserList *newNode, *auxNode;	
	
	newNode = (UserList *)malloc(sizeof(UserList));
	newNode->user = user;
	newNode->next = NULL;

	if (list) {
		for (auxNode = list; auxNode->next; auxNode = auxNode->next);
		auxNode->next = newNode;
	} else {
		list = newNode;
	}
	return list;
}

UserList *remove_user(UserList *list, char* username)
{
	UserList* aux;
	UserList* aux_prev;

	aux = list;
	aux_prev = NULL;
	
	// empty list
	if(!list)
		return NULL;

	// delete first user
	if( strcmp(list->user.username, username) == 0 )
		return NULL;


	while(aux->next)
	{
		if( strcmp(aux->user.username, username) == 0 )
		{
			// delete
			aux_prev->next = aux->next;
			//free(aux);
		}

		aux_prev = aux;
		aux = aux->next;
	}

	return NULL;
}

UserList *free_list(UserList *list) {
	if (!list) return NULL;

	UserList *auxNode;
	while (list) {
		auxNode = list;
		list = list->next;
		free(auxNode);	
	}	
	free(list);
	return NULL;
}

User get_user(UserList *list, char* username)
{
	UserList* aux = list;

	while(aux)
	{
		if(strcmp( aux->user.username, username) == 0)
			return (aux->user);

		aux = aux->next;
	}

	return list->user;
}

void increase_user_session(UserList *list, char* username, int sockfd, int server_sync_sockfd)
{
	if (!list) return;
	 
	UserList *auxNode;
	for (auxNode = list; auxNode; auxNode = auxNode->next) {
		if (strcmp(username, auxNode->user.username) == 0) {
			auxNode->user.sessions_amount = auxNode->user.sessions_amount + 1;
			auxNode->user.all_sessions_sockets[auxNode->user.sessions_amount - 1][0] = sockfd;
			auxNode->user.all_sessions_sockets[auxNode->user.sessions_amount - 1][1] = server_sync_sockfd;
		}
	}
}

void decrease_user_session(UserList *list, char* username)
{
	if (!list) return;
	 
	UserList *auxNode;
	for (auxNode = list; auxNode; auxNode = auxNode->next) {
		if (strcmp(username, auxNode->user.username) == 0) {
			auxNode->user.sessions_amount = auxNode->user.sessions_amount - 1;
		}
	}
}

void set_user_sync_notification(UserList *list, char* username, char file_name_sync[256])
{
	if (!list) return;
	 
	UserList *auxNode;
	for (auxNode = list; auxNode; auxNode = auxNode->next) {
		if (strcmp(username, auxNode->user.username) == 0) {
			auxNode->user.sync_needed = 1;
			strcpy(auxNode->user.file_name_sync, file_name_sync);
		}
	}
}

void turn_off_user_sync_notification(UserList *list, char* username)
{
	if (!list) return;
	 
	UserList *auxNode;
	for (auxNode = list; auxNode; auxNode = auxNode->next) {
		if (strcmp(username, auxNode->user.username) == 0) {
			auxNode->user.sync_needed = 0;
		}
	}
}

void turn_on_user_deletion_notification(UserList *list, char* username)
{
	if (!list) return;
	 
	UserList *auxNode;
	for (auxNode = list; auxNode; auxNode = auxNode->next) {
		if (strcmp(username, auxNode->user.username) == 0) {
			auxNode->user.deletion_needed = 1;
		}
	}
}

void turn_off_user_deletion_notification(UserList *list, char* username)
{
	if (!list) return;
	 
	UserList *auxNode;
	for (auxNode = list; auxNode; auxNode = auxNode->next) {
		if (strcmp(username, auxNode->user.username) == 0) {
			auxNode->user.deletion_needed = 0;
		}
	}
}

void set_all_sessions_sockets(UserList *list, char* username, int newsockfd, int new_server_sync_sockfd, int i)
{
	if (!list) return;
	 
	UserList *auxNode;
	for (auxNode = list; auxNode; auxNode = auxNode->next) {
		if (strcmp(username, auxNode->user.username) == 0) {
			auxNode->user.all_sessions_sockets[i][0] = newsockfd;
			auxNode->user.all_sessions_sockets[i][1] = new_server_sync_sockfd;
		}
	}
}

bool search_user(UserList *list, char* username) {
	if (!list) return false;
	 
	UserList *auxNode;
	for (auxNode = list; auxNode; auxNode = auxNode->next) {
		if (strcmp(username, auxNode->user.username) == 0) {
			return true;
		}
	}
	return false;
}

void print_user_list(UserList *list) {
	int i;
	
	printf("all users connected to server\n");
	
	UserList *auxNode = list;
    while (auxNode) {
		printf("username: %s\n", auxNode->user.username);
		printf("Sessions: %d\n", auxNode->user.sessions_amount);
		
		for(i = 0; i < auxNode->user.sessions_amount; i++)
			printf("Al sockets: %d and %d\n", auxNode->user.all_sessions_sockets[i][0], auxNode->user.all_sessions_sockets[i][1]);
		auxNode = auxNode->next;
	}
}
