#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <sys/time.h>

#define MAX_NUMERO_CLIENTS 20
#define MAX_NUMERO_TAVOLI 15
#define BUFFER_SIZE 1024
#define WELCOME_SERVER "AVVIO DEL SERVER\nCOMANDI DISPONIBILI:\n1) stat ->  restituisce le comande del giorno per tavolo o per status ('a', 'p' o 's') \n2) stop ->  il server si arresta (solo se non ci sono comande in attesa o in preparazione\n"

int main(){
    int sockfd;
    struct sockaddr_in server_addr, client_addr;

    // Creazione del socket tramite funzione SOCKET
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        perror("ERRORE: Impossibile creare un nuovo socket");
        exit(1);
    }  

    // Inizializzazione array e tabella 

    // Inizializzazione della struttura per l'indirizzo del server

    // BIND sul nuovo socket creato
    if(bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        perror("ERRORE: Binding del socket errata");
        exit(1);
    }

    // LISTEN sul nuovo socket creato
    if(listen(sockfd, MAX_NUMERO_CLIENTS) < 0){
        perror("ERRORE: Non è stato possibile avviare l'ascolto delle connessioni");
        exit(1);
    }

    // Reset del file descriptor della select 

    // Stampa del benvenuto del server
    printf(WELCOME_SERVER);
    fflush(stdout);

    // Ciclo principale
    while(1){

    }
}