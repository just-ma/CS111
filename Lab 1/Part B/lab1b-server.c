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

char buf[2048];
char cbuf[2048];
char tbuf[2] = {0x0D,0x0A};

int toChild[2];
int fromChild[2];
pid_t childPID = -1;

int socketFD, newSocketFD;

int portNum = -1;
int compressFlag = 0;

z_stream server_to_shell;
z_stream shell_to_server;

void closeServer(){
    if (compressFlag){
        inflateEnd(&server_to_shell);
        deflateEnd(&shell_to_server);
    }
    
    int stat;
    if (waitpid(childPID, &stat, 0) == -1){
        fprintf(stderr, "ERROR: waitpid() failed\n");
        exit(1);
    }
    
    fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", (0x007f & stat), (0xff00 & stat)>>8);
    close(socketFD);
    close(newSocketFD);
    exit(0);
}

void shellReadWrite(){
    struct pollfd arr[2];
    arr[0].fd = newSocketFD;       // first element in array refers to socket
    arr[1].fd = fromChild[0];   // second element in array refers to pipe
    arr[0].events = POLLIN | POLLHUP | POLLERR;
    arr[1].events = POLLIN | POLLHUP | POLLERR;
    
    int ret;
    ssize_t c;
    
    while (1){
        ret = poll(arr, 2, 0);
        if (ret < 0){
            fprintf(stderr, "ERROR: poll() failed\n");
            closeServer();
            exit(1);
        } else if (ret > 0){
            if (arr[0].revents & POLLIN){               // From Socket
                char* pointer = buf;
                char* cpointer = cbuf;
                c = read(newSocketFD, buf, 2048);
                if (c < 0){
                    fprintf(stderr, "ERROR: read() failed in shell\n");
                    close(toChild[1]);
                    continue;
                }

                if (compressFlag){
                    server_to_shell.avail_in = c;
                    server_to_shell.next_in = (Bytef*)buf;
                    server_to_shell.avail_out = 2048;
                    server_to_shell.next_out = (Bytef*)cpointer;
                    
                    do{
                        inflate(&server_to_shell, Z_SYNC_FLUSH);
                    } while (server_to_shell.avail_in > 0);
                    
                    pointer = cpointer;
                    c = 2048-server_to_shell.avail_out;
                }
                
                for (int n = 0; n < c; n++){
                    if (pointer[n] == 0x04){
                        close(toChild[1]);
                        break;
                    } else if (pointer[n] == 0x0D){
                        write(toChild[1], tbuf+1, 1);
                    } else if (pointer[n] == 0x03){
                        kill(childPID, SIGINT);
                    } else {
                        write(toChild[1], pointer+n, 1);
                    }
                }
            }
            if (arr[1].revents & POLLIN){               // From Shell
                char* pointer = buf;
                char* cpointer = cbuf;
                c = read(fromChild[0], buf, 2048);
                if (c < 0){
                    fprintf(stderr, "ERROR: read() failed in shell\n");
                    closeServer();
                    exit(1);
                }
                
                if (compressFlag){
                    shell_to_server.avail_in = c;
                    shell_to_server.next_in = (Bytef*)buf;
                    shell_to_server.avail_out = 2048;
                    shell_to_server.next_out = (Bytef*)cpointer;
                    
                    do{
                        deflate(&shell_to_server, Z_SYNC_FLUSH);
                    } while (shell_to_server.avail_in > 0);
                    
                    pointer = cpointer;
                    c = 2048-shell_to_server.avail_out;
                }
                write(newSocketFD, pointer, c);
            }
            if ((arr[1].revents & (POLLHUP | POLLERR))){
                closeServer();
                exit(0);
            }
        }
    }
}

void handler(int signal){
    if (signal == SIGINT){
        kill(childPID, SIGINT);
    }
    if (signal == SIGPIPE){
        closeServer();
        exit(0);
    }
}

void setUpSocket(){
    socklen_t clientLength;
    struct sockaddr_in serverAddress, clientAddress;
    
    socketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFD < 0) {
        fprintf(stderr, "ERROR: socket() failed in setting up socket\n");
        exit(1);
    }
    
    memset((char*)&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(portNum);
    
    if (bind(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
        fprintf(stderr, "ERROR: bind() failed in setting up socket\n");
        exit(1);
    }
    
    listen(socketFD, 5);
    clientLength = sizeof(clientAddress);
    
    newSocketFD = accept(socketFD, (struct sockaddr*)&clientAddress, &clientLength);
    if (newSocketFD < 0) {
        fprintf(stderr, "ERROR: accept() failed in setting up socket\n");
        exit(1);
    }
}

int main(int argc, char *argv[])
{
    struct option long_opts[] = {
        {"port",        required_argument,  NULL,   'p'},
        {"compress",    optional_argument,  NULL,   'c'},
        {0,             0,                  0,      0}
    };
    
    int ret = 0;
    
    while (1) {
        ret = getopt_long (argc, argv, "cp:", long_opts, NULL);
        if (ret < 0){
            break;
        }
        switch (ret)
        {
            case 'c':
                compressFlag = 1;
                break;
            case 'p':
                portNum = atoi(optarg);
                break;
            default:
                fprintf(stderr, "ERROR: Invalid argument.\nRequired argument is: --port=PORTNUMBER\nOptional arguments are: --log=FILENAME, --compress");
                exit(1);
        }
    }
    
    if (portNum < 0) {
        fprintf(stderr, "ERROR: --port flag is a required argument\n");
        exit(1);
    }
    
    signal(SIGINT, handler);
    signal(SIGPIPE, handler);
    
    if (compressFlag){
        shell_to_server.zalloc = Z_NULL;
        shell_to_server.zfree = Z_NULL;
        shell_to_server.opaque = Z_NULL;
        
        deflateInit(&shell_to_server, Z_DEFAULT_COMPRESSION);
        
        server_to_shell.zalloc = Z_NULL;
        server_to_shell.zfree = Z_NULL;
        server_to_shell.opaque = Z_NULL;
        
        inflateInit(&server_to_shell);
    }
    
    setUpSocket();
    
    if (pipe(toChild) == -1){
        fprintf(stderr, "ERROR: pipe() to child failed\n");
        exit(1);
    }
    if (pipe(fromChild) == -1){
        fprintf(stderr, "ERROR: pipe() from child failed\n");
        exit(1);
    }
    
    childPID = fork();
    
    if (childPID > 0){              // Parent
        close(toChild[0]);          // close stdin from pipe to child process
        close(fromChild[1]);        // close stdout to pipe from child process
        
        shellReadWrite();
    }
    else if (childPID == 0){        // Child
        close(toChild[1]);          // close stdout to pipe to child process
        close(fromChild[0]);        // close stdin from pipe from child process
        dup2(toChild[0], STDIN_FILENO);
        dup2(fromChild[1], STDOUT_FILENO);
        dup2(fromChild[1], STDERR_FILENO);
        close(toChild[0]);
        close(fromChild[1]);
        
        char *eargv[2];
        char efilename[] = "/bin/bash";
        eargv[0] = efilename;
        eargv[1] = NULL;
        if (execvp(efilename, eargv) == -1){
            fprintf(stderr, "ERROR: execvp() failed\n");
            closeServer();
            exit(1);
        }
    }
    else {
        fprintf(stderr, "ERROR: fork() failed\n");
        closeServer();
        exit(1);
    }
    
    return 0;
}
