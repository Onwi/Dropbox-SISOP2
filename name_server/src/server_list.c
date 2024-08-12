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

SERVER server_list_get_server(SERVER_LIST_NODE* server_list, int id)
{
    SERVER_LIST_NODE* aux;

    for(aux = server_list; (!aux) || (aux->server.id != id); aux = aux->next);

    return aux->server;
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
        printf("\n");
		
		auxNode = auxNode->next;
	}
}

void server_list_make_it_coordinator(SERVER_LIST_NODE* server_list, int id)
{
    SERVER_LIST_NODE* auxNode;

    // Try to find server with given id
    for(auxNode = server_list; (!auxNode) || (auxNode->server.id != id); auxNode = auxNode->next);

    // If found, make it coordinator
    if(auxNode)
        auxNode->server.is_coordinator = 1;
}

void server_list_make_it_backup(SERVER_LIST_NODE* server_list, int id)
{
    SERVER_LIST_NODE* auxNode;

    // Try to find server with given id
    for(auxNode = server_list; (!auxNode) || (auxNode->server.id != id); auxNode = auxNode->next);

    // If found, make it coordinator
    if(auxNode)
        auxNode->server.is_coordinator = 0;
}

void server_list_replicate_file(SERVER_LIST_NODE* server_list, int sockfd, FILE* fp, char file_name[FILE_NAME_MAX_SIZE + 1], unsigned int file_size, char username[USERNAME_MAX_SIZE + 1])
{
    SERVER_LIST_NODE* aux;
    char buffer[MESSAGE_SIZE + 1];

    for(aux = server_list; aux; aux = aux->next)
    {
        // If server is backup, replicate file
        if(aux->server.is_coordinator == 0)
        {
            // Send username for replication
            strcpy(buffer, username);
            send_msg(aux->server.sockfd, buffer);
            
            // Send file name for replication
            strcpy(buffer, file_name);
            send_msg(aux->server.sockfd, buffer);

            // Send file size for replication
            itoa(file_size, buffer);
            send_msg(aux->server.sockfd, buffer);

            // Send file data for replication
            send_file(aux->server.sockfd, fp, file_size);
        }
    }
}
