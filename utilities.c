#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

void writeToCSV(double *array, int producers, int consumers, int elements)
{
    FILE *filepointer;
    char *filename = (char *)malloc(6 * sizeof(char));
    sprintf(filename, "%dP%dC%dE.csv", producers, consumers, elements);
    filepointer = fopen(filename, "w"); //create a file
    for (int i = 0; i < elements; i++)
    {
        fprintf(filepointer, "%.6f", array[i]); //write each pixel value
        fprintf(filepointer, "\n");
    }
}

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

