default:
	gcc -g -Wall -Wextra -o lab4c_tcp -lm -lmraa lab4c_tcp.c
	gcc -g -Wall -Wextra -o lab4c_tls -lm -lmraa -lcrypto -lssl lab4c_tls.c
clean:
	rm -f *.txt lab4c_tcp lab4c_tls *.tar.gz
dist:
	tar -czf lab4c-604805167.tar.gz README Makefile lab4c_tcp.c lab4c_tls.c
