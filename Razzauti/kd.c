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

#define BUFFER_SIZE 1024
#define WELCOME_KD "************** KITCHEN DEVICE **************\n"
#define COMANDI "Digita un comando: \n\n1) take \t\t --> accetta una comanda \n2) show \t\t --> mostra le comande accettate (in preparazione) \n3) ready com. \t\t --> imposta lo stato della comanda \n"

#define MAX_WORDS 50 // numero massimo di parole che possono essere estratte dalla frase
#define LEN_ID 2 // lunghezza codici fissati per identificare il tipo di client al server (client-kd-td)
#define LEN_COMANDO 5 // lunghezza dei comandi da mandare al server

#define MAX_PIATTI 15 // numero massimo di piatti nel menu
#define MAX_COMANDE_IN_ATTESA 10
#define DESCRIZIONE 100 // descrizione del piatto

int main(int argc, char *argv[]){
    // variabili per i socket
    int sockfd, ret; 
    char buffer[BUFFER_SIZE];
    struct sockaddr_in server_addr;

    // variabili di utilità
    char id[] = "K\0";
    int chunk_len, j; 
    int quante_comande = 0; // numero di comande che deve gestire il server
    char *chunk;
    int chunk_count = 0; // Il numero di parole estratte dalla frase
    char *info[MAX_WORDS]; // L'array di puntatori in cui vengono memorizzate le parole
    char *codice = NULL;

    // variabili per la select
    fd_set master; // set di descrittori da monitorare
    fd_set read_fds; // sed di descrittori pronti
	int fdmax; // descrittore max

    uint32_t len_HO; // lunghezza del messaggio espressa in host order
    uint32_t len_NO; // lunghezza del messaggio espressa in network order
    uint16_t stampaC;

    // CREAZIONE SOCKET
    // Creazione del socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("Errore nella creazione del socket");
        exit(1);
    }

    memset((void *)&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(4242);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

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

    printf(WELCOME_KD);
    printf(COMANDI);
    fflush(stdout);
    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        read_fds = master;
        select(fdmax + 1, &read_fds, NULL, NULL, NULL);
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
            check_errori(ret, sockfd);
            if (strncmp(buffer, "STOP", strlen("STOP")) == 0)
            { // se il server ha chiamato "stop", avviso e termino.
                printf("AVVISO: il server e' stato arrestato.\nARRESTO IN CORSO...\n");
                fflush(stdout);
                exit(0);
                close(sockfd);
            }                       // altrimenti, il server sta avvisando che una comanda e' in preparazione/in servizio.
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
                if (chunk[chunk_len - 1] == '\n') // Inserisco il fine stringa
                {
                    chunk[chunk_len - 1] = '\0';
                }

                // Aggiungi la parola all'array di parole
                info[chunk_count] = chunk;
                chunk_count++; // Incrementa il contatore di parole estratte

                // Estrai la prossima parola
                chunk = strtok(NULL, " "); // Utilizza 'NULL' come primo parametro per estrarre le parole successive
            }

            if (strcmp(info[0], "take") == 0)
            {
                // mando codice "take"
                codice = "take\0";
                ret = send(sockfd, (void *)codice, LEN_COMANDO, 0);
                check_errori(ret, sockfd);

                ret = recv(sockfd, &len_NO, sizeof(uint32_t), 0);
                check_errori(ret, sockfd);

                len_HO = ntohl(len_NO);
                ret = recv(sockfd, buffer, len_HO, 0);
                check_errori(ret, sockfd);

                if (strncmp(buffer, "STOP", strlen("STOP")) == 0)
                {
                    printf("NESSUNA COMANDA TROVATA. \n");
                    fflush(stdout);
                    continue;
                }
                // Se non ho ricevuto lo STOP, c'era una comanda in Attesa. Ricevo le informazioni
                sscanf(buffer, "%s", coda_comande_kd[quante_comande].tav_num);

                ret = recv(sockfd, &stampaC, sizeof(uint16_t), 0);
                check_errori(ret, sockfd);
                stampaC++;

                printf("Com%d Tavolo %s:\n", stampaC, coda_comande_kd[quante_comande].tav_num);
                j = 0;
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

                    sscanf(buffer, "%2s %d", coda_comande_kd[quante_comande].desc[j], &coda_comande_kd[quante_comande].quantita[j]);
                    j++;
                    printf("%s\n", buffer);
                    fflush(stdout);
                }
                quante_comande++;
            }
            else if (strcmp(info[0], "show") == 0)
            {
                codice = "show\0";
                // mando codice "show"
                ret = send(sockfd, (void *)codice, LEN_COMANDO, 0);
                check_errori(ret, sockfd);

                for (;;)
                {
                    ret = recv(sockfd, &len_NO, sizeof(uint32_t), 0);
                    check_errori(ret, sockfd);

                    len_HO = ntohl(len_NO);
                    ret = recv(sockfd, buffer, len_HO, 0);
                    check_errori(ret, sockfd);

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
            else if (strcmp(info[0], "ready") == 0)
            {
                // mando codice "ready"
                codice = "read\0"; // mando codice "read"
                ret = send(sockfd, (void *)codice, LEN_COMANDO, 0);
                check_errori(ret, sockfd);

                len_HO = strlen(info[1]) + 1;
                len_NO = htonl(len_HO);
                ret = send(sockfd, &len_NO, sizeof(uint32_t), 0); // mando la dimensione
                check_errori(ret, sockfd);

                ret = send(sockfd, info[1], len_HO, 0); // mando il messaggio
                check_errori(ret, sockfd);

                ret = recv(sockfd, (void *)buffer, LEN_ID, 0);
                check_errori(ret, sockfd);
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