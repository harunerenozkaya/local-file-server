all: compile run

compile:
	gcc -c queue.c -o queue.o
	gcc -c file_operations.c -o file_operations.o
	gcc server.c queue.o file_operations.o -o server.o
	gcc client.c file_operations.o -o client.o

run:
	./server.o /tmp/folder 5