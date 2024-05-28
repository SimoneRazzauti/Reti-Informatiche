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

// Funzione che controlla che la data inserita con la 'find' sia valida nel formato gg-mm-aa hh e che sia futura rispetto alla data attuale
int check_data(int GG, int MM, int AA, int HH){
    // Ottengo il tempo corrente
    time_t current_time = time(NULL);

    // Controllo che il mese sia valido (deve essere compreso tra 1 e 12)
    if (MM < 1 || MM > 12){
        return 0; // Ritorna 0 se il mese non e' valido
    }

    // Verifico se l'anno e' bisestile
    int giorni_in_febbraio = (AA % 4 == 0 && (AA % 100 != 0 || AA % 400 == 0)) ? 29 : 28;

    // Controllo che il giorno sia valido in base al mese e all'anno
    switch (MM){
        case 2:
            if (GG < 1 || GG > giorni_in_febbraio){
                return 0; // Ritorna 0 se il giorno non e' valido per febbraio
            }
            break;
        case 4:
        case 6:
        case 9:
        case 11:
            if (GG < 1 || GG > 30){
                return 0; // Ritorna 0 se il giorno non e' valido per aprile, giugno, settembre o novembre
            }
            break;
        default:
            if (GG < 1 || GG > 31){
                return 0; // Ritorna 0 se il giorno non e' valido per gli altri mesi
            }
            break;
    }

    // Verifico l'ora (deve essere compresa tra 10 e 23)
    if (HH < 10 || HH > 23)
    {
        return 0; // Ritorna 0 se l'ora non e' valida
    }

    // Inizializzo una variabile time_t per contenere il tempo della data inserita
    time_t input_time = 0;
    // Ottengo la struttura tm corrispondente alla data inserita
    struct tm input_timeinfo = { 0 };
    input_timeinfo.tm_mday = GG;                    // Giorno del mese (1-31)
    input_timeinfo.tm_mon = MM - 1;                 // Mese dell'anno (0-11)
    input_timeinfo.tm_year = (AA < 100 ? AA + 2000 - 1900 : AA - 1900); // Anno (numero di anni trascorsi dal 1900)
    input_timeinfo.tm_hour = HH;                    // Ora del giorno (0-23)

    // Converto la struttura tm in un tempo in formato time_t
    input_time = mktime(&input_timeinfo);

    // Confronto il tempo corrente con il tempo della data passata come parametro
    // e restituisco 1 se la data e' futura, altrimenti 0
    return (input_time >= current_time);
}

// Gestione errori e gestione in caso di chiusura del socket remoto, termino.
void check_errori(int ret, int socket){
    if (ret == -1){
        perror("Errore: errore di comunicazione col server\n");
        printf("ARRESTO IN CORSO...\n\n");
        fflush(stdout);
        close(socket);
        exit(1);
    }
    else if (ret == 0){
        printf("\nAVVISO: il server ha chiuso la connessione.\nARRESTO IN CORSO...\n");
        fflush(stdout);
        close(socket);
        exit(1);
    }
}

// Cerca_prenotazione e' una funzione che mi serve per realizzare la lista da restituire al cliente che esegue la Find, utilizzata nella controlla_tavoli_liberi(...).
// Se un tavolo e' occupato per la stessa data e ora, non viene inviato.

int cerca_prenotazione(char *tavolinoX, char *data, int ora, char *percorsoFile)
{
    char arraycopia[DESCRIZIONE]; // array di passaggio per le informazioni
    int GG, MM, AA, HH, giorno, mese, anno;
    char tavolino[5];
    FILE *fileP = fopen(percorsoFile, "r"); // apre il file in modalita'  lettura

    if (fileP == NULL)
    {
        printf("Errore nell'apertura del file cerca_prenotazione\n");
        exit(1); // esce dal programma in caso di errore
    }
    // legge le frasi dal file
    while (fgets(arraycopia, DESCRIZIONE, fileP) != NULL)
    {
        sscanf(arraycopia, "%*s %s %d-%d-%d %d %*s %*d-%*d-%*d", tavolino, &GG, &MM, &AA, &HH);
        sscanf(data, "%d-%d-%d", &giorno, &mese, &anno);
        if ((strcmp(tavolino, tavolinoX) == 0) && GG == giorno && MM == mese && AA == anno && HH == ora)
        { // se e' occupato
            return 0;
        }
    }

    fclose(fileP); // chiude il file
    return 1;
}

