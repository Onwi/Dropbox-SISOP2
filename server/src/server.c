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

#define PORT 4000
#define SERVER_SYNC_PORT 4001
#define USERNAME_READ_ERROR 1001
#define SESSION_LIMIT_ERROR 1002
#define SESSION_FINISHED 1003
#define USER_EXIT 1004


UserList* user_list;
THREAD_LIST* thread_list;
pthread_mutex_t lock;

/* reverse:  reverse string s in place */
 void reverse(char s[])
 {
     int i, j;
     char c;

     for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
         c = s[i];
         s[i] = s[j];
         s[j] = c;
     }
}  

void itoa(int n, char s[])
 {
     int i, sign;

     if ((sign = n) < 0)  /* record sign */
         n = -n;          /* make n positive */
     i = 0;
     do {       /* generate digits in reverse order */
         s[i++] = n % 10 + '0';   /* get next digit */
     } while ((n /= 10) > 0);     /* delete it */
     if (sign < 0)
         s[i++] = '-';
     s[i] = '\0';
     reverse(s);
} 

void my_write_read_for_files(int sockfd, char action, FILE* fp, long int file_size)
{
  int bytes_read_or_write = 0;
  int total_bytes_read_or_write = 0;
  char* temp_buffer;
  printf("Inside write/read for files\n");
  printf("%c\n", action);

  temp_buffer = (char*) malloc(file_size);
  
  if (action == 'w')
  {
      fread(temp_buffer, 1, file_size, fp);

      while(total_bytes_read_or_write < file_size)
      {
        bytes_read_or_write = write(sockfd, temp_buffer, file_size - total_bytes_read_or_write);
        printf("Bytes written: %d\n", bytes_read_or_write);
        if (bytes_read_or_write < 0)
        {
          fprintf(stderr, "ERROR writing.\n");
          exit(1);
        }

        total_bytes_read_or_write += bytes_read_or_write;
      }
  }
  else 
  {
      while(total_bytes_read_or_write < file_size)
      {
        bytes_read_or_write = read(sockfd, temp_buffer, file_size - total_bytes_read_or_write);
        printf("Bytes written: %d\n", bytes_read_or_write);
        if (bytes_read_or_write < 0)
        {
          fprintf(stderr, "ERROR writing.\n");
          exit(1);
        }

        total_bytes_read_or_write += bytes_read_or_write;
      }

	  printf("Estou aqui\n");
	  printf("%s\n", temp_buffer);
	  printf("Fwrote: %ld\n", fwrite(temp_buffer, 1, file_size, fp));
  }

  printf("Estou aqui agr\n");
  free(temp_buffer);
}


void my_write_read(int sockfd, char buffer[256], char action)
{
  int bytes_read_or_write = 0;
  int total_bytes_read_or_write = 0;
  printf("%c\n", action);
  printf("Sending or reading: %s\n", buffer);
  
  if (action == 'w')
  {
    while(total_bytes_read_or_write < 256)
    {
      bytes_read_or_write = write(sockfd, buffer, 256 - total_bytes_read_or_write);
      printf("Bytes written: %d\n", bytes_read_or_write);
      if (bytes_read_or_write < 0)
      {
        fprintf(stderr, "ERROR writing.\n");
        exit(1);
      }

      total_bytes_read_or_write += bytes_read_or_write;
    }
  }
  else
  {
    bzero(buffer, 256);

    while(total_bytes_read_or_write < 256)
    {
      bytes_read_or_write = read(sockfd, buffer, 256 - total_bytes_read_or_write);
      printf("Bytes read: %d\n", bytes_read_or_write);
      if (bytes_read_or_write < 0)
      {
        fprintf(stderr, "ERROR reading.\n");
        exit(1);
      }

      total_bytes_read_or_write += bytes_read_or_write;
    }
  }
}

