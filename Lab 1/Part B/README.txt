NAME: Justin Ma
EMAIL: justinma98@g.ucla.edu

README:
        Identification information and a brief overview of the included files.

Makefile:
        Script that includes the default compile of the lab1b executable. It al\
so includes clean, which removes all files created by make, and dist, which gen\
erates the tar.gz file.

lab1b-client.c:
        C source module for the client specified for lab 1 part b. The code sup\
ports the required --port=PORTNUM argument and the optional --log=LOGFILE, --co\
mpress arguments. The client process sends standard input to a server process w\
hich is connected by a socket.

lab1b-server.c:
	C source module for the server specified for lab 1 part b. The code sup\
ports the required --port=PORTNUM argument and the optional --compress argument\
. The server process accepts bytes from the socket, and passes it to a shell pr\
ocess. The shell process runs bash and its output is forwarded back to the sock\
et.