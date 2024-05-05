#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#include "../include/user.h"

#define PORT 4000

int main(int argc, char *argv[]) {
	int sockfd, newsockfd, n;
	socklen_t clilen;
	char buffer[256];
	struct sockaddr_in serv_addr, cli_addr;
	
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
        printf("ERROR opening socket");
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(serv_addr.sin_zero), 8);     
    
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
		printf("ERROR on binding");
	
	listen(sockfd, 5);
	
	clilen = sizeof(struct sockaddr_in);
	

	UserList *list = init();

	int i = 0;
	while (i < 2) {
		if ((newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen)) == -1) 
			printf("ERROR on accept");
		
		User newUser;
		newUser.clientAddr = cli_addr;
		if (i == 0) {
			newUser.username = "user0";
		} else {
			newUser.username = "user1";
		}
		list = insert_user(list, newUser);
		i++;
	}

	print_user_list(list);	
	
	if ((newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen)) == -1) 
		printf("ERROR on accept");
	
	bzero(buffer, 256);
	
	/* read from the socket */
	n = read(newsockfd, buffer, 256);
	if (n < 0) 
		printf("ERROR reading from socket");
	printf("Here is the message: %s\n", buffer);
	
	/* write in the socket */ 
	n = write(newsockfd,"I got your message", 18);
	if (n < 0) 
		printf("ERROR writing to socket");

	close(newsockfd);
	close(sockfd);
	return 0; 
}
