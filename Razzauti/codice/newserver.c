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

int main(int argc, char* argv[]){
    int sockfd, new_sockfd;
    struct sockaddr_in server_addr, client_addr; // per le primitive bind e accept
    in_port_t door = htons(atoi(argv[1])); // uso la funzione atoi che fa parte della libreria stdlib.h per convertire una stringa in un intero, in questo modo converto la stringa passata dall'utente in un intero che rappresenta la porta del server 
    char buffer[BUFFER_SIZE];
    int ret, len;

    // Creazione del socket tramite funzione SOCKET
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        perror("ERRORE: Impossibile creare un nuovo socket");
        exit(1);
    }  

    // Inizializzazione array e tabella per tavoli e comande

    // Inizializzazione della struttura per l'indirizzo del server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = door;

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
        // Pronto descrittore socket di ascolto

        // Dimensione dell'indirizzo del client
        socklen_t client_len = sizeof(client_addr);

        // Accetto nuove connesioni
        new_sockfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_len);
        if(new_sockfd < 0){
            perror("ERRORE: Impossibile creare un nuovo socket di ascolto\n");
            exit(1);
        }

        // Creo la risposta
        strcpy(buffer,"HELLO!");
        len = strlen(buffer);

        // Invio risposta senza \0
        ret = send(new_sockfd, (void*)buffer, len, 0);

        if(ret < 0){
            perror("Errore nella fase di invio della send\n");
            exit(1);
        }
        close(new_sockfd);
    }
    close(sockfd);
    return 0;
}