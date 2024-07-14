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

#define PORT 4000
#define SERVER_SYNC_PORT 4001
#define USERNAME_MAX_SIZE 32

pthread_mutex_t lock, user_feedback_lock, user_input_listener_lock;
char user_input[256];
int new_input_notification = 0;
char sync_dir_name[256];


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

/* reverse:  reverse string s in place */
 void reverse(char s[])
 {
     int i, j;
     char c;

     for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
         c = s[i];
         s[i] = s[j];
         s[j] = c;
     }
}  

// 
void itoa(int n, char s[])
 {
     int i, sign;

     if ((sign = n) < 0)  /* record sign */
         n = -n;          /* make n positive */
     i = 0;
     do {       /* generate digits in reverse order */
         s[i++] = n % 10 + '0';   /* get next digit */
     } while ((n /= 10) > 0);     /* delete it */
     if (sign < 0)
         s[i++] = '-';
     s[i] = '\0';
     reverse(s);
}  

void my_write_read_for_files(int sockfd, char action, FILE* fp, long int file_size)
{
  int bytes_read_or_write = 0;
  int total_bytes_read_or_write = 0;
  char* temp_buffer;
  printf("Inside write/read for files\n");
  printf("%c\n", action);

  temp_buffer = (char*) malloc(file_size);
  
  if (action == 'w')
  {
      fread(temp_buffer, 1, file_size, fp);

      while(total_bytes_read_or_write < file_size)
      {
        bytes_read_or_write = write(sockfd, temp_buffer, file_size - total_bytes_read_or_write);
        printf("Bytes written: %d\n", bytes_read_or_write);
        if (bytes_read_or_write < 0)
        {
          fprintf(stderr, "ERROR writing.\n");
          exit(1);
        }

        total_bytes_read_or_write += bytes_read_or_write;
      }
  }
  else 
  {
      while(total_bytes_read_or_write < file_size)
      {
        bytes_read_or_write = read(sockfd, temp_buffer, file_size - total_bytes_read_or_write);
        printf("Bytes written: %d\n", bytes_read_or_write);
        if (bytes_read_or_write < 0)
        {
          fprintf(stderr, "ERROR writing.\n");
          exit(1);
        }

        total_bytes_read_or_write += bytes_read_or_write;
      }

	  printf("Estou aqui\n");
	  printf("%s\n", temp_buffer);
	  //fwrite(temp_buffer, 1, file_size, fp);
    printf("Fwrote: %ld\n", fwrite(temp_buffer, 1, file_size, fp));
  }

  printf("Estou aqui agr\n");
  free(temp_buffer);
}


void my_write_read(int sockfd, char buffer[256], char action)
{
  int bytes_read_or_write = 0;
  int total_bytes_read_or_write = 0;
  printf("%c\n", action);
  printf("Sending or reading: %s\n", buffer);
  
  if (action == 'w')
  {
    while(total_bytes_read_or_write < 256)
    {
      bytes_read_or_write = write(sockfd, buffer, 256 - total_bytes_read_or_write);
      printf("Bytes written: %d\n", bytes_read_or_write);
      if (bytes_read_or_write < 0)
      {
        fprintf(stderr, "ERROR writing.\n");
        exit(1);
      }

      total_bytes_read_or_write += bytes_read_or_write;
    }
  }
  else
  {
    bzero(buffer, 256);

    while(total_bytes_read_or_write < 256)
    {
      bytes_read_or_write = read(sockfd, buffer, 256 - total_bytes_read_or_write);
      printf("Bytes read: %d\n", bytes_read_or_write);
      if (bytes_read_or_write < 0)
      {
        fprintf(stderr, "ERROR reading.\n");
        exit(1);
      }

      total_bytes_read_or_write += bytes_read_or_write;
    }
  }
}

void* server_sync_handler(void* args)
{
  int server_sync_sockfd = *(int*) args;

  DIR *dp;
  struct dirent *ep;   
  dp = opendir (sync_dir_name);
  char synch_buffer[256];

  printf("\n\n\n\n\n");

  strcpy(synch_buffer, "Synch request");


  if(!dp)
  {
    perror ("Couldn't open the directory");
    return 0;
  }

  while ((ep = readdir (dp)))
    if (strcmp(ep->d_name, ".") == 0 || strcmp(ep->d_name, "..") == 0)
    {
      // Do nothing
    }
    else
      printf("%s ", ep->d_name);
        
  printf("\n");
  
  

  while(1)
  {
    // Server Sync
    my_write_read(server_sync_sockfd, synch_buffer, 'r');
    printf("%s\n", synch_buffer);
  }

  (void) closedir (dp);
  return 0;
}

