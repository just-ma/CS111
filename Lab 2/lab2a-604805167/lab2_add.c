#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

int threadNum = 1;
int iterNum = 1;
long long counter = 0;
int opt_yield = 0;
int opt_sync = 0;

pthread_mutex_t mutexLock = PTHREAD_MUTEX_INITIALIZER;
int spinLock = 0;
int csLock = 0;

void add(long long *pointer, long long value) {
    long long sum = *pointer + value;
    if (opt_yield)
        sched_yield();
    *pointer = sum;
}

void* threadFun(){
    for (int n = 0; n < iterNum; n++){
        switch(opt_sync){
            case 0:
                add (&counter, 1);
                break;
            case 1:
                pthread_mutex_lock(&mutexLock);
                add (&counter, 1);
                pthread_mutex_unlock(&mutexLock);
                break;
            case 2:
                while (__sync_lock_test_and_set(&spinLock, 1)){
                    continue;
                }
                add (&counter, 1);
                __sync_lock_release(&spinLock);
                break;
            case 3:
                while(__sync_val_compare_and_swap(&csLock, 0, 1) == 1){
                    continue;
                }
                if (opt_yield)
                    sched_yield();
                long long sum = counter + 1;
                counter = sum;
                __sync_bool_compare_and_swap(&csLock, 1, 0);
        }
    }
    
    for (int n = 0; n < iterNum; n++){
        switch(opt_sync){
            case 0:
                add (&counter, -1);
                break;
            case 1:
                pthread_mutex_lock(&mutexLock);
                add (&counter, -1);
                pthread_mutex_unlock(&mutexLock);
                break;
            case 2:
                while (__sync_lock_test_and_set(&spinLock, 1)){
                    continue;
                }
                add (&counter, -1);
                __sync_lock_release(&spinLock);
                break;
            case 3:
                while(__sync_val_compare_and_swap(&csLock, 0, 1) == 1){
                    continue;
                }
                if (opt_yield)
                    sched_yield();
                long long sum = counter - 1;
                counter = sum;
                __sync_bool_compare_and_swap(&csLock, 1, 0);
        }
    }
    return NULL;
}

int main(int argc, char** argv)
{
    struct option long_opts[] = {
        {"threads",     required_argument,  NULL,   't'},
        {"iterations",  required_argument,  NULL,   'i'},
        {"yield",       no_argument,        NULL,   'y'},
        {"sync",        required_argument,  NULL,   's'},
        {0,             0,                  0,      0}
    };
    
    int ret = 0;
    
    while (1) {
        ret = getopt_long (argc, argv, "i:s:t:y", long_opts, NULL);
        if (ret < 0){
            break;
        }
        switch (ret)
        {
            case 'i':
                iterNum = atoi(optarg);
                break;
            case 's':
                switch(*optarg){
                    case 'm':
                        opt_sync = 1;
                        break;
                    case 's':
                        opt_sync = 2;
                        break;
                    case 'c':
                        opt_sync = 3;
                        break;
                    default:
                        fprintf(stderr, "ERROR: Invalid --sync argument. Valid arguments are --sync=[msc]\n");
                        exit(1);
                }
                break;
            case 't':
                threadNum = atoi(optarg);
                break;
            case 'y':
                opt_yield = 1;
                break;
            default:
                fprintf(stderr, "ERROR: Invalid argument. Valid arguments are --threads=#, --iterations=#, --yield=\n");
                exit(1);
        }
    }
    
    struct timespec beginTime;
    clock_gettime(CLOCK_MONOTONIC, &beginTime);
    
    pthread_t *threads = malloc(threadNum * sizeof(pthread_t));
    if (threads == NULL){
        fprintf(stderr, "ERROR: malloc of threads failed\n");
        exit(1);
    }
    
    for (int n = 0; n < threadNum; n++){
        if(pthread_create(&threads[n], NULL, threadFun, NULL) != 0){
            fprintf(stderr, "ERROR: pthread_create failed\n");
            free(threads);
            exit(1);
        }
    }
    
    for (int n = 0; n < threadNum; n++){
        if(pthread_join(threads[n], NULL) != 0){
            fprintf(stderr, "ERROR: pthread_join failed\n");
            free(threads);
            exit(1);
        }
    }
    
    struct timespec finishTime;
    clock_gettime(CLOCK_MONOTONIC, &finishTime);
    
    long long totalOps = iterNum * threadNum * 2;
    long long totalTime = 1000000000*   (finishTime.tv_sec -  beginTime.tv_sec) +
                                        (finishTime.tv_nsec - beginTime.tv_nsec);
    long long averageTime = totalTime/totalOps;
    
    printf("add");
    if (opt_yield)
        printf("-yield");
    if (opt_sync == 1)
        printf("-m");
    else if (opt_sync == 2)
        printf("-s");
    else if (opt_sync == 3)
        printf("-c");
    else
        printf("-none");
    
    printf(",%d,%d,%lld,%lld,%lld,%lld\n", threadNum, iterNum, totalOps, totalTime, averageTime, counter);
    
    free (threads);
}
