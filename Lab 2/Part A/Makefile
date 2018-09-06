default:
	gcc -Wall -Wextra -pthread -o lab2_list lab2_list.c SortedList.c

tests:
	chmod +x testCases.sh
	./testCases.sh

graphs:
	gnuplot lab2_list.gp

clean:
	rm -f lab2_list *.o *.csv *.png *.tar.gz

dist:
	tar -czf lab2a-604805167.tar.gz lab2_list.c README Makefile SortedList.c SortedList.h *.csv *.png *.gp testCases.sh 
