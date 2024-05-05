#include <stdbool.h>
#include <netinet/in.h>

typedef struct user{
	char *username;
	struct sockaddr_in clientAddr;
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
UserList *free_list(UserList *list);
bool search_user(UserList *list, User user);
void print_user_list(UserList *list);
