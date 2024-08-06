#include "../../shared/include/definitions.h"
#include "../../shared/include/communication.h"
#include "../include/inotify.h"
#include <string.h>
#include <netdb.h>
#include <dirent.h>
#include <time.h>
#include <ftw.h>


int new_input_notification = 0;
char user_input[USER_INPUT_MAX_SIZE + 1];
char sync_dir_path[9 + USERNAME_MAX_SIZE + 1];
char username[USERNAME_MAX_SIZE + 1];
pthread_mutex_t upload_lock, user_input_listener_lock, delete_lock;


void* user_input_handler(void* args)
{
    char th_user_input[USER_INPUT_MAX_SIZE + 1];

    while(1)
    {
        bzero(th_user_input, USER_INPUT_MAX_SIZE + 1);
        do
        {
            printf("User input: ");
            fgets(th_user_input, USER_INPUT_MAX_SIZE + 1, stdin);
            printf("Input: %s\n", th_user_input);
            printf("Size: %ld\n", strlen(th_user_input));
        }
        while(strcmp(th_user_input, "\n") == 0 || strlen(th_user_input) == 0 || strlen(th_user_input) >= USER_INPUT_MAX_SIZE);

        pthread_mutex_lock(&user_input_listener_lock);
        bzero(user_input, USER_INPUT_MAX_SIZE + 1);
        strcpy(user_input, th_user_input);
        new_input_notification = 1;
        pthread_mutex_unlock(&user_input_listener_lock);

        sleep(1);
    }
}

void* server_sync_handler(void* args)
{
    int server_sync_sockfd;
    unsigned int file_size;
    char buffer[MESSAGE_SIZE + 1];
    char file_path[FILE_PATH_MAX_SIZE + 1];
    FILE* fp;


    server_sync_sockfd = *(int*) args;

    while(1)
    {
        receive_msg(server_sync_sockfd, buffer); // Get synchronization type
        printf("Synch type: %s\n", buffer);

        if(strcmp(buffer, "Upload") == 0)
        {
            receive_msg(server_sync_sockfd, buffer); // Get file size
            printf("Buffer: %s\n", buffer);
            file_size = atoi(buffer);
            printf("File size: %d\n", file_size);

            printf("Needs synch AQUIIIIIIIIIIIIIIIIIIIIIIIII\n");
        
            receive_msg(server_sync_sockfd, buffer); // Get file name

            strcpy(file_path, "sync_dir_");
            strcat(file_path, username);
            strcat(file_path, "/");
            strcat(file_path, buffer);

            fp = fopen(file_path, "wb");

            if(fp)
                printf("Abri file da propagacao\n");
            else
                printf("Nao abri file da propagacao\n");

            printf("File path: %s\n", file_path);

            receive_file(server_sync_sockfd, fp, file_size); // Get file data

            printf("Received file: %s\n", file_path);

            fclose(fp);
        }
        else if(strcmp(buffer, "Delete") == 0)
        {
            // Get file path
            receive_msg(server_sync_sockfd, buffer);
            strcpy(file_path, buffer);
            printf("File path: %s\n", buffer);

            remove(file_path);
        }
    }
}

void handle_list_client()
{
    DIR *dp;
    struct dirent *ep;   
    dp = opendir (sync_dir_path);
    struct stat st;// = {0};
    char file_path[FILE_PATH_MAX_SIZE + 1];


    if(!dp)
    {
        perror ("Couldn't open the directory");
        return;
    }

    while ((ep = readdir (dp)))
        if (strcmp(ep->d_name, ".") == 0 || strcmp(ep->d_name, "..") == 0)
        {
            // Do nothing 
        }
        else
        {
            //st = (struct stat*) malloc(sizeof(struct stat));
            strcpy(file_path, sync_dir_path);
            strcat(file_path, "/");
            strcat(file_path, ep->d_name);
            printf("File path: %s\n", file_path);
            stat(file_path, &st);
            printf("%s\n", ep->d_name);

            //printf("atime = %d\nmtime = %d\nctime = %d\n", atime, mtime, ctime);

            printf("File access time %s", ctime(&st.st_atime));
            printf("File modify time %s", ctime(&st.st_mtime));
            printf("File changed time %s", ctime(&st.st_ctime));
            printf("\n");
            //free(st);
        }
}

int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    int rv = remove(fpath);

    if (rv)
        perror(fpath);

    return rv;
}

