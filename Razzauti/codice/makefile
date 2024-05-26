all: server client td kd

server: server.c strutture.h funzioni.c funzioni.h
	gcc -Wall -o server server.c funzioni.c

client: client.c
	gcc -Wall -o cli client.c

td: td.c
	gcc -Wall -o td td.c

kd: kd.c
	gcc -Wall -o kd kd.c

clean:
	rm -f server client td kd
