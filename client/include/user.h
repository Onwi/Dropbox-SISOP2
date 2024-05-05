#include <stdbool.h>
#include <netinet/in.h>

/* function to be ran everytime a new user tries to connect to server
 * should return a new User, with connection stablished to server */
void *get_connection();

/* upload file "filename" to sync_dir in the server, and sync with all devices 
 * returns true if file was synced successfully, false otherwise */
bool upload_file(char *filename);

/* download file "filename" from the server to the current dir 
 * returns true if file was found and successfully downloaded, false otherwise */
bool download_file(char *filename);

/* delete file "filename" from sync_dir, propagate changes to server
 * returns true if file was delete successfully, false otherwise */
bool delete_file(char *filename);

/* list all files from server's sync_dir 
 * returns a list with all filenames ;  */
void list_server(char *filename);

/* list all files from client local's sync_dir 
 * returns a list with all filenames ;  */
void list_client(char *filename);

/* close connection with server
 * returns true if connection was closed, false otherwise */
bool close_connection();
