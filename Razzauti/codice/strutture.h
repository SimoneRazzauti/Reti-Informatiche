/* Strutture e definizioni necessarie al server */
/* -------------------------------------------- */

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

#define nTavoli 16
#define nPiatti 8
#define dimEntryMeny 64
#define nMaxClient 16
#define nMaxTd nTavoli
#define nMaxKd 8
#define BUFFER_SIZE 1024
#define BENVENUTO_SERVER "BENVENUTO NEL SERVER!\nCOMANDI DISPONIBILI:\n1) stat ->  restituisce le comande del giorno per tavolo o per status ('a', 'p' o 's') \n2) stop ->  il server si arresta (solo se non ci sono comande in attesa o in preparazione\n"

/*           Gestione dei tavoli 
   ---------------------------------------
   Viene parsato il file "tavoli.txt" dopo
   l'accensione del server.
   --------------------------------------- 
*/

struct tavolo
{
    int numero;     // Univoco nel ristorante
    int nPosti;
    char sala[32];
    char descrizione[64];
};

/*       Gestione delle prenotazioni 
   ---------------------------------------
   Per ogni tavolo, esiste una lista di 
   prenotazioni ordinata in base al time-
   stamp di esecuzione.
   --------------------------------------- 
*/

struct prenotazione
{
    char cognome[64];
    char data_ora[12];    // Non della richiesta, ma della prenotazione
    char pwd[5];
    struct prenotazione *prossima;
    /* 
    Il tavolo non è necessario poiché corrisponde
    all'indice in cui viene salvato all'interno
    dell'array "prenotazioni".
    */
};

/*           Gestione dei piatti 
   ---------------------------------------
   Viene parsato il file "menu.txt" dopo
   l'accensione del server.
   --------------------------------------- 
*/

struct piatto
{
    char codice[2];
    int prezzo;
    /* 
    Il tavolo non è necessario poiché corrisponde
    all'indice in cui viene salvato all'interno
    dell'array "comande".
    */
};

/*         Gestione delle comande 
   ---------------------------------------
   Per ogni tavolo, esiste una lista di 
   comande ordinata in base al timestamp
   di richiesta. Vengono cancellate alla
   richiesta del conto.
   --------------------------------------- 
*/

enum stato_comanda { in_attesa, in_preparazione, in_servizio };

struct comanda
{
    int nComanda;
    int quantita[nPiatti];
    /* 
    Quantità dell'i-esimo piatto corrispondete
    nell'array dei piatti.
    */
    time_t timestamp;    // utilizzato per trovare la meno recente
    enum stato_comanda stato;
    int kd; // SocketId del kd da cui è stato prelevato
    /* 
    Il tavolo non è necessario poiché corrisponde
    all'indice in cui viene salvato all'interno
    dell'array "comande".
    */
    struct comanda *prossima;
};

/*           Gestione dei Thread 
   ---------------------------------------
   Ogni volta che un socket registrato
   viene comunicato al server, quest'ultimo
   risponde mediante un thread, che viene
   inserito in una lista per controllare
   la terminazione una volta decisa la
   chiusura del server.
   --------------------------------------- 
*/

/* --------- Strutture globali ---------*/
// Array per i Socket
int socket_client[nMaxClient];
int socket_td[nMaxTd]; // associati all'indice del tavolo
int socket_kd[nMaxKd];

// Strutture per gestione dei thread
struct lis_thread *listaThread;

// Altro
char menu_text[nPiatti * dimEntryMeny];
struct piatto menu[nPiatti];
int numeroComanda; // UUID comanda
/* ------------------------------------ */

/* ------- Strutture globali FD ------- */
// Set di descrittori da monitorare
fd_set master;

// Set dei descrittori pronti
fd_set read_fds;

// Descrittore max
int fdmax;
/* ------------------------------------ */
