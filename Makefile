
all: minls

minlib.o: minlib.c minlib.h
	gcc -Wall -g -c -o minlib.o minlib.c

minls.o: minls.c
	gcc -Wall -g -c -o minls.o minls.c

minls: minls.o minlib.o
	gcc -Wall -g minls.o minlib.o -o minls

clean:
	rm -f *.o
	rm -f minls
