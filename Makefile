queue.o: src/queue.h src/queue.c
	gcc -c src/queue.c

bin/main: src/main.c queue.o
	gcc src/main.c queue.o -o bin/main -lrt -lpthread

bin/client: src/client.c queue.o
	gcc src/client.c queue.o -o bin/client -lrt -lpthread