#include "../include/inotify.h"
#include "../../shared/include/definitions.h"
#include "../../shared/include/communication.h"

#define EVENT_SIZE (100 * (sizeof(struct inotify_event) + FILE_NAME_MAX_SIZE + 1))

extern void handle_upload(int sockfd, char buffer[MESSAGE_SIZE + 1]);
extern void handle_delete(int sockfd, char buffer[MESSAGE_SIZE + 1]);
extern pthread_mutex_t upload_lock;
extern pthread_mutex_t delete_lock;


void *listen_inotify(void *args)
{
    char dir_path[FILE_PATH_MAX_SIZE + 1];
    struct inotify_event *pevent;
    char event_buffer[EVENT_SIZE];
    char *p;
    int error = INOTIFY_ERROR;
    struct sync_dir_listener_struct my_sync_dir_listener_struct;
    int sockfd;
    char buffer[MESSAGE_SIZE + 1];
    char file_path[FILE_PATH_MAX_SIZE + 1];
    ssize_t r;


    my_sync_dir_listener_struct = *(struct sync_dir_listener_struct*) args;
    sockfd = my_sync_dir_listener_struct.sockfd;
    strcpy(dir_path, my_sync_dir_listener_struct.dir_path);

    int fd = inotify_init();
    if (fd < 0)
    {
        perror("inotify_init"); /* errno é setado */
        pthread_exit(&error);
    }

    int wd = inotify_add_watch(fd, dir_path, IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO/*| IN_MODIFY | IN_MOVE*/);

    if (wd < 0)
    {
        perror("inotify_add_watch");
        pthread_exit(&error);
    }

    while (1)
    {
        // Read next event
        bzero(event_buffer, EVENT_SIZE);
        r = read(fd, event_buffer, EVENT_SIZE);

        if (r <= 0)
        {
            pthread_exit(&error);
        }

        printf("An event has been read\n");
        printf("Read size: %ld\n", r);


        // Handle all events found    
        for (p = event_buffer; p < event_buffer + r;)
        {
            pevent = (struct inotify_event *)p;
            p += (sizeof(struct inotify_event) + pevent->len);
            
            if (pevent->mask & IN_MOVED_FROM || pevent->mask & IN_DELETE)
            {
                if(pevent->mask & IN_MOVED_FROM)
                    printf("in moved from\n");
                if(pevent->mask & IN_DELETE)
                    printf("in delete\n");

                strcpy(file_path, pevent->name);
                strcpy(buffer, "delete ");
                strcat(buffer, file_path);
                printf("%s\n", buffer);

                pthread_mutex_lock(&delete_lock);
                handle_delete(sockfd, buffer);
                pthread_mutex_unlock(&delete_lock);
            }
            
            if (pevent->mask & IN_CREATE || pevent->mask & IN_MOVED_TO)
            {
                if(pevent->mask & IN_CREATE)
                    printf("in create\n");
                if(pevent->mask & IN_MOVED_TO)
                    printf("in moved to\n");

                strcpy(file_path, dir_path);
                strcat(file_path, "/");
                strcat(file_path, pevent->name);

                strcpy(buffer, "upload ");
                strcat(buffer, file_path);

                printf("%s\n", buffer);

                pthread_mutex_lock(&upload_lock);
                handle_upload(sockfd, buffer);
                pthread_mutex_unlock(&upload_lock);
            }
        }
    }
}
