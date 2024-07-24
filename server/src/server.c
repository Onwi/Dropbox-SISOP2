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

#include "../include/user.h"
#include "../include/thread_list.h"
#include "../../shared/include/communication.h"

#define PORT 4000
#define SERVER_SYNC_PORT 4001
#define USERNAME_READ_ERROR 1001
#define SESSION_LIMIT_ERROR 1002
#define SESSION_FINISHED 1003
#define USER_EXIT 1004


UserList* user_list;
THREAD_LIST* thread_list;
pthread_mutex_t lock;

struct sync_struct
{
	int socket;
	char username[256];
};

void handle_sync_download(int new_server_sync_sockfd, char file_name[256], char username[256])
{
	char file_path[256];
	char buffer[256];
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
	char synch_buffer[256];
	bzero(synch_buffer, 256);
	int new_server_sync_sockfd;
	char username[256];
	User user;
	int i;

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
			{
				//pthread_mutex_lock(&lock);
				handle_sync_download(/*new_server_sync_sockfd*/ user.all_sessions_sockets[i][1], user.file_name_sync, username);
				//pthread_mutex_unlock(&lock);
			}

			//pthread_mutex_lock(&lock);
			turn_off_user_sync_notification(user_list, username);
			pthread_mutex_unlock(&lock);
		}
	}
	
	close(new_server_sync_sockfd);
	int val = SESSION_FINISHED;
	pthread_exit(&val);
}


void handle_download(int newsockfd, char buffer[256], char username[256])
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

void *user_thread(void *arg) {
	User th_user  = *(User *) arg;
	int newsockfd, new_server_sync_sockfd;
	char th_buffer[256];
	char th_synch_buffer[256];	
	bzero(th_buffer, 256);
	pthread_t server_sync_thread;
	struct stat st = {0};
	char user_file_path[256], user_file_path_copy[256];
	int th_return_value;
	FILE *fp;
	struct sync_struct my_sync_struct;
	char file_name_sync[256]; 
	int session_number;

	printf("Handling user thread: %s\n", th_user.username);


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
		strcpy(th_user.username, th_buffer);
		printf("User %s has been found online.\n", th_buffer);
		th_user = get_user(user_list, th_buffer);

		if (th_user.sessions_amount == MAX_SESSION)
		{
			fprintf(stderr, "ERROR max user session reached\n");

			strcpy(th_buffer, "exit");
			send_msg(newsockfd, th_buffer);

			close(newsockfd);
			close(new_server_sync_sockfd);
			int val = SESSION_LIMIT_ERROR;
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

	
	//get_sync_dir();
	// Create sync dir path
	strcpy(user_file_path, "sync_dir_");
	strcat(user_file_path, th_user.username);
	
	// If user directory doesnt exist, create one
	if (stat(user_file_path, &st) == -1)
		mkdir(user_file_path, 0700);


	// Create Sync thread
	my_sync_struct.socket = new_server_sync_sockfd;
	strcpy(my_sync_struct.username, th_user.username);
	pthread_create(&server_sync_thread, NULL, server_sync_handler, /*&new_server_sync_sockfd*/ &my_sync_struct);
	int not_exit =1;
	while (1)
	{
		printf("Handling user thread: %s\n", th_user.username);

		strcpy(user_file_path_copy, user_file_path);

		printf("Next command: %s\n", th_buffer);

		// Read user request
		receive_msg(newsockfd, th_buffer);


		// Handle upload
		if (strstr(th_buffer, "upload"))
		{
			printf("User wants to upload\n");

			// Get file name
			receive_msg(newsockfd, th_buffer);
			printf("File name: %s\n", th_buffer);

			strcpy(file_name_sync, th_buffer);

			strcat(user_file_path_copy, "/");
			strcat(user_file_path_copy, th_buffer);

			printf("File name: %s", user_file_path_copy);	

			fp = fopen(user_file_path_copy, "wb");


			// Get file size
			receive_msg(newsockfd, th_buffer);
			printf("%d\n", atoi(th_buffer));

			// Get file data
			receive_file(newsockfd, fp, atoi(th_buffer));

			// Get name for sync notification
			receive_msg(newsockfd, th_buffer);

			pthread_mutex_lock(&lock);
			set_user_sync_notification(user_list, th_buffer, file_name_sync);
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
	struct sockaddr_in serv_sync_addr, cli_sync_addr;
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
		{
			free(user);
		}
	}

	close(sockfd);
	close(server_sync_sockfd);
	return 0;
}
