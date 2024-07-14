#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#include "../include/server_utils.h"

int getCommandFromUser(int client_socket) {
	int *command;
	return read(client_socket, command, sizeof(command));
}

pthread_mutex_t mutex_lock;

void *handle_client(void *arg) {

	printf("ia ia o, ia ia o");
	Infos *client_info = (Infos *) arg;
	int device_socket = (* client_info).socket;
	UserList *list = (* client_info).user_list;	

	printf("\n\nlist address: %p\n", &list);

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
	
	printf("username: %s\n", username);
 
	pthread_mutex_lock(&mutex_lock);
	User *user = get_user(list, username);
	pthread_mutex_unlock(&mutex_lock);	
	
	char returnvalue[256];
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
		char sync_dir[100] = "sync_dir_";
		if (mkdir(strcat(sync_dir, username), 777) != 0) {
     		printf("error creating dir");
    	}
	} else if (user->sessions_amount < 2) {
		user->sessions_amount++;
		// it's the second time user is connecting
		// we need to sync his local sync_dir folder with what we have in server

		// use mutex to sync it all
		strcpy(returnvalue, "loggin in second device");
	} else {
		strcpy(returnvalue, "already logged in two devices");
	}
	
	num_bytes = write(device_socket, returnvalue, 256);
	if (num_bytes < 0) 
		printf("ERROR writing to socket");

	// now we keed to keep listen for clients requests
	
	close(device_socket);
	int val = 10;
	pthread_exit(&val);
}
