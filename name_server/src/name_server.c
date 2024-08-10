#include "../include/server_list.h"
#include <bits/pthreadtypes.h>
#include "../include/thread_list.h"


SERVER_LIST_NODE* server_list;
int available_server_id;
int available_port;
pthread_mutex_t server_lock;
THREAD_LIST* thread_list;


int socket_setup(int* sockfd, struct sockaddr_in* serv_addr, socklen_t *clilen)
{
    // Create sockets
    *sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (*sockfd == -1)
	{
        printf("ERROR opening socket\n");
		exit(1);
	}

	// Bind sockets
	serv_addr->sin_family = AF_INET;
	serv_addr->sin_port = htons(NAME_SERVER_PORT);
	serv_addr->sin_addr.s_addr = INADDR_ANY;
	bzero(&(serv_addr->sin_zero), 8);
  
	if (bind(*sockfd, (struct sockaddr *) serv_addr, sizeof(*serv_addr)) < 0)
	{
		printf("ERROR on binding socket\n");
		exit(1);
	}
	
	// Add listeners to sockets
	listen(*sockfd, 5);

	*clilen = sizeof(struct sockaddr_in);

    return 0;
}

void* server_thread(void* args)
{
    SERVER server;
    char buffer[MESSAGE_SIZE + 1];
    int val = 0;

    
    pthread_mutex_lock(&server_lock);
    server.id = available_server_id;
    available_server_id--;

    // The first server to connect is coordinator
    if(server.id == INT_MAX)
        server.is_coordinator = 1;
    else
        server.is_coordinator = 0;

    server.sockfd = *(int*) args;


    // Send server id
    itoa(server.id, buffer);
    printf("%s\n", buffer);
    send_msg(server.sockfd, buffer);

    // Send server port
    itoa(available_port, buffer);
    available_port += 2;
    send_msg(server.sockfd, buffer);

    // Get server hostname
    receive_msg(server.sockfd, buffer);
    strcpy(server.hostname, buffer);
    printf("Server hostname: %s\n", server.hostname);

    // Add server to server list
    server_list = server_list_insert(server_list, server);

    server_list_print(server_list);
    pthread_mutex_unlock(&server_lock);


    pthread_exit(&val);
}

int main(int argc, char *argv[])
{
	int sockfd, new_sockfd;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
    pthread_t current_thread;


    // Setup sockets
    socket_setup(&sockfd, &serv_addr, &clilen);

    // Init global variables
    available_server_id = INT_MAX;
    available_port = PORT;
    server_list = server_list_init();
    thread_list = create_thread_list();


	// Server waits for next connection
	while(1)
	{
		if ((new_sockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen)) == -1)
        {
			printf("ERROR on accepting socket\n");
			continue;
		}

        current_thread = get_last_thread(thread_list);
		thread_list = add_to_thread_list(thread_list);

        pthread_create(&current_thread, NULL, server_thread, &new_sockfd);
	}

	close(sockfd);
	return 0;
}
