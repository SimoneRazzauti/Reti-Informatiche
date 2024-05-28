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
