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

void send_all_sync_dir_replication(int name_server_sockfd)
{
	DIR* dp, *current_dir;
	int number_of_files, number_of_sync_dirs;
	struct dirent *ep, *ep_current;
    unsigned int file_size;
    char buffer[MESSAGE_SIZE + 1], file_path[FILE_PATH_MAX_SIZE + 1];
    FILE* fp;


	dp = opendir("./");
	number_of_sync_dirs = 0;

	// Count number of sync dirs
	while((ep = readdir(dp)))
	{
		if (strcmp(ep->d_name, ".") == 0 || strcmp(ep->d_name, "..") == 0 || (ep->d_type != DT_DIR) || !strstr(ep->d_name, "sync_dir"))
		{
			// Do nothing
		}
		else
		{
			number_of_sync_dirs++;
		}
	}

	rewinddir(dp);

    // Send number of sync dirs for replication
    itoa(number_of_sync_dirs, buffer);
    send_msg(name_server_sockfd, buffer);

	while((ep = readdir(dp)))
	{
		if (strcmp(ep->d_name, ".") == 0 || strcmp(ep->d_name, "..") == 0 || (ep->d_type != DT_DIR) || !strstr(ep->d_name, "sync_dir"))
		{
			// Do nothing 
		}
		else
		{
			// Send sync dir name for replication
			strcpy(buffer, ep->d_name);
			send_msg(name_server_sockfd, buffer);
			
			current_dir = opendir(ep->d_name);

            number_of_files = 0;

			while((ep_current = readdir(current_dir)))
			{
				if (strcmp(ep_current->d_name, ".") == 0 || strcmp(ep_current->d_name, "..") == 0)
				{
					// Do nothing 
				}
				else
				{
					number_of_files++;
				}
			}

			printf("Directory %s has %d files.\n", ep->d_name, number_of_files);

	        rewinddir(current_dir);

            // Send number of files
            itoa(number_of_files, buffer);
            send_msg(name_server_sockfd, buffer);

            while((ep_current = readdir(current_dir)))
            {
                if (strcmp(ep_current->d_name, ".") == 0 || strcmp(ep_current->d_name, "..") == 0)
				{
					// Do nothing 
				}
                else
                {               
                    printf("Name: %s\n", ep_current->d_name);

                    // File path
                    strcpy(file_path, ep->d_name);
                    strcat(file_path, "/");
                    strcat(file_path, ep_current->d_name);

                    fp = fopen(file_path /*ep_current->d_name*/, "rb");

                    // Send file name for replication
                    strcpy(buffer, ep_current->d_name);
                    send_msg(name_server_sockfd, buffer);

                    // Send file size for replication
                    fseek(fp, 0, SEEK_END);
                    file_size = ftell(fp);
                    rewind(fp);
                    itoa(file_size, buffer);
                    send_msg(name_server_sockfd, buffer);

					// Send file data for replication
                    send_file(name_server_sockfd, fp, file_size);

                    fclose(fp);
                }
            }
		}
	}
}

void* server_thread(void* args)
{
    SERVER server;
    char buffer[MESSAGE_SIZE + 1], file_name[FILE_NAME_MAX_SIZE + 1], username[USERNAME_MAX_SIZE + 1], sync_dir_path[9 + USERNAME_MAX_SIZE + 1], file_path[FILE_PATH_MAX_SIZE + 1];
    int number_of_files, number_of_sync_dirs, i, j, val = 0;
    FILE* fp;
    unsigned int file_size;
    struct stat st;

    
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
        //new_coordinator = 1;
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

    // Get sync dir for replication (from coordenator)
    if(server.is_coordinator)
    {
        // Get number of sync dirs for replication
        receive_msg(server.sockfd, buffer);
        number_of_sync_dirs = atoi(buffer);

        printf("Number of dirs: %d\n", number_of_sync_dirs);

        for(i = 0; i < number_of_sync_dirs; i++)
        {
            // Get sync dir name for replication
            receive_msg(server.sockfd, buffer);
            strcpy(sync_dir_path, buffer);

            printf("Dir name: %s\n", sync_dir_path);

            // Create sync dir
            mkdir(sync_dir_path, 0700);

            // Get number of files for replication
            receive_msg(server.sockfd, buffer);
            number_of_files = atoi(buffer);

            for(j = 0; j < number_of_files; j++)
            {
                // Get file name for replication
                receive_msg(server.sockfd, buffer);
                strcpy(file_name, buffer);

                strcpy(file_path, sync_dir_path);
                strcat(file_path, "/");
                strcat(file_path, file_name);

                fp = fopen(file_path, "wb");

                // Get file size for replication
                receive_msg(server.sockfd, buffer);
                file_size = atoi(buffer);

                // Get file data for replication
                receive_file(server.sockfd, fp, file_size);

                fclose(fp);
            }
        }
    }
    // Send sync dir for replication (to backup)
    else 
    {
        printf("Sending dir to backup\n");
        send_all_sync_dir_replication(server.sockfd);
    }

    // Wait for server request
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

                // Make sure sync_dir exists
                strcpy(file_path, "sync_dir_");
                strcat(file_path, username);
                if(stat(file_path, &st))
                    mkdir(file_path, 0700);
                
                // Get file name for replication
                receive_msg(server.sockfd, buffer);
                strcpy(file_name, buffer);
                printf("File name: %s\n", file_name);

                // Open file
                strcat(file_path, "/");
                strcat(file_path, file_name);              
                fp = fopen(file_path, "wb");

                // Get file size for replication
                receive_msg(server.sockfd, buffer);
                file_size = atoi(buffer);

                receive_file(server.sockfd, fp, file_size);

                fclose(fp);

                fp = fopen(file_path, "rb");

                server_list_replicate_file(server_list, fp, file_name, file_size, username);

                fclose(fp);
            }

            else if(strstr(buffer, "Delete"))
            {
                // Get file path for delete replication
                receive_msg(server.sockfd, buffer);
                strcpy(file_path, buffer);
                
                remove(file_path);

                server_list_replicate_delete_file(server_list, file_path);
            }

            else if(strstr(buffer, "New sync dir"))
            {
                // Get new sync dir path
                receive_msg(server.sockfd, buffer);
                strcpy(sync_dir_path, buffer);

                mkdir(sync_dir_path, 0700);

                server_list_replicate_new_sync_dir(server_list, sync_dir_path);
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

    // Get connection for the first time

    // Send coordinator hostname
    strcpy(buffer, coordinator_server.hostname);
    send_msg(new_sockfd, buffer);

    // Send coordinator server port
    itoa(coordinator_server.port, buffer);
    send_msg(new_sockfd, buffer);

    while(1)
    {
        //pthread_mutex_lock(&server_lock);
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
        //pthread_mutex_unlock(&server_lock);
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

    printf("estou aqui\n");

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
        printf("estou aqui 2\n");
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
