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

#include "funzioni.c" // funzioni varie
#include "strutture.h" // strutture

#define BUFFER_SIZE 1024 // dimensione max del buffer
#define WELCOME_CLIENT "BENVENUTO CLIENTE\nComandi disponibili...\n\nfind --> ricerca la disponibilita' per una prenotazione\nbook --> invia una prenotazione\nesc --> termina il client\n" // messaggio di benvenuto 

#define MAX_WORDS 10 // Numero massimo di parole che possono essere estratte dalla frase
#define MAX_WORD_LENGTH 50 // Lunghezza massima di ogni parola
#define LEN_ID 2  // lunghezza codici fissati per identificare il tipo di client al server (client-kd-td)
#define LEN_COMANDO 5 // lunghezza dei comandi da mandare al server

int main(int argc, char *argv[]){
    // variabili per i socket
    int sockfd, ret; 
    char buffer[BUFFER_SIZE];
    struct sockaddr_in server_addr;

    // variabili di utilità
    char id[] = "C\0"; // codice per riconoscere il Client
    char cognome[25];
    int giorno, mese, anno, ora;
    int quantita, chunk_len, nPersone, tavoliDisp = 0; // tavoliDisp = numero tavoli restituiti dalla Find
    int priorita = 0; // server a controllare che prima si faccia la find e dopo book
    char *chunk; // per l'estrazione delle parole dal buffer
    int chunk_count = 0; // numero di parole estratte dal buffer
    char *info[MAX_WORDS]; // array di puntatori in cui vengono memorizzate le parole estratte dal buffer con la strtok
    char *codice = NULL; // find o book

    // variabili per la select
    fd_set master; // set di descrittori da monitorare
    fd_set read_fds; // sed di descrittori pronti
    int fdmax;// Descrittore max

    uint32_t len_HO; // lunghezza del messaggio scambiato espressa in host order
    uint32_t len_NO; // lunghezza del messaggio scambiato espressa in network order

    // CREAZIONE del socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        perror("Errore: impossibile creare un nuovo socket\n");
        exit(1);
    }

    // CREAZIONE indirizzo del server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(4242);
	inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    // CONNESSIONE
    ret = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0){
        perror("Errore: errore nella connessione\n");
        printf("ARRESTO IN CORSO...\n");
        fflush(stdout);
        exit(1);
    }

    // Resetto i descrittori di socket
    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    // Aggiungo il socket di ascolto 'listener' e 'stdin' (0) ai socket monitorati
    FD_SET(sockfd, &master);
    FD_SET(0, &master);

    // Aggiorno il nuovo fdmax
    fdmax = sockfd;

    // Invio codice id client = 'C' al server
    ret = send(sockfd, (void *)id, LEN_ID, 0);
    check_errori(ret, sockfd);

    ret = recv(sockfd, (void *)buffer, LEN_ID, 0);
    check_errori(ret, sockfd);
    if (buffer[0] != 'S'){
        perror("Ci sono troppi Client connessi, per favore RIPROVARE.\n\n");
        fflush(stdout);
        close(sockfd);
        exit(1);
    }

    printf(WELCOME_CLIENT);
    fflush(stdout);

    // CICLO PRINCIPALE
    while (1){
        memset(buffer, 0, BUFFER_SIZE); // ripulsco il buffer di comunicazione
        read_fds = master; // copia del set da monitorare
        ret = select(fdmax + 1, &read_fds, NULL, NULL, NULL);
		if(ret < 0) {
			perror("Errore nella select!");
			exit(1);
		}

        // CASO 1: PRONTO SOCKET DI COMUNICAZIONE
        if (FD_ISSET(sockfd, &read_fds)){

            // ricevo la lunghezza del messaggio
            ret = recv(sockfd, &len_NO, sizeof(uint32_t), 0);
            check_errori(ret, sockfd);
            len_HO = ntohl(len_NO);

            // ricevo il messaggio 
            ret = recv(sockfd, buffer, len_HO, 0);
            check_errori(ret, sockfd);

            // se il server ha detto "stop", avviso e termino.
            if (strncmp(buffer, "STOP", strlen("STOP")) == 0){ // strncmp compara le prime n lettere con n passato come terzo parametro
                printf("\nAVVISO: il server si e' arrestato tramite comando STOP.\n\n");
                fflush(stdout);
                close(sockfd);
                exit(0);
            }              
            // altrimenti, il server sta inviando un messaggio da stampare
            printf("%s\n", buffer); // stampo il messaggio
            fflush(stdout);
        }        
        // CASO 2: PRONTO SOCKET stdin
        else{
            // salvo nel buffer il contenuto digitato da stdin 
            fgets(buffer, BUFFER_SIZE, stdin);

            // Estrae le parole dalla frase utilizzando 'strtok' e lo spazio come delimitatore a blocchi di chunks
            chunk = strtok(buffer, " ");
            chunk_count = 0;

            // Finche' ci sono parole da estrarre e non si supera il limite massimo
            while (chunk != NULL && chunk_count < MAX_WORDS){ 
                // Rimuove il carattere di fine riga dalla parola se presente
                chunk_len = strlen(chunk);
                if (chunk[chunk_len - 1] == '\n'){
                    chunk[chunk_len - 1] = '\0';
                }

                // Aggiunge la parola all'array di parole
                info[chunk_count] = chunk; // Memorizza il puntatore alla parola nell'array
                chunk_count++;                        // Incrementa il contatore di parole estratte

                // Estrae la prossima parola
                chunk = strtok(NULL, " "); // Utilizza 'NULL' come primo parametro per estrarre le parole successive
            }

            // ******* Adesso tutte le parole estratte da input sono salvate nell'array info

            // Se la prima parola e' find ho digitato il comando find 
            if (strcmp(info[0], "find") == 0){ 
                // info[1] = cognome, info[2] = Num. persone, info[3] = data e info[4] = ora

                // inizializzo le variabili
                sscanf(info[2], "%d", &nPersone);
                sscanf(info[3], "%d-%d-%d", &giorno, &mese, &anno);
                sscanf(info[4], "%d", &ora);

                codice = "find\0";

                // controllo data inserita
                if (check_data(giorno, mese, anno, ora)){
                    
                    // invio codice "find" al server
                    ret = send(sockfd, (void *)codice, LEN_COMANDO, 0);
                    check_errori(ret, sockfd);

                    sscanf(info[1], "%s", cognome);

                    sprintf(buffer, "%d-%d-%d-%d %d %s", nPersone, giorno, mese, anno, ora, cognome);
                    len_HO = strlen(buffer) + 1;
                    len_NO = htonl(len_HO);

                    // invio lunghezza del messaggio
                    ret = send(sockfd, &len_NO, sizeof(uint32_t), 0);
                    check_errori(ret, sockfd);

                    // invio il messaggio
                    ret = send(sockfd, (void *)buffer, len_HO, 0);
                    check_errori(ret, sockfd);

                    // delay per il trasferimento
                    sleep(1);

                    tavoliDisp = 0;
                    for (;;){
                        // ricevo la lunghezza del messaggio
                        ret = recv(sockfd, &len_NO, sizeof(uint32_t), 0);
                        check_errori(ret, sockfd);
                        len_HO = ntohl(len_NO);

                        // ricevo il messaggio
                        ret = recv(sockfd, buffer, len_HO, 0);
                        check_errori(ret, sockfd);

                        if (strncmp(buffer, "STOP", strlen("STOP")) == 0) { 
                            // per fermare il loop
                            printf("\n");
                            fflush(stdout);
                            if (tavoliDisp == 0){ // se nessun tavolo e' stato proposto, avviso
                                printf("Non ci sono tavoli disponibili. \nPer favore, prova ad inserire una data diversa o un numero inferiore di commensali.\n");
                                fflush(stdout);
                            }
                            break;
                        }
                        // stampo i tavoli che il server mi fornisce
                        tavoliDisp++;
                        printf("I tavoli disponibili che soddisfano la tua richiesta sono:\n");
                        printf("%s\n", buffer);
                    }
                    fflush(stdout);

                    // TAVOLI DISPONIBILI
                    priorita = 1; // adesso e' possibile selezionare book
                }
                else{
                    printf("Hai inserito una data non valida oppure non e' una data futura, riprova.\n");
                    continue;
                }
            }
            
            // Se la prima parola estratta dal buffer e' book gestisco il comando book
            else if (strcmp(info[0], "book") == 0 && priorita == 0){
                printf("Non puoi prenotare un tavolo prima di effettuare una prenotazione. Riprova con una find.\n\n");
                fflush(stdout);
            }
            // book corretta, gestisco...
            else if (strcmp(info[0], "book") == 0 && priorita == 1){
                // la seconda parola estratta deve essere un decimale e si riferisce al tavolo nella lista dei tavoli proposti dal server
                sscanf(info[1], "%d", &quantita);

                // se il numeri scelto e' maggiore di quanti la find abbia restituito
                if (tavoliDisp < quantita) {
                    printf("Numero non valido, ri eseguire la prenotazione...\n");
                    fflush(stdout);
                    continue;
                }
                else{
                    // invio codice "book" al server
                    codice = "book\0";
                    ret = send(sockfd, (void *)codice, LEN_COMANDO, 0);
                    check_errori(ret, sockfd);

                    // innvio i dettagli della prenotazione al server
                    sprintf(buffer, "%d %d-%d-%d-%d %d %s", quantita, nPersone, giorno, mese, anno, ora, cognome);
                    len_HO = strlen(buffer) + 1;
                    len_NO = htonl(len_HO);

                    // invio la dimensione
                    ret = send(sockfd, &len_NO, sizeof(uint32_t), 0);
                    check_errori(ret, sockfd);

                    // invio tutti i dati per la prenotazione
                    ret = send(sockfd, (void *)buffer, len_HO, 0);
                    check_errori(ret, sockfd);

                    // ricevo se ha confermato prenotazione o no
                    memset(buffer, 0, BUFFER_SIZE); // pulisco il buffer
                    ret = recv(sockfd, &len_NO, sizeof(uint32_t), 0);
                    check_errori(ret, sockfd);
                    len_HO = ntohl(len_NO);

                    // ricevo il messaggio
                    ret = recv(sockfd, buffer, len_HO, 0);
                    check_errori(ret, sockfd);

                    // mutua esclusione
                    if (strcmp(buffer, "NO") == 0){
                        printf("Ripetere prenotazione. \n Il tavolo che hai scelto e' stato occupato. \n");
                    }

                    // **** prenotazione effettuata con successo
                    else{
                        memset(buffer, 0, BUFFER_SIZE);
                        ret = recv(sockfd, &len_NO, sizeof(uint32_t), 0);
                        check_errori(ret, sockfd);
                        len_HO = ntohl(len_NO);

                        ret = recv(sockfd, buffer, len_HO, 0);
                        check_errori(ret, sockfd);
                        printf("Prenotazione effettuata con successo, codice: %s.\n", buffer);
                        priorita = 0;
                        continue;
                    }
                }
            }
            else if (strcmp(info[0], "esc") == 0){
                printf("Uscita in corso...\nArrivederci :)\n\n");
                fflush(stdout);
                // TO DO: si potrebbe inviare un messaggio al server per dire che il client si e' disconnesso
                close(sockfd);
                exit(0);
            }
            else{ // nessun comando inserito valido
                printf("Errore: Comando non consentito, RIPROVA...\n\n");
                continue;
            }
        }
    }
    close(sockfd);
    return 0;
}
