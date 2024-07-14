#include <pthread.h>


typedef struct thread_list
{
    pthread_t thread;
    struct thread_list* next;
} THREAD_LIST;


THREAD_LIST* create_thread_list(void);
THREAD_LIST* add_to_thread_list(THREAD_LIST* list);
pthread_t get_last_thread(THREAD_LIST* list);