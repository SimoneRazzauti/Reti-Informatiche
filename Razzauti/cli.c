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

#define WELCOME_CLIENT "BENVENUTO CLIENT\nEcco i comandi disponibili...\n\nfind --> ricerca la disponibilità per una prenotazione\nbook --> invia una prenotazione\nesc --> termina il client\n"
#define BUFFER_SIZE 1024 // dimensione massima del buffer

#define MAX_WORDS 10       // Numero massimo di parole che possono essere estratte dalla frase
#define MAX_WORD_LENGTH 50 // Lunghezza massima di ogni parola

#define GIORNI_IN_UN_MESE 31 // informazioni che mi serviranno nella funziona di verifica Data corretta
#define GIORNI_FEBBRAIO 28
#define GIORNI_FEBBRAIO_BISESTILE 29
#define GIORNI_IN_UN_MESE_CORTO 30

#define validLen 2  // Lunghezza codici fissati tra client - server per send/ recv
#define codiceLen 5 // lunghezza dei codici da mandare al server

// Verifica se la data e l'ora passate come parametro sono valide
int data_valida(int GG, int MM, int AA, int HH)
{
    int is_valid = 0;
    if (MM < 1 || MM > 12)
    {
        return 0; // mese non valido
    }
    // Verifica se l'anno è bisestile
    if (AA % 4 == 0 && (AA % 100 != 0 || AA % 400 == 0))
    {
        switch (MM)
        { // verifica mesi
        case 2:
            is_valid = (GG >= 1 && GG <= GIORNI_FEBBRAIO_BISESTILE); // verifica il mese di febbraio bisestile
            break;
        case 4:
        case 6:
        case 9:
        case 11:
            is_valid = (GG >= 1 && GG <= GIORNI_IN_UN_MESE_CORTO); // verifica i mesi con 30 giorni
            break;
        default:
            is_valid = (GG >= 1 && GG <= GIORNI_IN_UN_MESE); // verifica i mesi con 31 giorni
            break;
        }
    }
    else
    { // anno non bisestile
        switch (MM)
        { // verifica mesi
        case 2:
            is_valid = (GG >= 1 && GG <= GIORNI_FEBBRAIO); // verifica il mese di febbraio non bisestile
            break;
        case 4:
        case 6:
        case 9:
        case 11:
            is_valid = (GG >= 1 && GG <= GIORNI_IN_UN_MESE_CORTO); // verifica i mesi con 30 giorni
            break;
        default:
            is_valid = (GG >= 1 && GG <= GIORNI_IN_UN_MESE); // verifica i mesi con 31 giorni
            break;
        }
    }
    return is_valid && HH >= 10 && HH <= 23; // verifica ora. Orario valido dalle 10 alle 23.
}
int data_futura(int GG, int MM, int AA, int HH)
{
    // Inizializzo a zero una variabile time_t per contenere il tempo corrente
    time_t input_time = 0;
    // Ottengo la struttura tm corrispondente al tempo corrente
    struct tm *input_timeinfo = localtime(&input_time);
    // Imposto i campi della struttura tm con i valori passati come parametro
    input_timeinfo->tm_mday = GG;
    input_timeinfo->tm_mon = MM - 1;
    // Converto l'anno in formato a quattro cifre, se necessario
    input_timeinfo->tm_year = (AA < 100 ? AA + 2000 - 1900 : AA - 1900);
    input_timeinfo->tm_hour = HH;
    // Converto la struttura tm in un tempo in formato time_t
    input_time = mktime(input_timeinfo);
    // Confronto il tempo corrente con il tempo della data passata come parametro
    // e restituisco 1 se la data è futura, 0 altrimenti
    return (input_time >= time(NULL));
}

