#include <stdio.h>
#include <stdlib.h> 
#include <getopt.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>

#include "SortedList.h"

int threadNum = 1;
int iterNum = 1;
int opt_sync = 0;
int opt_yield = 0;

static pthread_mutex_t mutexLock = PTHREAD_MUTEX_INITIALIZER;
static int spinLock = 0;

SortedList_t *list;
SortedListElement_t *elements;
pthread_t *threads;
int *count;

void freeMe(){
    free(list);
    free(elements);
    free(threads);
    free(count);
}

void handler(int signal){
    if (signal == SIGSEGV){
        fprintf(stderr, "ERROR: Segmentation fault: %s\n", strerror(signal));
        freeMe();
        exit(0);
    }
}

void secondStep(int n){
    SortedListElement_t *tempE = SortedList_lookup(list, elements[n].key);
    if (tempE == NULL){
        fprintf(stderr, "ERROR: SortedList_lookup could not find previously inserted element\n");
        freeMe();
        exit(2);
    }
    if (SortedList_delete(tempE) == 1){
        fprintf(stderr, "ERROR: SortedList_delete detected corrupted list\n");
        freeMe();
        exit(2);
    }
}

void* threadFun(void *eNum){
    int start = *((int*)eNum);
    int total = iterNum*threadNum;
    for (int n = start; n < total; n += threadNum){
        switch(opt_sync){
            case 0:
                SortedList_insert(list, &elements[n]);
                break;
            case 1:
                pthread_mutex_lock(&mutexLock);
                SortedList_insert(list, &elements[n]);
                pthread_mutex_unlock(&mutexLock);
                break;
            case 2:
                while (__sync_lock_test_and_set(&spinLock, 1)){
                    continue;
                }
                SortedList_insert(list, &elements[n]);
                __sync_lock_release(&spinLock);
                break;
        }
    }
    
    int temp = 0;
    switch(opt_sync){
        case 0:
            temp = SortedList_length(list);
            break;
        case 1:
            pthread_mutex_lock(&mutexLock);
            temp = SortedList_length(list);
            pthread_mutex_unlock(&mutexLock);
            break;
        case 2:
            while (__sync_lock_test_and_set(&spinLock, 1)){
                continue;
            }
            temp = SortedList_length(list);
            __sync_lock_release(&spinLock);
            break;
    }
    
    if(temp == -1){
        fprintf(stderr, "ERROR: SortedList_length detected corrupted list\n");
        freeMe();
        exit(2);
    }

    for (int n = start; n < total; n += threadNum){
        switch(opt_sync){
            case 0:
                secondStep(n);
                break;
            case 1:
                pthread_mutex_lock(&mutexLock);
                secondStep(n);
                pthread_mutex_unlock(&mutexLock);
                break;
            case 2:
                while (__sync_lock_test_and_set(&spinLock, 1)){
                    continue;
                }
                secondStep(n);
                __sync_lock_release(&spinLock);
                break;
        }
    }
    return NULL;
}

