#include "../../shared/include/communication.h"
#include "../../shared/include/definitions.h"
#include "../include/user.h"
#include "../include/thread_list.h"
#include <dirent.h>


UserList* user_list;
THREAD_LIST* thread_list;
pthread_mutex_t lock;

struct sync_struct
{
	int socket;
	char username[USERNAME_MAX_SIZE + 1];
};

typedef struct sockets
{
	int sockfd;
	int server_sync_sockfd;
} SOCKETS;

void handle_download(int newsockfd, char buffer[MESSAGE_SIZE + 1], char username[USERNAME_MAX_SIZE + 1])
{
	char file_name[FILE_NAME_MAX_SIZE + 1], file_path[FILE_PATH_MAX_SIZE + 1];
  	FILE *fp;
	unsigned int file_size;

	// Get file name
	receive_msg(newsockfd, buffer);
	strcpy(file_name, buffer);

	printf("User wants to download: %s\n", file_name);

	strcpy(file_path, "sync_dir_");
	strcat(file_path, username);
	strcat(file_path, "/");
	strcat(file_path, file_name);

	printf("Whole path: %s\n", file_path);

	fp = fopen(file_path, "rb");

	if(!fp)
	{
		strcpy(buffer, "0");
		printf("size: %s\n", buffer);
		send_msg(newsockfd, buffer); // Send file size
		return;
	}

	// Compute file dize
	fseek(fp, 0L, SEEK_END);
	file_size = ftell(fp);
	rewind(fp);
	itoa(file_size, buffer);

	printf("size: %s\n", buffer);
	send_msg(newsockfd, buffer); // Send file size

	send_file(newsockfd, fp, atoi(buffer)); // Send file data

	fclose(fp);
	return;
}

void get_sync_dir(int newsockfd, char sync_dir_path[9 + USERNAME_MAX_SIZE + 1])
{
	int number_of_files;
	DIR *dp;
	struct dirent *ep;
	char buffer[MESSAGE_SIZE + 1];
	char file_path[FILE_PATH_MAX_SIZE + 1]; 
	struct stat st = {0};
	FILE* fp;
	unsigned int file_size;

	
	// If user directory doesnt exist, create one
	if (stat(sync_dir_path, &st) == -1)
		mkdir(sync_dir_path, 0700);

	  
	dp = opendir(sync_dir_path);

	if(!dp)
	{
		perror ("Couldn't open the directory");
		return;
	}

	// Counts the number of files
	number_of_files = 0;
	while ((ep = readdir (dp)))
		if (strcmp(ep->d_name, ".") == 0 || strcmp(ep->d_name, "..") == 0)
		{
			// Do nothing 
		}
		else
		{
			number_of_files++;
			//stat(ep->d_name, &st);
			//printf("%s\n", ep->d_name);
		}

	// Sends the number of files
	itoa(number_of_files, buffer);
	send_msg(newsockfd, buffer);

	rewinddir(dp);

	// Send every file for synchronization
	while ((ep = readdir (dp)))
		if (strcmp(ep->d_name, ".") == 0 || strcmp(ep->d_name, "..") == 0)
		{
			// Do nothing 
		}
		else
		{
			strcpy(file_path, sync_dir_path);
			strcat(file_path, "/");
			strcat(file_path, ep->d_name);
			fp = fopen(file_path, "rb");
			printf("Opening: %s\n", file_path);

			// Sends file name
			strcpy(buffer, ep->d_name);
			send_msg(newsockfd, buffer);

			// Compute file size
			fseek(fp, 0L, SEEK_END);
			file_size = ftell(fp);
			rewind(fp);
			itoa(file_size, buffer);

			// Send file size
			send_msg(newsockfd, buffer);

			// Send file data
			send_file(newsockfd, fp, file_size);

			fclose(fp);
		}
}

