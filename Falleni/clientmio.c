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

#define MAX_WORDS 10       // Numero massimo di parole che possono essere estratte dalla frase
#define MAX_WORD_LENGTH 50 // Lunghezza massima di ogni parola

#define GIORNI_IN_UN_MESE 31 // informazioni che mi serviranno nella funziona di verifica Data corretta
#define GIORNI_FEBBRAIO 28
#define GIORNI_FEBBRAIO_BISESTILE 29
#define GIORNI_IN_UN_MESE_CORTO 30

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

// Funzione che analizza se la data inserita tramite comando find è una data futura
int data_futura(int GG, int MM, int AA, int HH){
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

/* ########### FINE FUNZIONI DI UTILITA' ###################### */

int main(int argc, char *argv[]){

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

    // Ricezione della lunghezza del messaggio dal server
    ret = riceviLunghezza(sockfd, &lmsg);
    check_errors(ret, sockfd);

    // ripulisco il buffer per la ricezione
    memset(buffer, 0, BUFFER_SIZE);

    // Ricezione del messaggio dal server
    ret = ricevi(sockfd, lmsg, buffer);
    check_errors(ret, sockfd);

    // Verifica del messaggio di risposta del server
    //if (buffer[0] != 'S'){ variante precedente
    if(strcmp(buffer, "SERVER_OK") != 0){
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

		// Inizializzo il set read_fds, manipolato dalla select(), la select eliminerà da read_fds tutti quei descrittori che non sono pronti, lasciando quindi una lista di descrittori pronti che devono essere letti
        read_fds = master; 

        // Controllo i descrittori
		ret = select(fdmax+1, &read_fds, NULL, NULL, NULL);
		if(ret < 0) {
			perror("Errore nella select!");
			exit(1);
		}

        // Scorro i descrittori "i"
        for(i = 0; i <= fdmax; i++){
            if(FD_ISSET(i, &read_fds)){
                strcpy(buffer, ""); // pulisco il buffer inserendo il carattere di fine stringa
                if(i != 0){ // il descrittore pronto è quello di sockfd
                    ret = riceviLunghezza(sockfd, &lmsg);
                    check_errors(ret, i);
                    ret = ricevi(sockfd, lmsg, buffer);
                    check_errors(ret, i);
                    if(strcmp(buffer, "STOP\0") == 0){
                        printf("SERVER chiuso correttamente tramite comando STOP\n");
                        fflush(stdout);

                        // Chiudo la connessione
                        close(sockfd);
                        return 0;
                    }
                    printf("%s\n", buffer);
                    fflush(stdout);
                }
                else if(i == 0){ // il descrittore pronto è quello dello stdin
                    fgets(buffer, BUFFER_SIZE, stdin); // leggo dallo stdin e copio il contenuto in buffer finchè non premo invio
                    if(strcmp(buffer, "esc\0") == 0){ // se digito esc chiudo la connessione 
                        printf("Chiusura....\nArrivederci\n");

                        // Invio un messaggio di disconnessione al server
                        strcpy(buffer, "DISCONNECT");
                        invia(sockfd, buffer);

                        // Chiudo la connesione
                        close(sockfd);
                        return 0;
                    }

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

                    // CODICE DIGITATO: FIND
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
                                ret = invia(sockfd, codice);
                                check_errors(ret, sockfd);

                                sscanf(datiInformazioni[1], "%s", cognome);

                                sprintf(buffer, "%d-%d-%d-%d %d %s", numPersone, giorno, mese, anno, ora, cognome);

                                // mando il vero e proprio MESSAGGIO
                                ret = invia(sockfd, buffer);
                                check_errors(ret, sockfd);

                                sleep(1);

                                printf("I tavoli disponibili che soddisfano la tua richiesta sono:\n");
                                tavoliDisp = 0;
                                for (;;)
                                {
                                    ret = riceviLunghezza(sockfd, &lmsg);
                                    check_errors(ret, sockfd);

                                    ret = ricevi(sockfd, lmsg, buffer);
                                    check_errors(ret, sockfd);

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

                    // CODICE INSERITO: BOOK
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
                            ret = invia(sockfd, codice);
                            check_errors(ret, sockfd);

                            sprintf(buffer, "%d %d-%d-%d-%d %d %s", quantita, numPersone, giorno, mese, anno, ora, cognome);
                            ret = riceviLunghezza(sockfd, &lmsg);
                            check_errors(ret, sockfd);

                            // mando tutti i dati per la prenotazione
                            ret = invia(sockfd, buffer);
                            check_errors(ret, sockfd);

                            // ricevo se ha confermato prenotazione o no
                            ret = riceviLunghezza(sockfd, &lmsg);
                            check_errors(ret, sockfd);
                            
                            ret = ricevi(sockfd, lmsg, buffer);
                            check_errors(ret, sockfd);

                            if (strcmp(buffer, "NO") == 0)
                            {
                                printf("Ripetere prenotazione. \n Il tavolo scelto è stato occupato. \n");
                            }
                            else
                            {
                                memset(buffer, 0, sizeof(buffer));
                                // ricevo se ha confermato prenotazione o no
                                ret = riceviLunghezza(sockfd, &lmsg);
                                check_errors(ret, sockfd);
                                
                                ret = ricevi(sockfd, lmsg, buffer);
                                check_errors(ret, sockfd);
                                printf("PRENOTAZIONE EFFETTUATA, codice: %s.\n", buffer);
                                fflush(stdout);
                            }
                        }
                    }
                    else
                    { // nessun comando inserito
                        printf("ERRORE! Comando inserito non valido. RIPROVARE...\n\n");
                        fflush(stdout);
                        continue;
                    }
                }
            }
        }
    }
}