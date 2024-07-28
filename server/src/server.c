#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/stat.h>
#include <dirent.h>

#include "../include/user.h"
#include "../include/thread_list.h"
#include "../../shared/include/communication.h"

#define PORT 4000
#define SERVER_SYNC_PORT 4001
#define USERNAME_READ_ERROR 1001
#define SESSION_LIMIT_ERROR 1002
#define SESSION_FINISHED 1003
#define USER_EXIT 1004
#define USERNAME_MAX_SIZE 32


UserList* user_list;
THREAD_LIST* thread_list;
pthread_mutex_t lock;

struct sync_struct
{
	int socket;
	char username[USERNAME_MAX_SIZE];
};

void handle_sync_download(int new_server_sync_sockfd, char file_name[256], char username[USERNAME_MAX_SIZE])
{
	char file_path[256];
	char buffer[MESSAGE_SIZE];
  	FILE *fp;
	unsigned int file_size;

	printf("Sync file: %s\n", file_name);

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
		send_msg(new_server_sync_sockfd, buffer); // Sends file size = "0"
		return;
	}

	// Compute file dize
	fseek(fp, 0L, SEEK_END);
	file_size = ftell(fp);
	rewind(fp);
	itoa(file_size, buffer);

	printf("size: %s\n", buffer);

	send_msg(new_server_sync_sockfd, buffer); // Sends file size
	send_msg(new_server_sync_sockfd, file_name); // Sends file name
	send_file(new_server_sync_sockfd, fp, atoi(buffer)); // Sends file data

	fclose(fp);
	return;
}

void *server_sync_handler(void *arg)
{
	struct sync_struct my_sync_struct  = *(struct sync_struct *) arg;
	int th_sync_needed;
	char synch_buffer[MESSAGE_SIZE];
	int new_server_sync_sockfd;
	char username[USERNAME_MAX_SIZE];
	User user;
	int i;

	bzero(synch_buffer, MESSAGE_SIZE);

	new_server_sync_sockfd = my_sync_struct.socket;
	strcpy(username, my_sync_struct.username);

	while(1)
	{
		pthread_mutex_lock(&lock);
		user = get_user(user_list, username);
		th_sync_needed = user.sync_needed;
		pthread_mutex_unlock(&lock);

		if(th_sync_needed)
		{
			printf("Need synch: %s\n", user.file_name_sync);			

			pthread_mutex_lock(&lock);
			for(i = 0; i < user.sessions_amount; i++)
				handle_sync_download(user.all_sessions_sockets[i][1], user.file_name_sync, username);

			turn_off_user_sync_notification(user_list, username);
			pthread_mutex_unlock(&lock);
		}
	}
	
	close(new_server_sync_sockfd);
	int val = SESSION_FINISHED;
	pthread_exit(&val);
}


void handle_download(int newsockfd, char buffer[MESSAGE_SIZE], char username[USERNAME_MAX_SIZE])
{
	char file_name[256], file_path[256];
  	FILE *fp;
	long int file_size;

	receive_msg(newsockfd, file_name); // Gets file name

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
		send_msg(newsockfd, buffer); // Sends file size
		return;
	}

	// Compute file dize
	fseek(fp, 0L, SEEK_END);
	file_size = ftell(fp);
	rewind(fp);
	itoa(file_size, buffer);

	printf("size: %s\n", buffer);
	send_msg(newsockfd, buffer); // Sends file size

	send_file(newsockfd, fp, atoi(buffer)); // Sends file data

	fclose(fp);
	return;
}

