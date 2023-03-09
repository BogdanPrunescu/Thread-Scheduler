#include "queue.h"

Queue_t *new_thread(thread_t *thread) {

    Queue_t *tmp = (Queue_t *) malloc(sizeof(Queue_t));

    tmp->data = thread;
    tmp->next = NULL;

    return tmp;
}

void push(Queue_t **head, thread_t *thread) {

    Queue_t *start = (*head);

    Queue_t *tmp = new_thread(thread);

    if ((*head) == NULL) {
        (*head) = new_thread(thread);
        free(tmp);
        return;
    }

    if ((*head)->data->priority < thread->priority) {
        tmp->next = *head;
        (*head) = tmp;
    } else {
        while (start->next != NULL &&
            start->next->data->priority >= thread->priority) {
                start = start->next;
            }

        tmp->next = start->next;
        start->next = tmp;
    }

}

thread_t *peek(Queue_t **head) {
    if ((*head) != NULL)
        return (*head)->data;
    else
        return NULL;
}

void pop(Queue_t **head) {
    Queue_t *tmp = (*head);
    
    (*head) = (*head)->next;
    free(tmp);
}

void free_heap(Queue_t **head) {

    if ((*head) == NULL)
        return;

    while ((*head)->next != NULL) {
        Queue_t *tmp = (*head);

        free((*head));
        (*head)->next = tmp;
    }

}
