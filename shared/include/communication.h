#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/stat.h>

#define MESSAGE_SIZE 256

void reverse(char s[]);
void itoa(int n, char s[]);
int send_msg(int sockfd, char buffer[MESSAGE_SIZE]);
int receive_msg(int sockfd, char buffer[MESSAGE_SIZE]);
int send_file(int sockfd, FILE* fp, unsigned int file_size);
int receive_file(int sockfd, FILE* fp, unsigned int file_size);