NAME: Justin Ma
EMAIL: justinma98@g.ucla.edu

README:
	Identification information and a brief overview of the included files.

Makefile:
	Script that includes the default compile of the lab1a executable. It also includes clean, which removes all files created by make, and dist, which generates the tar.gz file.

lab1.c:
	C source module specified for lab 1 part a. The code supports the optional flag --shell, which creates a shell process connected to the terminal process via pipes.
	Sources include the following:
	poll() - https://linux.die.net/man/3/poll
	termios - http://pubs.opengroup.org/onlinepubs/007904875/functions/tcgetattr.html
		- http://pubs.opengroup.org/onlinepubs/009695399/functions/tcsetattr.html
		- https://support.sas.com/documentation/onlinedoc/sasc/doc/lr2/tsetatr.htm
	noncanonical implementation - https://www.gnu.org/software/libc/manual/html_node/Noncanon-Example.html
	waitpid() - https://linux.die.net/man/2/waitpid