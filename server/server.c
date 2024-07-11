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

#define PORT 4000

UserList *users_list;

int main(int argc, char *argv[])
{
	int sockfd, newsockfd, n;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	pthread_t th1, th2, th3;
	

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
	while (connection_number < 4) {
		if ((newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen)) == -1) {
			printf("ERROR on accept");
		}

		Infos *infos = malloc(sizeof(Infos));
		(* infos).socket = newsockfd;
		(* infos).user_list = users_list;
		
		switch (connection_number) {
			case 0:
				pthread_create(&th1, NULL, handle_client, &infos);
				break;
			case 1:
				pthread_create(&th2, NULL, handle_client, &infos);
				break;
			case 2:
				pthread_create(&th3, NULL, handle_client, &infos);
				break;
			default:
				printf("maxed number of connections reached!\n");
		}
		connection_number++;
	}
	
	pthread_join(th1, NULL);
	pthread_join(th2, NULL);
	pthread_join(th3, NULL);
	
	close(sockfd);
	return 0; 
}