void *server_sync_handler(void *arg)
{
	int new_server_sync_sockfd  = *(int *) arg;
	char synch_buffer[256];
	bzero(synch_buffer, 256);


	while(1)
	{
		strcpy(synch_buffer, "Server to client sync");
		my_write_read(new_server_sync_sockfd, synch_buffer, 'w');
		//printf("%s\n", synch_buffer);
		sleep(1);
	}
	/*
	while( strcmp(synch_buffer, "exit") != 0 )
	{
		bzero(synch_buffer, 256);
		//fgets(buffer, 256, "Synch msg");
		strcpy(synch_buffer, "Synch msg");
		my_write_read(new_server_sync_sockfd, synch_buffer, 'w');

		printf("Server response handler: %s\n", synch_buffer);

		sleep(5);
	}
	*/
	close(new_server_sync_sockfd);
	int val = SESSION_FINISHED;
	pthread_exit(&val);
}

void handle_download(int newsockfd, char buffer[256], char username[256])
{
	char file_name[256], file_path[256];
  	FILE *fp;
	long int file_size;

	my_write_read(newsockfd, file_name, 'r'); // Gets file name
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
		my_write_read(newsockfd, buffer, 'w'); // Sends file size
		//fclose(fp);
		return;
	}

	// Compute file dize
	fseek(fp, 0L, SEEK_END);
	file_size = ftell(fp);
	rewind(fp);
	itoa(file_size, buffer);

	printf("size: %s\n", buffer);
	my_write_read(newsockfd, buffer, 'w'); // Sends file size

	my_write_read_for_files(newsockfd, 'w', fp, atoi(buffer)); // Sends file data

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

	printf("Handling user thread: %s\n", th_user.username);

	newsockfd = th_user.sockets[0];
	new_server_sync_sockfd = th_user.sockets[1];

	// Read user name
	my_write_read(newsockfd, th_buffer, 'r');
	my_write_read(new_server_sync_sockfd, th_synch_buffer, 'r');

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
			my_write_read(newsockfd, th_buffer, 'w');

			close(newsockfd);
			close(new_server_sync_sockfd);
			int val = SESSION_LIMIT_ERROR;
			pthread_mutex_unlock(&lock);
			pthread_exit(&val);
		}

		increase_user_session(user_list, th_user.username);
	}
	pthread_mutex_unlock(&lock);

	print_user_list(user_list);

	strcpy(th_buffer, "Connected");
	my_write_read(newsockfd, th_buffer, 'w'); // Send ok to client

	//pthread_create(&server_sync_thread, NULL, server_sync_handler, &new_server_sync_sockfd);

	// Create sync dir path
	//strcat(user_file_path, th_user.username);
	//strcat(user_file_path, "/");
	//strcat(user_file_path, "sync_dir");
	strcpy(user_file_path, "sync_dir_");
	strcat(user_file_path, th_user.username);
	
	// If user directory doesnt exist, create one
	if (stat(user_file_path, &st) == -1)
		mkdir(user_file_path, 0700);

	while (1)
	{
		printf("Handling user thread: %s\n", th_user.username);

		strcpy(user_file_path_copy, user_file_path);

		// Read user request
		my_write_read(newsockfd, th_buffer, 'r');

		if (strstr(th_buffer, "upload"))
		{
			printf("User wants to upload\n");

			// Get file name
			my_write_read(newsockfd, th_buffer, 'r');
			printf("File name: %s\n", th_buffer);

			strcat(user_file_path_copy, "/");
			strcat(user_file_path_copy, th_buffer);

			printf("File name: %s", user_file_path_copy);	

			fp = fopen(user_file_path_copy, "wb");


			// Get file size
			my_write_read(newsockfd, th_buffer, 'r');
			printf("%d\n", atoi(th_buffer));

			// Get file data
			my_write_read_for_files(newsockfd, 'r', fp, atoi(th_buffer));

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
			decrease_user_session(user_list, th_user.username);
			pthread_mutex_unlock(&lock);

			strcpy(th_buffer, "exit");
			my_write_read(newsockfd, th_buffer, 'w');

			close(newsockfd);
			close(new_server_sync_sockfd);
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