void get_sync_dir(int newsockfd, char sync_dir_path[8 + USERNAME_MAX_SIZE])
{
	int number_of_files;
	DIR *dp;
	struct dirent *ep;
	char buffer[MESSAGE_SIZE];
	char file_path[256]; 
	struct stat st = {0};
	FILE* fp;
	int file_size;

	
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

void handle_delete(int newsockfd, char buffer[MESSAGE_SIZE], char username[USERNAME_MAX_SIZE])
{
    char file_name[256], file_path[256];
    
    receive_msg(newsockfd, buffer); // Get file name
    strcpy(file_name, buffer);

    // Create file path
    strcpy(file_path, "sync_dir_");
    strcat(file_path, username);
    strcat(file_path, "/");
    strcat(file_path, file_name);

    if(remove(file_path) == 0)
        printf("File %s deleted\n", file_name);
    else
    {
        printf("Could not delete %s\n", file_name);
        perror(" ");
    }
}

void *user_thread(void *arg) {
	User th_user  = *(User *) arg;
	int newsockfd, new_server_sync_sockfd, val, file_size;
	char th_buffer[MESSAGE_SIZE];
	char th_synch_buffer[MESSAGE_SIZE];	
	char sync_dir_path[8 + USERNAME_MAX_SIZE];
	char file_path[256];
	char file_name[256];
	int th_return_value;
	FILE *fp;
	pthread_t server_sync_thread;
	struct sync_struct my_sync_struct;


	printf("Handling user thread: %s\n", th_user.username);
	bzero(th_buffer, MESSAGE_SIZE);
	bzero(th_synch_buffer, MESSAGE_SIZE);

	newsockfd = th_user.sockets[0];
	new_server_sync_sockfd = th_user.sockets[1];

	// Read user name
	receive_msg(newsockfd, th_buffer);
	receive_msg(new_server_sync_sockfd, th_synch_buffer);

	printf("Dentro da thread do cliente\n");
	printf("User %s has logged in on socket %d and server sync socket: %d\n", th_buffer, newsockfd, new_server_sync_sockfd);

	/* add user to user list */
	pthread_mutex_lock(&lock);
	if (!search_user(user_list, th_buffer))
	{
		strcpy(th_user.username, th_buffer);
		printf("User %s has not been found online.\n", th_buffer);
		th_user.sessions_amount = 1;
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
			send_msg(newsockfd, th_buffer);

			close(newsockfd);
			close(new_server_sync_sockfd);
			val = SESSION_LIMIT_ERROR;
			pthread_mutex_unlock(&lock);
			pthread_exit(&val);
		}
 
		increase_user_session(user_list, th_user.username);
		pthread_mutex_unlock(&lock);
	}
	printf("Setting to index: %d\n", th_user.sessions_amount - 1);

	pthread_mutex_lock(&lock);
	th_user = get_user(user_list, th_buffer);
	set_all_sessions_sockets(user_list, th_user.username, newsockfd, new_server_sync_sockfd, th_user.sessions_amount - 1);
	pthread_mutex_unlock(&lock);

	print_user_list(user_list);

	strcpy(th_buffer, "Connected");
	send_msg(newsockfd, th_buffer); // Send ok to client

	// Create and synchronize sync dir
	strcpy(sync_dir_path, "sync_dir_");
	strcat(sync_dir_path, th_user.username);
	get_sync_dir(newsockfd, sync_dir_path);

	// Create Sync thread
	my_sync_struct.socket = new_server_sync_sockfd;
	strcpy(my_sync_struct.username, th_user.username);
	pthread_create(&server_sync_thread, NULL, server_sync_handler, &my_sync_struct);

	while (1)
	{
		printf("Handling user thread: %s\n", th_user.username);

		// Read user request
		receive_msg(newsockfd, th_buffer);

		// Handle upload
		if (strstr(th_buffer, "upload"))
		{
			printf("User wants to upload\n");

			// Get file name
			receive_msg(newsockfd, th_buffer);
			strcpy(file_name, th_buffer);
			printf("File name: %s\n", file_name);

			strcpy(file_path, sync_dir_path);
			strcat(file_path, "/");
			strcat(file_path, file_name);
			printf("File path: %s\n", file_path);

			fp = fopen(file_path, "wb");

			// Get file size
			receive_msg(newsockfd, th_buffer);
			file_size = atoi(th_buffer);
			printf("%d\n", file_size);

			// Get file data
			receive_file(newsockfd, fp, file_size);

			// Get name for sync notification
			receive_msg(newsockfd, th_buffer);

			pthread_mutex_lock(&lock);
			set_user_sync_notification(user_list, th_buffer, file_name);
			pthread_mutex_unlock(&lock);

			fclose(fp);
		}
		else if(strstr(th_buffer, "download"))
		{
			printf("User wants to download\n");
			handle_download(newsockfd, th_buffer, th_user.username);
		}
		else if(strstr(th_buffer, "delete"))
		{
			printf("User wants to delete\n");
            handle_delete(newsockfd, th_buffer, th_user.username);
		}
		else if(strstr(th_buffer, "list_server"))
		{
			printf("User wants to list the server\n");
		}
		else if(strstr(th_buffer, "list_client"))
		{
			printf("User wants to list the client\n");
		}
		else if(strstr(th_buffer, "get_sync_dir"))
		{
			printf("User wants to get the sync dir\n");
		}
		else if(strstr(th_buffer, "exit"))
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

			//close(newsockfd);
			//close(new_server_sync_sockfd);
			th_return_value = USER_EXIT;
			pthread_exit(&th_return_value);
		}
	}
}

int main(int argc, char *argv[])
{
	int sockfd, newsockfd;
	int server_sync_sockfd, new_server_sync_sockfd;
	int first_socket_ok, second_socket_ok;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	struct sockaddr_in serv_sync_addr;
	pthread_t current_thread;
	User* user;


	user_list = init();
	thread_list = create_thread_list();

	// sockets creation
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
        printf("ERROR opening socket\n");
		exit(1);
	}

	if ((server_sync_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
        printf("ERROR opening server sync socket\n");
		exit(1);
	}

	// sockets binding	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(serv_addr.sin_zero), 8);

	serv_sync_addr.sin_family = AF_INET;
	serv_sync_addr.sin_port = htons(SERVER_SYNC_PORT);
	serv_sync_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(serv_sync_addr.sin_zero), 8);   
    
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	{
		printf("ERROR on binding socket\n");
		exit(1);
	}

	if (bind(server_sync_sockfd, (struct sockaddr *) &serv_sync_addr, sizeof(serv_sync_addr)) < 0)
	{
		printf("ERROR on binding server sync socket\n");
		exit(1);
	}
	
	// add listeners to sockets
	listen(sockfd, 5);
	listen(server_sync_sockfd, 5);

	clilen = sizeof(struct sockaddr_in);


	// Server waits for next connection
	while(1)
	{
		user = (User*) malloc(sizeof(User));
		first_socket_ok = 1;
		second_socket_ok = 1;

		if ((newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen)) == -1) {
			printf("ERROR on accepting socket\n");
			first_socket_ok = 0;
		}

		if ((new_server_sync_sockfd = accept(server_sync_sockfd, (struct sockaddr *) &cli_addr, &clilen)) == -1) {
			printf("ERROR on accepting server sync socket\n");
			second_socket_ok = 0;
		}

		// if both sockets were created, then create the user thread
		if(first_socket_ok && second_socket_ok)
		{
			user->sockets[0] = newsockfd;
			user->sockets[1] = new_server_sync_sockfd;
			user->sync_needed = 0;
	
			current_thread = get_last_thread(thread_list);
			thread_list = add_to_thread_list(thread_list);

			pthread_create(&current_thread, NULL, user_thread, user);
		}
		else
			free(user);
	}

	close(sockfd);
	close(server_sync_sockfd);
	return 0;
}
