#include <stdlib.h>
#include "../include/thread_list.h"




THREAD_LIST* create_thread_list(void)
{
    THREAD_LIST* new_list = (THREAD_LIST*) malloc(sizeof(THREAD_LIST));

    new_list->next = NULL;

    return new_list;
}

THREAD_LIST* add_to_thread_list(THREAD_LIST* list)
{
    THREAD_LIST* aux;

    THREAD_LIST* new_list_node = (THREAD_LIST*) malloc(sizeof(THREAD_LIST));
    new_list_node->next = NULL;

    if (!list)
        return NULL;

    for(aux = list; aux->next; aux = aux->next);

    aux->next = new_list_node;

    return list;
}

pthread_t get_last_thread(THREAD_LIST* list)
{
    THREAD_LIST* aux;
    
    if (!list)
        return -1;

    for(aux = list; aux->next; aux = aux->next);

    return aux->thread;
}