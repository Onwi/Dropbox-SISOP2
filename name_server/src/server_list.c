#include "../include/server_list.h"


SERVER_LIST_NODE* server_list_init()
{
    return NULL;
}

SERVER_LIST_NODE* server_list_insert(SERVER_LIST_NODE* server_list, SERVER server)
{
    SERVER_LIST_NODE *newNode, *auxNode;	
	
	newNode = (SERVER_LIST_NODE *) malloc(sizeof(SERVER_LIST_NODE));
	newNode->server = server;
	newNode->next = NULL;

	if (server_list)
    {
		for (auxNode = server_list; auxNode->next; auxNode = auxNode->next);
		auxNode->next = newNode;
	} else
    {
		server_list = newNode;
	}

	return server_list;
}

SERVER_LIST_NODE* server_list_remove(SERVER_LIST_NODE* server_list, int id)
{
    SERVER_LIST_NODE* aux, *aux_prev;

	aux = server_list;
	aux_prev = NULL;
	
	// empty list
	if(!server_list)
		return NULL;

	// delete first user
	if(server_list->server.id == id)
		return server_list->next;


	while(aux->next)
	{
		if( aux->server.id == id )
		{
			// delete
			aux_prev->next = aux->next;
			//free(aux);
		}

		aux_prev = aux;
		aux = aux->next;
	}

	return server_list;
}

void server_list_print(SERVER_LIST_NODE* server_list)
{	
	SERVER_LIST_NODE* auxNode;

    printf("All servers connected\n");
	
	auxNode = server_list;

    while (auxNode)
    {
		printf("Server id: %u\n", auxNode->server.id);
		printf("Server sockfd: %d\n", auxNode->server.sockfd);
        printf("Is coordinator: %d\n", auxNode->server.is_coordinator);
        printf("Server port: %d\n", auxNode->server.port);
        printf("Server hostname: %s\n", auxNode->server.hostname);
		
		auxNode = auxNode->next;
	}
}