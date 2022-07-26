all: task2c

task2c: main.o LineParser.o looper
	gcc -g -Wall -o task2c main.o LineParser.o looper
	rm *.o

task2c.o: main.c
	gcc -g -Wall -c -o main.o task2c.c

LineParser.o: LineParser.c 
	gcc -g -Wall -c -o LineParser.o LineParser.c

looper: looper.c
	gcc -g -Wall -o looper looper.c

.PHONY: clean
clean:
	rm -f *.o task2c