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

#define BUFFER_SIZE 1024 // dimensione max del buffer
#define WELCOME_CLIENT "BENVENUTO CLIENTE\nComandi disponibili...\n\nfind --> ricerca la disponibilità per una prenotazione\nbook --> invia una prenotazione\nesc --> termina il client\n" // messaggio di benvenuto 

#define MAX_WORDS 10       // Numero massimo di parole che possono essere estratte dalla frase
#define MAX_WORD_LENGTH 50 // Lunghezza massima di ogni parola

#define LEN_ID 2  // lunghezza codici fissati per identificare il tipo di client al server (client-kd-td)
#define codiceLen 5 // lunghezza dei codici da mandare al server

int main(int argc, char *argv[]){
    int sockfd, ret; // variabili per i socket
    char buffer[BUFFER_SIZE];
    struct sockaddr_in server_addr;

    // variabili di utilità
    char *id; // codice per riconoscere il Client
    char *codice = NULL;           // find o book
    char cognome[30];
    int quantita, wordLen, numPersone, tavoliDisp = 0; // tavoliDisp = numero tavoli restituiti dalla Find
    int ordine = 0;                                    // per controllare che chiami prima la find
    int giorno, mese, anno, ora;

    char *datiInformazioni[MAX_WORDS]; // L'array di puntatori in cui vengono memorizzate le parole
    int word_count = 0;                // Il numero di parole estratte dalla frase

    // set di descrittori da monitorare
    fd_set master; 

    // sed di descrittori pronti
    fd_set read_fds;

    // Descrittore max
	int fdmax;

    uint32_t len_HO; // lunghezza del messaggio espressa in host order
    uint32_t len_NO; // lunghezza del messaggio espressa in network order

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
    strcpy(id, "C\0");
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
    while (1)
    {
        memset(buffer, 0, sizeof(buffer)); // In caso contrario, rimarrebbe l'ultima cosa che c'era dentro.
        read_fds = master;
        select(fdmax + 1, &read_fds, NULL, NULL, NULL);

        if (FD_ISSET(sockfd, &read_fds))
        { // PRONTO SOCKET DI COMUNICAZIONE
            ret = recv(sockfd, &len_NO, sizeof(uint32_t), 0);
            check_errori(ret, sockfd);

            len_HO = ntohl(len_NO);
            ret = recv(sockfd, buffer, len_HO, 0);
            if (strncmp(buffer, "STOP", strlen("STOP")) == 0)
            { // se il server ha usato lo "stop", avviso e termino.
                printf("AVVISO: il server è stato arrestato.\n\n");
                fflush(stdout);
                close(sockfd);
                exit(0);
            }                       // altrimenti, il server sta inviando un messaggio da stampare
            printf("%s\n", buffer); // Mi serve allora solo stampare il messaggio.
            fflush(stdout);
        }
        else
        {

            fgets(buffer, BUFFER_SIZE, stdin);

            // Estrae le parole dalla frase utilizzando la funzione 'strtok'
            char *word = strtok(buffer, " "); // Estrae la prima parola utilizzando lo spazio come delimitatore
            word_count = 0;
            while (word != NULL && word_count < MAX_WORDS)
            { // Finchè ci sono parole da estrarre e non si supera il limite massimo
                // Rimuove il carattere di fine riga dalla parola se presente
                wordLen = strlen(word);
                if (word[wordLen - 1] == '\n')
                {
                    word[wordLen - 1] = '\0';
                }

                // Aggiunge la parola all'array di parole
                datiInformazioni[word_count] = word; // Memorizza il puntatore alla parola nell'array
                word_count++;                        // Incrementa il contatore di parole estratte

                // Estrae la prossima parola
                word = strtok(NULL, " "); // Utilizza 'NULL' come primo parametro per estrarre le parole successive
            }

            if (strcmp(datiInformazioni[0], "find") == 0)
            { // se siamo qui datiInformazioni[1] = cognome, datiInformazioni[2] = Num. persone, datiInformazioni[3] = data e datiInformazioni[4] = ora
                // analizza la stringa e assegna i valori alle variabili
                sscanf(datiInformazioni[3], "%d-%d-%d", &giorno, &mese, &anno);
                sscanf(datiInformazioni[4], "%d", &ora);
                sscanf(datiInformazioni[2], "%d", &numPersone);

                codice = "find\0";
                // controllo data inserita
                if (check_data(giorno, mese, anno, ora)){
                  

                        // mando codice "find"
                        ret = send(sockfd, (void *)codice, codiceLen, 0);
                        check_errori(ret, sockfd);

                        sscanf(datiInformazioni[1], "%s", cognome);

                        sprintf(buffer, "%d-%d-%d-%d %d %s", numPersone, giorno, mese, anno, ora, cognome);
                        len_HO = strlen(buffer) + 1;
                        len_NO = htonl(len_HO);

                        // mando lunghezza del messaggio
                        ret = send(sockfd, &len_NO, sizeof(uint32_t), 0);
                        check_errori(ret, sockfd);

                        // mando il vero e proprio MESSAGGIO
                        ret = send(sockfd, (void *)buffer, len_HO, 0);
                        check_errori(ret, sockfd);

                        sleep(1);

                        printf("I tavoli disponibili che soddisfano la tua richiesta sono:\n");
                        tavoliDisp = 0;
                        for (;;)
                        {
                            ret = recv(sockfd, &len_NO, sizeof(uint32_t), 0);
                            check_errori(ret, sockfd);
                            len_HO = ntohl(len_NO);

                            ret = recv(sockfd, buffer, len_HO, 0);
                            check_errori(ret, sockfd);

                            if (strncmp(buffer, "STOP", strlen("STOP")) == 0) // per fare terminare il loop
                            {
                                printf("\n");
                                fflush(stdout);
                                if (tavoliDisp == 0)
                                { // se nessun tavolo è stato proposto, avviso
                                    printf("Non sono disponibili tavoli che soddisfino i requisiti richiesti. \nTi invitiamo a provare con una data differente o con un numero inferiore di ospiti.\n");
                                    fflush(stdout);
                                }
                                break;
                            }
                            tavoliDisp++;
                            printf("%s\n", buffer);
                        }
                        fflush(stdout);

                        // TAVOLI DISPONIBILI
                        ordine = 1; // adesso è possibile selezionare book
                }
                else{
                    printf("Hai inserito una data non valida oppure non è una data futura, riprova.\n");
                    continue;
                }

                // dopo questo possiamo andare a book. Inserito alla fine, dopo tutte le conferme è valido
            }
            else if (strcmp(datiInformazioni[0], "book") == 0 && ordine == 0)
            {
                printf("Prima del book eseguire una Find correttamente\n\n");
                fflush(stdout);
            }
            else if (strcmp(datiInformazioni[0], "book") == 0 && ordine == 1)
            {

                sscanf(datiInformazioni[1], "%d", &quantita);

                if (tavoliDisp < quantita) // se il numeri scelto è > di quanti la find abbia restituito
                {
                    printf("Numero non valido, ri eseguire la prenotazione...\n");
                    // ordine = 0;
                    fflush(stdout);
                    continue;
                }
                else
                {
                    // mando codice "book"
                    codice = "book\0";
                    ret = send(sockfd, (void *)codice, codiceLen, 0);
                    check_errori(ret, sockfd);

                    sprintf(buffer, "%d %d-%d-%d-%d %d %s", quantita, numPersone, giorno, mese, anno, ora, cognome);
                    len_HO = strlen(buffer) + 1;
                    len_NO = htonl(len_HO);

                    ret = send(sockfd, &len_NO, sizeof(uint32_t), 0);
                    check_errori(ret, sockfd);

                    // mando tutti i dati per la prenotazione
                    ret = send(sockfd, (void *)buffer, len_HO, 0);
                    check_errori(ret, sockfd);

                    // ricevo se ha confermato prenotazione o no
                    ret = recv(sockfd, &len_NO, sizeof(uint32_t), 0);
                    check_errori(ret, sockfd);
                    len_HO = ntohl(len_NO);

                    ret = recv(sockfd, buffer, len_HO, 0);
                    check_errori(ret, sockfd);

                    if (strcmp(buffer, "NO") == 0)
                    {
                        printf("Ripetere prenotazione. \n Il tavolo scelto è stato occupato. \n");
                    }
                    else
                    {
                        memset(buffer, 0, sizeof(buffer));
                        // ricevo se ha confermato prenotazione o no
                        ret = recv(sockfd, &len_NO, sizeof(uint32_t), 0);
                        check_errori(ret, sockfd);
                        len_HO = ntohl(len_NO);

                        ret = recv(sockfd, buffer, len_HO, 0);
                        check_errori(ret, sockfd);
                        printf("PRENOTAZIONE EFFETTUATA, codice: %s.\n", buffer);
                    }
                }
            }
            else if (strcmp(datiInformazioni[0], "esc") == 0)
            {
                printf("Uscita in corso...\n\n");
                fflush(stdout);
                close(sockfd);
                exit(0);
            }
            else
            { // nessun comando inserito
                printf("ERRORE! Comando inserito non valido. RIPROVARE...\n\n");
                continue;
            }
        }
    }

    close(sockfd);
    return 0;
}