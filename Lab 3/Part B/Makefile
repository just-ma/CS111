#NAME: Justin Ma

default:
	echo "#!/bin/sh" > lab3b
	echo 'python lab3b.py $$@' >> lab3b
	chmod +x lab3b
 
clean:
	rm -f *.tar.gz lab3b

dist: clean default
	tar -czf lab3b-804726225.tar.gz lab3b.py Makefile README
