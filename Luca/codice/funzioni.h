/* Contiene i prototipi delle funzioni necessarie al Server */
/* -------------------------------------------------------- */

// Carica i tavoli dal file e li mette nell'array
void caricaTavoli();

// Carica il menu dal file e lo mette nell'array
void caricaMenu();

// Ritorna 1 nel caso ci siano attualmente delle comande
// "in_preparazione" o "in_servizio". 0 altrimenti
int comandeInSospeso();

// Invia al socket in input la lunghezza ed il messaggio dentro buffer
int invia(int j, char* buffer);

// Ricevi dal socket in input la lunghezza del messaggio e lo mette dentro lmsg
int riceviLunghezza(int j, int *lmsg);

// Riceve dal socket in input il messaggio e lo mette dentro buffer
int ricevi(int j, int lunghezza, char* buffer);

// Prende uno stato_comanda e inserisce dentro il buffer tutte le informazioni
// delle comande in quello stato di qualunque tavolino
void elencoComande(char* buffer, enum stato_comanda stato);

// Prende un tavolo e inserisce dentro il buffer tutte le informazioni
// delle comande inerenti a quel tavolino
void elencoComandeTavolo(char* buffer, int tavolo);

// Inserisce in base alla lettera c, il socket id nell'array relativo
int inserisci(int i, char* c);

// Prende i parametri della find ed inserisce nel buffer le disponibilità
int cercaDisponibilita(int nPers, char* dataora, char* buffer, char* disponibilita);

// Gestisce UNA richiesta da parte di un client
void *gestisciClient(void* i);

// Gestisce UNA richiesta da parte di un table device
void *gestisciTd(void* i);

// Gestisce UNA richiesta da parte di un kitchen device
void *gestisciKd(void* i);

// Dealloca tutte le strutture
void deallocaStrutture();

// Trova il massimo tra 3 interi
int max(int v1, int v2, int v3);