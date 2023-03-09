#include "so_scheduler.h"
#include "queue.h"
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>

#define MAX_SIZE 1000

typedef struct {

    int is_initiated; // is 1 if the scheduler is initialized
    unsigned int time_quantum; // max time for a thread in running state
    unsigned int io; // max io events avaliable
    thread_t *running_thread; // pointer to the running thread

    Queue_t *ready_threads; // priority queue

    thread_t **threads; // array for all the threads created
    unsigned int threads_no; // number of threads created

    thread_t ***waiting_io_sem; // array of lists for all the threads in
                                // waiting state

    unsigned int *io_sem_no; // array to keep track of number of threads
                             // waiting for an io event

} scheduler_t;

static scheduler_t my_sched;

void start_thread(void *args);
static void schedule(void);

int so_init(unsigned int time_quantum, unsigned int io) {

    // check for valid parameters
    if (my_sched.is_initiated || time_quantum < 1 || io > SO_MAX_NUM_EVENTS)
        return -1;

    // init scheduler
    my_sched.is_initiated = 1;
    my_sched.time_quantum = time_quantum;
    my_sched.io = io;
    my_sched.ready_threads = NULL;
    my_sched.threads = malloc(sizeof(thread_t *) * MAX_SIZE);
    my_sched.threads_no = 0;
    my_sched.waiting_io_sem = malloc(sizeof(thread_t **) * (io + 5));

    for (int i = 0; i <= io; i++)
        my_sched.waiting_io_sem[i] = malloc(sizeof(thread_t *) * 1000);

    my_sched.io_sem_no = calloc(io + 5, sizeof(unsigned int));
    return 0;
}

tid_t so_fork(so_handler *func, unsigned int priority) {

    if (func == 0 || priority > SO_MAX_PRIO)
        return INVALID_TID;

    thread_t *thread = malloc(sizeof(thread_t));

    // initialize thread
    thread->tid = 0;
    thread->priority = priority;
    thread->time_left = my_sched.time_quantum;
    thread->func = func;
    thread->is_waiting = 0;
    sem_init(&thread->thread_sem, 0, 0);

    // create thread
    pthread_create(&thread->tid ,NULL ,(void *) start_thread ,(void *) thread);

    my_sched.threads[my_sched.threads_no++] = thread;

    // add thread into queue
    push(&my_sched.ready_threads, thread);

    // if there is no thread running run the scheduler
    if (my_sched.running_thread == NULL)
        schedule();
    // comsume one operation from origin thread
    else
        so_exec();

    return thread->tid;
    
}

int so_wait(unsigned int io) {

    // check for valid parameters
    if (io >= my_sched.io)
        return -1;

    // add thread into io's waiting array
    my_sched.waiting_io_sem[io][my_sched.io_sem_no[io]++] = 
        my_sched.running_thread;

    my_sched.running_thread->is_waiting = 1;

    // consume time
    so_exec();

    return 0;
    
}

int so_signal(unsigned int io) {

    // check for valid parameters
    if (io >= my_sched.io)
        return -1;

    // add all threads into queue, making them ready
    for (int i = 0; i < my_sched.io_sem_no[io]; i++) {
        my_sched.waiting_io_sem[io][i]->is_waiting = 0;
        push(&my_sched.ready_threads, my_sched.waiting_io_sem[io][i]);
    }

    unsigned int nr_thread_awaken = my_sched.io_sem_no[io];

    my_sched.io_sem_no[io] = 0;

    // consume time
    so_exec();

    return nr_thread_awaken;

}

void so_exec(void) {

    my_sched.running_thread->time_left--;
    schedule();

}

void so_end(void) {

    if (my_sched.is_initiated == 0)
        return;

    // free all threads
    for (int i = 0; i < my_sched.threads_no; i++) {
        pthread_join(my_sched.threads[i]->tid, NULL);
        sem_destroy(&my_sched.threads[i]->thread_sem);
        free(my_sched.threads[i]);
    }

    if (my_sched.threads != NULL)
        free(my_sched.threads);

    // free queue
    free_heap(&my_sched.ready_threads);

    // free semaphores
    for (int i = 0; i <= my_sched.io; i++)
        free(my_sched.waiting_io_sem[i]);

    free(my_sched.waiting_io_sem);
    free(my_sched.io_sem_no);

    my_sched.is_initiated = 0;

}

void start_thread(void *args) {

    thread_t *thread = (thread_t *) args;

    // wait until thread is in running state
    sem_wait(&thread->thread_sem);

    thread->func(thread->priority);

    // thread reached terminated state
    my_sched.running_thread = NULL;
    schedule();

}

static void schedule(void) {

    thread_t *next_thread = peek(&my_sched.ready_threads);

    // if there is no running thread run the first thread inside queue
    if (my_sched.running_thread == NULL && next_thread != NULL) {

        my_sched.running_thread = next_thread;
        my_sched.running_thread->time_left = my_sched.time_quantum;

        pop(&my_sched.ready_threads);
        sem_post(&my_sched.running_thread->thread_sem);
        return;
    }

    //change running thread if he is in waiting state
    if (my_sched.running_thread != NULL
        && my_sched.running_thread->is_waiting == 1) {
        if (next_thread) {

            thread_t *paused_thread = my_sched.running_thread;

            my_sched.running_thread = next_thread;
            my_sched.running_thread->time_left = my_sched.time_quantum;

            // take the new thread out of queue
            pop(&my_sched.ready_threads);

            // increment thread's semaphore to continue thread's execution
            sem_post(&my_sched.running_thread->thread_sem);

            // pause thread that was previously running
            sem_wait(&paused_thread->thread_sem);
            return;
        }
    }

    // change running thread if a higher-prio thread has appeared
    if (next_thread
        && next_thread->priority > my_sched.running_thread->priority) {

            thread_t *paused_thread = my_sched.running_thread;
            my_sched.running_thread = next_thread;
            my_sched.running_thread->time_left = my_sched.time_quantum;

            pop(&my_sched.ready_threads);

            // add in queue paused_thread
            push(&my_sched.ready_threads, paused_thread);

            sem_post(&my_sched.running_thread->thread_sem);

            // pause thread that was previously running
            sem_wait(&paused_thread->thread_sem);
            return;
    }

    // is running thread has no time left 
    if (my_sched.running_thread && my_sched.running_thread->time_left == 0) {

        // change thread to a higher or equal prio thread
        if (next_thread &&
            next_thread->priority >= my_sched.running_thread->priority) {

            thread_t *paused_thread = my_sched.running_thread;
            my_sched.running_thread = next_thread;
            my_sched.running_thread->time_left = my_sched.time_quantum;

            pop(&my_sched.ready_threads);
            // add in queue paused_thread
            push(&my_sched.ready_threads, paused_thread);

            sem_post(&my_sched.running_thread->thread_sem);

            // pause thread that was running
            sem_wait(&paused_thread->thread_sem);
            return;

        // reset running thread time
        } else {
            my_sched.running_thread->time_left = my_sched.time_quantum;
            return;
        }
    }
}
