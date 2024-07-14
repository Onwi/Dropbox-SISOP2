#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <dirent.h>

#include "../include/server_utils.h"
#include "../communication/communication.h"

int getCommandFromUser(int client_socket) {
	int *command;
	return read(client_socket, command, sizeof(command));
}

pthread_mutex_t mutex_lock;

void *handle_client(void *arg) {
	Infos *client_info = (Infos *) arg;
	int device_socket = (* client_info).socket;
	UserList *list = (* client_info).user_list;	

	int num_bytes;
	char username[256];
	
	// we let the client know we are connected
	char message_to_client[256] = "success in connect";
	num_bytes = write(device_socket, message_to_client, 256);
	if (num_bytes < 0) {
		printf("ERROR writing in socket");
	}

	// read username from user 
	num_bytes = read(device_socket, username, 256);
	if (num_bytes < 0) 
		printf("ERROR reading from socket");
 
	pthread_mutex_lock(&mutex_lock);
	User *user = get_user(list, username);
	pthread_mutex_unlock(&mutex_lock);	
	
	char returnvalue[256];
	char sync_dir[100] = "sync_dir_";
	if (!user) {
		User newUser;
		newUser.username = username;
		newUser.sessions_amount = 1;
		newUser.sockets[0] = device_socket;
		strcpy(returnvalue, "loggin in first device");
		
		pthread_mutex_lock(&mutex_lock);	
		list = insert_user(list, newUser);
		pthread_mutex_unlock(&mutex_lock);	
		
		// at this point we know this is the first connection fron user
		// so we can create sync_dir_username, this will be clients dir in server
		if (mkdir(strcat(sync_dir, username), S_IRWXU | S_IRWXG | S_IRWXO) != 0) {
     		printf("error creating dir\n");
    	}
	} else if (user->sessions_amount < 2) {
		user->sessions_amount++;
		strcpy(returnvalue, "loggin in second device");
		num_bytes = write(device_socket, returnvalue, 256);
		if (num_bytes < 0) 
			printf("ERROR writing to socket");
		
    // it's the second time user is connecting
		// we need to sync his local sync_dir folder with what we have in server
		pthread_mutex_lock(&mutex_lock);	
		
		struct dirent *dir_stream;
		char path[100];
		getcwd(path, sizeof(path));
		strcat(path, "/");
		strcat(sync_dir, username);
		strcat(path, sync_dir);
			
		DIR *dr = opendir(path);
		if (dr == NULL) printf("failed opening user dir");
		
		// go thru all files
		while ((dir_stream = readdir(dr)) != NULL) {
			if (strcmp(dir_stream->d_name, ".") != 0 && strcmp(dir_stream->d_name, "..") != 0) {

        char filename[150];
        strcat(filename, path);
        strcat(filename, "/");
        strcat(filename, dir_stream->d_name);
				
		    FILE *file = fopen(filename, "r");
        if (file != NULL) {
          printf("file opened %p\n", file);
          fseek(file, 0, SEEK_END);
          long file_size = ftell(file);
          fseek(file, 0, SEEK_SET);

          Packet packet;
          printf("size of packted: %ld\n", sizeof(packet));
          
          strcpy(packet.filename, dir_stream->d_name);
          printf("packet filename %s\n", packet.filename);
         
          packet.file_size = file_size;	
          printf("packet filesize %ld\n", packet.file_size);
          
          // ******************************************************
          // nao esta lendo o arquivo corretamente
          fread(packet.payload, file_size, 1, file);
          printf("packet payload %s\n", packet.payload);
          
          upload_file(device_socket, &packet, sizeof(packet));
			    fclose(file);
        } else {
          printf("error opening file\n");
        }
			}
	  }
		closedir(dr);

		pthread_mutex_unlock(&mutex_lock);
	} else {
		strcpy(returnvalue, "already logged in two devices");
	}

  	if (strcmp(returnvalue, "loggin in second device") != 0) {
		num_bytes = write(device_socket, returnvalue, 256);
		if (num_bytes < 0) 
			printf("ERROR writing to socket");
	}
	

	// now we keed to keep listen for clients requests
	
	close(device_socket);
	int val = 10;
	pthread_exit(&val);
}
