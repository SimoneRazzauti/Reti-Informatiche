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
    char nome[2]; // contiene la descrizione del piatto
    int costo;    // contiene il costo del piatto
    char desc[DESCRIZIONE];
};

// creo il menu composto da MAX_PIATTI 
struct piatto menu[MAX_PIATTI]; 

struct comanda{
    int num_comande;
    char desc[MAX_PIATTI][DESCRIZIONE]; // sarà A1, A3, P2 ecc... il codice del piatto
    int quantita[MAX_PIATTI];           // Quantità scelta del piatto XX
};

struct comanda coda_comande[MAX_COMANDE_IN_ATTESA]; // coda di comande in attesa