int main(int argc, char ** argv) {
    struct option long_opts[] = {
        {"threads",     required_argument,  NULL,   't'},
        {"iterations",  required_argument,  NULL,   'i'},
        {"yield",       required_argument,  NULL,   'y'},
        {"sync",        required_argument,  NULL,   's'},
        {0,             0,                  0,      0}
    };
    
    int ret = 0;
    int length = 0;
    
    while (1) {
        ret = getopt_long (argc, argv, "i:st:y:", long_opts, NULL);
        if (ret < 0){
            break;
        }
        switch (ret)
        {
            case 'i':
                iterNum = atoi(optarg);
                break;
            case 't':
                threadNum = atoi(optarg);
                break;
            case 's':
                if (strlen(optarg) != 1){
                    fprintf(stderr, "ERROR: Invalid --sync argument length. Must be 1 character\n");
                    exit(1);
                }
                switch(optarg[0]){
                    case 'm':
                        opt_sync = 1;
                        break;
                    case 's':
                        opt_sync = 2;
                        break;
                    default:
                        fprintf(stderr, "ERROR: Invalid --sync argument. Valid arguments are --sync=[ms]\n");
                        exit(1);
                }
                break;
            case 'y':
                length = (int)strlen(optarg);
                for (int n = 0; n < length; n++){
                    if (optarg[n] == 'i'){
                        opt_yield |= 0x01;
                    } else if (optarg[n] == 'd'){
                        opt_yield |= 0x02;
                    } else if (optarg[n] == 'l'){
                        opt_yield |= 0x04;
                    } else {
                        fprintf(stderr, "ERROR: Invalid --yield argument\n");
                        exit(1);
                    }
                }
                break;
            default:
                fprintf(stderr, "ERROR: Invalid argument. Valid arguments are --threads=#, --iterations=#, --yield=[idl]\n");
                exit(1);
        }
    }
    
    signal(SIGSEGV, handler);
    
    list = malloc(sizeof(SortedList_t));
    if (list == NULL){
        fprintf(stderr, "ERROR: malloc of list failed\n");
        exit(1);
    }
    list->next = list;
    list->prev = list;
    list->key = NULL;
    
    int totalE = iterNum * threadNum;
    elements = malloc(totalE * sizeof(SortedListElement_t));
    if (elements == NULL){
        fprintf(stderr, "ERROR: malloc of elements failed\n");
        free(list);
        exit(1);
    }
    
    for (int n = 0; n < totalE; n++){
        char *key = malloc( 4 * sizeof(char));
        for (int n = 0; n < 3; n++){
            key[n] = (rand() % 26) + 'A';
        }
        key[3] = '\0';
        elements[n].key = key;
    }
    
    struct timespec beginTime;
    clock_gettime(CLOCK_MONOTONIC, &beginTime);
    
    threads = malloc(threadNum * sizeof(pthread_t));
    if (threads == NULL){
        fprintf(stderr, "ERROR: malloc of threads failed\n");
        free(list);
        free(elements);
        exit(1);
    }
    
    count = malloc(threadNum * sizeof(int));
    if (count == NULL){
        fprintf(stderr, "ERROR: malloc of count failed\n");
        free(list);
        free(elements);
        free(threads);
        exit(1);
    }
    
    for (int n = 0; n < threadNum; n++){
        count[n] = n;
        if(pthread_create(&threads[n], NULL, threadFun, &count[n]) != 0){
            fprintf(stderr, "ERROR: pthread_create failed\n");
            freeMe();
            exit(1);
        }
    }
    
    for (int n = 0; n < threadNum; n++){
        if(pthread_join(threads[n], NULL) != 0){
            fprintf(stderr, "ERROR: pthread_join failed\n");
            freeMe();
            exit(1);
        }
    }
    
    struct timespec finishTime;
    clock_gettime(CLOCK_MONOTONIC, &finishTime);
    
    int finalLength = SortedList_length(list);
    if (finalLength > 0){
        fprintf(stderr, "ERROR: final length of list was not 0\n");
        freeMe();
        exit(2);
    }
    if (finalLength == -1){
        fprintf(stderr, "ERROR: SortedList_length() detected corrupted list\n");
        freeMe();
        exit(2);
    }

    printf("list-");
    int check = 0;
    if (opt_yield & 0x01){
        printf("i");
        check = 1;
    }
    if (opt_yield & 0x02){
        printf("d");
        check = 1;
    }
    if (opt_yield & 0x04){
        printf("l");
        check = 1;
    }
    if (check == 0){
        printf("none");
    }
    
    long long totalTime = 1000000000*   (finishTime.tv_sec -  beginTime.tv_sec) +
                                        (finishTime.tv_nsec - beginTime.tv_nsec);
    
    long long operationNum = totalE * 3;
    long long averageTime = totalTime/operationNum;

    switch(opt_sync){
        case 1:
            printf("-m");
            break;
        case 2:
            printf("-s");
            break;
        default:
            printf("-none");
    }
    
    printf(",%d,%d,1,%lld,%lld,%lld\n", threadNum, iterNum, operationNum, totalTime, averageTime);
    
    freeMe();
    exit(0);
}
list-il-none,4,16,1,192,49274940,256640
list-dl-none,4,16,1,192,98445424,512736
list-dl-none,8,16,1,384,117425833,305796
list-dl-none,2,32,1,192,49884909,259817
list-dl-none,4,32,1,384,61046253,158974
list-dl-none,8,32,1,768,84291968,109755
list-dl-none,12,32,1,1152,148083032,128544
list-none-m,16,1000,1,48000,701865971,14622
list-none-m,24,1000,1,72000,6194032483,86028
