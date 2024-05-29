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

#define BUFFER_SIZE 1024 // lunghezza del buffer di comunicazione client-server
#define MAX_PIATTI 15 // nnumero massimo di piatti nel menu
#define MAX_COMANDE_IN_ATTESA 10 // numero massimo di comande in coda di attes
#define DESCRIZIONE 100 // descrizione del piatto
#define MAX_CLIENTS 10 // numero massimo di clienti connessi
#define MAX_TAVOLI 10 // numero massimo di tavoli nel ristorante
#define MAX_SALA_DESC 20 // il numero massimo di caratteri per descrivere una sala
#define MAX_KITCHENDEVICES 10 // numero massio di kitchen device connessi
#define BUFFER_SIZE 1024 // lunghezza del buffer di comunicazione client-server
#define LEN_ID 2 // lunghezza codici fissati per identificare il tipo di client al server (client-kd-td)
#define LEN_COMANDO 5 // lunghezza dei comandi da mandare ai vari client
#define MAX_WORDS 50 // numero massimo di parole che possono essere estratte dalla frase
#define MAX_WORD_LENGTH 50 // Lunghezza massima di ogni parola

// struttura per descrivere un piatto
struct piatto{
    char id[2]; // contiene la descrizione del piatto
    int costo; // contiene il costo del piatto
    char desc[DESCRIZIONE];
};

// creo il menu composto da MAX_PIATTI 
struct piatto menu[MAX_PIATTI]; 

// struttura di una comanda
struct comanda{
    int id_comanda; // numero della comanda
    char desc[MAX_PIATTI][DESCRIZIONE]; // sarà A1, A3, P2 ecc... il codice del piatto
    int quantita[MAX_PIATTI]; // Quantita' scelta del piatto XX
};

struct comanda coda_comande[MAX_COMANDE_IN_ATTESA]; // coda di comande in attesa di essere accettate da un kd

// struttura della comanda kd
struct comanda_kd{
    char tav_num[5]; // numero del tavolo da cui proviene la comanda
    char desc[MAX_PIATTI][DESCRIZIONE]; // sarà A1, A3, P2 ecc... il codice del piatto
    int quantita[MAX_PIATTI]; // Quantita' scelta del piatto XX
};

struct comanda_kd coda_comande_kd[MAX_COMANDE_IN_ATTESA]; // coda di comande in attesa di essere servite da kd

// struttura della comanda server
struct comanda_server{
    char tav_num[5]; // numero del tavolo da cui proviene la comanda
    int id_comanda; // quante comande ha la comanda X
    char desc[MAX_PIATTI][DESCRIZIONE]; // sarà A1, A3, P2 ecc... il codice del piatto
    int quantita[MAX_PIATTI]; // quantita
    char stato; // stato della comanda: a (attesa), p (preparazione) o s (servizio)
    uint16_t id;
    int td_assegnato; // socket di comunicazione connesso a quello del td a cui la comanda e' stata assegnata
    int kd_assegnato; // socket di comunicazione connesso a quello del kd a cui la comanda e' stata assegnata
};

struct comanda_server serv_coda_comande[MAX_COMANDE_IN_ATTESA];    // coda di comande in attesa/ preparazione
struct comanda_server serv_comande_servite[MAX_COMANDE_IN_ATTESA]; // coda di comande in servizio

// per lo stato del server
int quante_comande = 0; // contatore di quante comande sono in stato d'attesa o di preparazione
int quante_servite = 0; // contatore di quante comande sono in stato servito

int indicetavolo = 0; // indice di quanti tavoli sono stati proposti, utilizzato nelle funzioni
int array_tds[MAX_TAVOLI]; // contiene i sockfd dei socket di comunicazione usati per comunicare con i table device
int array_kds[MAX_KITCHENDEVICES]; // contiene i sockfd dei socket di comunicazione usati per comunicare con i kitchen device


struct tavolo{
    char sala[MAX_SALA_DESC];      // contiene la descrizione della sala in cui si trova il tavolo
    char tav[5];                   // codice tavolo
    int posti;                     // contiene il numero di posti del tavolo
    char descrizione[DESCRIZIONE]; // contiene la descrizione del tavolo
};

struct client{
    int socket; // sockfd del socket di comunicazione usato per comunicare con il client
    struct tavolo tavoli_proposti[MAX_TAVOLI]; // puntatore alla struttura tavolo per salvare le informazioni del tavoli proposti
};

struct client client_fds[MAX_CLIENTS]; // array di client attualmente connessi