all: server client td kd

server: server.o
	gcc -Wall server.o -o server

client: client.o
	gcc -Wall -o client.o -o client

td: td.o
	gcc -Wall td.o -o td

kd: kd.o
	gcc -Wall kd.o -o kd

clean:
	rm *.o server client td kd
