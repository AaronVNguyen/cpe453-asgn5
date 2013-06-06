
all: minls minget

minlib.o: minlib.c minlib.h
	gcc -Wall -g -c -o minlib.o minlib.c

minls.o: minls.c
	gcc -Wall -g -c -o minls.o minls.c

minget.o: minget.c
	gcc -Wall -g -c -o minget.o minget.c

minls: minls.o minlib.o
	gcc -Wall -g minls.o minlib.o -o minls

minget: minget.o minlib.o
	gcc -Wall -g minget.o minlib.o -o minget

clean:
	rm -f *.o
	rm -f minls
	rm -f minget
