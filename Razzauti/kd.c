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
    char *chunk; // per l'estrazione delle parole dal buffer
    int chunk_count = 0; // Il numero di parole estratte dalla frase
    char *info[MAX_WORDS]; // L'array di puntatori in cui vengono memorizzate le parole
    char *codice = NULL; // take, show, ready com
    int printed = 0;  // flag per la stampa condizionale del comando show

    // variabili per la select
    fd_set master; // set di descrittori da monitorare
    fd_set read_fds; // sed di descrittori pronti
	int fdmax; // descrittore max

    uint32_t len_HO; // lunghezza del messaggio espressa in host order
    uint32_t len_NO; // lunghezza del messaggio espressa in network order
    uint16_t stampaC;

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

    printf(WELCOME_KD); // stampo il benvenuto
    printf(COMANDI); // stampo la lista dei comandi
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

            // ricevo la lunghezzza del messaggio
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

            // ******* Adesso tutte le parole estratte da input sono salvate nell'array info, ci sono 3 casi 1) take 2) show 3) ready

            // CASO 1: accetto una comanda, rifiuto se ho troppe comande pendenti
            if (strcmp(info[0], "take") == 0){
                // mando codice "take" al server per la sezione di script server
                codice = "take\0";
                ret = send(sockfd, (void *)codice, LEN_COMANDO, 0);
                check_errori(ret, sockfd);

                // ricevo la risposta del server
                ret = recv(sockfd, &len_NO, sizeof(uint32_t), 0);
                check_errori(ret, sockfd);

                len_HO = ntohl(len_NO);
                ret = recv(sockfd, buffer, len_HO, 0);
                check_errori(ret, sockfd);
                
                // Se non ci sono comande in attesa stampo un messaggio al kitchen device
                if (strncmp(buffer, "STOP", strlen("STOP")) == 0){
                    printf("NON CI SONO COMANDE IN ATTESA.\n");
                    fflush(stdout);
                    continue;
                }

                // C'è almeno una comanda in attesa -> ricevo le informazioni
                sscanf(buffer, "%s", coda_comande_kd[quante_comande].tav_num);

                // ricevo il numero della comanda dal server
                ret = recv(sockfd, &stampaC, sizeof(uint16_t), 0);
                check_errori(ret, sockfd);
                stampaC++;

                // stampo il numero della comanda e il tavolo assegnato
                printf("Com%d Tavolo %s:\n", stampaC, coda_comande_kd[quante_comande].tav_num);
                j = 0;

                // stampo l'elenco di tutti i piatti della comanda
                for (;;){
                    ret = recv(sockfd, &len_NO, sizeof(uint32_t), 0);
                    check_errori(ret, sockfd);

                    len_HO = ntohl(len_NO);
                    ret = recv(sockfd, buffer, len_HO, 0);
                    check_errori(ret, sockfd);

                    // per uscire dal loop quando ho stampato tutto il buffer
                    if (strncmp(buffer, "STOP", strlen("STOP")) == 0){
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
            // CASO 2: se ci sono comande in preparazione stampo l'elenco 
            else if (strcmp(info[0], "show") == 0){
                codice = "show\0";
                // mando codice "show" per la sezione del codice server
                ret = send(sockfd, (void *)codice, LEN_COMANDO, 0);
                check_errori(ret, sockfd);

                // stampo l'elenco delle comande in preparazione
                for (;;){
                    ret = recv(sockfd, &len_NO, sizeof(uint32_t), 0);
                    check_errori(ret, sockfd);

                    len_HO = ntohl(len_NO);
                    ret = recv(sockfd, buffer, len_HO, 0);
                    check_errori(ret, sockfd);
                    
                    // per uscire dal loop quando ho stampato tutto il buffer
                    if (strncmp(buffer, "STOP", strlen("STOP")) == 0){
                        if (!printed) {
                            printf("NON CI SONO COMANDE IN PREPARAZIONE\n");
                            fflush(stdout);
                        }
                        break;
                    }
                    printf("%s\n\n", buffer);
                    fflush(stdout);
                    printed = 1;
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