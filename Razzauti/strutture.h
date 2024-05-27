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
    int id_comanda;
    char desc[MAX_PIATTI][DESCRIZIONE]; // sarà A1, A3, P2 ecc... il codice del piatto
    int quantita[MAX_PIATTI];           // Quantita' scelta del piatto XX
};

struct comanda coda_comande[MAX_COMANDE_IN_ATTESA]; // coda di comande in attesa

struct comanda_kd{
    char tav_num[5];                    // numero del tavolo da cui proviene la comanda
    char desc[MAX_PIATTI][DESCRIZIONE]; // sarà A1, A3, P2 ecc... il codice del piatto
    int quantita[MAX_PIATTI];           // quantita
};

struct comanda_kd coda_comande_kd[MAX_COMANDE_IN_ATTESA]; // coda di comande in attesa