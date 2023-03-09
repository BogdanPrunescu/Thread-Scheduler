#ifndef HEAP_H_
#define HEAP_H_

#include "so_scheduler.h"
#include <semaphore.h>
#include <stdlib.h>

typedef struct {

    pthread_t tid; // thread id
    unsigned int priority;
    int is_waiting; // is 1 if thread is in waiting state
    unsigned int time_left; // time left if thread is in running state
    so_handler *func; // function that needs to be run by thread

    sem_t thread_sem; // thread's semaphore

} thread_t;

typedef struct Queue_t {

    thread_t *data;

    struct Queue_t *next;
} Queue_t;

Queue_t *new_thread(thread_t *thread);

void push(Queue_t **head, thread_t *thread);

void pop(Queue_t **head);

thread_t *peek(Queue_t **head);

void free_heap(Queue_t **head);

#endif
