all: compile run

compile:
	gcc -c queue.c -o queue.o
	gcc server.c queue.o -o server.o
	gcc client.c -o client.o

run:
	./server.o Here 5