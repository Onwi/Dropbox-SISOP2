#include "../include/communication.h"


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

int send_msg(int sockfd, char buffer[MESSAGE_SIZE + 1])
{
    int bytes_written = 0;
    int total_bytes_written = 0;

    printf("Sending: %s\n", buffer);

    while(total_bytes_written < MESSAGE_SIZE)
    {
        bytes_written = write(sockfd, buffer, MESSAGE_SIZE - total_bytes_written);

        if(bytes_written < 0)
        {
            fprintf(stderr, "ERROR sending msg\n");
            return -1;
        }

        total_bytes_written += bytes_written;
    }

    printf("Message sent.\n");

    return 0;
}

int receive_msg(int sockfd, char buffer[MESSAGE_SIZE + 1])
{
    int bytes_read = 0;
    int total_bytes_read = 0;

    while(total_bytes_read < MESSAGE_SIZE)
    {
        bytes_read = read(sockfd, buffer, MESSAGE_SIZE - total_bytes_read);

        if(bytes_read < 0)
        {
            fprintf(stderr, "ERROR receiving msg\n");
            return -1;
        }

        total_bytes_read += bytes_read;
    }

    printf("Read: %s\n", buffer);

    return 0;
}

int send_file(int sockfd, FILE* fp, unsigned int file_size)
{
    int bytes_written = 0;
    int total_bytes_written = 0;
    int number_of_chunks;
    int chunk_remaining;
    char temp_buffer[MESSAGE_SIZE + 1];
    int i = 0;

    number_of_chunks = file_size / MESSAGE_SIZE;
    chunk_remaining = file_size % MESSAGE_SIZE;
    bzero(temp_buffer, MESSAGE_SIZE + 1);

    printf("Chunks: %d\n", number_of_chunks);
    printf("Remaining: %d\n", chunk_remaining);

    // Sends all complete chunks
    for(i = 0; i < number_of_chunks; i++)
    {
        total_bytes_written = 0;

        fread(temp_buffer, 1, MESSAGE_SIZE, fp);

        while(total_bytes_written < MESSAGE_SIZE)
        {
            bytes_written = write(sockfd, temp_buffer, MESSAGE_SIZE - total_bytes_written);

            printf("Bytes written: %d\n", bytes_written);

            if(bytes_written < 0)
            {
                fprintf(stderr, "ERROR sending file data\n");
                return -1;
            }

            total_bytes_written += bytes_written;
        }
    }

    fread(temp_buffer, 1, chunk_remaining, fp);

    // Sends last chunk
    total_bytes_written = 0;

    while(total_bytes_written < chunk_remaining)
    {
        bytes_written = write(sockfd, temp_buffer, chunk_remaining - total_bytes_written);

        printf("Bytes written: %d\n", bytes_written);

        if(bytes_written < 0)
        {
            fprintf(stderr, "ERROR sending file data\n");
            return -1;
        }

        total_bytes_written += bytes_written;
    }

    printf("File data sent.\n");

    return 0;
}

int receive_file(int sockfd, FILE* fp, unsigned int file_size)
{
    int bytes_read = 0;
    int total_bytes_read = 0;
    int number_of_chunks;
    int chunk_remaining;
    char temp_buffer[MESSAGE_SIZE + 1];
    int i = 0;

    number_of_chunks = file_size / MESSAGE_SIZE;
    chunk_remaining = file_size % MESSAGE_SIZE;
    bzero(temp_buffer, MESSAGE_SIZE + 1);

    printf("Chunks: %d\n", number_of_chunks);
    printf("Remaining: %d\n", chunk_remaining);    

    // Gets all complete chunks
    for(i = 0; i < number_of_chunks; i++)
    {
        total_bytes_read = 0;

        while(total_bytes_read < MESSAGE_SIZE)
        {
            bytes_read = read(sockfd, temp_buffer, MESSAGE_SIZE - total_bytes_read);

            printf("Bytes read: %d\n", bytes_read);

            if(bytes_read < 0)
            {
                fprintf(stderr, "ERROR sending file data\n");
                return -1;
            }

            total_bytes_read += bytes_read;
        }

        fwrite(temp_buffer, 1, MESSAGE_SIZE, fp);
    }

    // Gets last chunk
    total_bytes_read = 0;

    while(total_bytes_read < chunk_remaining)
    {
        bytes_read = read(sockfd, temp_buffer, chunk_remaining - total_bytes_read);

        printf("Bytes read: %d\n", bytes_read);

        if(bytes_read < 0)
        {
            fprintf(stderr, "ERROR sending file data\n");
            return -1;
        }

        total_bytes_read += bytes_read;
    }

    fwrite(temp_buffer, 1, chunk_remaining, fp);

    printf("File data received.\n");

    return 0;
}
