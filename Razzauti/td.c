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

#define BUFFER_SIZE 1024 // dimensione massima del buffer
#define WELCOME "Benvenuto.\n"
#define WELCOME_TD1 "Inserisci il codice prenotazione: "
#define WELCOME_TD2 "\n***************** BENVENUTO AL RISTORANTE *****************\n Digita un comando:\n1) help --> mostra i dettagli dei comandi\n2) menu --> mostra il menu dei piatti\n3) comanda --> invia una comanda\n4) conto --> chiede il conto\n\n"
#define HELPER "Comandi:\nmenu -> stampa il menu\ncomanda -> invia una comanda in cucina\n\t   NOTA: deve essere nel formato\n \t   {<piatto_1-quantita'_1>...<piatto_n-quantita'_n>}\nconto -> richiesta del conto\n\n"

#define MAX_WORDS 50 // numero massimo di parole che possono essere estratte dalla frase
#define LEN_ID 2 // lunghezza codici fissati per identificare il tipo di client al server (client-kd-td)
#define LEN_COMANDO 5 // lunghezza dei comandi da mandare al server

#define MAX_PIATTI 15 // numero massimo di piatti nel menu
#define MAX_COMANDE_IN_ATTESA 10
#define DESCRIZIONE 100 // descrizione del piatto

// Controlla se i piatti selezionati siano nel Menu
int check_menu(char info[15], int quanti_piatti){
    int x = 0;
    char piattoN[2];
    sscanf(info, "%2s-%*d", piattoN); // mi salvo il piatto per controllare che sia nel Menu
    for (x = 0; x <= quanti_piatti; x++){
        // se trova il piatto nel menu
        if (strcmp(menu[x].id, piattoN) == 0){ 
            return 1;
        }
    }
    return 0;
}

