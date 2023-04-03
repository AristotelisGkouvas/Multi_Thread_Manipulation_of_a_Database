#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "../engine/db.h"

#define KSIZE (16)
#define VSIZE (1000)

#define LINE "+-----------------------------+----------------+------------------------------+-------------------+\n"
#define LINE1 "---------------------------------------------------------------------------------------------------\n"

struct data {                                                   // struct to pass argument on write test and read test.
    long int count;
    int thread_count;
    int r;
    DB* db;
};

pthread_mutex_t read_info;                                      // we use this to count all the read values.
pthread_mutex_t write_info;                                     // for random stats to print.

long int info;                                                  
long int random_w_ncount;										// variables for keeping stats of a random write thread. 
double random_w_cost;											
long int random_rd_ncount;										// variables for keeping stats of a random read thread. 
double random_rd_cost;
int random_rd_found;
int random_rd;                                                  // variables for keeping stats of a random read thread.
int random_w;                                                   // variables for keeping stats of a random write thread. 

long long get_ustime_sec(void);
void _random_key(char *key,int length);

void *_write_test(void *arg);
void *_read_test(void *arg);