void gestione_errore(int ret, int socket)
{ // Gestione errori e gestione in caso di chiusura del socket remoto, termino.
    if (ret == -1)
    {
        perror("ERRORE nella comunicazione con il server\n");
        printf("ARRESTO IN CORSO...\n\n");
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

int main(int argc, char *argv[])
{
    int sockfd, ret, max_fd; // variabili per i socket
    char buffer[BUFFER_SIZE];
    struct sockaddr_in serv_addr, cli_addr;
    in_port_t porta = htons(atoi(argv[1])); // utilizzo della funzione atoi per convertire la stringa rappresentante il numero di porta inserito dall'utente da terminale in un intero

    // variabili di utilità
    char identificativo[] = "C\0"; // codice per riconoscere il Client
    char *codice = NULL;           // find o book
    char cognome[30];
    int quantita, wordLen, numPersone, tavoliDisp = 0; // tavoliDisp = numero tavoli restituiti dalla Find
    int ordine = 0;                                    // per controllare che chiami prima la find
    int giorno, mese, anno, ora;

    char *datiInformazioni[MAX_WORDS]; // L'array di puntatori in cui vengono memorizzate le parole
    int word_count = 0;                // Il numero di parole estratte dalla frase

    fd_set master; // variabili per set
    fd_set read_fds;

    uint32_t len_HO; // lunghezza del messaggio espressa in host order
    uint32_t len_NO; // lunghezza del messaggio espressa in network order

    // CREAZIONE SOCKET
    memset((void *)&cli_addr, 0, sizeof(cli_addr));
    memset((void *)&serv_addr, 0, sizeof(serv_addr));

    // Creazione del socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("Errore nella creazione del socket");
        exit(1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(4242);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr); // localhost

    ret = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (ret == -1)
    {
        perror("ERRORE nella connect()");
        printf("ARRESTO IN CORSO...\n");
        fflush(stdout);
        exit(1);
    }

    // CREAZIONE SET E ALTRE INIZIALIZZAZIONI
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    FD_SET(0, &master);
    FD_SET(sockfd, &master);

    max_fd = sockfd;
    // invio codice identificativo Client
    ret = send(sockfd, (void *)identificativo, validLen, 0);
    gestione_errore(ret, sockfd);

    ret = recv(sockfd, (void *)buffer, validLen, 0);
    gestione_errore(ret, sockfd);
    if (buffer[0] != 'S')
    {
        perror("Troppi Client connessi. RIPROVARE.\n\n");
        fflush(stdout);
        close(sockfd);
        exit(1);
    }

    printf(WELCOME_CLIENT);
    fflush(stdout);
    while (1)
    {
        memset(buffer, 0, sizeof(buffer)); // In caso contrario, rimarrebbe l'ultima cosa che c'era dentro.
        read_fds = master;
        select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        if (FD_ISSET(sockfd, &read_fds))
        { // PRONTO SOCKET DI COMUNICAZIONE
            ret = recv(sockfd, &len_NO, sizeof(uint32_t), 0);
            gestione_errore(ret, sockfd);

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
                if (data_valida(giorno, mese, anno, ora))
                {
                    if (data_futura(giorno, mese, anno, ora))
                    {

                        // mando codice "find"
                        ret = send(sockfd, (void *)codice, codiceLen, 0);
                        gestione_errore(ret, sockfd);

                        sscanf(datiInformazioni[1], "%s", cognome);

                        sprintf(buffer, "%d-%d-%d-%d %d %s", numPersone, giorno, mese, anno, ora, cognome);
                        len_HO = strlen(buffer) + 1;
                        len_NO = htonl(len_HO);

                        // mando lunghezza del messaggio
                        ret = send(sockfd, &len_NO, sizeof(uint32_t), 0);
                        gestione_errore(ret, sockfd);

                        // mando il vero e proprio MESSAGGIO
                        ret = send(sockfd, (void *)buffer, len_HO, 0);
                        gestione_errore(ret, sockfd);

                        sleep(1);

                        printf("I tavoli disponibili che soddisfano la tua richiesta sono:\n");
                        tavoliDisp = 0;
                        for (;;)
                        {
                            ret = recv(sockfd, &len_NO, sizeof(uint32_t), 0);
                            gestione_errore(ret, sockfd);
                            len_HO = ntohl(len_NO);

                            ret = recv(sockfd, buffer, len_HO, 0);
                            gestione_errore(ret, sockfd);

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
                    else
                    {
                        printf("La data inserita non risulta essere futura rispetto all'attuale momento corrente.\n");
                        continue;
                    }
                }
                else
                {
                    printf("La data inserita non è valida.\n");
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
                    gestione_errore(ret, sockfd);

                    sprintf(buffer, "%d %d-%d-%d-%d %d %s", quantita, numPersone, giorno, mese, anno, ora, cognome);
                    len_HO = strlen(buffer) + 1;
                    len_NO = htonl(len_HO);

                    ret = send(sockfd, &len_NO, sizeof(uint32_t), 0);
                    gestione_errore(ret, sockfd);

                    // mando tutti i dati per la prenotazione
                    ret = send(sockfd, (void *)buffer, len_HO, 0);
                    gestione_errore(ret, sockfd);

                    // ricevo se ha confermato prenotazione o no
                    ret = recv(sockfd, &len_NO, sizeof(uint32_t), 0);
                    gestione_errore(ret, sockfd);
                    len_HO = ntohl(len_NO);

                    ret = recv(sockfd, buffer, len_HO, 0);
                    gestione_errore(ret, sockfd);

                    if (strcmp(buffer, "NO") == 0)
                    {
                        printf("Ripetere prenotazione. \n Il tavolo scelto è stato occupato. \n");
                    }
                    else
                    {
                        memset(buffer, 0, sizeof(buffer));
                        // ricevo se ha confermato prenotazione o no
                        ret = recv(sockfd, &len_NO, sizeof(uint32_t), 0);
                        gestione_errore(ret, sockfd);
                        len_HO = ntohl(len_NO);

                        ret = recv(sockfd, buffer, len_HO, 0);
                        gestione_errore(ret, sockfd);
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