// Funzione utile per avere la lista dei tavoli liberi. Leggo dal file nella cartella tavoli/lista.txt e con l'aiuto di cerca_prenotazione mi salvo SOLO i tavoli disponibili e che rispettano la quantita' di persone
void controlla_tavoli_liberi(char *data, int ora, int NPersone, char *percorsoFile, int i)
{
    char arraycopia[DESCRIZIONE]; // array di passaggio per le informazioni
    int n = 0;

    char tavolino[5];

    FILE *file = fopen("tavoli/lista.txt", "r"); // apre il file in modalita' lettura
    int quanti;
    if (file == NULL)
    {
        printf("Errore nell'apertura del file controlla_tavoli_liberi\n");
        exit(1); // esce dal programma in caso di errore
    }
    // legge le frasi dal file & non superando il numero MAX di tavoli consentiti nel ristorante
    while (fgets(arraycopia, DESCRIZIONE, file) != NULL && n < MAX_TAVOLI)
    {
        sscanf(arraycopia, "%s %*s %d %*s", tavolino, &quanti);
        if (NPersone <= quanti)
        {
            if (cerca_prenotazione(tavolino, data, ora, percorsoFile))
            { // se NON e' gia' stato prenotato
                sscanf(arraycopia, "%s %s %d %s", client_fds[i].tavoli_proposti[indicetavolo].tav, client_fds[i].tavoli_proposti[indicetavolo].sala,
                       &client_fds[i].tavoli_proposti[indicetavolo].posti, client_fds[i].tavoli_proposti[indicetavolo].descrizione);
                indicetavolo++;
            }
        }
        n++;
    }

    fclose(file); // chiude il file
}

// Nel caso in cui non esiste il file nella cartella prenotazioni. Significa = 0 prenotazioni, quindi per semplificare restituisco tutti i tavoli, senza controllare prenotazioni.
void imposta_tavoli(int NPersone, char *percorsoFile, int i)
{
    char arraycopia[DESCRIZIONE]; // array di passaggio per le informazioni
    int n = 0;

    FILE *file = fopen(percorsoFile, "r"); // apre il file in modalita' lettura
    int quanti;
    if (file == NULL)
    {
        printf("Errore nell'apertura del file imposta_tavoli\n");
        exit(1); // esce dal programma in caso di errore
    }
    // legge le frasi dal file
    while (fgets(arraycopia, DESCRIZIONE, file) != NULL && n < MAX_TAVOLI)
    {
        sscanf(arraycopia, "%*s %*s %d %*s", &quanti);
        if (NPersone <= quanti)
        {
            sscanf(arraycopia, "%s %s %d %s", client_fds[i].tavoli_proposti[indicetavolo].tav, client_fds[i].tavoli_proposti[indicetavolo].sala,
                   &client_fds[i].tavoli_proposti[indicetavolo].posti, client_fds[i].tavoli_proposti[indicetavolo].descrizione);
            indicetavolo++;
        }
        n++;
    }

    fclose(file); // chiude il file
}

// Inserisce una prenotazione
int prenotazione_tavolo(char *percorsoFile, int GG, int MM, int AA, int HH, char *cognome, char *tav)
{
    // nel file prenotazione avro': codice tavolo GG-MM-AA HH cognome data_prenotazione
    // percorsoFile = prenotazioni/giorno.txt

    char arraycopia[DESCRIZIONE];
    int giorno, mese, anno, ora;
    char tavolo[10];

    time_t rawtime;
    struct tm *timeinfo;
    FILE *destination_file;

    FILE *file;
    if (access(percorsoFile, F_OK) == -1)
    {                                                // il file non esiste
        destination_file = fopen(percorsoFile, "w"); // creo il file se non esiste.
        if (destination_file == NULL)
        {
            printf("errore");
            return 0;
        }
        fclose(destination_file);
    }
    else
    {
        file = fopen(percorsoFile, "r"); // apre il file in modalita'  lettura

        if (file == NULL)
        {
            printf("Errore nell'apertura del file prenotazione\n");
            exit(1); // esce dal programma in caso di errore
        }

        while (fgets(arraycopia, DESCRIZIONE, file) != NULL)
        {
            sscanf(arraycopia, "%*s %s %d-%d-%d %d %*s %*d-%*d-%*d", tavolo, &giorno, &mese, &anno, &ora);

            if ((strcmp(tavolo, tav) == 0) && GG == giorno && MM == mese && AA == anno && HH == ora)
            { // se e' stato prenotato nel frattempo, restituisco 0

                fclose(file);
                return 0;
            }
        }
        fclose(file); // chiude il file
    }

    file = fopen(percorsoFile, "a"); // se e' ancora libero, aggiungo la prenotazione al file e restituisco 1
    if (file == NULL)
    {
        perror("ERRORE nella scrittura del file 'prenotazioni.txt'");
        printf("ARRESTO IN CORSO...\n");
        fflush(stdout);
        exit(1);
    }

    char codiceID[50]; // codice che servira' nel Table Device. Codice id
    sprintf(codiceID, "%s-%d-%d-%d-%d", tav, GG, MM, AA, HH);

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    fprintf(file, "%s %s %d-%d-%d %d %s %d-%d-%d\n", codiceID, tav, GG, MM, AA, HH, cognome, timeinfo->tm_mday, timeinfo->tm_mon + 1, (timeinfo->tm_year) % 100);
    fclose(file);
    return 1;
}

