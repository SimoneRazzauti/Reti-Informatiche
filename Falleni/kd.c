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

#define PORT 4242
#define BUFFER_SIZE 1024

#define MAX_WORDS 50 // Numero massimo di parole che possono essere estratte dalla frase
#define MAX_PIATTI 10
#define MAX_COMANDE_IN_ATTESA 10
#define DESCRIZIONE 100
#define validLen 2
#define codiceLen 5

struct comanda
{
    char tav_num[5];                    // numero del tavolo da cui proviene la comanda
    char desc[MAX_PIATTI][DESCRIZIONE]; // sarà A1, A3, P2 ecc... il codice del piatto
    int quantita[MAX_PIATTI];           // quantita
};

struct comanda coda_comande[MAX_COMANDE_IN_ATTESA]; // coda di comande in attesa

void comandi_table()
{
    printf("Digita un comando: \n\n");
    printf("1) take \t\t --> accetta una comanda \n");
    printf("2) show \t\t --> mostra le comande accettate (in preparazione) \n");
    printf("3) ready com. \t\t --> imposta lo stato della comanda \n");
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
    fd_set master; // variabili per set
    fd_set read_fds;

    int max_fd;
    int sockfd, ret, wordLen, j; // variabili per i socket + variabili utili

    in_port_t porta = htons(atoi(argv[1]));

    struct sockaddr_in serv_addr, cli_addr;

    int quante_comande = 0;

    char identificativo[] = "K\0";
    char *codice = NULL;
    char *word;
    char buffer[BUFFER_SIZE];

    char *datiInformazioni[MAX_WORDS]; // L'array di puntatori in cui vengono memorizzate le parole
    int word_count = 0;                // Il numero di parole estratte dalla frase

    uint32_t len_HO; // lunghezza del messaggio espressa in host order
    uint32_t len_NO; // lunghezza del messaggio espressa in network order
    uint16_t stampaC;

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

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

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
    // invio codice identificativo
    ret = send(sockfd, (void *)identificativo, validLen, 0);
    gestione_errore(ret, sockfd);

    ret = recv(sockfd, (void *)buffer, validLen, 0);
    gestione_errore(ret, sockfd);
    if (buffer[0] != 'S')
    {
        perror("Pieno.\n\n");
        close(sockfd);
    }

    printf("************** BENVENUTO **************\n");
    comandi_table();
    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        read_fds = master;
        select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (FD_ISSET(sockfd, &read_fds))
        { // PRONTO SOCKET DI COMUNICAZIONE
            ret = recv(sockfd, &len_NO, sizeof(uint32_t), 0);
            if (ret == 0)
            {
                printf("AVVISO: il server ha chiuso il socket remoto.\nARRESTO IN CORSO...\n");
                fflush(stdout);
                exit(1);
            }

            len_HO = ntohl(len_NO);
            ret = recv(sockfd, buffer, len_HO, 0);
            gestione_errore(ret, sockfd);
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
            word = strtok(buffer, " "); // Estrai la prima parola utilizzando lo spazio come delimitatore
            word_count = 0;
            while (word != NULL && word_count < MAX_WORDS)
            { // Finche' ci sono parole da estrarre e non si supera il limite massimo
                // Rimuovi il carattere di fine riga dalla parola se presente
                wordLen = strlen(word);
                if (word[wordLen - 1] == '\n') // Inserisco il fine stringa
                {
                    word[wordLen - 1] = '\0';
                }

                // Aggiungi la parola all'array di parole
                datiInformazioni[word_count] = word;
                word_count++; // Incrementa il contatore di parole estratte

                // Estrai la prossima parola
                word = strtok(NULL, " "); // Utilizza 'NULL' come primo parametro per estrarre le parole successive
            }

            if (strcmp(datiInformazioni[0], "take") == 0)
            {
                // mando codice "take"
                codice = "take\0";
                ret = send(sockfd, (void *)codice, codiceLen, 0);
                gestione_errore(ret, sockfd);

                ret = recv(sockfd, &len_NO, sizeof(uint32_t), 0);
                gestione_errore(ret, sockfd);

                len_HO = ntohl(len_NO);
                ret = recv(sockfd, buffer, len_HO, 0);
                gestione_errore(ret, sockfd);

                if (strncmp(buffer, "STOP", strlen("STOP")) == 0)
                {
                    printf("NESSUNA COMANDA TROVATA. \n");
                    fflush(stdout);
                    continue;
                }
                // Se non ho ricevuto lo STOP, c'era una comanda in Attesa. Ricevo le informazioni
                sscanf(buffer, "%s", coda_comande[quante_comande].tav_num);

                ret = recv(sockfd, &stampaC, sizeof(uint16_t), 0);
                gestione_errore(ret, sockfd);
                stampaC++;

                printf("Com%d Tavolo %s:\n", stampaC, coda_comande[quante_comande].tav_num);
                j = 0;
                for (;;)
                {
                    ret = recv(sockfd, &len_NO, sizeof(uint32_t), 0);
                    gestione_errore(ret, sockfd);

                    len_HO = ntohl(len_NO);
                    ret = recv(sockfd, buffer, len_HO, 0);
                    gestione_errore(ret, sockfd);

                    if (strncmp(buffer, "STOP", strlen("STOP")) == 0)
                    {
                        printf("\n");
                        fflush(stdout);
                        break;
                    }

                    sscanf(buffer, "%2s %d", coda_comande[quante_comande].desc[j], &coda_comande[quante_comande].quantita[j]);
                    j++;
                    printf("%s\n", buffer);
                    fflush(stdout);
                }
                quante_comande++;
            }
            else if (strcmp(datiInformazioni[0], "show") == 0)
            {
                codice = "show\0";
                // mando codice "show"
                ret = send(sockfd, (void *)codice, codiceLen, 0);
                gestione_errore(ret, sockfd);

                for (;;)
                {
                    ret = recv(sockfd, &len_NO, sizeof(uint32_t), 0);
                    gestione_errore(ret, sockfd);

                    len_HO = ntohl(len_NO);
                    ret = recv(sockfd, buffer, len_HO, 0);
                    gestione_errore(ret, sockfd);

                    if (strncmp(buffer, "STOP", strlen("STOP")) == 0)
                    { // Per uscire dal loop
                        printf("\n");
                        fflush(stdout);
                        break;
                    }

                    printf("%s\n", buffer);
                    fflush(stdout);
                }
            }
            else if (strcmp(datiInformazioni[0], "ready") == 0)
            {
                // mando codice "ready"
                codice = "read\0"; // mando codice "read"
                ret = send(sockfd, (void *)codice, codiceLen, 0);
                gestione_errore(ret, sockfd);

                len_HO = strlen(datiInformazioni[1]) + 1;
                len_NO = htonl(len_HO);
                ret = send(sockfd, &len_NO, sizeof(uint32_t), 0); // mando la dimensione
                gestione_errore(ret, sockfd);

                ret = send(sockfd, datiInformazioni[1], len_HO, 0); // mando il messaggio
                gestione_errore(ret, sockfd);

                ret = recv(sockfd, (void *)buffer, validLen, 0);
                gestione_errore(ret, sockfd);
                if (buffer[0] == 'S')
                {
                    printf("COMANDA IN SERVIZIO.\n\n");
                }
                else
                {
                    printf("ERRORE: COMANDA NON TROVATA.\n\n");
                }
            }
            else
            {
                printf("ERRORE: comando non valido.\n");
            }
        }
    }

    close(sockfd);
    return 0;
}