#include <stdbool.h>

typedef struct user{
	char *username;
	int server_ip_address; // TODO: is this really an int?
	int port;
	// more information to be added
}User;

/* function to be ran everytime a new user tries to connect to server
 * should return a new User, with connection stablished to server */
User *get_connection(char *username, int server_ip_address, int port);

/* upload file "filename" to sync_dir in the server, and sync with all devices 
 * returns true if file was synced successfully, false otherwise */
bool upload_file(char *filename, User *user);

/* download file "filename" from the server to the current dir 
 * returns true if file was found and successfully downloaded, false otherwise */
bool download_file(char *filename, User *user);

/* delete file "filename" from sync_dir, propagate changes to server
 * returns true if file was delete successfully, false otherwise */
bool delete_file(char *filename, User *user);

/* list all files from server's sync_dir 
 * returns a list with all filenames ;  */
void list_server(char *filename, User *user);

/* list all files from client local's sync_dir 
 * returns a list with all filenames ;  */
void list_client(char *filename, User *user);

/* close connection with server
 * returns true if connection was closed, false otherwise */
bool close_connection(User *user);

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