void delete_propagation(char username[USERNAME_MAX_SIZE + 1], char file_path[FILE_PATH_MAX_SIZE + 1])
{
	User user;
    int i;
    char buffer[MESSAGE_SIZE + 1];

	pthread_mutex_lock(&lock);
	user = get_user(user_list, username);
	pthread_mutex_unlock(&lock);

    for(i = 0; i < user.sessions_amount; i++)
    {
        // Send synchronization type
        strcpy(buffer, "Delete");
        send_msg(user.all_sessions_sockets[i][1], buffer);

        // Send file name
        strcpy(buffer, file_path);
        printf("File path: %s\n", buffer);
        send_msg(user.all_sessions_sockets[i][1], buffer);
    }
}

void handle_delete(int newsockfd, char buffer[MESSAGE_SIZE + 1], char username[USERNAME_MAX_SIZE + 1])
{
    char file_name[FILE_NAME_MAX_SIZE + 1], file_path[FILE_PATH_MAX_SIZE + 1];
    
    receive_msg(newsockfd, buffer); // Get file name
    strcpy(file_name, buffer);

    // Create file path
    strcpy(file_path, "sync_dir_");
    strcat(file_path, username);
    strcat(file_path, "/");
    strcat(file_path, file_name);

    if(remove(file_path) == 0)
	{
        printf("File %s deleted\n", file_name);
		delete_propagation(username, file_path);
	}
    else
        printf("Could not delete %s\n", file_name);
}

void handle_upload(int newsockfd, User user, char sync_dir_path[9 + USERNAME_MAX_SIZE + 1])
{
	char buffer[MESSAGE_SIZE + 1], file_name[FILE_NAME_MAX_SIZE + 1], file_path[FILE_PATH_MAX_SIZE + 1];
	unsigned int file_size;
	FILE* fp;
    int i;

	printf("Inside: %d\n", user.sessions_amount);
	
	// Get file name
	receive_msg(newsockfd, buffer);
	strcpy(file_name, buffer);
	printf("File name: %s\n", file_name);

    // Create file path
	strcpy(file_path, sync_dir_path);
	strcat(file_path, "/");
	strcat(file_path, file_name);
	printf("File path: %s\n", file_path);

	fp = fopen(file_path, "wb");

	// Get file size
	receive_msg(newsockfd, buffer);
	file_size = atoi(buffer);
	printf("%d\n", file_size);

	// Get file data
	receive_file(newsockfd, fp, file_size);

    // Open file on "rb" mode
    fclose(fp);
    fp = fopen(file_path, "rb");

    // Upload propagation
    for(i = 0; i < user.sessions_amount; i++)
    {
        printf("User sessions: %d\n", user.sessions_amount);

        // Send synchronization type
        strcpy(buffer, "Upload");
        send_msg(user.all_sessions_sockets[i][1], buffer);

        // Send file size
        itoa(file_size, buffer);
        send_msg(user.all_sessions_sockets[i][1], buffer);

        // Send file name
		strcpy(buffer, file_name);
        send_msg(user.all_sessions_sockets[i][1], buffer);

        // Sends file data
        send_file(user.all_sessions_sockets[i][1], fp, file_size);

        rewind(fp);
    }

	fclose(fp);
}

