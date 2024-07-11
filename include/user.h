#include <stdbool.h>
#include <netinet/in.h>

#define MAX_SESSION 2

typedef struct user{
	char *username;
	int sessions_amount;
	int sockets[MAX_SESSION];
}User;


/* linked list to keep record of all active users */
typedef struct userNode{
	User user;
	struct userNode *next;
}UserList;

// basic linked list operations
UserList *init();
UserList *insert_user(UserList *list, User user); // end of list
UserList *free_list(UserList *list);
User *get_user(UserList *list, char* username);
void increase_user_session(UserList *list, char* username);
bool search_user(UserList *list, char* username);
void print_user_list(UserList *list);
