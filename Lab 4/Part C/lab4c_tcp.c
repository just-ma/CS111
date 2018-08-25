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
#include <sys/socket.h>
#include <netdb.h>

int stop = 0;
int pflag = 1;
int portflag = -1;
char sflag = 'F';
char *fflag = NULL;
int iflag = 0;
char *hflag = NULL;
FILE *logFile;
char buf[1024];
char prin[9];

int socketFD = -1;
struct hostent *serve;
struct sockaddr_in serveAdd;

void clean(){
    fclose(logFile);
}

void getTime(){
    time_t timer;
    struct tm* locT;
    time(&timer);
    locT = localtime(&timer);
    
    int hour = locT -> tm_hour;
    int minute = locT -> tm_min;
    int second = locT -> tm_sec;
    
    char tempchar;
    
    if (hour < 10){
        prin[0] = '0';
    }else{
        sprintf(&tempchar, "%d", hour/10);
        prin[0] = tempchar;
    }
    sprintf(&tempchar, "%d", hour%10);
    prin[1] = tempchar;
    prin[2] = ':';
    if (minute < 10){
        prin[3] = '0';
    }else{
        sprintf(&tempchar, "%d", minute/10);
        prin[3] = tempchar;
    }
    sprintf(&tempchar, "%d", minute%10);
    prin[4] = tempchar;
    prin[5] = ':';
    if (second < 10){
        prin[6] = '0';
    }else{
        sprintf(&tempchar, "%d", second/10);
        prin[6] = tempchar;
    }
    sprintf(&tempchar, "%d", second%10);
    prin[7] = tempchar;
    prin[8] = ' ';
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
            if(fflag != NULL) {
                fprintf(logFile, "START\n");
            }
        } else if (strncmp(input, "LOG ", 4) == 0) {
            if(fflag!=NULL) {
                fprintf(logFile, "%s\n", input);
            }
        } else if (strcmp(input, "OFF") == 0) {
            getTime();
            char prin2[19];
            strcpy(prin2, prin);
            strcat(prin2, "SHUTDOWN");
            prin2[17] = '\0';
            dprintf(socketFD, "%s", prin2);
            if (fflag != NULL){
                fprintf(logFile, "OFF\n");
                prin2[17] = '\n';
                prin2[18] = '\0';
                fprintf(logFile, "%s", prin2);
            }
            exit(0);
        }
        n++;
    }
}

void analogIO(){
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
    arr[0].fd = socketFD;
    arr[0].events = POLLIN | POLLHUP | POLLERR;
    
    for(;;) {
        int value = mraa_aio_read(aio);
        float rawTemp = 1023.0/value-1.0;
        rawTemp = rawTemp * 100000;
        float temperature = 1.0/(log(rawTemp/100000)/4275+1/298.15)-273.15;
        
        getTime();
        
        if (sflag == 'F'){
            temperature = 9/5 * temperature + 32;
        }
        char prin2[20];
        char tempT[6];
        sprintf(tempT, "%.1f\n", temperature);
        strcpy(prin2, prin);
        strcat(prin2, tempT);
        prin2[14] = '\0';
        dprintf(socketFD, "%s", prin2);
        if (fflag != NULL){
            fprintf(logFile, "%s", prin2);
        }
        
        time(&startTime);
        time(&endTime);
        
        while(difftime(endTime, startTime) < pflag){
            ret = poll(arr, 1, 0);
            if (ret < 0){
                fprintf(stderr, "ERROR: poll() failed in analogIO function\n");
                exit(1);
            } else {
                if (arr[0].revents & POLLIN){
                    c = read(socketFD, buf, 1024);
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
        {"id",          required_argument,  NULL,   'i'},
        {"host",        required_argument,  NULL,   'h'},
        {0,             0,                  0,      0}
    };
    
    int ret;
    while (1) {
        ret = getopt_long (argc, argv, "p:s:l:i:h:", long_opts, NULL);
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
                    fprintf(stderr, "ERROR: fopen() failed for log option\n");
                    exit(1);
                }
                atexit(clean);
            }
            break;
            case 'i':
            if (strlen(optarg) != 9){
                fprintf(stderr, "ERROR: id must be 9 digits long\n");
                exit(1);
            } else {
                iflag = atoi(optarg);
            }
            break;
            case 'h':
            hflag = optarg;
            if (hflag == NULL){
                fprintf(stderr, "ERROR: hostname empty\n");
                exit(1);
            }
            break;
            default:
            fprintf(stderr, "ERROR: Invalid argument. Valid arguments are --period=#, --scale=[C,F], --log=FILENAME, --id=[9 digit number], --host=HOSTNAME\n");
            exit(1);
        }
    }
    
    portflag = atoi(argv[optind]);
    
    if (hflag == NULL){
        fprintf(stderr, "ERROR: hostname argument required\n");
        exit(1);
    }
    if (iflag == 0){
        fprintf(stderr, "ERROR: id argument required\n");
        exit(1);
    }
    if (portflag == -1){
        fprintf(stderr, "ERROR: port number argument required\n");
        exit(1);
    }
    
    socketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFD < 0){
        fprintf(stderr, "ERROR: socket() failed\n");
        exit(1);
    }
    
    serve = gethostbyname(hflag);
    if (serve == NULL){
        fprintf(stderr, "ERROR: gethostbyname() failed\n");
        exit(1);
    }
    
    serveAdd.sin_family = AF_INET;
    serveAdd.sin_port = htons(portflag);
    memcpy((char*)&serveAdd.sin_addr.s_addr, (char*)serve->h_addr, serve->h_length);
    int test = -1;
    test = connect(socketFD, (struct sockaddr*)&serveAdd, sizeof(serveAdd));
    if (test < 0){
        fprintf(stderr, "ERROR: connect() failed\n");
        exit(1);
    } else{
        test = dprintf(socketFD, "ID=%d\n", iflag);
        if (test < 0){
            fprintf(stderr, "ERROR: write() failed to socket\n");
            exit(1);
        }
        if(fflag != NULL) {
            fprintf(logFile, "ID=%d\n", iflag);
        }
    }
    analogIO();
    exit(0);
}
