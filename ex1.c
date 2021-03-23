#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

#define QUEUESIZE 10
#define LOOP 20
#define P 1
#define Q 1

struct timeval tic()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv;
}

double toc(struct timeval begin)
{
    struct timeval end;
    gettimeofday(&end, NULL);
    double stime = ((double)(end.tv_sec - begin.tv_sec) * 1000) +
                   ((double)(end.tv_usec - begin.tv_usec) / 1000);
    stime = stime / 1000;
    return (stime);
}

// struct timeval tStart;
// tStart = tic();
// function
// time=toc(tStart);

void *producer(void *args);
void *consumer(void *args);

typedef struct
{
    void *(*work)(void *);
    void *arg;
} workFunction;

typedef struct {
    int functionArguement;
    struct timeval tv;
} arguement;

typedef struct
{
    workFunction *buf[QUEUESIZE];
    long head, tail;
    int full, empty;
    pthread_mutex_t *mut;
    pthread_cond_t *notFull, *notEmpty;
} queue;

queue *initializeQueue(void);
void deleteQueue(queue *q);
void queueAdd(queue *q, workFunction *in);
void queueDelete(queue *q, workFunction *out);

void *doWork(void *arg)
{
    int arguement = *((int *)arg);
    double result = 0;
    for (int i = 0; i < 1000; i++)
        result += arguement * arguement + cos(arguement);
    printf("Result is %f with arguement %d\n", result, arguement);
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
    for (int i = 0; i < P; i++)
        pthread_join(con[i], NULL);
    for (int i = 0; i < Q; i++)
        pthread_join(pro[i], NULL);
    deleteQueue(fifo);

    return 0;
}

void *producer(void *q)
{
    queue *fifo;
    int i;
    fifo = (queue *)q;
    int *array = (int *)malloc(LOOP * sizeof(int));
    for (i = 0; i < LOOP; i++)
        array[i] = i;

    workFunction *myStructs;
    arguement * myArguements;
    myStructs=(workFunction*)malloc(LOOP * sizeof (workFunction));
    myArguements=(arguement*)malloc(LOOP * sizeof (arguement));

    for (i = 0; i < LOOP; i++)
    {
        (myArguements+i)->functionArguement=array[i];
        (myArguements+i)->tv=tic();
        (myStructs+i)->arg =(myArguements+i);
        (myStructs+i)->work = doWork;
        // (myStructs+i)->arg = &array[i];
        pthread_mutex_lock(fifo->mut);
        while (fifo->full)
        {
            printf("Producer: queue FULL.\n");
            pthread_cond_wait(fifo->notFull, fifo->mut);
        }
        printf("Producer: loop %d\n", i);
        queueAdd(fifo, (myStructs+i));
        pthread_mutex_unlock(fifo->mut);
        pthread_cond_signal(fifo->notEmpty);
    }
    return (NULL);
}

void *consumer(void *q)
{
    queue *fifo;
    int i;
    fifo = (queue *)q;

    for (i = 0; i < LOOP; i++)
    {

        pthread_mutex_lock(fifo->mut);

        while (fifo->empty)
        {
            printf("Consumer: queue EMPTY.\n");
            pthread_cond_wait(fifo->notEmpty, fifo->mut);
        }
        printf("Consumer: loop %d\n", i);
        workFunction myStruct;
        queueDelete(fifo, &myStruct);
        printf("Consumer: ");
        (*myStruct.work)(myStruct.arg);
        pthread_mutex_unlock(fifo->mut);
        pthread_cond_signal(fifo->notFull);
    }

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