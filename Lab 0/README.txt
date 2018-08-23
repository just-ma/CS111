NAME: Justin Ma
EMAIL: justinma98@g.ucla.edu

================================================================================
lab0.c

The program copied standard input to standard output. Optional options included --input=filename, --output=filename, --segfault, and --catch.

I researched examples of getopt_long to understand how to use it.
Link: https://www.gnu.org/software/libc/manual/html_node/Getopt-Long-Option-Example.html#Getopt-Long-Option-Example

I researched how to use signal().
Link: http://man7.org/linux/man-pages/man7/signal.7.html

I research how to perform input and output redirection using open, creat, close, and dup.
Link: https://web.cs.ucla.edu/classes/spring18/cs111/projects/fd_juggling.html


================================================================================
Makefile

This file included default which which compiles lab0.c with -Wall, -Wextra, and -g options. Check ran the smoke-test shown below. Clean removed all files created by the Makefile with rm. Dist built the tarball.

I researched how to create a Makefile from the link below.
Link: https://www.cs.swarthmore.edu/~newhall/unixhelp/howto_makefiles.html


================================================================================
GDB screenshots

For backtrace.png, I opened gdb on lab0 and ran with --segfault, which stopped at when the null pointer was dereferenced. This occurred at line 85.

For breakpoint.png, I reopened gdb on lab0 and set a breakpoint at line 85. Then, I ran with --segfault. To confirm it, I checked whether I could access what the pointer was pointing at with print.


================================================================================
Smoke-test

I checked if the input was copied to output accurately and returned 0.

I checked if both --input and --output options worked 

I checked if an unrecognized argument produced an error and returned 1.

I checked if an unreadable input file produced an error and returned 2. 

I checked if an unwritable output file produced an error and returned 3.

I checked if --catch caught the segmentation error created by --segfault and returned 4.