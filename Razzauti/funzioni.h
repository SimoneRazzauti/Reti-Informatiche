// firme delle funzioni

#include "funzioni.c"
// Funzione che controlla che la data inserita con la 'find' sia valida nel formato gg-mm-aa hh e che sia futura rispetto alla data attuale
int check_data(int GG, int MM, int AA, int HH);

// Gestione errori e gestione in caso di chiusura del socket remoto, termino.
void gestione_errore(int ret, int socket);