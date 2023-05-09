all: compile run

compile:
	g++ server.cpp -o server.o
	g++ client.cpp -o client.o

run:
	./server.o