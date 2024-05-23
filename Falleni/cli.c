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
#define BUFFER_SIZE 1024 // dimensione massima del buffer in byte

/* ########### FINE FUNZIONI DI UTILITA' ###################### */

int main(int argc, char *argv[]){
    printf("ciao uno");
    // Variabili dei socket
    int sockfd, ret, i, lmsg;
    struct  sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];
    
    // Variabili di utilità per l'IO-MULTIPLEXING
    fd_set master; // set di descrittori da monitorare
    fd_set read_fds; // set di descrittori in stato pronti
    int fdmax; // descrittore max

    // Varibili per le funzioni find e book
    char *codice = NULL;           // find o book
    char cognome[30];
    int quantita, wordLen, numPersone, tavoliDisp = 0; // tavoliDisp = numero tavoli restituiti dalla Find
    int ordine = 0;                                    // per controllare che chiami prima la find
    int giorno, mese, anno, ora;

    char *datiInformazioni[MAX_WORDS]; // L'array di puntatori in cui vengono memorizzate le parole
    int word_count = 0;                // Il numero di parole estratte dalla frase

    // Creazione del socket tramite funzione SOCKET
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        perror("ERRORE: Impossibile creare un nuovo socket\n");
        fflush(stdout);
        exit(1);
    }
printf("ciao 2");
    // Inizializzazione della struttura dell indirizzo del server
    memset((void*)&server_addr, 0, sizeof(server_addr)); // pulizia
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORTA);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr); // localhost
printf("ciao 3");
    // Connessione tramite la primitiva CONNECT
    ret = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(ret < 0){
        perror("ERRORE: errore nella connect del client\n");
        printf("ARRESTO DEL SISTEMA IN CORSO...\n");
        fflush(stdout);
        exit(1);
    }
printf("ciao dopo la conenct");
    // Invio del codice identificativo al server: client == 'C'
    
}//ciao