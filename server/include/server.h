#include "../../shared/include/communication.h"
#include "../../shared/include/definitions.h"
#include "../include/user.h"
#include "../include/thread_list.h"
#include <netdb.h>
#include <string.h>


UserList* user_list;
THREAD_LIST* thread_list;
pthread_mutex_t lock;
int is_coordinator = 0;


/*===============================================================================================================================*/
// Structs ======================================================================================================================
struct sync_struct
{
	int socket;
	char username[USERNAME_MAX_SIZE + 1];
};

typedef struct sockets
{
	int sockfd;
	int server_sync_sockfd;
    int name_server_sockfd;
} SOCKETS;
/*===============================================================================================================================*/

void *user_thread(void *arg);
void handle_download(int newsockfd, char buffer[MESSAGE_SIZE + 1], char username[USERNAME_MAX_SIZE + 1]);

void handle_upload(int newsockfd, int name_server_sockfd, User user, char sync_dir_path[9 + USERNAME_MAX_SIZE + 1]);
void handle_delete(int newsockfd, char buffer[MESSAGE_SIZE + 1], char username[USERNAME_MAX_SIZE + 1]);


void delete_propagation(char username[USERNAME_MAX_SIZE + 1], char file_path[FILE_PATH_MAX_SIZE + 1]);
int sockets_setup(int* sockfd, int* server_sync_sockfd, struct sockaddr_in* serv_addr, struct sockaddr_in* serv_sync_addr, socklen_t *clilen, char* argv, int port);

void get_sync_dir(int newsockfd, char sync_dir_path[9 + USERNAME_MAX_SIZE + 1]);
int name_server_socket_setup(int* name_server_sockfd, struct sockaddr_in* name_server_addr, struct hostent* name_server, char* hostname);

void get_sync_dir_replication(int name_server_sockfd, char file_path[FILE_PATH_MAX_SIZE + 1]);