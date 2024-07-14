#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

int main(int argc, char *argv[])
{
  int sockfd, n;
  struct sockaddr_in serv_addr;
  struct hostent *server;

  if (argc < 4) {
    fprintf(stderr, "usage %s hostname\n", argv[0]);
    exit(0);
  }

  char username[256];
  strcpy(username, argv[1]);
  server = gethostbyname(argv[2]);
  int port = atoi(argv[3]);

  if (server == NULL)
  {
    fprintf(stderr, "ERROR, no such host\n");
    exit(0);
  }

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    printf("ERROR opening socket\n");

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
  bzero(&(serv_addr.sin_zero), 8);

  if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    printf("ERROR connecting\n");

  // we wait until server confirm we are connected
  char response[256];
  n = read(sockfd, response, 256);
  if (n < 0)
  {
    printf("ERROR reading from socket\n");
    exit(1);
  }
  else
  {
    // make sync_dir user local device
    if (mkdir("sync_dir", 777) != 0)
    {
      printf("error creating dir");
    }
  }

  n = write(sockfd, username, 256);
  if (n < 0)
    printf("ERROR writing to socket\n");

  n = read(sockfd, response, 256);
  if (n < 0)
    printf("ERROR reading from socket\n");

  printf("response from user creation: %s\n", response);
  if (strcmp(response, "already logged in two devices") == 0) {
    exit(1);
  }

  // we know have this user's device connected and local sync_dir created
  // now we need to keep listen for commands to be sent to server
  int should_exit = 1;
  while (should_exit != 0) {
    // keep listen
    int command = 5;
    switch (command) {
      case 0:
        // upload
        break;
      case 1:
        // download
        break;
      case 2:
        // delete
        break;
      case 3:
        // list_server
        break;
      case 4:
        // list_client
        break; 
      case 5:
        exit(1);
        break;
      
      default:
        break;
    }
  }

  close(sockfd);
  return 0;
}
