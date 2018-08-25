#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <mraa.h>
#include <poll.h>
#include <ctype.h>

int stop = 0;
int pflag = 1;
char sflag = 'F';
char *fflag = NULL;
FILE *logFile;
char buf[1024];

void clean(){
    fclose(logFile);
}

void printTime(FILE* file){
    time_t timer;
    struct tm* locT;
    time(&timer);
    locT = localtime(&timer);
    
    int hour = locT -> tm_hour;
    if (hour < 10){
        fprintf(file, "0");
    }
    fprintf(file, "%d:", hour);
    int minute = locT -> tm_min;
    if (minute < 10){
        fprintf(file, "0");
    }
    fprintf (file, "%d:", minute);
    int second = locT -> tm_sec;
    if (second < 10){
        fprintf (file, "0");
    }
    fprintf(file, "%d ", second);
}

void handleInput(char* inputoriginal){
    int len = (int)strlen(inputoriginal);
    int n = 0;
    while (n < len) {
        char* input = NULL;
        input = malloc(sizeof(char));
        int relativen = 0;
        while(inputoriginal[n] != '\n'){
            input[relativen] = inputoriginal[n];
            relativen++;
            n++;
        }
        input[relativen] = '\0';
        if(strncmp(input, "SCALE=", 6) == 0)
        {
            switch (input[6]){
                case 'F':
                    sflag = 'F';
                    if (fflag != NULL) {
                        fprintf(logFile, "SCALE=F\n");
                    }
                    break;
                case 'C':
                    sflag = 'C';
                    if (fflag != NULL){
                        fprintf(logFile, "SCALE=C\n");
                    }
                    break;
                default:
                    break;
            }
        } else if (strncmp(input, "PERIOD=", 7) == 0) {
            int n = 7;
            int newInput = 0;
            while (input[n] != '\0' && input[n] != '\n') {
                int temp = newInput * 10;
                newInput = temp + (input[n] - '0');
                n++;
            }
            if (newInput <= 0) {
                fprintf(stderr, "ERROR: Period must be greater than 0\n");
            } else {
                if (fflag != NULL) {
                    fprintf(logFile, "%s\n", input);
                }
                pflag = newInput;
            }
        } else if (strcmp(input, "STOP") == 0) {
            stop = 1;
            if(fflag != NULL) {
                fprintf(logFile, "STOP\n");
            }
        } else if (strcmp(input, "START") == 0) {
            stop = 0;
            if(fflag!=NULL) {
                fprintf(logFile, "START\n");
            }
        } else if (strncmp(input, "LOG ", 4) == 0) {
            if(fflag!=NULL) {
                fprintf(logFile, "%s\n", input);
            }
        } else if (strcmp(input, "OFF") == 0) {
            printTime(stdout);
            printf("SHUTDOWN");
            if (fflag != NULL){
                fprintf(logFile, "OFF\n");
                printTime(logFile);
                fprintf(logFile, "SHUTDOWN\n");
            }
            exit(0);
        }
        n++;
    }
}

void analogIO(){
    mraa_gpio_context aioButton;
    aioButton = mraa_gpio_init(60);
    if (aioButton == NULL) {
        fprintf(stderr, "Failed to initialize button\n");
        exit(1);
    }
    mraa_gpio_dir(aioButton, MRAA_GPIO_IN);
    
    mraa_aio_context aio;
    aio = mraa_aio_init(1);
    if (aio == NULL) {
        fprintf(stderr, "Failed to initialize AIO\n");
        exit(1);
    }
    
    ssize_t c;
    int ret;
    
    time_t startTime, endTime;
    
    struct pollfd arr[1];
    arr[0].fd = 0;
    arr[0].events = POLLIN | POLLHUP | POLLERR;

    for(;;) {
        int value = mraa_aio_read(aio);
        float rawTemp = 1023.0/value-1.0;
        rawTemp = rawTemp * 100000;
        float temperature = 1.0/(log(rawTemp/100000)/4275+1/298.15)-273.15;
        
        printTime(stdout);
        if (fflag != NULL){
            printTime(logFile);
        }
        if (sflag == 'F'){
            temperature = 9/5 * temperature + 32;
        }
        printf("%.1f\n", temperature);
        if (fflag != NULL){
            fprintf(logFile, "%.1f\n", temperature);
        }
        
        time(&startTime);
        time(&endTime);
        
        while(difftime(endTime, startTime) < pflag){
            if (mraa_gpio_read(aioButton)){
                printTime(stdout);
                printf("SHUTDOWN");
                if (fflag != NULL){
                    printTime(logFile);
                    fprintf(logFile, "SHUTDOWN\n");
                }
                exit(0);
            }
            ret = poll(arr, 1, 0);
            if (ret < 0){
                fprintf(stderr, "ERROR: poll() failed in analogIO function\n");
                exit(1);
            } else {
                if (arr[0].revents & POLLIN){
                    c = read(0, buf, 1024);
                    if (c < 0){
                        fprintf(stderr, "ERROR: read() failed in analogIO function\n");
                        exit(1);
                    }
                    char *pointer;
                    buf[c] = '\0';
                    pointer = buf;
                    handleInput(pointer);
                }
            }
            if (!stop){
                time(&endTime);
            }
        }
    }
    
    mraa_result_t status = mraa_aio_close(aio);
    if (status != 0) {
        fprintf(stderr, "ERROR: mraa_aio_close failed for temperature sensor\n");
        exit(1);
    }
}

int main(int argc, char ** argv) {
    struct option long_opts[] = {
        {"period",      required_argument,  NULL,   'p'},
        {"scale",       required_argument,  NULL,   's'},
        {"log",         required_argument,  NULL,   'l'},
        {0,             0,                  0,      0}
    };
    
    int ret;
    while (1) {
        ret = getopt_long (argc, argv, "p:s:l:", long_opts, NULL);
        if (ret < 0){
            break;
        }
        switch (ret) {
            case 'p':
                pflag = atoi(optarg);
                if (pflag <= 0){
                    fprintf(stderr, "ERROR: Invalid --period argument: Must be more than 0\n");
                    exit(1);
                }
                break;
            case 's':
                if (strlen(optarg) != 1){
                    fprintf(stderr, "ERROR: Invalid --scale argument: Length must be 1 character\n");
                    exit(1);
                }
                sflag = optarg[0];
                if (sflag != 'C' && sflag != 'F'){
                    fprintf(stderr, "ERROR: Invalid --scale argument: Options are 'C' and 'F'\n");
                    exit(1);
                }
                break;
            case 'l':
                fflag = optarg;
                if (fflag != NULL){
                    logFile = fopen(fflag, "a");
                    if (logFile == NULL){
                        fprintf(stderr,"ERROR: fopen() failed for log option\n");
                        exit(1);
                    }
                    atexit(clean);
                }
                break;
            default:
                fprintf(stderr, "ERROR: Invalid argument. Valid arguments are --period=#, --scale=[C,F], --log=FILENAME\n");
                exit(1);
        }
    }
    analogIO();
    exit(0);
}
