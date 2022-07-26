all: task3

task3: main.o LineParser.o 
	gcc -g -Wall -o task3 main.o LineParser.o
	rm *.o

task3.o: main.c
	gcc -g -Wall -c -o main.o task3.c

LineParser.o: LineParser.c
	gcc -g -Wall -c -o LineParser.o LineParser.c
	
.PHONY: clean
clean:
	rm -f *.o task3