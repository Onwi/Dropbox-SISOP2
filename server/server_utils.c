#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#include "../include/user.h"
#include "../include/server_utils.h"

int getCommandFromUser(int client_socket) {
	int *command;
	return read(client_socket, command, sizeof(command));
}

void *handle_client(void *arg) {
	Infos client_info = *(Infos *) arg;
	int device_socket= client_info.socket;
	UserList *list = client_info.user_list;	
	
	int num_bytes;
	char username[256];
	
	bzero(username, 256);

	// read username from user 
	num_bytes = read(device_socket, username, 256);

	if (num_bytes < 0) 
		printf("ERROR reading from socket");
	
	printf("running in socket: %d\n", device_socket);
	printf("Here is the message: %s\n", username);

	User *user = get_user(list, username);
	if (!user) {
		user = (User *) malloc(sizeof(User));
		user->username = username;
		user->sessions_amount = 0;
		user->sockets[0] = device_socket;
	} else {
		printf("user already logged in");
	}
		
	/* write in the socket */ 
	num_bytes = write(device_socket, "User logged in", 18);
	if (num_bytes < 0) 
		printf("ERROR writing to socket");

	close(device_socket);
	int val = 10;
	pthread_exit(&val);
}
