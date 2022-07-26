all: task1

task1: main.o LineParser.o
	gcc -g -Wall -o task1 main.o LineParser.o
	rm *.o

task1.o: main.c
	gcc -g -Wall -c -o main.o task1.c

LineParser.o: LineParser.c 
	gcc -g -Wall -c -o LineParser.o LineParser.c
	
.PHONY: clean
clean:
	rm -f *.o task1