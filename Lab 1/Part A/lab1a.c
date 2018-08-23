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

struct termios initialState;

char buf[2048];
char tbuf[2] = {0x0D,0x0A};

int toChild[2];
int fromChild[2];
pid_t childPID = -1;

int sflag = 0;

void resetTerm(){
    if (tcsetattr(STDIN_FILENO, TCSANOW, &initialState) < 0){
        fprintf(stderr, "ERROR: tcsetattr() failed on exit\n");
        exit(1);
    }
    if (sflag) {
        int stat;
        if (waitpid(childPID, &stat, 0) == -1){
            fprintf(stderr, "ERROR: waitpid() failed\n");
            exit(1);
        }
        fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", (0x007f & stat), (0xff00 & stat)>>8);
        exit(0);
    }
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

void termReadWrite(){
    ssize_t c = 0;
    while(1){
        c = read(STDIN_FILENO, buf, 2048);
        if (c < 0){
            fprintf(stderr, "ERROR: read() failed in terminal\n");
            resetTerm();
            exit(1);
        }
        for (int n = 0; n < c; n++){
            if (buf[n] == 0x04){
                resetTerm();
                exit(0);
            } else if (buf[n] == 0x0D || buf[n] == 0x0A){
                write(STDOUT_FILENO, tbuf, 2);
            } else {
                write(STDOUT_FILENO, buf+n, 1);
            }
        }
    }
}

void shellReadWrite(){
    struct pollfd arr[2];
    arr[0].fd = STDIN_FILENO;   // first element in array refers to stdin
    arr[1].fd = fromChild[0];   // second element in array refers to pipe
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
                    fprintf(stderr, "ERROR: read() failed in shell\n");
                    resetTerm();
                    exit(1);
                }
                for (int n = 0; n < c; n++){
                    if (buf[n] == 0x04){
                        close(toChild[1]);
                    } else if (buf[n] == 0x0D || buf[n] == 0x0A){
                        write(STDOUT_FILENO, tbuf, 2);
                        write(toChild[1], tbuf+1, 1);
                    } else if (buf[n] == 0x03){
                        kill(childPID, SIGINT);
                    } else {
                        write(STDOUT_FILENO, buf+n, 1);
                        write(toChild[1], buf+n, 1);
                    }
                }
            }
            if (arr[1].revents & POLLIN){               // From Shell
                c = read(fromChild[0], buf, 2048);
                if (c < 0){
                    fprintf(stderr, "ERROR: read() failed in shell\n");
                    resetTerm();
                    exit(1);
                }
                for (int n = 0; n < c; n++){
                    if (buf[n] == 0x04){
                        resetTerm();
                        exit(0);
                        
                    } else if (buf[n] == 0x0A){
                        write(STDOUT_FILENO, tbuf, 2);
                    } else {
                        write(STDOUT_FILENO, buf+n, 1);
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

void handler(int signal){
    if (signal == SIGINT){
        printf("got sigint\n");
        kill(childPID, SIGINT);
    }
    if (signal == SIGPIPE){
        printf("got sigpipe\n");
        resetTerm();
        exit(0);
    }
}

int main(int argc, char *argv[])
{
    setUpTerm();
    
    struct option long_opts[] = {
        {"shell",       optional_argument,  NULL,   's'},
        {0,             0,                  0,      0}
    };
    
    int ret = 0;
    
    while (1) {
        ret = getopt_long (argc, argv, "s", long_opts, NULL);
        if (ret < 0){
            break;
        }
        switch (ret)
        {
            case 's':
                sflag = 1;
                break;
            default:
                fprintf(stderr, "ERROR: Invalid argument. Avaliable option is: --shell\n");
                resetTerm();
                exit(1);
        }
    }
    
    if (sflag == 1){
        signal(SIGINT, handler);
        signal(SIGPIPE, handler);
        
        if (pipe(toChild) == -1){
            fprintf(stderr, "ERROR: pipe() to child failed\n");
            resetTerm();
            exit(1);
        }
        if (pipe(fromChild) == -1){
            fprintf(stderr, "ERROR: pipe() from child failed\n");
            resetTerm();
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
                resetTerm();
                exit(1);
            }
        }
        else {
            fprintf(stderr, "ERROR: fork() failed\n");
            resetTerm();
            exit(1);
        }
    } else{                             // no --shell
        termReadWrite();
    }
    
    return 0;
}