int main(int argc, char *argv[]){
    // variabili per i socket
    int sockfd, ret; 
    char buffer[BUFFER_SIZE];
    struct sockaddr_in server_addr;

    // variabili di utiltà
    int welcome = 1; // per l'output
    char id[] = "T\0"; // codice per riconoscere il TABLE DEVICE
    int chunk_len, prezzo, a, k, j, quanti_piatti = 0;
    int quante_comande = 0; // numero di comande che deve gestire il server
    char tavoloMemorizzato[5]; //memorizza il tavolo assegnato al cliente come stringa
    int richiestaMenu = 0; // server per richiedere il menu al server la prima volta o casharlo
    int errore = 0; // controllo se il piatto scelto e' nel menu
    int ordine = 0; // controlo se e' stato prima richiesto il menu per inviare una comanda o un conto
    char *chunk; // per l'estrazione delle parole dal buffer
    uint16_t chunk_count = 0; // Il numero di parole estratte dalla frase
    char *info[MAX_WORDS]; // array di puntatori in cui vengono memorizzate le parole estratte dal buffer con la strtok
    char *codice = NULL; // help, menu, comanda, conto

    // variabili per la select
    fd_set master; // set di descrittori da monitorare
    fd_set read_fds; // sed di descrittori pronti
	int fdmax; // descrittore max

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

    // Invio codice id td = 'T' al server
    ret = send(sockfd, (void *)id, LEN_ID, 0);
    check_errori(ret, sockfd);

    ret = recv(sockfd, (void *)buffer, LEN_ID, 0);
    check_errori(ret, sockfd);
    if (buffer[0] != 'S'){
        perror("Il ristorante e' pieno\n\n");
        fflush(stdout);
        close(sockfd);
        exit(1);
    }

    // Check-in del table device
    while (1){
        // stampo anche il benvenuto se il table device e' stato avviato per la prima volta
        if(welcome)
            printf(WELCOME);
        printf(WELCOME_TD1);
        fflush(stdout);
        
        memset(buffer, 0, BUFFER_SIZE); // ripulsco il buffer di comunicazione
        read_fds = master; // copia del set da monitorare
        ret = select(fdmax + 1, &read_fds, NULL, NULL, NULL);
		if(ret < 0) {
			perror("Errore nella select!");
			exit(1);
		}

        // controllo che il server non abbia chiuso la connessione mentre aspetto il codice del tavolo
        if (FD_ISSET(sockfd, &read_fds)){
 
            // ricevo la lunghezza del messaggio
            ret = recv(sockfd, &len_NO, sizeof(uint32_t), 0);
            check_errori(ret, sockfd);
            len_HO = ntohl(len_NO);

            // ricevo il messaggio del server
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
           
        // salvo su buffer il contenuto dello stdin
        fgets(buffer, BUFFER_SIZE, stdin);
        codice = "code\0";

        // invio codice "code" per identificare la sezione nello script server
        ret = send(sockfd, (void *)codice, LEN_COMANDO, 0);
        check_errori(ret, sockfd);

        len_HO = strlen(buffer) + 1;
        len_NO = htonl(len_HO);

        // invio lunghezza del codice
        ret = send(sockfd, &len_NO, sizeof(uint32_t), 0);
        check_errori(ret, sockfd);

        // invio MESSAGGIO codice
        ret = send(sockfd, (void *)buffer, len_HO, 0);
        check_errori(ret, sockfd);

        // tavolo da 0-9
        if (buffer[2] == '-'){ 
            sscanf(buffer, "%2s", tavoloMemorizzato);
        } 
        // tavolo da 10-99
        else if (buffer[3] == '-'){ 
            sscanf(buffer, "%3s", tavoloMemorizzato);
        }

        // ricevo la risposta dal server
        ret = recv(sockfd, (void *)buffer, LEN_ID, 0);
        check_errori(ret, sockfd);

        // S = ok
        if (buffer[0] == 'S'){ 
            printf("Codice corretto...\n\n");
            // delay per una migliore leggibilità
            sleep(1);
            break;
        }else{
            printf("Codice errato, inserisci un codice valdo.\n\n");
            welcome = 0;
            continue;
        }
    }

    printf(WELCOME_TD2);
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

            // ricevo il messaggio del server
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

            // ******* Adesso tutte le parole estratte da input sono salvate nell'array info, ci sono 5 casi 1) help 2) menu 3) comanda 4) conto 5) esc

            // CASO 1: stampo l'helper
            if (strcmp(info[0], "help") == 0){
                printf(HELPER);
                fflush(stdout);
            }

            // CASO 2: stampo l'elenco dei piatti sul menu
            else if (strcmp(info[0], "menu") == 0){
                codice = "menu\0";
                // Se e' la prima volta che si chiede il Menu. -> chiedo al Server -> Salvo nell'array Menu per le prossime volte
                if (richiestaMenu == 0){
                    // mando codice "menu" per la sezione di script server
                    ret = send(sockfd, (void *)codice, LEN_COMANDO, 0);
                    check_errori(ret, sockfd);
                    quanti_piatti = 0;
                    for (;;){
                        ret = recv(sockfd, &len_NO, sizeof(uint32_t), 0);
                        check_errori(ret, sockfd);

                        len_HO = ntohl(len_NO);
                        ret = recv(sockfd, buffer, len_HO, 0);
                        check_errori(ret, sockfd);

                        // se ho finito di stampare il menu, esco dal loop
                        if (strncmp(buffer, "STOP", strlen("STOP")) == 0){
                            printf("\n");
                            fflush(stdout);
                            break;
                        }

                        // salvo il menu corrente
                        sscanf(buffer, "%2s - %d %*s - %[^\n]", menu[quanti_piatti].id, &menu[quanti_piatti].costo, menu[quanti_piatti].desc); 

                        quanti_piatti++;
                        printf("%s", buffer); // stampo il menu
                        fflush(stdout);
                    }
                    richiestaMenu = 1;
                    printf("\n");
                }else{ // se il menu e' gia' stato chiesto 1 volta -> lo casho
                    for (j = 0; j < quanti_piatti; j++){
                        printf("%s - %s - %d Euro \n", menu[j].desc, menu[j].id, menu[j].costo); // stampo il menu
                        fflush(stdout);
                    }
                    printf("\n");
                }
                // aggiorno ordine, il protocollo e' che prima si richieda il menu e successivamente gli altri comandi ad esclusione dell'help
                ordine = 1;
            }
            else if (ordine == 0){
                printf("AVVISO: Per favore, controlla il Menu' giornaliero prima di effettuare una comanda o richiedere il conto.\n\n");
                fflush(stdout);
            }

            // CASO 3: gestisco l'inivio della comanda al server
            else if ((strcmp(info[0], "comanda") == 0) && ordine == 1){
                for (j = 1; j < chunk_count; j++){ 
                    // Controllo se i piatti scelti vanno bene
                    if (check_menu(info[j], quanti_piatti) == 0){
                        errore = 1;
                        break;
                    }
                }
                if (errore == 1){
                    printf("Errore: Hai scelto un piatto che non e' presente nel Menu'. Riprova.\n\n");
                    fflush(stdout);
                    errore = 0;
                }else{
                    codice = "comm\0"; 
                    // mando codice "comm" per la sezione dello script server
                    ret = send(sockfd, (void *)codice, LEN_COMANDO, 0);
                    check_errori(ret, sockfd);

                    // invio il numero di chunk che si deve aspettare il server
                    ret = send(sockfd, &chunk_count, sizeof(uint16_t), 0);
                    check_errori(ret, sockfd);

                    // invio il numero di quante comande deve gestire il server
                    ret = send(sockfd, &quante_comande, sizeof(uint16_t), 0);
                    check_errori(ret, sockfd);

                    // invio il codice del tavolo
                    len_HO = strlen(tavoloMemorizzato) + 1;
                    len_NO = htonl(len_HO);
                    // mando lunghezza del codice
                    ret = send(sockfd, &len_NO, sizeof(uint32_t), 0);
                    check_errori(ret, sockfd);
                    // mando MESSAGGIO TXX
                    ret = send(sockfd, (void *)tavoloMemorizzato, len_HO, 0);
                    check_errori(ret, sockfd);

                    // quante comande ho avuto, se superano il numero massimo di comande bisogna aspettare
                    ret = recv(sockfd, buffer, LEN_ID, 0);
                    check_errori(ret, sockfd);

                    if (buffer[0] == 'N'){ // cucina piena
                        printf("In questo momento la cucina non può gestire ulteriori comande, ti preghiamo cortesemente di aspettare :) \n\n");
                        fflush(stdout);
                        continue;
                    }
                    k = 0;
                    // invio piatti con la loro quanità 
                    for (j = 1; j < chunk_count; j++){
                        len_HO = strlen(info[j]) + 1;
                        len_NO = htonl(len_HO);

                        // mando lunghezza del codice
                        ret = send(sockfd, &len_NO, sizeof(uint32_t), 0);
                        check_errori(ret, sockfd);

                        // mando MESSAGGIO comande PIATTO-QUANTITA
                        ret = send(sockfd, (void *)info[j], len_HO, 0);
                        check_errori(ret, sockfd);

                        sscanf(info[j], "%2s-%d", coda_comande[quante_comande].desc[k],
                               &coda_comande[quante_comande].quantita[k]); // mi salvo le comande

                        k++;
                    }

                    // aggiorno i campi delle strutture
                    coda_comande[quante_comande].id_comanda = chunk_count - 1;
                    quante_comande++;

                    printf("SUCCESSO: comanda inviata! Ti preghiamo di attendere la preparazione\n\n");
                }
            }

            // CASO 4: richiedo il conto, gestisco l'invio del saldo finale
            else if ((strcmp(info[0], "conto") == 0) && ordine == 1){

                // mando codice "cont" per la sezione dello script del server
                codice = "cont\0"; 
                ret = send(sockfd, (void *)codice, LEN_COMANDO, 0);
                check_errori(ret, sockfd);

                // invio il codice del tavolo
                len_HO = strlen(tavoloMemorizzato) + 1;
                len_NO = htonl(len_HO);
                // mando lunghezza del codice
                ret = send(sockfd, &len_NO, sizeof(uint32_t), 0);
                check_errori(ret, sockfd);
                // mando MESSAGGIO TXX
                ret = send(sockfd, (void *)tavoloMemorizzato, len_HO, 0);
                check_errori(ret, sockfd);

                // ricevo la risposta del server
                ret = recv(sockfd, buffer, LEN_ID, 0);
                check_errori(ret, sockfd);

                if (buffer[0] == 'S'){
                    printf("Conto finale: \n");
                    fflush(stdout);
                }
                else{
                    printf("Attendi: deve sempre arrivare qualche piatto... \n");
                    fflush(stdout);
                    continue;
                }
                prezzo = 0;
                // per tutte le comande
                for (j = 0; j < quante_comande; j++){ 
                    // percorro il contenuto di tutta la comanda
                    for (a = 0; a < coda_comande[j].id_comanda; a++){ 
                        // manda la comanda id x quantita = prezzo euro
                        // percorro tutti i piatti
                        for (k = 0; k < MAX_PIATTI; k++){ 
                            if (strcmp(coda_comande[j].desc[a], menu[k].id) == 0){
                                prezzo += coda_comande[j].quantita[a] * menu[k].costo; // prezzo totale
                                sprintf(buffer, "%s x %d = %d euro", coda_comande[j].desc[a],
                                        coda_comande[j].quantita[a], coda_comande[j].quantita[a] * menu[k].costo);
                            }
                        }

                        // stampo lo "scontrino"
                        printf("%s\n", buffer); 
                        fflush(stdout);
                    }
                }
                printf("Totale: %d Euro\n", prezzo);
                fflush(stdout);
                close(sockfd);
                exit(0);
            }

            // CASO 5: uscita forzata del client, chiusura
            else if (strcmp(info[0], "esc") == 0){
                printf("Uscita in corso...\nArrivederci :)\n\n");
                fflush(stdout);
                // TO DO: si potrebbe inviare un messaggio al server per dire che il td si e' disconnesso
                close(sockfd);
                exit(0);
            }else{ // nessun comando inserito valido
                printf("Errore: Comando non consentito, RIPROVA...\n\n");
                continue;
            }
        }
    }
    close(sockfd);
    return 0;
}