#include "../include/inotify.h"
#include "../../shared/include/definitions.h"
#include "../../shared/include/communication.h"

#define EVENT_SIZE (100 * (sizeof(struct inotify_event) + FILE_NAME_MAX_SIZE + 1))

void *listen_inotify(void *args)
{
    char dirpath[FILE_PATH_MAX_SIZE + 1];
    strcpy(dirpath, (char *)args);
    struct inotify_event *pevent;
    char event_buffer[EVENT_SIZE];
    char *p;
    int error = IN_NOTIFY_ERROR;

    int fd = inotify_init();
    if (fd < 0)
    {
        perror("inotify_init"); /* errno é setado */
        pthread_exit(&error);
    }

    int wd = inotify_add_watch(fd, dirpath, IN_CREATE | IN_MODIFY | IN_DELETE | IN_MOVE);

    if (wd < 0)
    {
        perror("inotify_add_watch");
        pthread_exit(&error);
    }

    while (1)
    {
        ssize_t r = read(fd, event_buffer, EVENT_SIZE);
        if (r <= 0)
        {
            pthread_exit(&error);
        }
        printf("preso no while\n");
            
        for (p = event_buffer; p < event_buffer + r; p += sizeof(struct inotify_event*) + pevent->len)
        {
            pevent = (struct inotify_event *)p;
            
            if (pevent->mask & IN_MOVE || pevent->mask & IN_DELETE)
            {
                printf("in delete\n");
                printf("%s\n", pevent->name);
            }
            
            if (pevent->mask & IN_CREATE || pevent->mask & IN_MODIFY)
            {
                printf("in create\n");
                printf("%s\n", pevent->name);
            }
        }
    }
}