// Verifica se il codice inserito dal T. Device e' corretto.
int controlla_prenotazione(char stringa[30])
{
    char arraycopia[DESCRIZIONE];
    char nomeFile[70] = "prenotazioni/"; // stringa per il nome del file
    char codice[30];
    char data[30];
    int g, m, a;

    stringa[strcspn(stringa, "\n")] = '\0';
    sscanf(stringa, "T%*d-%d-%d-%d-%*d", &g, &m, &a); // La stringa di formato "T%d-%d-%d" indica che la stringa di input deve iniziare con la lettera "T". Se ha gia' fatto l'accesso la T -> diventa X. Cosi' da segnare il tavolo occupato.
    sprintf(data, "%d-%d-%d", g, m, a);

    strcat(data, ".txt\0"); // aggiunge l'estensione
    strcat(nomeFile, data); // nome del file data da cercare
    int n = 0;
    FILE *file = fopen(nomeFile, "r+"); // "r+" -> indica di aprire il file in modalità di lettura e scrittura simultanea. il file può essere sia letto che scritto contemporaneamente. Se il file specificato non esiste, verrà creato un nuovo file.
    if (file == NULL)
    {
        return 0;
    }
    while (fgets(arraycopia, DESCRIZIONE, file) != NULL)
    {
        sscanf(arraycopia, "%s %*s %*d-%*d-%*d %*d %*s %*d-%*d-%*d", codice);

        if (strcmp(stringa, codice) == 0)
        {
            // posiziona il puntatore del file all'inizio della riga modificata
            fseek(file, -strlen(arraycopia), SEEK_CUR);
            arraycopia[0] = 'X'; // la T diventa X. Spiegazione poco sopra.

            // scrive la nuova riga nel file
            fputs(arraycopia, file);

            // chiude il file
            fclose(file);
            return 1;
        }
        n++;
    }

    fclose(file); // chiude il file
    return 0;
}

// Comando stat del server in base alla lettera inserita (a, p, s)
void stampa_stat_lettera(char lettera)
{
    int a, j, check = 0;
    if (lettera == 's')
    {
        for (j = 0; j < quante_servite; j++)
        {
            printf("Comanda %d del tavolo %s. Stato <In SERVIZIO>\n", serv_comande_servite[j].id + 1, serv_comande_servite[j].tav_num);
            for (a = 0; a < serv_comande_servite[j].id_comanda; a++)
            {
                printf("Piatto scelto: %s x %d\n", serv_comande_servite[j].desc[a], serv_comande_servite[j].quantita[a]);
            }
        }
        if (quante_servite == 0)
        {
            printf("NESSUNA COMANDA IN SERVIZIO\n");
        }
        printf("\n");
    }
    else
    {
        for (j = 0; j < quante_comande; j++)
        {
            if (serv_coda_comande[j].stato == lettera && lettera == 'a')
            {
                check = 1;
                printf("Comanda %d del tavolo %s. Stato <In ATTESA>\n", serv_coda_comande[j].id + 1, serv_coda_comande[j].tav_num);
                for (a = 0; a < serv_coda_comande[j].id_comanda; a++)
                {
                    printf("Piatto scelto: %s x %d\n", serv_coda_comande[j].desc[a], serv_coda_comande[j].quantita[a]);
                }
            }
            else if (serv_coda_comande[j].stato == lettera && lettera == 'p')
            {
                check = 1;
                printf("Comanda %d del tavolo %s. Stato <In PREPARAZIONE>\n", serv_coda_comande[j].id + 1, serv_coda_comande[j].tav_num);
                for (a = 0; a < serv_coda_comande[j].id_comanda; a++)
                {
                    printf("Piatto scelto: %s x %d\n", serv_coda_comande[j].desc[a], serv_coda_comande[j].quantita[a]);
                }
            }
        }
        if (check == 0)
        {
            printf("NESSUNA COMANDA PRESENTE\n");
        }
        printf("\n");
    }
    return;
}

