#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#include "../include/server_utils.h"

#define PORT 4000

UserList *users_list;
pthread_mutex_t lock;

int main(int argc, char *argv[]) {
	int sockfd, newsockfd, n;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	pthread_t th1, th2, th3, th4, th5, th6;
	
	if (pthread_mutex_init(&lock, NULL) != 0) { 
        printf("mutex init has failed\n"); 
		exit(1);
    } 

	// we need a default user so users_list can have
	// same address in all theread
	User default_user;
	default_user.sessions_amount=0;
	strcpy(default_user.username, "defuser");
	users_list = insert_user(users_list, default_user);

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
        printf("ERROR opening socket");
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(serv_addr.sin_zero), 8);     
    
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
		printf("ERROR on binding");
	
	listen(sockfd, 3);
	clilen = sizeof(struct sockaddr_in);
	
	int connection_number = 0;
	while (connection_number < 5) {
		if ((newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen)) == -1) {
			printf("ERROR on accept");
		}

		pthread_mutex_lock(&lock);
		Infos *infos = malloc(sizeof(Infos));
		(* infos).socket = newsockfd;
		(* infos).user_list = users_list;
		pthread_mutex_unlock(&lock);

		switch (connection_number) {
			case 0:
				pthread_create(&th1, NULL, handle_client, infos);
				break;
			case 1:
				pthread_create(&th2, NULL, handle_client, infos);
				break;
			case 2:
				pthread_create(&th3, NULL, handle_client, infos);
				break;
			case 3:
				pthread_create(&th4, NULL, handle_client, infos);
				break;
			default:
				printf("max number of connections reached!\n");
		}
		connection_number++;
	}
	
	pthread_join(th1, NULL);
	pthread_join(th2, NULL);
	pthread_join(th3, NULL);
	pthread_join(th4, NULL);
	
	close(sockfd);
	return 0; 
}
