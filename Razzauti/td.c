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
#define WELCOME "Benvenuto"
#define WELCOME_TD1 "\nInserisci il codice prenotazione: "
#define WELCOME_TD2 "\n***************** BENVENUTO AL RISTORANTE *****************\n Digita un comando:\n1) help --> mostra i dettagli dei comandi\n2) menu --> mostra il menu dei piatti\n3) comanda --> invia una comanda\n4) conto --> chiede il conto\n"
#define HELP "Comandi:\nmenu -> stampa il menu\ncomanda -> invia una comanda in cucina\n\t\t   NOTA: deve essere nel formato\n \t\t   {<piatto_1-quantità_1>...<piatto_n-quantità_n>}\nconto -> richiesta del conto\n"

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
        if (strcmp(menu[x].nome, piattoN) == 0){ 
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


    int welcome = 1; // per l'output
    

    int chunk_len, prezzo, a, k, j, quanti_piatti = 0;
    int quante_comande = 0;
    char id[] = "T\0"; // codice per riconoscere il TABLE DEVICE

    char tavoloMemorizzato[5];

    char *chunk;
    int menuPresente = 0;

    int errore = 0; // controllo se il piatto scelto è nel menu

    int richiesto = 0;                 // controlo se e' stato richiesto il menu per il controllo del Conto
    uint16_t chunk_count = 0;           // Il numero di parole estratte dalla frase
    char *info[MAX_WORDS]; // array di puntatori in cui vengono memorizzate le parole estratte dal buffer con la strtok
    char *codice = NULL; // help, menu, comand, conto

    // variabili per la select
    fd_set master; // set di descrittori da monitorare
    fd_set read_fds; // sed di descrittori pronti
	int fdmax; // descrittore max

    uint32_t len_HO; // lunghezza del messaggio espressa in host order
    uint32_t len_NO; // lunghezza del messaggio espressa in network order

    // CREAZIONE SOCKET
    memset((void *)&cli_addr, 0, sizeof(cli_addr));
    memset((void *)&server_addr, 0, sizeof(server_addr));
    // Creazione del socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("Errore nella creazione del socket");
        exit(1);
    }

    // Inizializzazione della struttura
    cli_addr.sin_family = AF_INET;
    cli_addr.sin_port = porta;
    cli_addr.sin_addr.s_addr = INADDR_ANY;
    ret = bind(sockfd, (struct sockaddr *)&cli_addr, sizeof(cli_addr));
    if (ret == -1)
    {
        perror("ERRORE nella bind()");
        printf("ARRESTO IN CORSO...\n");
        fflush(stdout);
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(4242);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    ret = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
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

    fdmax = sockfd;
    // invio codice id
    ret = send(sockfd, (void *)id, LEN_ID, 0);
    check_errori(ret, sockfd);

    ret = recv(sockfd, (void *)buffer, LEN_ID, 0);
    check_errori(ret, sockfd);
    if (buffer[0] != 'S')
    {
        perror("Pieno.\n\n");
        close(sockfd);
    }

    while (1)
    { // validazione codice per identificare il Table D.

        if(welcome)
            printf(WELCOME);
        printf(WELCOME_TD1);
        fflush(stdout);

        fgets(buffer, BUFFER_SIZE, stdin);
        codice = "code\0";

        // mando codice "code" per identificare la sezione
        ret = send(sockfd, (void *)codice, LEN_COMANDO, 0);
        check_errori(ret, sockfd);

        len_HO = strlen(buffer) + 1;
        len_NO = htonl(len_HO);
        // mando lunghezza del codice
        ret = send(sockfd, &len_NO, sizeof(uint32_t), 0);
        check_errori(ret, sockfd);

        // mando MESSAGGIO codice
        ret = send(sockfd, (void *)buffer, len_HO, 0);
        check_errori(ret, sockfd);

        if (buffer[2] == '-')
        { // tavolo da 0-9
            sscanf(buffer, "%2s", tavoloMemorizzato);
        }
        else if (buffer[3] == '-')
        { // tavolo da 10-99
            sscanf(buffer, "%3s", tavoloMemorizzato);
        }
        ret = recv(sockfd, (void *)buffer, LEN_ID, 0);
        check_errori(ret, sockfd);

        if (buffer[0] == 'S')
        { // S = confermato
            printf("Codice corretto...\n\n");
            break;
        }
        else
        {
            printf("Ripetere digitazione. Codice errato.\n\n");
            welcome = 0;
        }
    }

    printf(WELCOME_TD2);
    fflush(stdout);
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
            check_errori(ret, sockfd);
            if (strncmp(buffer, "STOP", strlen("STOP")) == 0)
            { // se il server ha chiamato "stop", avviso e termino.
                printf("AVVISO: il server è stato arrestato.\nARRESTO IN CORSO...\n");
                fflush(stdout);
                exit(0);
                close(sockfd);
            }                       // altrimenti, il server sta avvisando che una comanda è in preparazione/in servizio.
            printf("%s\n", buffer); // Mi serve solo stampare il messaggio.
            fflush(stdout);
        }
        else
        {
            // Lettura del messaggio da stdin
            fgets(buffer, BUFFER_SIZE, stdin);

            // Estrai le parole dalla frase utilizzando la funzione 'strtok'
            chunk = strtok(buffer, " "); // Estrai la prima parola utilizzando lo spazio come delimitatore
            chunk_count = 0;
            while (chunk != NULL && chunk_count < MAX_WORDS)
            { // Finche' ci sono parole da estrarre e non si supera il limite massimo
                // Rimuovi il carattere di fine riga dalla parola se presente
                chunk_len = strlen(chunk);
                if (chunk[chunk_len - 1] == '\n')
                {
                    chunk[chunk_len - 1] = '\0';
                }

                // Aggiungi la parola all'array di parole
                info[chunk_count] = chunk; // Memorizza il puntatore alla parola nell'array
                chunk_count++;                        // Incrementa il contatore di parole estratte

                // Estrai la prossima parola
                chunk = strtok(NULL, " "); // Utilizza 'NULL' come primo parametro per estrarre le parole successive
            }

            if (strcmp(info[0], "help") == 0)
            {
                printf(HELP);
                fflush(stdout);
            }
            else if (strcmp(info[0], "menu") == 0)
            {
                codice = "menu\0";
                if (menuPresente == 0)
                { // Se è la prima volta che si chiede il Menu. -> Richiediamo al Server -> Salviamo nell'array Menu per le prossime volte

                    // mando codice "menu"
                    ret = send(sockfd, (void *)codice, LEN_COMANDO, 0);
                    check_errori(ret, sockfd);
                    quanti_piatti = 0;
                    for (;;)
                    {
                        ret = recv(sockfd, &len_NO, sizeof(uint32_t), 0);
                        check_errori(ret, sockfd);

                        len_HO = ntohl(len_NO);
                        ret = recv(sockfd, buffer, len_HO, 0);
                        check_errori(ret, sockfd);

                        if (strncmp(buffer, "STOP", strlen("STOP")) == 0)
                        {
                            printf("\n");
                            fflush(stdout);
                            break;
                        }

                        sscanf(buffer, "%2s - %d %*s - %[^\n]", menu[quanti_piatti].nome, &menu[quanti_piatti].costo, menu[quanti_piatti].desc); // MI SALVO IL MENU

                        quanti_piatti++;
                        printf("%s", buffer); // stampo il menu
                        fflush(stdout);
                    }
                    menuPresente = 1;
                    printf("\n");
                }
                else
                { // se il menu e' gia' stato chiesto 1 volta -> NON chiedo al Server
                    for (j = 0; j < quanti_piatti; j++)
                    {
                        printf("%s - %s - %d Euro \n", menu[j].desc, menu[j].nome, menu[j].costo); // stampo il menu
                        fflush(stdout);
                    }
                    printf("\n");
                }
                richiesto = 1;
            }
            else if (richiesto == 0)
            {
                printf("AVVISO: Controlla il Menu' prima di fare una comanda o chiedere il conto.\n\n");
                fflush(stdout);
            }
            else if ((strcmp(info[0], "comanda") == 0) && richiesto == 1)
            {
                for (j = 1; j < chunk_count; j++)
                { // Controllo se i piatti scelti vanno bene
                    if (check_menu(info[j], quanti_piatti) == 0)
                    {
                        errore = 1;
                        break;
                    }
                }
                if (errore == 1)
                {
                    printf("ERRORE: Un piatto scelto non e' presente nel Menu! \n\n");
                    fflush(stdout);
                    errore = 0;
                }
                else
                {
                    codice = "comm\0"; // mando codice "comm"
                    ret = send(sockfd, (void *)codice, LEN_COMANDO, 0);
                    check_errori(ret, sockfd);

                    ret = send(sockfd, &chunk_count, sizeof(uint16_t), 0);
                    check_errori(ret, sockfd);

                    ret = send(sockfd, &quante_comande, sizeof(uint16_t), 0);
                    check_errori(ret, sockfd);

                    len_HO = strlen(tavoloMemorizzato) + 1;
                    len_NO = htonl(len_HO);
                    // mando lunghezza del codice
                    ret = send(sockfd, &len_NO, sizeof(uint32_t), 0);
                    check_errori(ret, sockfd);
                    // mando MESSAGGIO TXX
                    ret = send(sockfd, (void *)tavoloMemorizzato, len_HO, 0);
                    check_errori(ret, sockfd);

                    // quante comande ho avuto
                    ret = recv(sockfd, buffer, LEN_ID, 0);
                    check_errori(ret, sockfd);

                    if (buffer[0] == 'N')
                    { // cucina piena
                        printf("Aspettare, la cucina è piena di ordini! :) \n\n");
                        fflush(stdout);
                        continue;
                    }
                    k = 0;
                    for (j = 1; j < chunk_count; j++)
                    {
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

                    coda_comande[quante_comande].num_comande = chunk_count - 1;

                    quante_comande++;

                    printf("AVVISO: COMANDA SPEDITA! \n\n");
                }
            }
            else if ((strcmp(info[0], "conto") == 0) && richiesto == 1)
            {

                codice = "cont\0"; // mando codice "cont"
                ret = send(sockfd, (void *)codice, LEN_COMANDO, 0);
                check_errori(ret, sockfd);

                len_HO = strlen(tavoloMemorizzato) + 1;
                len_NO = htonl(len_HO);
                // mando lunghezza del codice
                ret = send(sockfd, &len_NO, sizeof(uint32_t), 0);
                check_errori(ret, sockfd);
                // mando MESSAGGIO TXX
                ret = send(sockfd, (void *)tavoloMemorizzato, len_HO, 0);
                check_errori(ret, sockfd);

                ret = recv(sockfd, buffer, LEN_ID, 0);
                check_errori(ret, sockfd);

                if (buffer[0] == 'S')
                {
                    printf("Conto: \n");
                    fflush(stdout);
                }
                else
                {
                    printf("Deve sempre arrivare qualche piatto... \n");
                    fflush(stdout);
                    continue;
                }
                prezzo = 0;
                for (j = 0; j < quante_comande; j++)
                { // tutte le comande
                    for (a = 0; a < coda_comande[j].num_comande; a++)
                    { // percorro il contenuto di tutta la comanda
                        // manda la comanda nome x quantita = prezzo euro
                        for (k = 0; k < MAX_PIATTI; k++)
                        { // percorro tutti i piatti
                            if (strcmp(coda_comande[j].desc[a], menu[k].nome) == 0)
                            {
                                prezzo += coda_comande[j].quantita[a] * menu[k].costo; // prezzo totale
                                sprintf(buffer, "%s x %d = %d euro", coda_comande[j].desc[a],
                                        coda_comande[j].quantita[a], coda_comande[j].quantita[a] * menu[k].costo);
                            }
                        }

                        printf("%s\n", buffer); // stampo "scontrino"
                        fflush(stdout);
                    }
                }
                printf("Totale: %d Euro\n", prezzo);
                fflush(stdout);
                close(sockfd);
                exit(0);
            }
            else if (strcmp(info[0], "esc") == 0)
            {
                printf("Uscita in corso...\n\n");
                fflush(stdout);
                close(sockfd);
                exit(0);
            }
            else
            {
                printf("ERRORE: comando non valido.\n");
                fflush(stdout);
            }
        }
    }

    close(sockfd);
    return 0;
}