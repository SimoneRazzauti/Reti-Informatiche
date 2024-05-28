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

#define MAX_PIATTI 15 // nnumero massimo di piatti nel menu
#define MAX_COMANDE_IN_ATTESA 10
#define DESCRIZIONE 100 // descrizione del piatto

// Strutture per salvare le informazioni del menu e delle comande
struct piatto{
    char id[2]; // contiene la descrizione del piatto
    int costo;    // contiene il costo del piatto
    char desc[DESCRIZIONE];
};

// creo il menu composto da MAX_PIATTI 
struct piatto menu[MAX_PIATTI]; 

struct comanda{
    int id_comanda; // numero della comanda
    char desc[MAX_PIATTI][DESCRIZIONE]; // sarà A1, A3, P2 ecc... il codice del piatto
    int quantita[MAX_PIATTI]; // Quantita' scelta del piatto XX
};

struct comanda coda_comande[MAX_COMANDE_IN_ATTESA]; // coda di comande in attesa di essere accettate da un kd

struct comanda_kd{
    char tav_num[5]; // numero del tavolo da cui proviene la comanda
    char desc[MAX_PIATTI][DESCRIZIONE]; // sarà A1, A3, P2 ecc... il codice del piatto
    int quantita[MAX_PIATTI]; // Quantita' scelta del piatto XX
};

struct comanda_kd coda_comande_kd[MAX_COMANDE_IN_ATTESA]; // coda di comande in attesa di essere servite da kd

struct comanda_server
{
    char tav_num[5];                    // numero del tavolo da cui proviene la comanda
    int id_comanda;                    // quante comande ha la comanda X
    char desc[MAX_PIATTI][DESCRIZIONE]; // sarà A1, A3, P2 ecc... il codice del piatto
    int quantita[MAX_PIATTI];           // quantita
    char stato;                         // stato della comanda: a (attesa), p (preparazione) o s (servizio)
    uint16_t id;
    int td_assegnato; // socket di comunicazione connesso a quello del td a cui la comanda e' stata assegnata
    int kd_assegnato; // socket di comunicazione connesso a quello del kd a cui la comanda e' stata assegnata
};

struct comanda_server serv_coda_comande[MAX_COMANDE_IN_ATTESA];    // coda di comande in attesa/ preparazione
struct comanda_server serv_comande_servite[MAX_COMANDE_IN_ATTESA]; // coda di comande in servizio

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