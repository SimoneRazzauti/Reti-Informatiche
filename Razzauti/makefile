all: cli td kd server

server: server.o
	gcc -Wall server.o -o server

cli: cli.o funzioni.c funzioni.h
	gcc -Wall cli.o -o cli

kd: kd.o
	gcc -Wall kd.o -o kd

td: td.o
	gcc -Wall td.o -o td

clean:
	rm *.o cli td kd server