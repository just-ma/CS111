#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <termios.h>
#include <getopt.h>
#include <poll.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include "zlib.h"
#include <sys/socket.h>
#include <ulimit.h>

struct termios initialState;

char buf[2048];
char cbuf[2048];
char tbuf[2] = {0x0D,0x0A};

int socketFD;

int portNum = -1;
int logFlag = 0;
int logFD = 0;
int compressFlag = 0;

z_stream stdin_to_server;
z_stream server_to_stdout;

void resetTerm(){
    if (compressFlag){
        inflateEnd(&server_to_stdout);
        deflateEnd(&stdin_to_server);
    }
    if (tcsetattr(STDIN_FILENO, TCSANOW, &initialState) < 0){
        fprintf(stderr, "ERROR: tcsetattr() failed on exit\n");
        exit(1);
    }
    exit(0);
}

void setUpTerm(){
    struct termios term;
    tcgetattr(STDIN_FILENO, &initialState);
    tcgetattr(STDIN_FILENO, &term);
    
    term.c_iflag = ISTRIP;
    term.c_oflag = 0;
    term.c_lflag = 0;
    
    term.c_lflag &= ~(ICANON|ECHO);
    term.c_cc[VMIN] = 1;
    term.c_cc[VTIME] = 0;
    
    if (tcsetattr(STDIN_FILENO, TCSANOW, &term) < 0) {
        fprintf(stderr, "ERROR: tcsetattr() failed at set up\n");
        resetTerm();
        exit(1);
    }
}

void logging(char* buf, int size, int sent){
    if (ulimit(UL_SETFSIZE, 10000) < 0){
        fprintf(stderr, "ERROR: ulimit() failed when logging\n");
        resetTerm();
        exit(1);
    }
    char sentence[2048];
    if (sent){
        sprintf(sentence, "SENT %d bytes: %s\n", size, buf);
        if(write(logFD, sentence, strlen(sentence)) < 0){
            fprintf(stderr, "ERROR: log file is unwritable\n");
            resetTerm();
            exit(1);
        }
    } else {
        sprintf(sentence, "RECEIVED %d bytes: %s\n", size, buf);
        if(write(logFD, sentence, strlen(sentence)) < 0){
            fprintf(stderr, "ERROR: log file is unwritable\n");
            resetTerm();
            exit(1);
        }
    }
}

void serverReadWrite(){
    struct pollfd arr[2];
    arr[0].fd = STDIN_FILENO;   // first element in array refers to stdin
    arr[1].fd = socketFD;       // second element in array refers to socket
    arr[0].events = POLLIN | POLLHUP | POLLERR;
    arr[1].events = POLLIN | POLLHUP | POLLERR;
    
    int ret;
    ssize_t c;
    
    while (1){
        ret = poll(arr, 2, 0);
        if (ret < 0){
            fprintf(stderr, "ERROR: poll() failed\n");
            resetTerm();
            exit(1);
        } else if (ret > 0){
            if (arr[0].revents & POLLIN){               // From STDIN
                c = read(STDIN_FILENO, buf, 2048);
                if (c < 0){
                    fprintf(stderr, "ERROR: read() failed from STDIN\n");
                    resetTerm();
                    exit(1);
                }
                
                for (int n = 0; n < c; n++){
                    if (buf[n] == 0x0D || buf[n] == 0x0A){
                        write(STDOUT_FILENO, tbuf, 2);
                    }else{
                        write(STDOUT_FILENO, buf+n, 1);
                    }
                }
                
                char* pointer = buf;
                if (compressFlag){
                    stdin_to_server.avail_in = c;
                    stdin_to_server.next_in = (Bytef*)buf;
                    stdin_to_server.avail_out = 2048;
                    stdin_to_server.next_out = (Bytef*)cbuf;
                    
                    do{
                        deflate(&stdin_to_server, Z_SYNC_FLUSH);
                    } while (stdin_to_server.avail_in > 0);
                    
                    c = 2048 - stdin_to_server.avail_out;
                    pointer = cbuf;
                }
                pointer[c] = '\0';
                
                write(socketFD, pointer, c);
                
                if (logFlag)
                    logging(pointer, c, 1);
            }
            if (arr[1].revents & POLLIN){               // From Server
                char* pointer = buf;
                c = read(socketFD, buf, 2048);
                if (c < 0){
                    fprintf(stderr, "ERROR: read() failed from server\n");
                    resetTerm();
                    exit(1);
                } else if (c == 0){
                    resetTerm();
                    exit(0);
                }
                buf[c] = '\0';
                
                if (logFlag)
                    logging(pointer, c, 0);
                
                if (compressFlag){
                    server_to_stdout.avail_in = c;
                    server_to_stdout.next_in = (Bytef*)buf;
                    server_to_stdout.avail_out = 2048;
                    server_to_stdout.next_out = (Bytef*)cbuf;
                    
                    do{
                        inflate(&server_to_stdout, Z_SYNC_FLUSH);
                    } while (server_to_stdout.avail_in > 0);
                    
                    pointer = cbuf;
                    c = 2048 - server_to_stdout.avail_out;
                }
                
                for (int n = 0; n < c; n++){
                    if (pointer[n] == 0x04){
                        resetTerm();
                        exit(0);
                    } else if (pointer[n] == 0x0A){
                        write(STDOUT_FILENO, tbuf, 2);
                    }else{
                        write(STDOUT_FILENO, pointer+n, 1);
                    }
                }
            }
            if ((arr[1].revents & (POLLHUP | POLLERR))){
                resetTerm();
                exit(0);
            }
        }
    }
}

