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
#define WELCOME_CLIENT "\n*********************** BENVENUTO CLIENTE ************************\n*Comandi disponibili!*\n**\n* find --> ricerca la disponibilità per una prenotazione*\n* book --> invia una prenotazione*\n*esc --> termina il client*\n**\n**********************************************************\n"

/* #define MAX_WORDS 10       // Numero massimo di parole che possono essere estratte dalla frase
#define MAX_WORD_LENGTH 50 // Lunghezza massima di ogni parola

#define GIORNI_IN_UN_MESE 31 // informazioni che mi serviranno nella funziona di verifica Data corretta
#define GIORNI_FEBBRAIO 28
#define GIORNI_FEBBRAIO_BISESTILE 29
#define GIORNI_IN_UN_MESE_CORTO 30

#define validLen 2  // Lunghezza codici fissati tra client - server per send/ recv
#define codiceLen 5 // lunghezza dei codici da mandare al server */


/* ########### FUNZIONI DI UTILITA' (avrei potuto scriverle in un file separato ma dovrei riscrivere il makefile, ho lasciato tutto in un file per compattezza) */

// Gestione errori per la send e la receive tra client e server
void check_errors(int ret, int socket){ 
    if (ret == -1)
    {
        perror("ERRORE nella comunicazione con il server\nARRESTO IN CORSO...\n");
        fflush(stdout);
        close(socket);
        exit(1);
    }
    else if (ret == 0)
    {
        printf("AVVISO: il server ha chiuso il socket remoto.\nARRESTO IN CORSO...\n");
        fflush(stdout);
        close(socket);
        exit(1);
    }
}

// Invia al socket in input il messaggio dentro buffer
int invia(int j, char* buffer) {
	int len = strlen(buffer)+1;
	int lmsg = htons(len);
	int ret;

	// Invio la dimensione del messaggio
	ret = send(j, (void*) &lmsg, sizeof(uint16_t), 0);
	// Invio il messaggio
	ret = send(j, (void*) buffer, len, 0);

	// Comunico l'esito
	return ret;
}

// Ricevi dal socket in input la lunghezza del messaggio e lo mette dentro lmsg
int riceviLunghezza(int j, int *lmsg) {
	int ret;
	ret = recv(j, (void*)lmsg, sizeof(uint16_t), 0);
	return ret;
}

// Riceve dal socket in input il messaggio e lo mette dentro buffer
int ricevi(int j, int lunghezza, char* buffer) {
	int ret;
	ret = recv(j, (void*)buffer, lunghezza, 0);
	return ret;
}
/* ########### FINE FUNZIONI DI UTILITA' ###################### */

int main(int argc, char *argv[]){

    // Variabili dei socket
    int sockfd, ret, i, lmsg;
    struct  sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];
    
    // Variabili di utilità
    fd_set master; // set di descrittori da monitorare
    fd_set read_fds; // set di descrittori in stato pronti
    int fdmax; // descrittore max

    // Creazione del socket tramite funzione SOCKET
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        perror("ERRORE: Impossibile creare un nuovo socket\n");
        fflush(stdout);
        exit(1);
    }

    // Inizializzazione della struttura dell indirizzo del server
    memset((void*)&server_addr, 0, sizeof(server_addr)); // pulizia
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORTA);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr); // localhost

    // Connessione tramite la primitiva CONNECT
    ret = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(ret < 0){
        perror("ERRORE: errore nella connect del client\n");
        printf("ARRESTO DEL SISTEMA IN CORSO...\n");
        fflush(stdout);
        exit(1);
    }

    // Invio del codice identificativo al server: client == 'C'
    strcpy(buffer, "C\0");
    ret = invia(sockfd, buffer);
    check_errors(ret, sockfd);

    ret = riceviLunghezza(sockfd, &lmsg);
    check_errors(ret, sockfd);

    // ripulisco il buffer per la ricezione
    memset(buffer, 0, BUFFER_SIZE);

    ret = ricevi(sockfd, lmsg, buffer);
    check_errors(ret, sockfd);
    if (buffer[0] != 'S')
    {
        perror("Troppi Client connessi. RIPROVARE.\n\n");
        fflush(stdout);
        close(sockfd);
        exit(1);
    }

    // Reset dei descrittori di socket
    FD_ZERO(&master);
    FD_ZERO(&read_fds);

	// Aggiungo il socket di ascolto 'listener' e 'stdin' (0) ai socket monitorati
    FD_SET(sockfd, &master); 
    FD_SET(0, &master); 
    
    // Tengo traccia del nuovo fdmax
    fdmax = sockfd;

    // Stampa dei comandi del client da terminale
    printf(WELCOME_CLIENT);
    fflush(stdout);

    for(;;){
        memset(buffer, 0, BUFFER_SIZE); // ripulisco il buffer di comunicazione

    }
}