#include <stdbool.h>
#include <netinet/in.h>
#include <pthread.h>

#define MAX_SESSION 2
#define CONNECTIONS_PER_CLIENT 2

typedef struct user{
	char username[256];
	int sessions_amount;
	int sockets[CONNECTIONS_PER_CLIENT];
	int all_sessions_sockets[MAX_SESSION][CONNECTIONS_PER_CLIENT];
	pthread_t thread;
	int sync_needed;
	int deletion_needed;
	char file_name_sync[256];
	// more information to be added
}User;


/* linked list to keep record of all active users */
typedef struct userNode{
	User user;
	struct userNode *next;
}UserList;

// basic linked list operations
UserList *init();
UserList *insert_user(UserList *list, User user); // end of list
UserList *remove_user(UserList *list, char* username);
UserList *free_list(UserList *list);
User get_user(UserList *list, char* username);
void increase_user_session(UserList *list, char* username, int sockfd, int server_sync_sockfd);
void decrease_user_session(UserList *list, char* username);
bool search_user(UserList *list, char* username);
void print_user_list(UserList *list);