void setUpSocket(){
    struct sockaddr_in serverAddress;
    struct hostent *server = gethostbyname("localhost");
    
    socketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFD < 0){
        fprintf(stderr, "ERROR: socket failed\n");
        resetTerm();
        exit(1);
    }
    
    memset((char*)&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)server->h_addr, server->h_length);
    serverAddress.sin_port = htons(portNum);
    
    if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
        fprintf(stderr, "ERROR: connect() failed trying to connect to server\n");
        resetTerm();
        exit(1);
    }
}

int main(int argc, char *argv[])
{
    struct option long_opts[] = {
        {"port",        required_argument,  NULL,   'p'},
        {"log",         optional_argument,  NULL,   'l'},
        {"compress",    optional_argument,  NULL,   'c'},
        {0,             0,                  0,      0}
    };
    
    int ret = 0;
    
    while (1) {
        ret = getopt_long (argc, argv, "cl:p:", long_opts, NULL);
        if (ret < 0){
            break;
        }
        switch (ret)
        {
            case 'c':
                compressFlag = 1;
                break;
            case 'l':
                logFlag = 1;
                logFD = creat(optarg, 0666);
                break;
            case 'p':
                portNum = atoi(optarg);
                break;
            default:
                fprintf(stderr, "ERROR: Invalid argument.\nRequired argument is: --port=PORTNUMBER\nOptional arguments are: --log=FILENAME, --compress\n");
                resetTerm();
                exit(1);
        }
    }
    
    if (logFlag){
        if (logFD < 0){
            fprintf(stderr, "ERROR: log file is unwritable\n");
            resetTerm();
            exit(1);
        }
    }
    
    if (portNum < 0) {
        fprintf(stderr, "ERROR: --port flag is a required argument\n");
        resetTerm();
        exit(1);
    }
    
    if (compressFlag){
        stdin_to_server.zalloc = Z_NULL;
        stdin_to_server.zfree = Z_NULL;
        stdin_to_server.opaque = Z_NULL;
        
        deflateInit(&stdin_to_server, Z_DEFAULT_COMPRESSION);
        
        server_to_stdout.zalloc = Z_NULL;
        server_to_stdout.zfree = Z_NULL;
        server_to_stdout.opaque = Z_NULL;
        
        inflateInit(&server_to_stdout);
    }
    
    setUpTerm();
    setUpSocket();
    serverReadWrite();

    return 0;
}
