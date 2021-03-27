#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include "utilities.h"
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>

#define _GNU_SOURCE
#define QUEUESIZE 10
#define ELEMENTS 20
#define P 1
#define Q 4

int elementsLeft = ELEMENTS;
int elementsAdded = 0;
double times[ELEMENTS];

void *producer(void *args);
void *consumer(void *args);

typedef struct
{
    void *(*work)(void *);
    void *arg;
} workFunction;

typedef struct
{
    workFunction *buf[QUEUESIZE];
    long head, tail;
    int full, empty;
    pthread_mutex_t *mut;
    pthread_cond_t *notFull, *notEmpty;
} queue;

typedef struct
{
    int functionArgument;
    struct timeval tv;
} argument;

queue *initializeQueue(void);

void deleteQueue(queue *q);

void queueAdd(queue *q, workFunction *in);

void queueDelete(queue *q, workFunction *out);

void *doWork(void *arg)
{
    int argument = *((int *)arg);
    double result = 0;
    for (int i = 0; i < 10000; i++)
        result += argument * argument + cos(argument);
    printf("Result is %f with arguement %d\n", result, argument);
}

int main()
{
    queue *fifo;
    pthread_t pro[P], con[Q];

    fifo = initializeQueue();
    if (fifo == NULL)
    {
        fprintf(stderr, "main: Queue Init failed.\n");
        exit(1);
    }

    for (int i = 0; i < P; i++)
        pthread_create(&pro[i], NULL, producer, fifo);
    for (int i = 0; i < Q; i++)
        pthread_create(&con[i], NULL, consumer, fifo);
    for (int i = 0; i < Q; i++)
        pthread_join(pro[i], NULL);
    for (int i = 0; i < P; i++)
        pthread_join(con[i], NULL);

    deleteQueue(fifo);
    printf("Deleted queue\n");
    for (int i = 0; i < ELEMENTS; i++)
        printf("Time %d is %f\n", i, times[i]);
    writeToCSV(times, P, Q, ELEMENTS);

    return 0;
}

void *producer(void *q)
{
    queue *fifo;
    int i;
    fifo = (queue *)q;
    int *array = (int *)malloc(ELEMENTS * sizeof(int));

    for (i = 0; i < ELEMENTS; i++)
        array[i] = i;

    workFunction *myStructs;
    argument *myArguments;
    myStructs = (workFunction *)malloc(ELEMENTS * sizeof(workFunction));
    myArguments = (argument *)malloc(ELEMENTS * sizeof(argument));

    for (i = 1; elementsAdded <= ELEMENTS; i++)
    {
        pthread_mutex_lock(fifo->mut);
        (myArguments + i)->functionArgument = array[elementsAdded];
        (myArguments + i)->tv = tic();
        (myStructs + i)->arg = (myArguments + i);
        (myStructs + i)->work = doWork;
        elementsAdded++;
        while (fifo->full)
        {
            printf("Producer: queue FULL.\n");
            pthread_cond_wait(fifo->notFull, fifo->mut);
        }
        queueAdd(fifo, (myStructs + i));
        pthread_mutex_unlock(fifo->mut);
        pthread_cond_signal(fifo->notEmpty);
    }
    return (NULL);
}

void *consumer(void *q)
{
    queue *fifo;
    fifo = (queue *)q;
    pid_t tid;
    tid = syscall(SYS_gettid);

    while (elementsLeft >0)
    {
        workFunction myStruct;
        argument *myArgument;

        pthread_mutex_lock(fifo->mut);
        
        while (fifo->empty)
        {
            printf("Consumer: queue EMPTY.(%d)\n", elementsLeft);
            printf(" waiting %d(%d)\n",tid,elementsLeft);
            pthread_cond_wait(fifo->notEmpty, fifo->mut);
            if(elementsLeft==0){
                fifo->empty=0;
                printf("**finished %d(%d)\n",tid,elementsLeft);
                pthread_cond_signal(fifo->notEmpty);
                return(NULL);
            }
        }

        queueDelete(fifo, &myStruct);
        elementsLeft--;
        pthread_cond_signal(fifo->notEmpty);
        if(elementsLeft==0){
            fifo->empty=0;
            printf("**finished %d(%d)\n",tid,elementsLeft);
            return(NULL);
        }
         pthread_mutex_unlock(fifo->mut);
        pthread_cond_signal(fifo->notFull);


        myArgument = myStruct.arg;
        int functionArg = myArgument->functionArgument;
        struct timeval start = myArgument->tv;
        double elapsedTime = toc(start);

        printf("Consumed %d\n",tid);
        //printf("Consumer(%d): ", elementsLeft);
        //(*myStruct.work)(&functionArg);
        //printf("Elapsed time for execution: %f sec\n", elapsedTime);

        times[functionArg] = elapsedTime;

    }

    printf("**finished %d(%d)\n",tid,elementsLeft);
    return (NULL);
}

queue *initializeQueue(void)
{
    queue *q;

    q = (queue *)malloc(sizeof(queue));
    if (q == NULL)
        return (NULL);

    q->empty = 1;
    q->full = 0;
    q->head = 0;
    q->tail = 0;
    q->mut = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(q->mut, NULL);
    q->notFull = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
    pthread_cond_init(q->notFull, NULL);
    q->notEmpty = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
    pthread_cond_init(q->notEmpty, NULL);

    return (q);
}

void deleteQueue(queue *q)
{
    pthread_mutex_destroy(q->mut);
    free(q->mut);
    pthread_cond_destroy(q->notFull);
    free(q->notFull);
    pthread_cond_destroy(q->notEmpty);
    free(q->notEmpty);
    free(q);
}

void queueAdd(queue *q, workFunction *in)
{
    q->buf[q->tail] = in;
    q->tail++;
    if (q->tail == QUEUESIZE)
        q->tail = 0;
    if (q->tail == q->head)
        q->full = 1;
    q->empty = 0;

    return;
}

void queueDelete(queue *q, workFunction *out)
{
    *out = *q->buf[q->head];
    q->head++;
    if (q->head == QUEUESIZE)
        q->head = 0;
    if (q->head == q->tail)
        q->empty = 1;
    q->full = 0;

    return;
}
