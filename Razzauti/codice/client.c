#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

#define PORTA 4242 // porta del server in ascolto
#define LEN_REPLY 6 // lunghezza del messaggio di risposta
#define BUFFER_SIZE 1024 // dimensione massima del buffer

#define WELCOME_CLIENT "\n*********************** BENVENUTO CLIENTE ************************\n*               Comandi disponibili!                   *\n*                                                        *\n* find --> ricerca la disponibilità per una prenotazione *\n* book --> invia una prenotazione                        *\n* esc --> termina il client                              *\n*                                                        *\n**********************************************************\n"

int main(int argc, char *argv[]){
    int sockfd, ret; // variabili utili al socket
    int len;
    char buffer[BUFFER_SIZE];
    struct  sockaddr_in server_addr, client_addr;
    in_port_t door = htons(atoi(argv[1])); // utilizzo della funzione atoi per convertire la stringa rappresentante il numero di porta inserito dall'utente da terminale in un intero

    // Utility

    // Creazione del socket tramite funzione SOCKET
    memset((void*)&client_addr, 0, sizeof(client_addr)); // pulizia
    memset((void*)&server_addr, 0, sizeof(server_addr)); // pulizia

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        perror("ERRORE: Impossibile creare un nuovo socket");
        exit(1);
    }

    // Inizializzazione della struttura client
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = door;
    client_addr.sin_addr.s_addr = INADDR_ANY;

    ret = bind(sockfd, (struct sockaddr*)&client_addr, sizeof(client_addr));
    if(ret < 0){
        perror("ERRORE: errore nella bind del client\n");
        printf("ARRESTO DEL SISTEMA IN CORSO...\n");
        fflush(stdout);
        exit(1);
    }

    // Inizializzazione della struttura server
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORTA);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    ret = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(ret < 0){
        perror("ERRORE: errore nella connect del client\n");
        printf("ARRESTO DEL SISTEMA IN CORSO...\n");
        fflush(stdout);
        exit(1);
    }
    len = LEN_REPLY;
    ret = recv(sockfd, (void*)buffer, len, 0);
    if(ret < 0){
        perror("ERRORE: errore nella ricezione\n");
        fflush(stdout);
        close(sockfd);
        exit(1);
    }

    // Aggiungo il terminatore stringa che non viene inviato
    buffer[len] = '\0';

    printf("%s\n", buffer);
    fflush(stdout);
    printf(WELCOME_CLIENT);
    fflush(stdout);

    close(sockfd);
    return 0;
}