void *user_thread(void *arg) {
	SOCKETS new_sockets;
	User th_user;
	int newsockfd, new_server_sync_sockfd, val;
	char th_buffer[MESSAGE_SIZE + 1];
	char sync_dir_path[9 + USERNAME_MAX_SIZE + 1];
	int th_return_value;


    new_sockets = *(SOCKETS*) arg;
	newsockfd = new_sockets.sockfd;
	new_server_sync_sockfd = new_sockets.server_sync_sockfd;
	
	// Read user name
	receive_msg(newsockfd, th_buffer);

	printf("User %s has logged in on socket %d and server sync socket: %d\n", th_buffer, new_sockets.sockfd, new_sockets.server_sync_sockfd);

	// Add user to user list
	pthread_mutex_lock(&lock);
	if (!search_user(user_list, th_buffer))
	{
		strcpy(th_user.username, th_buffer);
		printf("User %s has not been found online.\n", th_buffer);
		th_user.sessions_amount = 1;
		th_user.all_sessions_sockets[0][0] = new_sockets.sockfd;
		th_user.all_sessions_sockets[0][1] = new_sockets.server_sync_sockfd;
		user_list = insert_user(user_list, th_user);
		pthread_mutex_unlock(&lock);
	}
	// User is already logged in on another device
	else
	{
		printf("User %s has been found online.\n", th_buffer);
		th_user = get_user(user_list, th_buffer);

		if (th_user.sessions_amount == MAX_SESSION)
		{
			fprintf(stderr, "ERROR max user sessions reached\n");

			strcpy(th_buffer, "exit");
			send_msg(new_sockets.sockfd, th_buffer);

			close(new_sockets.sockfd);
			close(new_sockets.server_sync_sockfd);
			val = SESSION_LIMIT_ERROR;
			pthread_mutex_unlock(&lock);
			pthread_exit(&val);
		}
		increase_user_session(user_list, th_user.username, new_sockets.sockfd, new_sockets.server_sync_sockfd);
		th_user = get_user(user_list, th_user.username);
		pthread_mutex_unlock(&lock);
	}
	printf("Setting to index: %d\n", th_user.sessions_amount - 1);

	print_user_list(user_list);

    // Send ok to client
	strcpy(th_buffer, "Connected");
	send_msg(newsockfd, th_buffer);

	// Create and synchronize sync dir
	strcpy(sync_dir_path, "sync_dir_");
	strcat(sync_dir_path, th_user.username);
	get_sync_dir(newsockfd, sync_dir_path);

	while(1)
	{
		printf("Handling user thread: %s\n", th_user.username);

		// Read user request
		receive_msg(newsockfd, th_buffer);

		// Handle user request
		if (strstr(th_buffer, "upload")) // Handle upload
		{
			printf("User wants to upload\n");

			pthread_mutex_lock(&lock);
			th_user = get_user(user_list, th_user.username);
			handle_upload(newsockfd, th_user, sync_dir_path);
			pthread_mutex_unlock(&lock);
		}
		else if(strstr(th_buffer, "download")) // Handle download
		{
			printf("User wants to download\n");
			handle_download(newsockfd, th_buffer, th_user.username);
		}
		else if(strstr(th_buffer, "delete")) // Handle delete
		{
			printf("User wants to delete\n");
            handle_delete(newsockfd, th_buffer, th_user.username);
		}
		else if(strstr(th_buffer, "list_server")) // Handle list server
		{
			printf("User wants to list the server\n");
		}
		else if(strstr(th_buffer, "exit")) // Handle exit
		{
			printf("User wants to exit\n");

			pthread_mutex_lock(&lock);
			printf("User wants to exit: %s\n", th_user.username);
			decrease_user_session(user_list, th_user.username);
			th_user = get_user(user_list, th_user.username);

			if(th_user.sessions_amount == 0)
				remove_user(user_list, th_user.username);

			pthread_mutex_unlock(&lock);

			print_user_list(user_list);

			close(newsockfd);
			close(new_server_sync_sockfd);
			th_return_value = USER_EXIT;
			pthread_exit(&th_return_value);
		}
	}
}

