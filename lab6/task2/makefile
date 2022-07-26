all: task2

task2: main.o LineParser.o 
	gcc -g -Wall -o task2 main.o LineParser.o 
	rm *.o

task2.o: main.c
	gcc -g -Wall -c -o main.o task2.c

LineParser.o: LineParser.c 
	gcc -g -Wall -c -o LineParser.o LineParser.c
	
.PHONY: clean
clean:
	rm -f *.o task2