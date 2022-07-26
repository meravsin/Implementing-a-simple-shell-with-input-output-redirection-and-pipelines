all: task0a

task0a: main.o LineParser.o
	gcc -g -Wall -o task0a main.o LineParser.o
	rm *.o

task0a.o: main.c
	gcc -g -Wall -c -o main.o task0a.c

LineParser.o: LineParser.c 
	gcc -g -Wall -c -o LineParser.o LineParser.c
	
.PHONY: clean
clean:
	rm -f *.o task0a