int sockets_setup(int* sockfd, int* server_sync_sockfd, int* name_server_sockfd, struct sockaddr_in* serv_addr, struct sockaddr_in* serv_sync_addr, struct sockaddr_in* name_server_addr, socklen_t *clilen, struct hostent *name_server, char* argv)
{
    // If server doesnt exist, end client    
    name_server = gethostbyname(argv);
    if(name_server == NULL)
    {
        fprintf(stderr, "ERROR, no such host\n");
        exit(1);
    }
    
    // Create sockets
    *sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (*sockfd == -1)
	{
        printf("ERROR opening socket\n");
		exit(1);
	}

    *server_sync_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if ((*server_sync_sockfd) == -1)
	{
        printf("ERROR opening server sync socket\n");
		exit(1);
	}

    *name_server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (*name_server_sockfd == -1)
	{
        printf("ERROR opening name server socket\n");
		exit(1);
	}    

	// Bind sockets
	serv_addr->sin_family = AF_INET;
	serv_addr->sin_port = htons(PORT);
	serv_addr->sin_addr.s_addr = INADDR_ANY;
	bzero(&(serv_addr->sin_zero), 8);

    serv_sync_addr->sin_family = AF_INET;
	serv_sync_addr->sin_port = htons(SERVER_SYNC_PORT);
	serv_sync_addr->sin_addr.s_addr = INADDR_ANY;
	bzero(&(serv_sync_addr->sin_zero), 8);

    name_server_addr->sin_family = AF_INET;  
    name_server_addr->sin_port = htons(NAME_SERVER_PORT);
    name_server_addr->sin_addr = *((struct in_addr *)name_server->h_addr_list[0]);
    bzero(&(name_server_addr->sin_zero), 8);
  
	if (bind(*sockfd, (struct sockaddr *) serv_addr, sizeof(*serv_addr)) < 0)
	{
		printf("ERROR on binding socket\n");
        perror(": ");
		exit(1);
	}

    if (bind(*server_sync_sockfd, (struct sockaddr *) serv_sync_addr, sizeof(*serv_sync_addr)) < 0)
	{
		printf("ERROR on binding server sync socket\n");
		exit(1);
	}
	
	// Add listeners to sockets
	listen(*sockfd, 5);
    listen(*server_sync_sockfd, 5);

	*clilen = sizeof(struct sockaddr_in);
    
    // Connect to name server
    if (connect(*name_server_sockfd, (struct sockaddr *) name_server_addr, sizeof(*name_server_addr)) < 0)
    {
        fprintf(stderr, "ERROR connecting to name server\n");
        exit(1);
    }

/*
*/
    return 0;
}

int main(int argc, char *argv[])
{
	int sockfd, server_sync_sockfd, name_server_sockfd, first_socket_ok, second_socket_ok;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr, serv_sync_addr, name_server_addr;
	pthread_t current_thread;
	User* user;
	SOCKETS new_sockets;
    struct hostent *name_server = NULL;


    // If arguments are wrong, end server
    if(argc != 2)
    {
        fprintf(stderr, "usage %s name_server\n", argv[0]);
        return 1;
    }

    // Setup sockets
    sockets_setup(&sockfd, &server_sync_sockfd, &name_server_sockfd,
          &serv_addr, &serv_sync_addr, &name_server_addr,
        &clilen, name_server, argv[1]);

    // Init global variables
	user_list = init();
	thread_list = create_thread_list();

	// Server waits for next connection
	while(1)
	{
		user = (User*) malloc(sizeof(User));
		first_socket_ok = 1;
		second_socket_ok = 1;

		if ((new_sockets.sockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen)) == -1) {
			printf("ERROR on accepting socket\n");
			first_socket_ok = 0;
		}

		if ((new_sockets.server_sync_sockfd = accept(server_sync_sockfd, (struct sockaddr *) &cli_addr, &clilen)) == -1) {
			printf("ERROR on accepting server sync socket\n");
			second_socket_ok = 0;
		}

		// if both sockets were accepted, create the user thread
		if(first_socket_ok && second_socket_ok)
		{
			user->sockets[0] = new_sockets.sockfd;
			user->sockets[1] = new_sockets.server_sync_sockfd;
			user->sync_needed = 0;
	
			current_thread = get_last_thread(thread_list);
			thread_list = add_to_thread_list(thread_list);

			pthread_create(&current_thread, NULL, user_thread, &new_sockets);
		}
		else
			free(user);
	}

	close(sockfd);
	close(server_sync_sockfd);
	return 0;
}
