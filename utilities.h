
#ifndef UTILITIES_H
#define UTILITIES_H
#include <sys/time.h>

double toc(struct timeval begin);

struct timeval tic();

void writeToCSV(double *array, int producers, int consumers, int elements);

#endif