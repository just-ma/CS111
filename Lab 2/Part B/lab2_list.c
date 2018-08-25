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
int listNum = 1;
int opt_sync = 0;
int opt_yield = 0;

pthread_mutex_t *mutexLock;
int *spinLock;

SortedList_t **list;
SortedListElement_t *elements;
pthread_t *threads;
int *count;

long long* threadTimes;

void freeMe(){
    int n;
    for (n = 0; n < listNum; n++){
        free(list[n]);
    }
    free(list);
    free(elements);
    free(threadTimes);
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

int hash(const char* key) {
    unsigned int code = 5381;
    int i;
    for (i = 0; i < 3; i++) {
        code = ((code << 5) + code) + key[i];
    }
    return code;
}

void secondStep(int n, int bucket){
    SortedListElement_t *tempE = SortedList_lookup(list[bucket], elements[n].key);
    
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
    int n;
    int bucket = 0;
    struct timespec beginTime;
    struct timespec finishTime;
    
    for (n = start; n < total; n += threadNum){
        bucket = hash(elements[n].key) % listNum;
        switch(opt_sync){
            case 0:
                SortedList_insert(list[bucket], &elements[n]);
                break;
            case 1:
                clock_gettime(CLOCK_MONOTONIC, &beginTime);
                pthread_mutex_lock(&mutexLock[bucket]);
                clock_gettime(CLOCK_MONOTONIC, &finishTime);
                threadTimes[start] += 1000000000*   (finishTime.tv_sec -  beginTime.tv_sec) +
                                                    (finishTime.tv_nsec - beginTime.tv_nsec);
                SortedList_insert(list[bucket], &elements[n]);
                pthread_mutex_unlock(&mutexLock[bucket]);
                break;
            case 2:
                clock_gettime(CLOCK_MONOTONIC, &beginTime);
                while (__sync_lock_test_and_set(&spinLock[bucket], 1)){
                    continue;
                }
                clock_gettime(CLOCK_MONOTONIC, &finishTime);
                threadTimes[start] += 1000000000*   (finishTime.tv_sec -  beginTime.tv_sec) +
                                                    (finishTime.tv_nsec - beginTime.tv_nsec);
                SortedList_insert(list[bucket], &elements[n]);
                __sync_lock_release(&spinLock[bucket]);
                break;
        }
    }
    
    int temp = 0;
    int tempTot = 0;
    switch(opt_sync){
        case 0:
            for (n = 0; n < listNum; n++){
                temp = SortedList_length(list[n]);
                if(temp == -1){
                    fprintf(stderr, "ERROR: SortedList_length detected corrupted list\n");
                    freeMe();
                    exit(2);
                }
                tempTot += temp;
            }
            break;
        case 1:
            for (n = 0; n < listNum; n++){
                clock_gettime(CLOCK_MONOTONIC, &beginTime);
                pthread_mutex_lock(&mutexLock[n]);
                clock_gettime(CLOCK_MONOTONIC, &finishTime);
                threadTimes[start] += 1000000000*   (finishTime.tv_sec -  beginTime.tv_sec) +
                                                    (finishTime.tv_nsec - beginTime.tv_nsec);
                temp = SortedList_length(list[n]);
                pthread_mutex_unlock(&mutexLock[n]);
                if(temp == -1){
                    fprintf(stderr, "ERROR: SortedList_length detected corrupted list\n");
                    freeMe();
                    exit(2);
                }
                tempTot += temp;
            }
            break;
        case 2:
            for (n = 0; n < listNum; n++){
                clock_gettime(CLOCK_MONOTONIC, &beginTime);
                while (__sync_lock_test_and_set(&spinLock[n], 1)){
                    continue;
                }
                clock_gettime(CLOCK_MONOTONIC, &finishTime);
                threadTimes[start] += 1000000000*   (finishTime.tv_sec -  beginTime.tv_sec) +
                                                    (finishTime.tv_nsec - beginTime.tv_nsec);
                temp = SortedList_length(list[n]);
                __sync_lock_release(&spinLock[n]);
                
                if(temp == -1){
                    fprintf(stderr, "ERROR: SortedList_length detected corrupted list\n");
                    freeMe();
                    exit(2);
                }
                tempTot += temp;
            }
            break;
    }
    
    for (n = start; n < total; n += threadNum){
        bucket = hash(elements[n].key) % listNum;
        switch(opt_sync){
            case 0:
                secondStep(n, bucket);
                break;
            case 1:
                clock_gettime(CLOCK_MONOTONIC, &beginTime);
                pthread_mutex_lock(&mutexLock[bucket]);
                clock_gettime(CLOCK_MONOTONIC, &finishTime);
                threadTimes[start] += 1000000000*   (finishTime.tv_sec -  beginTime.tv_sec) +
                                                    (finishTime.tv_nsec - beginTime.tv_nsec);
                secondStep(n, bucket);

                pthread_mutex_unlock(&mutexLock[bucket]);
                break;
            case 2:
                clock_gettime(CLOCK_MONOTONIC, &beginTime);
                while (__sync_lock_test_and_set(&spinLock[bucket], 1)){
                    continue;
                }
                clock_gettime(CLOCK_MONOTONIC, &finishTime);
                threadTimes[start] += 1000000000*   (finishTime.tv_sec -  beginTime.tv_sec) +
                                                    (finishTime.tv_nsec - beginTime.tv_nsec);
                secondStep(n, bucket);
                __sync_lock_release(&spinLock[bucket]);
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
        {"lists",       required_argument,  NULL,   'l'},
        {0,             0,                  0,      0}
    };
    
    int ret = 0;
    int length = 0;
    
    while (1) {
        ret = getopt_long (argc, argv, "i:l:s:t:y:", long_opts, NULL);
        if (ret < 0){
            break;
        }
        switch (ret)
        {
            case 'i':
                iterNum = atoi(optarg);
                break;
            case 'l':
                listNum = atoi(optarg);
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
                int n;
                for (n = 0; n < length; n++){
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
    int n;
    
    mutexLock = malloc(listNum * sizeof(pthread_mutex_t));
    if (mutexLock == NULL){
        fprintf(stderr, "ERROR: malloc of mutexLock failed\n");
        exit(1);
    }
    spinLock = malloc(listNum * sizeof(int));
    if (spinLock == NULL){
        fprintf(stderr, "ERROR: malloc of spinLock failed\n");
        exit(1);
    }
    
    list = malloc(listNum * sizeof(SortedList_t*));
    for (n = 0; n < listNum; n++){
        list[n] = malloc(sizeof(SortedList_t));
        if (list[n] == NULL){
            fprintf(stderr, "ERROR: malloc of list failed\n");
            exit(1);
        }
        
        list[n]->next = list[n];
        list[n]->prev = list[n];
        list[n]->key = NULL;
        
        pthread_mutex_init(&mutexLock[n], NULL);
        spinLock[n] = 0;
    }

    int totalE = iterNum * threadNum;
    elements = malloc(totalE * sizeof(SortedListElement_t));
    if (elements == NULL){
        fprintf(stderr, "ERROR: malloc of elements failed\n");
        for (n = 0; n < listNum; n++){
            free(list[n]);
        }
        free(list);
        exit(1);
    }
    
    for (n = 0; n < totalE; n++){
        char *key = malloc( 4 * sizeof(char));
        int m;
        for (m = 0; m < 3; m++){
            key[m] = (rand() % 26) + 'A';
        }
        key[3] = '\0';
        elements[n].key = key;
    }
    
    threadTimes = malloc(threadNum * sizeof(long long));
    if (threadTimes == NULL){
        fprintf(stderr, "ERROR: malloc of threadTimes failed\n");
        for (n = 0; n < listNum; n++){
            free(list[n]);
        }
        free(list);
        free(elements);
        exit(1);
    }
    
    threads = malloc(threadNum * sizeof(pthread_t));
    if (threads == NULL){
        fprintf(stderr, "ERROR: malloc of threads failed\n");
        for (n = 0; n < listNum; n++){
            free(list[n]);
        }
        free(list);
        free(elements);
        free(threadTimes);
        exit(1);
    }
    
    count = malloc(threadNum * sizeof(int));
    if (count == NULL){
        fprintf(stderr, "ERROR: malloc of count failed\n");
        for (n = 0; n < listNum; n++){
            free(list[n]);
        }
        free(list);
        free(elements);
        free(threadTimes);
        free(threads);
        exit(1);
    }
    
    struct timespec beginTime;
    clock_gettime(CLOCK_MONOTONIC, &beginTime);
    
    for (n = 0; n < threadNum; n++){
        count[n] = n;
        if(pthread_create(&threads[n], NULL, threadFun, &count[n]) != 0){
            fprintf(stderr, "ERROR: pthread_create failed\n");
            freeMe();
            exit(1);
        }
    }
    
    for (n = 0; n < threadNum; n++){
        if(pthread_join(threads[n], NULL) != 0){
            fprintf(stderr, "ERROR: pthread_join failed\n");
            freeMe();
            exit(1);
        }
    }
    
    struct timespec finishTime;
    clock_gettime(CLOCK_MONOTONIC, &finishTime);
    
    int finalLength;
    for (n = 0; n < listNum; n++){
        finalLength = SortedList_length(list[n]);
        if (finalLength > 0){
                fprintf(stderr, "ERROR: final length of sublist was not 0\n");
                freeMe();
                exit(2);
        }
        if (finalLength == -1){
            fprintf(stderr, "ERROR: SortedList_length() detected corrupted sublist\n");
            freeMe();
            exit(2);
        }
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
    
    long long sumTimes = 0;
    for (n = 0; n < threadNum; n++){
        sumTimes += threadTimes[n];
    }
    sumTimes /= operationNum;

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
    
    printf(",%d,%d,%d,%lld,%lld,%lld,%lld\n", threadNum, iterNum, listNum, operationNum, totalTime, averageTime, sumTimes);
    
    freeMe();
    exit(0);
}
