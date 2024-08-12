#include "../include/server_list.h"
#include <bits/pthreadtypes.h>
#include "../include/thread_list.h"


SERVER_LIST_NODE* server_list;
SERVER coordinator_server;
int available_server_id;
int available_port;
int new_coordinator;
pthread_mutex_t server_lock;
THREAD_LIST* thread_list;
THREAD_LIST* frontend_thread_list;


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

int frontend_socket_setup(int* sockfd, struct sockaddr_in* serv_addr, socklen_t *clilen)
{
    // Create sockets
    *sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (*sockfd == -1)
	{
        printf("ERROR opening frontend socket\n");
		exit(1);
	}

	// Bind sockets
	serv_addr->sin_family = AF_INET;
	serv_addr->sin_port = htons(FRONTEND_NAME_SERVER_PORT);
	serv_addr->sin_addr.s_addr = INADDR_ANY;
	bzero(&(serv_addr->sin_zero), 8);
  
	if (bind(*sockfd, (struct sockaddr *) serv_addr, sizeof(*serv_addr)) < 0)
	{
		printf("ERROR on binding frontend socket\n");
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
    char buffer[MESSAGE_SIZE + 1], file_name[FILE_NAME_MAX_SIZE + 1];
    int val = 0;
    FILE* fp;
    unsigned int file_size;
    char username[USERNAME_MAX_SIZE + 1];

    
    pthread_mutex_lock(&server_lock);
    server.id = available_server_id;
    available_server_id--; 
    server.sockfd = *(int*) args;

    // Send server id
    itoa(server.id, buffer);
    printf("%s\n", buffer);
    send_msg(server.sockfd, buffer);

    // Send server port
    server.port = available_port;
    itoa(available_port, buffer);
    available_port += 2;
    send_msg(server.sockfd, buffer);

    // Get server hostname
    receive_msg(server.sockfd, buffer);
    strcpy(server.hostname, buffer);
    printf("Server hostname: %s\n", server.hostname);

    // Add server to server list
    server_list = server_list_insert(server_list, server);

    // The first server to connect is coordinator
    if(server.id == INT_MAX)
    {
        server_list_make_it_coordinator(server_list, server.id);
        new_coordinator = 1;
        coordinator_server = server;
    }
    else
    {
        server_list_make_it_backup(server_list, server.id);
    }

    server = server_list_get_server(server_list, server.id);

    printf("local server:\nServer id: %d\nServer socket: %d\nServer is coordinator: %d\nServer port: %d\nServer hostname: %s\n", server.id, server.sockfd, server.is_coordinator, server.port, server.hostname);

    server_list_print(server_list);
    pthread_mutex_unlock(&server_lock);

    while(1)
    {
        if(server.is_coordinator)
        {
            // Get replication request
            receive_msg(server.sockfd, buffer);

            if(strstr(buffer, "Upload"))
            {
                // Get username for replication
                receive_msg(server.sockfd, buffer);
                strcpy(username, buffer);
                
                // Get file name for replication
                receive_msg(server.sockfd, buffer);
                strcpy(file_name, buffer);
                printf("File name: %s\n", file_name);
                
                fp = fopen(file_name, "wb");

                // Get file size for replication
                receive_msg(server.sockfd, buffer);
                file_size = atoi(buffer);

                receive_file(server.sockfd, fp, file_size);

                fclose(fp);

                fp = fopen(file_name, "rb");

                server_list_replicate_file(server_list, server.sockfd, fp, file_name, file_size, username);
            }
        }
    }

    pthread_exit(&val);
}

void* frontend_thread(void* args)
{
    int val;
    int new_sockfd = *(int*) args;
    char buffer[MESSAGE_SIZE + 1];

    while(1)
    {
        pthread_mutex_lock(&server_lock);
        if(new_coordinator)
        {
            // Send coordinator hostname
            strcpy(buffer, coordinator_server.hostname);
            send_msg(new_sockfd, buffer);

            // Send coordinator server port
            itoa(coordinator_server.port, buffer);
            send_msg(new_sockfd, buffer);

            new_coordinator = 0;
        }

        sleep(1);
        pthread_mutex_unlock(&server_lock);
    }

    val = 0;
    pthread_exit(&val);
}

void* frontend_handler(void* args)
{
    int sockfd, new_sockfd, val;
    socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
    pthread_t frontend_thread_descriptor;

    // Setup sockets
    frontend_socket_setup(&sockfd, &serv_addr, &clilen);

    // Name server waits for next frontend connection
	while(1)
	{
		if ((new_sockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen)) == -1)
        {
			printf("ERROR on accepting socket\n");
			continue;
		}

        // Create thread for frontends connection
        frontend_thread_descriptor = get_last_thread(frontend_thread_list);
        frontend_thread_list = add_to_thread_list(frontend_thread_list);

        pthread_create(&frontend_thread_descriptor, NULL, frontend_thread, &new_sockfd);
	}

	close(sockfd);

    val = 0;
    pthread_exit(&val);
}

int main(int argc, char *argv[])
{
	int sockfd, new_sockfd;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
    pthread_t current_thread, frontend_handler_thread;


    // Setup server sockets
    socket_setup(&sockfd, &serv_addr, &clilen);

    // Init global variables
    available_server_id = INT_MAX;
    available_port = PORT;
    new_coordinator = 0;
    server_list = server_list_init();
    thread_list = create_thread_list();
    frontend_thread_list = create_thread_list();

    // Thread that handles client connections
    pthread_create(&frontend_handler_thread, NULL, frontend_handler, NULL);

	// Name server waits for next server connection
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