// stat in base al tavolino scritto.
void stampa_stat_tavolo(char tavolino[5])
{
    int a, j, check = 0;
    for (j = 0; j < quante_comande; j++)
    {
        if (strcmp(serv_coda_comande[j].tav_num, tavolino) == 0)
        {
            printf("Comanda %d - Stato <", serv_coda_comande[j].id + 1);
            if (serv_coda_comande[j].stato == 'a')
                printf("in ATTESA>\n");
            else if (serv_coda_comande[j].stato == 'p')
                printf("in PREPARAZIONE>\n");
            for (a = 0; a < serv_coda_comande[j].id_comanda; a++)
            {
                printf("Piatto scelto: %s x %d\n", serv_coda_comande[j].desc[a], serv_coda_comande[j].quantita[a]);
            }
        }
        check = 1;
    }

    for (j = 0; j < quante_servite; j++)
    {
        if (strcmp(serv_comande_servite[j].tav_num, tavolino) == 0)
        {
            printf("Comanda %d - Stato <In SERVIZIO>\n", serv_comande_servite[j].id + 1);
            for (a = 0; a < serv_comande_servite[j].id_comanda; a++)
            {
                printf("Piatto scelto: %s x %d\n", serv_comande_servite[j].desc[a], serv_comande_servite[j].quantita[a]);
            }
            check = 1;
        }
    }
    if (check == 0)
        printf("NESSUNA COMANDA PRESENTE\n");

    printf("\n");

    return;
}

// Comando stat che stampa tutto, senza specificare nulla.
void stampa_stat_all()
{
    int a, j, check = 0;
    for (j = 0; j < quante_comande; j++)
    {
        printf("Comanda %d - Stato <", serv_coda_comande[j].id + 1);
        if (serv_coda_comande[j].stato == 'a')
            printf("in ATTESA> del tavolo: %s\n", serv_coda_comande[j].tav_num);
        else if (serv_coda_comande[j].stato == 'p')
            printf("in PREPARAZIONE> del tavolo: %s\n", serv_coda_comande[j].tav_num);
        for (a = 0; a < serv_coda_comande[j].id_comanda; a++)
        {
            printf("Piatto scelto: %s x %d\n", serv_coda_comande[j].desc[a], serv_coda_comande[j].quantita[a]);
        }
        check = 1;
    }

    for (j = 0; j < quante_servite; j++)
    {
        printf("Comanda %d - Stato <In SERVIZIO> del tavolo: %s\n", serv_comande_servite[j].id + 1, serv_comande_servite[j].tav_num);
        for (a = 0; a < serv_comande_servite[j].id_comanda; a++)
        {
            printf("Piatto scelto: %s x %d\n", serv_comande_servite[j].desc[a], serv_comande_servite[j].quantita[a]);
        }
        check = 1;
    }

    if (check == 0)
        printf("NESSUNA COMANDA PRESENTE\n");
    printf("\n");

    return;
}

