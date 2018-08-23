#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

void segcatch(int s){
    if (s == SIGSEGV){
        char message[41] = "Caught segmentation fault. Quitting...\n";
        write(2, message, 40);
        exit(4);
    }
}

int main(int argc, char *argv[]) {
    
    struct option long_opts[] = {
        {"input",       required_argument,  NULL,   'i'},
        {"output",      required_argument,  NULL,   'o'},
        {"segfault",    no_argument,        NULL,   's'},
        {"catch",       no_argument,        NULL,   'c'},
        {0,             0,                  0,    0}
    };
    
    int ret = 0;
    int cflag = 0;
    char *in = NULL;
    char *out = NULL;
    int sflag = 0;
    
    while (1) {
        ret = getopt_long(argc, argv, "ci:o:s", long_opts, NULL);
        if (ret < 0){
            break;
        }
        switch (ret)
        {
            case 'c':
                cflag = 1;
                break;
            case 'i':
                in = optarg;
                break;
            case 'o':
                out = optarg;
                break;
            case 's':
                sflag = 1;
                break;
            default:
	      fprintf(stderr, "Invalid Argument. Avaliable options are: --input=filename --output=filename --catch --segfault. Error: %s\n", strerror(errno));
                exit(1);
        }
    }
    
    if (in){
        int input = open(in, O_RDONLY);
        if (input >= 0){
            close(0);
            dup(input);
            close(input);
        } else{
	  fprintf(stderr, "Could not open the following input file: %s. Error:%s\n", in, strerror(errno));
            exit(2);
        }
    }
    if (out){
        int output = creat(out, 0666);
        if (output >= 0){
            close(1);
            dup(output);
            close(output);
        } else{
	  fprintf(stderr, "Could not open the following output file: %s. Error:%s\n", out, strerror(errno));
            exit(3);
        }
    }
    if (cflag){
        signal(SIGSEGV, segcatch);
    }
    if (sflag){
        int *pointer = NULL;
        *pointer = 1;
    }
    
    char buf;
    int go = (int)read(0, &buf, sizeof(char));
    while (go > 0) {
        write(1, &buf, sizeof(char));
        go = (int)read(0, &buf, sizeof(char));
    }
    
    exit (0);
}
