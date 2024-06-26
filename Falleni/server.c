#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/time.h>
#include <time.h>

#define MAX_CLIENTS 10
#define MAX_TAVOLI 15
#define BUFFER_SIZE 1024

#define MAX_PIATTI 10
#define MAX_KITCHENDEVICES 10
#define MAX_COMANDE_IN_ATTESA 10

#define validLen 2
#define codiceLen 5

#define DESCRIZIONE 100
#define MAX_SALA_DESC 20

int indicetavolo = 0; // indice di quanti tavoli sono stati proposti, utilizzato nelle funzioni

int array_tds[MAX_TAVOLI];         // contiene i sockfd dei socket di comunicazione usati per comunicare con i table device
int array_kds[MAX_KITCHENDEVICES]; // contiene i sockfd dei socket di comunicazione usati per comunicare con i kitchen device

struct comanda
{
    char tav_num[5];                    // numero del tavolo da cui proviene la comanda
    int num_comande;                    // quante comande ha la comanda X
    char desc[MAX_PIATTI][DESCRIZIONE]; // sarà A1, A3, P2 ecc... il codice del piatto
    int quantita[MAX_PIATTI];           // quantita
    char stato;                         // stato della comanda: a (attesa), p (preparazione) o s (servizio)
    uint16_t id;
    int td_assegnato; // socket di comunicazione connesso a quello del td a cui la comanda è stata assegnata
    int kd_assegnato; // socket di comunicazione connesso a quello del kd a cui la comanda è stata assegnata
};

struct comanda coda_comande[MAX_COMANDE_IN_ATTESA];    // coda di comande in attesa/ preparazione
struct comanda comande_servite[MAX_COMANDE_IN_ATTESA]; // coda di comande in servizio

int quante_comande = 0;
int quante_servite = 0;

struct tavolo
{
    char sala[MAX_SALA_DESC];      // contiene la descrizione della sala in cui si trova il tavolo
    char tav[5];                   // codice tavolo
    int posti;                     // contiene il numero di posti del tavolo
    char descrizione[DESCRIZIONE]; // contiene la descrizione del tavolo
};

struct client
{
    int socket; // sockfd del socket di comunicazione usato per comunicare con il client
    struct tavolo tavoli_proposti[MAX_TAVOLI];
};

struct client client_fds[MAX_CLIENTS]; // array di client attualmente connessi

void comandi_disponibili()
{
    printf("\n*********************** BENVENUTO ***********************\n");
    printf("*                  Digita un comando:                   *\n");
    printf("*                                                       *\n");
    printf("* stat --> mostra lo stato delle comande ai vari tavoli *\n");
    printf("* stop --> termina il server                            *\n");
    printf("*                                                       *\n");
    printf("*********************************************************\n");
    fflush(stdout);
}

