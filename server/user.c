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

User *get_user(UserList *list, char* username) {
	UserList* aux = list;

	while(aux) {
		if(strcmp(aux->user.username, username) == 0 )
			return &(aux->user);

		aux = aux->next;
	}
	return NULL;
}

void increase_user_session(UserList *list, char* username)
{
	if (!list) return;
	 
	UserList *auxNode;
	for (auxNode = list; auxNode; auxNode = auxNode->next) {
		if (strcmp(username, auxNode->user.username) == 0) {
			auxNode->user.sessions_amount = auxNode->user.sessions_amount + 1;
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
	printf("all users connected to server\n");
	
	UserList *auxNode = list;
    while (auxNode) {
		printf("username: %s\n", auxNode->user.username);
		printf("Sessions: %d\n", auxNode->user.sessions_amount);
		auxNode = auxNode->next;
	}
}
