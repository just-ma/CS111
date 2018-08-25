.SILENT:
default:
	gcc -o lab4b -Wall -Wextra -lmraa -lm lab4b.c
check:
	chmod +x check.sh
	./check.sh
clean:
	rm -f lab4b lab4b-604805167.tar.gz
dist:
	tar -czf lab4b-604805167.tar.gz README Makefile lab4b.c check.sh