// Cerca_prenotazione è una funzione che mi serve per realizzare la lista da restituire al cliente che esegue la Find, utilizzata nella controlla_tavoli_liberi(...).
// Se un tavolo è occupato per la stessa data e ora, non viene inviato.

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
        { // se è occupato
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

    char codiceID[50]; // codice che servira' nel Table Device. Codice identificativo
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
            printf("Comanda %d del tavolo %s. Stato <In SERVIZIO>\n", comande_servite[j].id + 1, comande_servite[j].tav_num);
            for (a = 0; a < comande_servite[j].num_comande; a++)
            {
                printf("Piatto scelto: %s x %d\n", comande_servite[j].desc[a], comande_servite[j].quantita[a]);
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
            if (coda_comande[j].stato == lettera && lettera == 'a')
            {
                check = 1;
                printf("Comanda %d del tavolo %s. Stato <In ATTESA>\n", coda_comande[j].id + 1, coda_comande[j].tav_num);
                for (a = 0; a < coda_comande[j].num_comande; a++)
                {
                    printf("Piatto scelto: %s x %d\n", coda_comande[j].desc[a], coda_comande[j].quantita[a]);
                }
            }
            else if (coda_comande[j].stato == lettera && lettera == 'p')
            {
                check = 1;
                printf("Comanda %d del tavolo %s. Stato <In PREPARAZIONE>\n", coda_comande[j].id + 1, coda_comande[j].tav_num);
                for (a = 0; a < coda_comande[j].num_comande; a++)
                {
                    printf("Piatto scelto: %s x %d\n", coda_comande[j].desc[a], coda_comande[j].quantita[a]);
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
        if (strcmp(coda_comande[j].tav_num, tavolino) == 0)
        {
            printf("Comanda %d - Stato <", coda_comande[j].id + 1);
            if (coda_comande[j].stato == 'a')
                printf("in ATTESA>\n");
            else if (coda_comande[j].stato == 'p')
                printf("in PREPARAZIONE>\n");
            for (a = 0; a < coda_comande[j].num_comande; a++)
            {
                printf("Piatto scelto: %s x %d\n", coda_comande[j].desc[a], coda_comande[j].quantita[a]);
            }
        }
        check = 1;
    }

    for (j = 0; j < quante_servite; j++)
    {
        if (strcmp(comande_servite[j].tav_num, tavolino) == 0)
        {
            printf("Comanda %d - Stato <In SERVIZIO>\n", comande_servite[j].id + 1);
            for (a = 0; a < comande_servite[j].num_comande; a++)
            {
                printf("Piatto scelto: %s x %d\n", comande_servite[j].desc[a], comande_servite[j].quantita[a]);
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
        printf("Comanda %d - Stato <", coda_comande[j].id + 1);
        if (coda_comande[j].stato == 'a')
            printf("in ATTESA> del tavolo: %s\n", coda_comande[j].tav_num);
        else if (coda_comande[j].stato == 'p')
            printf("in PREPARAZIONE> del tavolo: %s\n", coda_comande[j].tav_num);
        for (a = 0; a < coda_comande[j].num_comande; a++)
        {
            printf("Piatto scelto: %s x %d\n", coda_comande[j].desc[a], coda_comande[j].quantita[a]);
        }
        check = 1;
    }

    for (j = 0; j < quante_servite; j++)
    {
        printf("Comanda %d - Stato <In SERVIZIO> del tavolo: %s\n", comande_servite[j].id + 1, comande_servite[j].tav_num);
        for (a = 0; a < comande_servite[j].num_comande; a++)
        {
            printf("Piatto scelto: %s x %d\n", comande_servite[j].desc[a], comande_servite[j].quantita[a]);
        }
        check = 1;
    }

    if (check == 0)
        printf("NESSUNA COMANDA PRESENTE\n");
    printf("\n");

    return;
}

// gestisce la recv/send ed errori.
void errori_ritorno(int ret, int i, int max_fd, int n_table, int n_kitchen, int n_clients, fd_set *master)
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
    { // nel caso in cui c'è stato un errore, lo stampo a video
        perror("ERRORE nella comunicazione con un socket remoto");
        return;
    }
    else if (ret == 0) // da capire chi ha chiuso e abbassare il numero
    {
        if (max_fd != 0)
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
                        if (coda_comande[b].kd_assegnato == i)
                        {
                            if (coda_comande[b].td_assegnato > 0)
                            { // Avviso i T.D. del K. Device che ha abbandonato la cucina.
                                strcpy(comando, "** Kitchen Device ha smesso di lavorare **\n");
                                len_H = strlen(comando) + 1;
                                len_N = htonl(len_H);
                                ret = send(coda_comande[b].td_assegnato, &len_N, sizeof(uint32_t), 0); // mando la dimensione
                                if (ret < 0)
                                {
                                    perror("Errore nell'invio del messaggio");
                                    exit(1);
                                }
                                ret = send(coda_comande[b].td_assegnato, comando, len_H, 0); // mando il messaggio
                                if (ret < 0)
                                {
                                    perror("Errore nell'invio del messaggio");
                                    exit(1);
                                }
                            }
                            coda_comande[b].stato = 'a'; // Rimetto la comanda in Attesa che qualche altro K. Device la prenda e tolgo il valore del suo socket dall'array
                            coda_comande[b].kd_assegnato = -1;
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
                        if (coda_comande[b].td_assegnato == i)
                        {
                            quante_comande--;
                            if (coda_comande[b].kd_assegnato > 0)
                            { // Avviso TUTTI i K.D. che il cliente ha lasciato il ristorante.
                                sprintf(comando, "** Cliente del tavolo %s ha lasciato il ristorante. **\n", coda_comande[b].tav_num);

                                len_H = strlen(comando) + 1;
                                len_N = htonl(len_H);
                                ret = send(coda_comande[b].kd_assegnato, &len_N, sizeof(uint32_t), 0); // mando la dimensione
                                if (ret < 0)
                                {
                                    perror("Errore nell'invio del messaggio");
                                    exit(1);
                                }
                                ret = send(coda_comande[b].kd_assegnato, comando, len_H, 0); // mando il messaggio
                                if (ret < 0)
                                {
                                    perror("Errore nell'invio del messaggio");
                                    exit(1);
                                }
                            }

                            // Rimuovo tutte le sue comande permettendo di poter fare lo STOP del server o di non far prendere al K.D. comande errate.
                            for (a = b; a < quante_comande; a++)
                            {
                                strcpy(coda_comande[a].tav_num, coda_comande[a + 1].tav_num);
                                coda_comande[a].stato = coda_comande[a + 1].stato;
                                coda_comande[a].id = coda_comande[a + 1].id;
                                coda_comande[a].kd_assegnato = coda_comande[a + 1].kd_assegnato;
                                coda_comande[a].td_assegnato = coda_comande[a + 1].td_assegnato;
                                coda_comande[a].num_comande = coda_comande[a + 1].num_comande;
                                for (c = 0; c < coda_comande[a + 1].num_comande; c++)
                                {
                                    strcpy(coda_comande[a].desc[c], coda_comande[a + 1].desc[c]);
                                    coda_comande[a].quantita[c] = coda_comande[a + 1].quantita[c];
                                }
                            }

                            memset(coda_comande[quante_comande + 1].tav_num, 0, sizeof(coda_comande[quante_comande + 1].tav_num));
                            memset(coda_comande[quante_comande + 1].desc, 0, sizeof(coda_comande[quante_comande + 1].desc));
                            memset(coda_comande[quante_comande + 1].quantita, 0, sizeof(coda_comande[quante_comande + 1].quantita));
                            coda_comande[quante_comande + 1].stato = 'x';
                            coda_comande[quante_comande + 1].kd_assegnato = -1;
                            coda_comande[quante_comande + 1].td_assegnato = -1;
                            coda_comande[quante_comande + 1].num_comande = 0;
                            coda_comande[quante_comande + 1].id = -1;
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

int main(int argc, char *argv[])
{
    int sockfd, max_fd, n_table, n_clients, n_kitchen, ch, check, c, com, newsockfd;
    int i, j, ret, a;
    int n, tavoloScelto;
    struct sockaddr_in serv_addr, cli_addr;
    char codPrenotazione[25], TavConto[5];  // TavConto = Tavolo che ha richiesto il Conto.
    in_port_t porta = htons(atoi(argv[1])); // utilizzo della funzione atoi per convertire la stringa rappresentante il numero di porta inserito dall'utente da terminale in un intero
    char buffer[BUFFER_SIZE];
    char comando[BUFFER_SIZE];
    fd_set master;
    fd_set read_fds;
    char *identificativo = "S\0";
    char *negazione = "N\0";
    char arraycopia[DESCRIZIONE];
    uint16_t n_comande = 0;
    uint16_t codice_id;
    char copia[5];
    char *stop;
    char *conferma;

    n_table = 0;
    n_clients = 0;
    n_kitchen = 0;

    int giorno, mese, anno, ora, numPersone;
    char data[20];
    char nomeFile[50] = "prenotazioni/"; // stringa per il nome del file
    FILE *fp;
    char cognome[30];

    uint32_t len_HO; // lunghezza del messaggio espressa in host order
    uint32_t len_NO; // lunghezza del messaggio espressa in network order

    // Creazione del socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("Errore nella creazione del socket");
        exit(1);
    }

    // INIZIALIZZAZIONE ARRAY E TABELLA
    memset(array_kds, -1, sizeof(array_kds));
    memset(array_tds, -1, sizeof(array_tds));
    memset(client_fds, 0, sizeof(client_fds)); // FORSE
    memset(coda_comande, 0, sizeof(coda_comande));
    memset(comande_servite, 0, sizeof(comande_servite));

    // Inizializzazione della struttura dell'indirizzo del server
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = porta;

    // Binding del socket all'indirizzo del server
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Errore nel binding del socket");
        exit(1);
    }

    // Inizio dell'ascolto delle connessioni in ingresso
    if (listen(sockfd, MAX_CLIENTS) < 0)
    {
        perror("Errore nell'avvio dell'ascolto delle connessioni");
        exit(1);
    }

    // Reset dei file descriptor della select
    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    FD_SET(sockfd, &master);
    FD_SET(0, &master);
    max_fd = sockfd;

    comandi_disponibili();
    while (1)
    {

        memset(comando, 0, sizeof(comando)); // metto a 0 comando, senno' se si preme semplicemente invio senza scrivere nulla, rimane l'ultima cosa ci era finita dentro

        read_fds = master;
        ret = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (ret < 0)
        {
            perror("ERRORE SELECT:");
            exit(1);
        }

        for (i = 0; i <= max_fd; i++)
        {
            if (FD_ISSET(i, &read_fds))
            {
                if (i == 0)
                { // PRONTO STDIN
                    memset(buffer, 0, sizeof(buffer));
                    memset(copia, 0, sizeof(copia));

                    fgets(buffer, BUFFER_SIZE, stdin);
                    sscanf(buffer, "%s %s", comando, copia);
                    int nT = 0;
                    if (strcmp(comando, "stat") == 0)
                    {
                        if (strlen(buffer) > 5)
                        {                                                                                 // è stato inserito qualcosa dopo stat
                            if (copia[0] != 'a' && copia[0] != 'p' && copia[0] != 's' && copia[0] != 'T') // se dice lo stato
                            {                                                                             // vedo se lo stato indicato è corretto
                                printf("ERRORE: stato non valido.\n");
                                fflush(stdout);
                                continue;
                            }
                            else if (copia[0] == 'a' || copia[0] == 'p' || copia[0] == 's')
                            {
                                stampa_stat_lettera(copia[0]);
                            }
                            else if (sscanf(copia, "T%d", &nT) == 1)
                            { // se dice il tavolo
                                if (nT < 1 || nT > MAX_TAVOLI)
                                { // vedo il numero del tavolo è valido
                                    printf("ERRORE: numero del tavolo non valido.\n");
                                    fflush(stdout);
                                    continue;
                                }
                                stampa_stat_tavolo(copia);
                                // stampa
                            }
                        }
                        else
                        { // tutto
                            stampa_stat_all();
                        }
                        fflush(stdout);
                    }

                    else if (strcmp(comando, "stop") == 0)
                    {
                        ch = 0; // controllo check
                        for (j = 0; j < quante_comande; j++)
                        {
                            if (coda_comande[j].stato == 'p' || coda_comande[j].stato == 'a')
                            {
                                printf("**ERRORE: Ci sono comande in preparazione o in attesa. RIPROVARE dopo. ** \n");
                                ch = 1;
                                break;
                            }
                        }

                        if (ch == 0)
                        {
                            stop = "STOP\0";
                            for (j = 0; j < n_clients; j++)
                            {
                                if (client_fds[j].socket != -1)
                                {
                                    len_HO = strlen(stop);
                                    len_NO = htonl(len_HO);
                                    ret = send(client_fds[j].socket, &len_NO, sizeof(uint32_t), 0); // mando la dimensione
                                    if (ret < 0)
                                    {
                                        perror("Errore nell'invio del messaggio chiusura CLIENT");
                                        exit(1);
                                    }
                                    ret = send(client_fds[j].socket, stop, len_HO, 0); // mando il messaggio
                                    if (ret < 0)
                                    {
                                        perror("Errore nell'invio del messaggio chiusura CLIENT2");
                                        exit(1);
                                    }
                                }
                            }
                            for (j = 0; j < n_kitchen; j++)
                            {
                                if (array_kds[j] != -1)
                                {
                                    len_HO = strlen(stop);
                                    len_NO = htonl(len_HO);
                                    ret = send(array_kds[j], &len_NO, sizeof(uint32_t), 0); // mando la dimensione
                                    if (ret < 0)
                                    {
                                        perror("Errore nell'invio del messaggio");
                                        exit(1);
                                    }
                                    ret = send(array_kds[j], stop, len_HO, 0); // mando il messaggio
                                    if (ret < 0)
                                    {
                                        perror("Errore nell'invio del messaggio");
                                        exit(1);
                                    }
                                }
                            }
                            for (j = 0; j < n_table; j++)
                            {
                                if (array_tds[j] != -1)
                                {
                                    len_HO = strlen(stop);
                                    len_NO = htonl(len_HO);
                                    ret = send(array_tds[j], &len_NO, sizeof(uint32_t), 0); // mando la dimensione
                                    if (ret < 0)
                                    {
                                        perror("Errore nell'invio del messaggio");
                                        exit(1);
                                    }
                                    ret = send(array_tds[j], stop, len_HO, 0); // mando il messaggio
                                    if (ret < 0)
                                    {
                                        perror("Errore nell'invio del messaggio");
                                        exit(1);
                                    }
                                }
                            }
                            printf("** SERVER DISCONNESSO CORRETTAMENTE ** \n");
                            exit(0);
                            close(sockfd);
                        }

                        fflush(stdout);
                    }

                    else
                    {
                        printf("ERRORE: comando non valido.\n");
                        fflush(stdout);
                    }
                }
                else if (i == sockfd)
                { // PRONTO DESCRITTORE SOCKET DI ASCOLTO
                    printf("\n");
                    fflush(stdout);
                    socklen_t cli_len = sizeof(cli_addr);
                    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &cli_len);
                    if (newsockfd < 0)
                    {
                        perror("Errore nell'accettazione della connessione");
                        exit(1);
                    }
                    n = recv(newsockfd, (void *)buffer, validLen, 0);
                    errori_ritorno(n, newsockfd, max_fd, n_table, n_kitchen, n_clients, &master);

                    switch (buffer[0])
                    {
                    case 'C':
                        if (n_clients >= MAX_CLIENTS)
                        {
                            fprintf(stderr, "Troppi client connessi, connessione rifiutata.\n");
                            fflush(stdout);
                            client_fds[i].socket = -1;
                            n_clients--;
                            FD_CLR(i, &master);
                            close(newsockfd);
                        }
                        else
                        {
                            if (send(newsockfd, (void *)identificativo, validLen, 0) == -1)
                            {
                                perror("Errore nella connessione al server");
                                exit(1);
                            }
                            client_fds[n_clients].socket = newsockfd;
                            n_clients++;
                        }
                        break;
                    case 'K':
                        if (n_kitchen >= MAX_KITCHENDEVICES)
                        {
                            fprintf(stderr, "Troppi kitchen device connessi, connessione rifiutata.\n");
                            fflush(stdout);
                            array_kds[i] = -1;
                            n_kitchen--;
                            FD_CLR(i, &master);
                            close(newsockfd);
                        }
                        else
                        {
                            if (send(newsockfd, (void *)identificativo, validLen, 0) == -1)
                            {
                                perror("Errore nella connessione al server");
                                exit(1);
                            }
                            array_kds[n_kitchen] = newsockfd;
                            n_kitchen++;
                        }
                        break;
                    case 'T':
                        if (n_table >= MAX_TAVOLI)
                        {
                            fprintf(stderr, "Troppi tavoli connessi, connessione rifiutata.\n");
                            fflush(stdout);
                            array_tds[i] = -1;
                            n_table--;
                            FD_CLR(i, &master);
                            close(newsockfd);
                        }
                        else
                        {
                            if (send(newsockfd, (void *)identificativo, validLen, 0) == -1)
                            {
                                perror("Errore nella connessione al server");
                                exit(1);
                            }
                            array_tds[n_table] = newsockfd;
                            n_table++;
                        }
                        break;
                    default:
                        break;
                    }

                    FD_SET(newsockfd, &master); // se sono qui, identificazione e' andata bene. Allora adesso metto sockfd nel set.
                    if (newsockfd > max_fd)
                    {
                        max_fd = newsockfd;
                    }
                }

                else
                {
                    memset(buffer, 0, sizeof(buffer));
                    n = recv(i, buffer, codiceLen, 0); // riceve il codice
                    errori_ritorno(n, i, max_fd, n_table, n_kitchen, n_clients, &master);

                    if (strncmp(buffer, "find", strlen("find")) == 0)
                    {

                        // ricevo lunghezza messaggio
                        ret = recv(i, &len_NO, sizeof(uint32_t), 0);
                        errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);
                        len_HO = ntohl(len_NO);

                        // ricevo "num.Persone-GG-MM-AA ora cognome"
                        ret = recv(i, buffer, len_HO, 0);
                        errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);

                        // ricevo

                        printf("\nUn cliente sta cercando di prenotare...\n");
                        sscanf(buffer, "%d-%d-%d-%d %d %s", &numPersone, &giorno, &mese, &anno, &ora, cognome);
                        sprintf(data, "%d-%d-%d", giorno, mese, anno);
                        strcpy(nomeFile, "prenotazioni/");
                        char dataV[15];
                        strcpy(dataV, data);
                        strcat(dataV, ".txt");   // aggiunge l'estensione
                        strcat(nomeFile, dataV); // nome del file data da cercare

                        if ((fp = fopen(nomeFile, "r")))
                        {
                            // il file esiste, quindi non sono presenti tutti i tavoli ipoteticamente c'e' stata una prenotazione
                            //
                            // faccio le varie cose
                            fclose(fp);

                            indicetavolo = 0;
                            controlla_tavoli_liberi(data, ora, numPersone, nomeFile, i); // metto i tavoli disonibili in un array

                            c = 1;
                            for (a = 0; a < indicetavolo; a++)
                            {
                                memset(buffer, 0, sizeof(buffer));
                                sprintf(buffer, "%d) %s %s %s", c, client_fds[i].tavoli_proposti[a].tav,
                                        client_fds[i].tavoli_proposti[a].sala, client_fds[i].tavoli_proposti[a].descrizione);

                                len_HO = strlen(buffer) + 1;
                                len_NO = htonl(len_HO);
                                ret = send(i, &len_NO, sizeof(uint32_t), 0); // mando la dimensione
                                errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);
                                ret = send(i, buffer, len_HO, 0); // mando il messaggio

                                errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);
                                c++;
                            }

                            stop = "STOP\0";
                            len_HO = strlen(stop);
                            len_NO = htonl(len_HO);
                            ret = send(i, &len_NO, sizeof(uint32_t), 0);
                            errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);
                            ret = send(i, stop, len_HO, 0);
                            errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);
                        }
                        else
                        {
                            // genero il file con tutti i tavoli
                            // passo tutti i tavoli perche' non c'erano prenotazioni
                            indicetavolo = 0;
                            imposta_tavoli(numPersone, "tavoli/lista.txt", i); // metto i tavoli disonibili in un array

                            c = 1;
                            stop = "STOP\0";
                            for (a = 0; a < indicetavolo; a++)
                            {
                                memset(buffer, 0, sizeof(buffer));
                                sprintf(buffer, "%d) %s %s %s", c, client_fds[i].tavoli_proposti[a].tav,
                                        client_fds[i].tavoli_proposti[a].sala, client_fds[i].tavoli_proposti[a].descrizione);

                                len_HO = strlen(buffer) + 1;
                                len_NO = htonl(len_HO);
                                ret = send(i, &len_NO, sizeof(uint32_t), 0); // mando la dimensione
                                if (ret < 0)
                                {
                                    perror("Errore nell'invio del messaggio");
                                    exit(1);
                                }
                                ret = send(i, buffer, len_HO, 0); // mando il messaggio
                                if (ret < 0)
                                {
                                    perror("Errore nell'invio del messaggio");
                                    exit(1);
                                }
                                c++;
                            }

                            len_HO = strlen(stop);
                            len_NO = htonl(len_HO);
                            ret = send(i, &len_NO, sizeof(uint32_t), 0);
                            errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);
                            ret = send(i, stop, len_HO, 0);
                            errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);
                        }
                    }
                    else if (strncmp(buffer, "book", strlen("book")) == 0)
                    {
                        tavoloScelto = 0;
                        // ricevo lunghezza messaggio
                        ret = recv(i, &len_NO, sizeof(uint32_t), 0);
                        errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);
                        len_HO = ntohl(len_NO);

                        // ricevo "num.Persone-GG-MM-AA ora cognome"
                        ret = recv(i, buffer, len_HO, 0);
                        errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);

                        sscanf(buffer, "%d %d-%d-%d-%d %d %s", &tavoloScelto, &numPersone, &giorno, &mese, &anno, &ora, cognome);
                        sprintf(data, "%d-%d-%d", giorno, mese, anno);

                        strcpy(nomeFile, "prenotazioni/");
                        char dataV[15];
                        strcpy(dataV, data);
                        strcat(dataV, ".txt");   // aggiunge l'estensione
                        strcat(nomeFile, dataV); // nome del file data da cercare

                        tavoloScelto--;
                        // controlla se puo' aggiungere la prenotazione
                        if (prenotazione_tavolo(nomeFile, giorno, mese, anno, ora, cognome, client_fds[i].tavoli_proposti[tavoloScelto].tav))
                        { // andata a buon fine
                            printf("AVVISO! Nuova prenotazione per: \nGiorno %d, Mese %d, Anno %d, Ora %d\n", giorno, mese, anno, ora);
                            sprintf(codPrenotazione, "%s-%s-%d", client_fds[i].tavoli_proposti[tavoloScelto].tav, data, ora);

                            conferma = "PRENOTAZIONE EFFETTUATA\0";
                            len_HO = strlen(conferma);
                            len_NO = htonl(len_HO);
                            ret = send(i, &len_NO, sizeof(uint32_t), 0); // mando la dimensione
                            errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);
                            ret = send(i, conferma, len_HO, 0); // mando il messaggio
                            errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);

                            len_HO = strlen(codPrenotazione);
                            len_NO = htonl(len_HO);
                            ret = send(i, &len_NO, sizeof(uint32_t), 0); // mando la dimensione
                            errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);
                            ret = send(i, codPrenotazione, len_HO, 0); // mando il messaggio
                            errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);
                        }
                        else
                        { // occupato nel frattempo
                            printf("NO\n\n");
                            conferma = "NO\0";
                            len_HO = strlen(conferma);
                            len_NO = htonl(len_HO);
                            ret = send(i, &len_NO, sizeof(uint32_t), 0); // mando la dimensione
                            errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);
                            ret = send(i, conferma, len_HO, 0); // mando il messaggio
                            errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);
                        }
                    }
                    else if (strncmp(buffer, "code", strlen("code")) == 0)
                    {
                        ret = recv(i, &len_NO, sizeof(uint32_t), 0);
                        errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);
                        len_HO = ntohl(len_NO);

                        // ricevo CODICE TX-GG-MM-AA
                        ret = recv(i, buffer, len_HO, 0);
                        errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);

                        if (controlla_prenotazione(buffer))
                        {
                            printf("Nuovo t. %d device arrivato al ristorante.\n", i);

                            memset(copia, 0, sizeof(copia));
                            if (buffer[2] == '-')
                            { // tavolo da 0-9
                                sscanf(buffer, "%2s", copia);
                            }
                            else if (buffer[3] == '-')
                            { // tavolo da 10-99
                                sscanf(buffer, "%3s", copia);
                            }
                            if (send(i, (void *)identificativo, validLen, 0) < 0)
                            {
                                perror("Errore nella ricezione del messaggio\n");
                                exit(1);
                            }
                        }
                        else
                        {
                            if (send(i, (void *)negazione, validLen, 0) < 0)
                            {
                                perror("Errore nella ricezione del messaggio\n");
                                exit(1);
                            }
                        }

                        sscanf(buffer, "%s-%d-%d-%d", comando, &giorno, &mese, &anno);
                    }
                    else if (strncmp(buffer, "menu", strlen("menu")) == 0)
                    {
                        fp = fopen("menu.txt", "r"); // apre il file in modalita'  lettura

                        if (fp == NULL)
                        {
                            printf("Errore nell'apertura del file menu.txt \n");
                            exit(1); // esce dal programma in caso di errore
                        }

                        // legge le frasi dal file -> Spedisco al T.D.
                        while (fgets(arraycopia, DESCRIZIONE, fp) != NULL)
                        {
                            len_HO = strlen(arraycopia) + 1;
                            len_NO = htonl(len_HO);
                            ret = send(i, &len_NO, sizeof(uint32_t), 0); // mando la dimensione
                            errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);
                            ret = send(i, arraycopia, len_HO, 0); // mando il messaggio
                            errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);
                        }

                        fclose(fp); // chiude il file

                        stop = "STOP\0"; // Per far terminare il loop ;; nel T.D.
                        len_HO = strlen(stop);
                        len_NO = htonl(len_HO);
                        ret = send(i, &len_NO, sizeof(uint32_t), 0);
                        errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);
                        ret = send(i, stop, len_HO, 0);
                        errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);
                    }
                    else if (strncmp(buffer, "comm", strlen("comm")) == 0)
                    {

                        if (recv(i, &n_comande, sizeof(uint16_t), 0) < 0)
                        { // quante comande ho ricevuto nell'ordine
                            perror("recv failed");
                            exit(EXIT_FAILURE);
                        }
                        if (recv(i, &codice_id, sizeof(uint16_t), 0) < 0)
                        { // quante comande ho avuto in totale
                            perror("recv failed");
                            exit(EXIT_FAILURE);
                        }

                        ret = recv(i, &len_NO, sizeof(uint32_t), 0); // ricevo il TXX e lo salvo in "copia"
                        errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);
                        len_HO = ntohl(len_NO);
                        ret = recv(i, copia, len_HO, 0);
                        errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);

                        if (quante_comande >= MAX_COMANDE_IN_ATTESA)
                        {
                            if (send(i, (void *)negazione, validLen, 0) == -1)
                            {
                                perror("Errore nella spedizione al server");
                                exit(1);
                            }
                        }
                        else
                        {
                            if (send(i, (void *)identificativo, validLen, 0) == -1)
                            {
                                perror("Errore nella spedizione al server");
                                exit(1);
                            }
                        }

                        for (j = 0; j < n_comande - 1; j++)
                        {
                            ret = recv(i, &len_NO, sizeof(uint32_t), 0);
                            errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);
                            len_HO = ntohl(len_NO);
                            ret = recv(i, buffer, len_HO, 0);
                            errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);
                            sscanf(buffer, "%2s-%d", coda_comande[quante_comande].desc[j], &coda_comande[quante_comande].quantita[j]);
                        }
                        strcpy(coda_comande[quante_comande].tav_num, copia);
                        coda_comande[quante_comande].stato = 'a';
                        coda_comande[quante_comande].td_assegnato = i;

                        coda_comande[quante_comande].num_comande = n_comande - 1;
                        coda_comande[quante_comande].id = codice_id;

                        quante_comande++;
                        strcpy(comando, "** Nuova comanda **"); // Mando l'avviso a tutti i K.D. connessi
                        for (j = 0; j <= max_fd; j++)
                        { // escludo lo 0
                            if (array_kds[j] != -1)
                            {
                                len_HO = strlen(comando) + 1;
                                len_NO = htonl(len_HO);
                                ret = send(array_kds[j], &len_NO, sizeof(uint32_t), 0); // mando la dimensione
                                if (ret < 0)
                                {
                                    perror("Errore nell'invio del messaggio");
                                    exit(1);
                                }
                                ret = send(array_kds[j], comando, len_HO, 0); // mando il messaggio
                                if (ret < 0)
                                {
                                    perror("Errore nell'invio del messaggio");
                                    exit(1);
                                }
                            }
                        }
                    }
                    else if (strncmp(buffer, "cont", strlen("cont")) == 0)
                    {
                        ret = recv(i, &len_NO, sizeof(uint32_t), 0);
                        errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);
                        len_HO = ntohl(len_NO);
                        ret = recv(i, TavConto, len_HO, 0);
                        errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);

                        check = 0;
                        for (j = 0; j < quante_comande; j++)
                        {
                            if (strcmp(coda_comande[j].tav_num, TavConto) == 0 && (coda_comande[j].stato == 'p' || coda_comande[j].stato == 'a')) // c'è sempre qualche ordinazione
                            {                                                                                                                     // Se ci sono sempre comande in Preparazione o in Attesa non può confermare il conto. (Il conto viene calcolato poi direttamente dal T.D. per alleggerire il carico del server)
                                ret = send(i, (void *)negazione, validLen, 0);
                                errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);
                                check = 1;
                                break;
                            }
                        }

                        if (check == 0)
                        {
                            ret = send(i, (void *)identificativo, validLen, 0);
                            errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);
                        }
                    }
                    else if (strncmp(buffer, "take", strlen("take")) == 0)
                    {

                        for (j = 0; j < quante_comande; j++)
                        {
                            if (coda_comande[j].stato == 'a')
                            { // le comande sono in ordine, la prima che trova in stato Attesa, la restituisce.

                                len_HO = strlen(coda_comande[j].tav_num) + 1;
                                len_NO = htonl(len_HO);
                                ret = send(i, &len_NO, sizeof(uint32_t), 0); // mando la dimensione
                                errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);

                                ret = send(i, coda_comande[j].tav_num, len_HO, 0); // mando codice tavolo
                                errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);

                                ret = send(i, &coda_comande[j].id, sizeof(uint16_t), 0); // mando il messaggio
                                errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);

                                for (a = 0; a < coda_comande[j].num_comande; a++) // Mando tutte le comande
                                {
                                    sprintf(comando, "%s %d", coda_comande[j].desc[a], coda_comande[j].quantita[a]);

                                    len_HO = strlen(comando) + 1;
                                    len_NO = htonl(len_HO);
                                    ret = send(i, &len_NO, sizeof(uint32_t), 0); // mando la dimensione
                                    errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);
                                    ret = send(i, comando, len_HO, 0); // mando il messaggio
                                    errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);
                                }
                                coda_comande[j].stato = 'p';      // Adesso sarà in Preparazione
                                coda_comande[j].kd_assegnato = i; // Socket del K.D. che ha preso la comanda

                                sprintf(comando, "** Comanda %d in PREPARAZIONE **", coda_comande[j].id + 1);
                                // AVVISO il T.D. che la comanda X è stata presa in carico
                                len_HO = strlen(comando) + 1;
                                len_NO = htonl(len_HO);
                                ret = send(coda_comande[j].td_assegnato, &len_NO, sizeof(uint32_t), 0); // mando la dimensione
                                if (ret < 0)
                                {
                                    perror("Errore nell'invio del messaggio take");
                                    exit(1);
                                }
                                ret = send(coda_comande[j].td_assegnato, comando, len_HO, 0); // mando il messaggio
                                if (ret < 0)
                                {
                                    perror("Errore nell'invio del messaggio tak2");
                                    exit(1);
                                }

                                break;
                            }
                        }
                        stop = "STOP\0";
                        len_HO = strlen(stop);
                        len_NO = htonl(len_HO);
                        ret = send(i, &len_NO, sizeof(uint32_t), 0);
                        errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);
                        ret = send(i, stop, len_HO, 0);
                        errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);
                    }
                    else if (strncmp(buffer, "show", strlen("show")) == 0)
                    {
                        for (j = 0; j < quante_comande; j++)
                        {
                            if (coda_comande[j].kd_assegnato == i && coda_comande[j].stato == 'p')
                            { // restituisco solo le comande del K.D. che ha eseguito il comando Show e che sono in preparazione, come da richiesta
                                sprintf(comando, "** Comanda N.%d %s **", coda_comande[j].id + 1, coda_comande[j].tav_num);
                                len_HO = strlen(comando) + 1;
                                len_NO = htonl(len_HO);
                                ret = send(i, &len_NO, sizeof(uint32_t), 0); // mando la dimensione
                                errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);
                                ret = send(i, comando, len_HO, 0); // mando il messaggio
                                errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);

                                for (a = 0; a < coda_comande[j].num_comande; a++)
                                {
                                    sprintf(comando, "%s %d", coda_comande[j].desc[a], coda_comande[j].quantita[a]);

                                    len_HO = strlen(comando) + 1;
                                    len_NO = htonl(len_HO);
                                    ret = send(i, &len_NO, sizeof(uint32_t), 0); // mando la dimensione
                                    errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);
                                    ret = send(i, comando, len_HO, 0); // mando il messaggio
                                    errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);
                                }
                            }
                        }

                        stop = "STOP\0";
                        len_HO = strlen(stop);
                        len_NO = htonl(len_HO);
                        ret = send(i, &len_NO, sizeof(uint32_t), 0);
                        errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);
                        ret = send(i, stop, len_HO, 0);
                        errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);
                    }
                    else if (strncmp(buffer, "read", strlen("read")) == 0)
                    {
                        check = 0;

                        ret = recv(i, &len_NO, sizeof(uint32_t), 0);
                        errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);
                        len_HO = ntohl(len_NO);
                        ret = recv(i, buffer, len_HO, 0);
                        errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);

                        sscanf(buffer, "com%d-%s", &com, TavConto);
                        com--;
                        for (j = 0; j < quante_comande; j++)
                        {
                            if (strcmp(coda_comande[j].tav_num, TavConto) == 0 && coda_comande[j].id == com)
                            { // trova la comanda scelta dal K.D. la mette nell'array delle comande servite, copiando i valori e modificando laddove necessario
                                strcpy(comande_servite[quante_servite].tav_num, coda_comande[j].tav_num);
                                comande_servite[quante_servite].id = coda_comande[j].id;
                                comande_servite[quante_servite].stato = 's';
                                comande_servite[quante_servite].kd_assegnato = coda_comande[j].kd_assegnato;
                                comande_servite[quante_servite].td_assegnato = coda_comande[j].td_assegnato;
                                comande_servite[quante_servite].num_comande = coda_comande[j].num_comande;
                                for (a = 0; a < coda_comande[j].num_comande; a++)
                                {
                                    strcpy(comande_servite[quante_servite].desc[a], coda_comande[j].desc[a]);
                                    comande_servite[quante_servite].quantita[a] = coda_comande[j].quantita[a];
                                }

                                quante_comande--;
                                // Scorro tutte le comande di una posizione.
                                for (a = j; a < quante_comande; a++)
                                {
                                    strcpy(coda_comande[a].tav_num, coda_comande[a + 1].tav_num);
                                    coda_comande[a].stato = coda_comande[a + 1].stato;
                                    coda_comande[a].id = coda_comande[a + 1].id;
                                    coda_comande[a].kd_assegnato = coda_comande[a + 1].kd_assegnato;
                                    coda_comande[a].td_assegnato = coda_comande[a + 1].td_assegnato;
                                    coda_comande[a].num_comande = coda_comande[a + 1].num_comande;
                                    for (c = 0; c < coda_comande[a + 1].num_comande; c++)
                                    {
                                        strcpy(coda_comande[a].desc[c], coda_comande[a + 1].desc[c]);
                                        coda_comande[a].quantita[c] = coda_comande[a + 1].quantita[c];
                                    }
                                }

                                memset(coda_comande[quante_comande + 1].tav_num, 0, sizeof(coda_comande[quante_comande + 1].tav_num));
                                memset(coda_comande[quante_comande + 1].desc, 0, sizeof(coda_comande[quante_comande + 1].desc));
                                memset(coda_comande[quante_comande + 1].quantita, 0, sizeof(coda_comande[quante_comande + 1].quantita));
                                coda_comande[quante_comande + 1].stato = 'x';
                                coda_comande[quante_comande + 1].kd_assegnato = -1;
                                coda_comande[quante_comande + 1].td_assegnato = -1;
                                coda_comande[quante_comande + 1].num_comande = 0;
                                coda_comande[quante_comande + 1].id = -1;

                                sprintf(comando, "** Comanda %d in SERVIZIO **", comande_servite[quante_servite].id + 1);
                                // avviso il T.D. che la comanda X è in SERVIZIO
                                len_HO = strlen(comando) + 1;
                                len_NO = htonl(len_HO);
                                ret = send(comande_servite[quante_servite].td_assegnato, &len_NO, sizeof(uint32_t), 0); // mando la dimensione
                                if (ret < 0)
                                {
                                    perror("Errore nell'invio del messaggio take");
                                    exit(1);
                                }
                                ret = send(comande_servite[quante_servite].td_assegnato, comando, len_HO, 0); // mando il messaggio
                                if (ret < 0)
                                {
                                    perror("Errore nell'invio del messaggio tak2");
                                    exit(1);
                                }

                                quante_servite++;
                                check = 1;
                                break;
                            }
                        }
                        if (check == 1)
                        {
                            printf("Comanda in servizio! \n\n");
                            ret = send(i, (void *)identificativo, validLen, 0);
                            errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);
                        }
                        else if (check == 0)
                        {
                            ret = send(i, (void *)negazione, validLen, 0);
                            errori_ritorno(ret, i, max_fd, n_table, n_kitchen, n_clients, &master);
                        }
                    }
                }
            }
        }
    }
    close(sockfd);
    return 0;
}