void handle_list_client(char username[256])
{
  DIR *dp;
  struct dirent *ep;   
  dp = opendir (sync_dir_name);
  struct stat *st;


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
      st = (struct stat*) malloc(sizeof(struct stat));
      stat(ep->d_name, st);
      printf("%s\n", ep->d_name);
      printf("File access time %s", ctime(&st->st_atime));
      printf("File modify time %s", ctime(&st->st_mtime));
      printf("File changed time %s", ctime(&st->st_ctime));
      printf("\n");
      free(st);
    }
}

void get_sync_dir(char username[256], int* server_sync_sockfd)
{
  struct stat st = {0};
  pthread_t server_sync_thread;

  bzero(sync_dir_name, 256);
  strcpy(sync_dir_name, "sync_dir_");
  strcat(sync_dir_name, username);

  // If user sync dir doesnt exist, create one
	if (stat(sync_dir_name, &st) == -1)
	{
		mkdir(sync_dir_name, 0700);
	}

  printf("%s\n", sync_dir_name);
  //pthread_create(&server_sync_thread, NULL, server_sync_handler, server_sync_sockfd);
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

  my_write_read(sockfd, buffer, 'w'); // Send upload request        
  
  // Send file name
  strcpy( buffer, strrchr(file_path, '/') );
  printf("%s\n", &(buffer[1]));
  my_write_read(sockfd, &(buffer[1]), 'w');

  // Send file size
  itoa(file_size, buffer);        
  printf("%s\n", buffer);        
  my_write_read(sockfd, buffer, 'w');

  // Send file data
  my_write_read_for_files(sockfd, 'w', fp, file_size);

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

  my_write_read(sockfd, buffer, 'w'); // Sends download request

  my_write_read(sockfd, file_name, 'w'); // Sends file name
  my_write_read(sockfd, buffer, 'r'); // Gets file size

  if (strcmp(buffer, "0") == 0)
  {
    fclose(fp);
    printf("Invalid file name\n");
    return;
  }

  my_write_read_for_files(sockfd, 'r', fp, atoi(buffer)); // Gets file data

  fclose(fp);
  return;
}

int main(int argc, char *argv[])
{
  int sockfd, server_sync_sockfd;
  socklen_t clilen;
  struct sockaddr_in serv_addr, serv_sync_addr;
  struct hostent *server;
  char buffer[256];
  pthread_t user_input_listener_thread;

	
  if (argc < 3) {
		fprintf(stderr,"usage %s username hostname\n", argv[0]);
		exit(0);
  }
	
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
	serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
	bzero(&(serv_addr.sin_zero), 8);

  serv_sync_addr.sin_family = AF_INET;     
	serv_sync_addr.sin_port = htons(SERVER_SYNC_PORT);    
	serv_sync_addr.sin_addr = *((struct in_addr *)server->h_addr);
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
  my_write_read(sockfd, buffer, 'w');
  my_write_read(server_sync_sockfd, "Synch request", 'w');

  // check for OK session
  my_write_read(sockfd, buffer, 'r');
  printf("Buffer: %s\n", buffer);
  
  // If connection failed, return
  if(strcmp(buffer, "exit") == 0)
  {
      close(sockfd);
      close(server_sync_sockfd);
      return 0;
  }

  get_sync_dir(argv[1], &server_sync_sockfd);

  // Create user input listener thread
  pthread_create(&user_input_listener_thread, NULL, user_input_handler, NULL);

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
    if(strstr(buffer, "upload"))  // Upload command
      handle_upload(sockfd, buffer);
    if(strstr(buffer, "download"))  // Download command
      handle_download(sockfd, buffer);
    if(strstr(buffer, "list_client"))  // List client command
      handle_list_client(argv[1]);
    
    else if(strstr(buffer, "exit"))  // Exit command
      break;
  } 
     
	close(sockfd);
  close(server_sync_sockfd);
      
  return 0;    
}