// Used to remove old sync dir
int rmrf(char *path)
{
    return nftw(path, unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
}

void get_sync_dir(int sockfd)
{
    int number_of_files, i;
    FILE* fp;
    char buffer[MESSAGE_SIZE + 1];
    char file_name[FILE_NAME_MAX_SIZE + 1];
    char file_path[FILE_PATH_MAX_SIZE + 1];
    unsigned int file_size;
    struct stat st = {0};  


    // If user sync dir exist, remote it
    if (stat(sync_dir_path, &st) == 0)
    {
        rmrf(sync_dir_path);
        printf("Sync dir deleted\n");
    }

    // Create sync dir
    mkdir(sync_dir_path, 0777);

    // Gets number of files to sync
    receive_msg(sockfd, buffer);
    number_of_files = atoi(buffer);


    // Get all files
    for(i = 0; i < number_of_files; i++)
    {
        receive_msg(sockfd, buffer); // Gets file name
        strcpy(file_name, buffer);

        strcpy(file_path, sync_dir_path);
        strcat(file_path, "/");
        strcat(file_path, file_name);

        fp = fopen(file_path, "wb");
        printf("Opening: %s\n", file_path);

        receive_msg(sockfd, buffer); // Gets file size
        file_size = atoi(buffer);

        receive_file(sockfd, fp, file_size); // Gets file data

        fclose(fp);
    }
}

void handle_upload(int sockfd, char buffer[MESSAGE_SIZE + 1])
{
    unsigned int file_size;
    FILE* fp;
    char file_path[FILE_PATH_MAX_SIZE + 1], file_name[FILE_NAME_MAX_SIZE + 1];


    strcpy(file_path, &buffer[7]); // Get file path from user input
    file_path[strcspn(file_path, "\n")] = 0; // Remove '\n'
    fp = fopen(file_path, "rb");

    // if function cannot open the file, return
    if(!fp)
        return;
    
    printf("File opened\n");

    // Compute file size
    fseek(fp, 0L, SEEK_END);
    file_size = ftell(fp);
    rewind(fp);

    printf("File size: %d\n", file_size);

    // Send upload request     
    send_msg(sockfd, buffer);

    // Send file name
    strcpy(file_name, strrchr(file_path, '/') + 1); // Copy file name without the '/'
    printf("File name: %s\n", file_name);
    strcpy(buffer, file_name);
    send_msg(sockfd, buffer);

    // Send file size
    itoa(file_size, buffer);        
    printf("%s\n", buffer);        
    send_msg(sockfd, buffer);

    // Send file data
    send_file(sockfd, fp, file_size);

    fclose(fp);
}

void handle_download(int sockfd, char buffer[MESSAGE_SIZE + 1])
{
    char file_name[FILE_NAME_MAX_SIZE + 1];
    FILE *fp;

    strcpy(file_name, &buffer[9]);
    file_name[strcspn(file_name, "\n")] = 0; // Remove '\n'

    fp = fopen(file_name, "wb");

    printf("User wants to download: %s\n", file_name);

    // Send download request
    send_msg(sockfd, buffer);

    // Send file name
    strcpy(buffer, file_name);
    send_msg(sockfd, buffer);
    receive_msg(sockfd, buffer); // Get file size


    if (strcmp(buffer, "0") == 0)
    {
        fclose(fp);
        printf("Invalid file name\n");
        return;
    }

    receive_file(sockfd, fp, atoi(buffer)); // Gets file data

    fclose(fp);
    return;
}

void handle_delete(int sockfd, char buffer[MESSAGE_SIZE + 1])
{
    char file_name[FILE_NAME_MAX_SIZE + 1], file_path[FILE_PATH_MAX_SIZE + 1];


    buffer[strcspn(buffer, "\n")] = 0; // Remove '\n'
    strcpy(file_name, &buffer[7]);

    // Send delete request
    send_msg(sockfd, buffer);

    // Send file name
    strcpy(buffer, file_name);
    send_msg(sockfd, buffer);

    // Create path to file being deleted
    strcpy(file_path, sync_dir_path);
    strcat(file_path, "/");
    strcat(file_path, file_name);

    if(remove(file_path) == 0)
        printf("File %s deleted\n", file_name);
    else
        printf("Could not delete %s\n", file_name);
}

int sockets_setup(int* sockfd, int* server_sync_sockfd, struct sockaddr_in* serv_addr, struct sockaddr_in* serv_sync_addr, struct hostent* server)
{
    // Create sockets
    if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        fprintf(stderr, "ERROR opening socket\n");
        return 1;
    }

    if ((*server_sync_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        fprintf(stderr, "ERROR opening server sync socket\n");
        return 1;
    }
        
    // Bind sockets
    serv_addr->sin_family = AF_INET;  
    serv_addr->sin_port = htons(PORT);
    serv_addr->sin_addr = *((struct in_addr *)server->h_addr_list[0]);
    bzero(&(serv_addr->sin_zero), 8);

    serv_sync_addr->sin_family = AF_INET;     
    serv_sync_addr->sin_port = htons(SERVER_SYNC_PORT);    
    serv_sync_addr->sin_addr = *((struct in_addr *)server->h_addr_list[0]);
    bzero(&(serv_sync_addr->sin_zero), 8);   
        
    // Connect sockets
    if (connect(*sockfd, (struct sockaddr *) serv_addr, sizeof(*serv_addr)) < 0)
    {
        fprintf(stderr, "ERROR connecting\n");
        return 1;
    }

    if (connect(*server_sync_sockfd, (struct sockaddr *) serv_sync_addr, sizeof(*serv_sync_addr)) < 0)
    {
        fprintf(stderr, "ERROR connecting on server sync\n");
        return 1;
    }

    return 0;
}

int establish_connection(int sockfd, int server_sync_sockfd, char* argv)
{
    char buffer[MESSAGE_SIZE + 1];

    // Send username
    strcpy(username, argv);
    strcpy(buffer, username);
    send_msg(sockfd, buffer);

    // Check for OK session
    receive_msg(sockfd, buffer);
    printf("Buffer: %s\n", buffer);
    
    // If connection failed, return
    if(strcmp(buffer, "exit") == 0)
    {
        fprintf(stderr, "Failed to establish connection with the server.\n");
        close(sockfd);
        close(server_sync_sockfd);
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int sockfd, server_sync_sockfd;
    struct sockaddr_in serv_addr, serv_sync_addr;
    struct hostent *server;
    char buffer[MESSAGE_SIZE + 1];
    struct sync_dir_listener_struct my_sync_dir_listener_struct;

    pthread_t user_input_listener_thread, sync_thread, sync_dir_listener_thread;
    struct stat st = {0};


    // If arguments are wrong, end client
    if(argc != 3)
    {
        fprintf(stderr,"usage %s username hostname\n", argv[0]);
        return 1;
    }

    // If username exceeds USERNAME_MAX_SIZE, end client
    if(strlen(argv[1]) > USERNAME_MAX_SIZE)
    {
        fprintf(stderr, "Max username size permitted: %d\n", USERNAME_MAX_SIZE);
        return 1;
    }

    // If server doesnt exist, end client    
    server = gethostbyname(argv[2]);
    if(server == NULL)
    {
        fprintf(stderr, "ERROR, no such host\n");
        return 1;
    }

    // If sockets setup fails, end client
    if(sockets_setup(&sockfd, &server_sync_sockfd, &serv_addr, &serv_sync_addr, server))
        return 1;

    // If connection fails, end client
    if(establish_connection(sockfd, server_sync_sockfd, argv[1]))
        return 1;

    // Get sync dir
    strcpy(sync_dir_path, "sync_dir_");
    strcat(sync_dir_path, username);
    get_sync_dir(sockfd);

    // Create user input listener thread
    pthread_create(&user_input_listener_thread, NULL, user_input_handler, NULL);

    // Create sync thread
    pthread_create(&sync_thread, NULL, server_sync_handler, &server_sync_sockfd);

    // Create sync dir listener thread
    my_sync_dir_listener_struct.sockfd = sockfd;
    strcpy(my_sync_dir_listener_struct.dir_path, sync_dir_path);
    pthread_create(&sync_dir_listener_thread, NULL, listen_inotify, &my_sync_dir_listener_struct);

    // User input handler
    while(1)
    {
        strcpy(buffer, "Waiting for user input");

        // Checks for new user input
        pthread_mutex_lock(&user_input_listener_lock);
        if (new_input_notification)
        {
            strcpy(buffer, user_input);
            new_input_notification = 0;
        }
        pthread_mutex_unlock(&user_input_listener_lock);

        // Handle client side user input
        if(strstr(buffer, "upload ")) // Upload command
            {
                pthread_mutex_lock(&upload_lock);
                handle_upload(sockfd, buffer);
                pthread_mutex_unlock(&upload_lock);
            }

        else if(strstr(buffer, "download ")) // Download command
            handle_download(sockfd, buffer);

        else if(strstr(buffer, "list_client")) // List client command
            handle_list_client();

        else if(strstr(buffer, "delete ")) // Remove command
            {
                pthread_mutex_lock(&delete_lock);
                handle_delete(sockfd, buffer);
                pthread_mutex_unlock(&delete_lock);
            }
        
        else if(strstr(buffer, "exit")) // Exit command
            break;
    }

    // Sends exit request
    strcpy(buffer, "exit");
    send_msg(sockfd, buffer);

    // If user sync dir exist, remote it
    if (stat(sync_dir_path, &st) == 0)
    {
        rmrf(sync_dir_path);
        printf("Sync dir deleted\n");
    }
        
    close(sockfd);
    close(server_sync_sockfd);
        
    return 0;
}