// gestisce la recv/send ed errori.
void errori_ritorno(int ret, int i, int fdmax, int n_table, int n_kitchen, int n_clients, fd_set *master)
{

    char comando[BUFFER_SIZE];
    int j, a, b, c, check;
    check = 0;
    uint32_t len_H; // lunghezza del messaggio espressa in host order
    uint32_t len_N; // lunghezza del messaggio espressa in network order

    if (ret < 0)
    {
        perror("Errore nella ricezione dei dati X");
        exit(1);
        return;
    }
    else if (ret == -1)
    { // nel caso in cui c'e' stato un errore, lo stampo a video
        perror("ERRORE nella comunicazione con un socket remoto");
        return;
    }
    else if (ret == 0) // da capire chi ha chiuso e abbassare il numero
    {
        if (fdmax != 0)
        {
            for (j = 0; j < n_clients; j++)
            {
                if (client_fds[j].socket == i)
                {
                    // Chiusura della connessione con il client
                    fflush(stdout);
                    close(i);
                    n_clients--;
                    client_fds[j].socket = -1;
                    FD_CLR(i, master);
                    break;
                }
            }
            for (j = 0; j < n_kitchen; j++)
            {
                if (array_kds[j] == i)
                {
                    // Chiusura della connessione con il k. device

                    for (b = 0; b < quante_comande; b++)
                    {
                        if (serv_coda_comande[b].kd_assegnato == i)
                        {
                            if (serv_coda_comande[b].td_assegnato > 0)
                            { // Avviso i T.D. del K. Device che ha abbandonato la cucina.
                                strcpy(comando, "** Kitchen Device ha smesso di lavorare **\n");
                                len_H = strlen(comando) + 1;
                                len_N = htonl(len_H);
                                ret = send(serv_coda_comande[b].td_assegnato, &len_N, sizeof(uint32_t), 0); // mando la dimensione
                                if (ret < 0)
                                {
                                    perror("Errore nell'invio del messaggio");
                                    exit(1);
                                }
                                ret = send(serv_coda_comande[b].td_assegnato, comando, len_H, 0); // mando il messaggio
                                if (ret < 0)
                                {
                                    perror("Errore nell'invio del messaggio");
                                    exit(1);
                                }
                            }
                            serv_coda_comande[b].stato = 'a'; // Rimetto la comanda in Attesa che qualche altro K. Device la prenda e tolgo il valore del suo socket dall'array
                            serv_coda_comande[b].kd_assegnato = -1;
                            check = 1;
                        }
                    }
                    if (check == 1)
                    {
                        printf("Un Kitchen Device ha abbandonato il ristorante.\n\n");
                    }

                    fflush(stdout);
                    close(i);
                    n_kitchen--;
                    array_kds[j] = -1;
                    FD_CLR(i, master);
                    break;
                }
            }
            for (j = 0; j < n_table; j++)
            {
                if (array_tds[j] == i)
                {
                    // Chiusura della connessione t. device
                    for (b = 0; b < quante_comande; b++)
                    {
                        if (serv_coda_comande[b].td_assegnato == i)
                        {
                            quante_comande--;
                            if (serv_coda_comande[b].kd_assegnato > 0)
                            { // Avviso TUTTI i K.D. che il cliente ha lasciato il ristorante.
                                sprintf(comando, "** Cliente del tavolo %s ha lasciato il ristorante. **\n", serv_coda_comande[b].tav_num);

                                len_H = strlen(comando) + 1;
                                len_N = htonl(len_H);
                                ret = send(serv_coda_comande[b].kd_assegnato, &len_N, sizeof(uint32_t), 0); // mando la dimensione
                                if (ret < 0)
                                {
                                    perror("Errore nell'invio del messaggio");
                                    exit(1);
                                }
                                ret = send(serv_coda_comande[b].kd_assegnato, comando, len_H, 0); // mando il messaggio
                                if (ret < 0)
                                {
                                    perror("Errore nell'invio del messaggio");
                                    exit(1);
                                }
                            }

                            // Rimuovo tutte le sue comande permettendo di poter fare lo STOP del server o di non far prendere al K.D. comande errate.
                            for (a = b; a < quante_comande; a++)
                            {
                                strcpy(serv_coda_comande[a].tav_num, serv_coda_comande[a + 1].tav_num);
                                serv_coda_comande[a].stato = serv_coda_comande[a + 1].stato;
                                serv_coda_comande[a].id = serv_coda_comande[a + 1].id;
                                serv_coda_comande[a].kd_assegnato = serv_coda_comande[a + 1].kd_assegnato;
                                serv_coda_comande[a].td_assegnato = serv_coda_comande[a + 1].td_assegnato;
                                serv_coda_comande[a].id_comanda = serv_coda_comande[a + 1].id_comanda;
                                for (c = 0; c < serv_coda_comande[a + 1].id_comanda; c++)
                                {
                                    strcpy(serv_coda_comande[a].desc[c], serv_coda_comande[a + 1].desc[c]);
                                    serv_coda_comande[a].quantita[c] = serv_coda_comande[a + 1].quantita[c];
                                }
                            }

                            memset(serv_coda_comande[quante_comande + 1].tav_num, 0, sizeof(serv_coda_comande[quante_comande + 1].tav_num));
                            memset(serv_coda_comande[quante_comande + 1].desc, 0, sizeof(serv_coda_comande[quante_comande + 1].desc));
                            memset(serv_coda_comande[quante_comande + 1].quantita, 0, sizeof(serv_coda_comande[quante_comande + 1].quantita));
                            serv_coda_comande[quante_comande + 1].stato = 'x';
                            serv_coda_comande[quante_comande + 1].kd_assegnato = -1;
                            serv_coda_comande[quante_comande + 1].td_assegnato = -1;
                            serv_coda_comande[quante_comande + 1].id_comanda = 0;
                            serv_coda_comande[quante_comande + 1].id = -1;
                            b--; // se l'ho trovato, sposto tutto di 1 potrei perdermi una comanda se non torno indietro
                            check = 1;
                        }
                    }
                    if (check == 1)
                    {
                        printf("Un Table Device ha abbandonato il ristorante. Comand* annullat*.\n\n");
                    }

                    fflush(stdout);
                    close(i);
                    n_table--;
                    array_tds[j] = -1;
                    FD_CLR(i, master);
                    break;
                }
            }
        }
    }
    return;
}