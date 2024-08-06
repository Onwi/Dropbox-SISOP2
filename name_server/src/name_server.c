#include "../../shared/include/communication.h"
#include "../../shared/include/definitions.h"


int socket_setup(int* sockfd, struct sockaddr_in* serv_addr, socklen_t *clilen)
{
    // Create sockets
    *sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (*sockfd == -1)
	{
        printf("ERROR opening socket\n");
		exit(1);
	}

	// Bind sockets
	serv_addr->sin_family = AF_INET;
	serv_addr->sin_port = htons(NAME_SERVER_PORT);
	serv_addr->sin_addr.s_addr = INADDR_ANY;
	bzero(&(serv_addr->sin_zero), 8);
  
	if (bind(*sockfd, (struct sockaddr *) serv_addr, sizeof(*serv_addr)) < 0)
	{
		printf("ERROR on binding socket\n");
		exit(1);
	}
	
	// Add listeners to sockets
	listen(*sockfd, 5);

	*clilen = sizeof(struct sockaddr_in);

    return 0;
}

int main(int argc, char *argv[])
{
	int sockfd, new_sockfd;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;


    socket_setup(&sockfd, &serv_addr, &clilen);

	// Server waits for next connection
	while(1)
	{
		if ((new_sockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen)) == -1) {
			printf("ERROR on accepting socket\n");
			return 1;
		}

        printf("You connected to the name server\n");
        break;
	}

	close(sockfd);
	return 0;
}
