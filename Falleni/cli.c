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

#define validLen 2 
#define PORTA 4242 // porta del server in ascolto
#define BUFFER_SIZE 1024 // dimensione massima del buffer in byte
#define WELCOME_CLIENT "\n*********************** BENVENUTO CLIENTE ************************\n*Comandi disponibili!*\n**\n* find --> ricerca la disponibilità per una prenotazione*\n* book --> invia una prenotazione*\n*esc --> termina il client*\n**\n**********************************************************\n"

#define MAX_WORDS 10       // Numero massimo di parole che possono essere estratte dalla frase
#define MAX_WORD_LENGTH 50 // Lunghezza massima di ogni parola

#define GIORNI_IN_UN_MESE 31 // informazioni che mi serviranno nella funziona di verifica Data corretta
#define GIORNI_FEBBRAIO 28
#define GIORNI_FEBBRAIO_BISESTILE 29
#define GIORNI_IN_UN_MESE_CORTO 30
/* ########### FINE FUNZIONI DI UTILITA' ###################### */


// Invia al socket in input il messaggio dentro buffer
int invia(int j, char* buffer) {
	int ret;
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

    // CREAZIONE SET E ALTRE INIZIALIZZAZIONI
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    FD_SET(0, &master);
    FD_SET(sockfd, &master);

    max_fd = sockfd;
    
printf("ciao dopo la conenct");
    // Invio del codice identificativo al server: client == 'C'
    
    // Invio del codice identificativo al server: client == 'C'
    strcpy(buffer, "C\0");
    int len = strlen(buffer)+1;

    ret = send(sockfd, (void *)buffer, validLen, 0);


    printf("OK");

    ret = recv(sockfd, (void *)buffer, validLen, 0);
    
    if (buffer[0] != 'S')
    {
        perror("Troppi Client connessi. RIPROVARE.\n\n");
        fflush(stdout);
        close(sockfd);
        exit(1);
    }
    while(1){

    }
}//ciao