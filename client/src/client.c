#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <ftw.h>
#include "../../shared/include/communication.h"

#define bzero(ptr, size) memset(ptr, 0, size)

#define PORT 4000
#define SERVER_SYNC_PORT 4001
#define USERNAME_MAX_SIZE 32

pthread_mutex_t lock, user_feedback_lock, user_input_listener_lock;
char user_input[256];
int new_input_notification = 0;
char sync_dir_path[256];
char username[USERNAME_MAX_SIZE];


void* user_input_handler(void* args)
{
    char th_user_input[256];

    while(1)
    {
        pthread_mutex_lock(&user_feedback_lock);
        printf("User input: ");
        pthread_mutex_unlock(&user_feedback_lock);
        
        bzero(th_user_input, 256);
        fgets(th_user_input, 256, stdin);

        pthread_mutex_lock(&user_input_listener_lock);
        strcpy(user_input, th_user_input);
        new_input_notification = 1;
        pthread_mutex_unlock(&user_input_listener_lock);

        sleep(1);
    }
}

void* server_sync_handler(void* args)
{
    int server_sync_sockfd = *(int*) args;
    char buffer[MESSAGE_SIZE];
    FILE* fp;
    long int file_size;
    char file_path[256];


    while(1)
    {
        receive_msg(server_sync_sockfd, buffer); // Gets file size
        file_size = atoi(buffer);

        printf("Needs synch AQUIIIIIIIIIIIIIIIIIIIIIIIII\n");
    
        receive_msg(server_sync_sockfd, buffer); // Gets file name

        strcpy(file_path, "sync_dir_");
        strcat(file_path, username);
        strcat(file_path, "/");
        strcat(file_path, buffer);

        fp = fopen(file_path, "wb");

        printf("File path: %s\n", file_path);

        receive_file(server_sync_sockfd, fp, file_size); // Gets file data

        printf("Received file: %s\n", file_path);

        fclose(fp);
    }

    return 0;
}

void handle_list_client()
{
    DIR *dp;
    struct dirent *ep;   
    dp = opendir (sync_dir_path);
    struct stat st;// = {0};


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
            stat(ep->d_name, &st);
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
    char buffer[MESSAGE_SIZE];
    char file_name[256];
    char file_path[256];
    long int file_size;
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
    printf("Cheguei aqui\n");
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

void handle_upload(int sockfd, char buffer[256])
{
    long int file_size;
    FILE* fp;
    char file_path[128];

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

    printf("%ld\n", file_size);

    //my_write_read(sockfd, buffer, 'w'); // Send upload request        
    send_msg(sockfd, buffer);

    // Send file name
    strcpy( buffer, strrchr(file_path, '/') );
    printf("%s\n", &(buffer[1]));
    send_msg(sockfd, &(buffer[1]));

    // Send file size
    itoa(file_size, buffer);        
    printf("%s\n", buffer);        
    send_msg(sockfd, buffer);

    // Send file data
    send_file(sockfd, fp, file_size);

    // Send name for sync notification
    send_msg(sockfd, username); 

    fclose(fp);
}

void handle_download(int sockfd, char buffer[256])
{
    char file_name[256];
    FILE *fp;

    strcpy(file_name, &buffer[9]);
    file_name[strcspn(file_name, "\n")] = 0; // Remove '\n'

    fp = fopen(file_name, "wb");

    printf("User wants to download: %s\n", file_name);

    send_msg(sockfd, buffer); // Sends download request
    send_msg(sockfd, file_name); // Sends file name
    receive_msg(sockfd, buffer); // Gets file size


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

void handle_remove(int sockfd, char buffer[256])
{

}

void handle_exit(int sockfd, char buffer[256])
{

}

int main(int argc, char *argv[])
{
    int sockfd, server_sync_sockfd;
    struct sockaddr_in serv_addr, serv_sync_addr;
    struct hostent *server;
    char buffer[256];
    pthread_t user_input_listener_thread;
    pthread_t sync_thread;

        
    if (argc < 3) {
        fprintf(stderr,"usage %s username hostname\n", argv[0]);
        exit(0);
    }

    strcpy(username, argv[1]);
        
    server = gethostbyname(argv[2]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    
    // Create sockets
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("ERROR opening socket\n");
        exit(1);
    }

    if ((server_sync_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("ERROR opening server sync socket\n");
        exit(1);
    }
        
    // Bind sockets
    serv_addr.sin_family = AF_INET;     
    serv_addr.sin_port = htons(PORT);    
    serv_addr.sin_addr = *((struct in_addr *)server->/*h_addr*/h_addr_list[0]);
    bzero(&(serv_addr.sin_zero), 8);

    serv_sync_addr.sin_family = AF_INET;     
    serv_sync_addr.sin_port = htons(SERVER_SYNC_PORT);    
    serv_sync_addr.sin_addr = *((struct in_addr *)server->h_addr_list[0]);
    bzero(&(serv_sync_addr.sin_zero), 8);   
        
    // Connect sockets
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("ERROR connecting\n");
        exit(1);
    }

    if (connect(server_sync_sockfd, (struct sockaddr *) &serv_sync_addr, sizeof(serv_sync_addr)) < 0)
    {
        printf("ERROR connecting on server sync\n");
        exit(1);
    }

    // Sends username
    strcpy(buffer, argv[1]);
    send_msg(sockfd, buffer);
    send_msg(server_sync_sockfd, "Synch request");

    // check for OK session
    receive_msg(sockfd, buffer);
    printf("Buffer: %s\n", buffer);
    
    // If connection failed, return
    if(strcmp(buffer, "exit") == 0)
    {
        close(sockfd);
        close(server_sync_sockfd);
        return 0;
    }

    // Get sync dir
    bzero(sync_dir_path, 256);
    strcpy(sync_dir_path, "sync_dir_");
    strcat(sync_dir_path, username);
    get_sync_dir(sockfd);

    // Create user input listener thread
    pthread_create(&user_input_listener_thread, NULL, user_input_handler, NULL);

    // Create sync thread
    pthread_create(&sync_thread, NULL, server_sync_handler, &server_sync_sockfd);

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
        if(strstr(buffer, "upload")) // Upload command
            handle_upload(sockfd, buffer);

        else if(strstr(buffer, "download")) // Download command
            handle_download(sockfd, buffer);

        else if(strstr(buffer, "list_client")) // List client command
            handle_list_client();

        else if(strstr(buffer, "remove")) // Remove command
            handle_remove(sockfd, buffer);
        
        else if(strstr(buffer, "exit")) // Exit command
            break;
    }

    // Sends exit request
    strcpy(buffer, "exit");
    send_msg(sockfd, buffer);
        
    close(sockfd);
    close(server_sync_sockfd);
        
    return 0;
}
