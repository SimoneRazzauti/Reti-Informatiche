#include "strutture.h" // Ci sono anche gli altri include
#include "funzioni.h" // Firme delle funzioni

int main(int argc, char* argv[]) {
    /* --- Strutture --- */
    // Le strutture vengono definite in "strutture.h" per
    // permettere l'utilizzo della libreria "funzioni.c"

    // Le inizializzo
    int indice;
    for(indice = 0; indice < nMaxClient; indice++)
        socket_client[indice] = -1;
    for(indice = 0; indice < nMaxTd; indice++)
        socket_td[indice] = -1;
    for(indice = 0; indice < nMaxKd; indice++)
        socket_kd[indice] = -1;
    
    for(indice = 0; indice < nTavoli; indice++)
        prenotazioni[indice] = NULL;
    for(indice = 0; indice < nTavoli; indice++)
        comande[indice] = NULL;
    listaThread = NULL;
    
    for(indice = 0; indice < nTavoli; indice++)
        tavoli_logged[indice] = 0;

    pthread_mutex_init(&tavoli_lock, NULL);
    pthread_mutex_init(&prenotazioni_lock, NULL);
    pthread_mutex_init(&comande_lock, NULL);
    pthread_mutex_init(&listaThread_lock, NULL);
    pthread_mutex_init(&socket_lock, NULL);
    pthread_mutex_init(&fd_lock, NULL);

    numeroComanda = 1;

    // Carico dai file "tavoli.txt" e "menu.txt"
    caricaTavoli();
    caricaMenu();

    /* --- Inizio ---*/

    // Stampo a video il "benvenuto" del server
    printf(BENVENUTO_SERVER);
    fflush(stdout);

    struct sockaddr_in my_addr, cl_addr;
    int ret, newfd, listener, addrlen, i;
    char buffer[BUFFER_SIZE];
    char bufferOut[BUFFER_SIZE];
    int portNumber = atoi(argv[1]);

    /* Creazione indirizzo del server */
    listener = socket(AF_INET, SOCK_STREAM, 0);

    /* Creazione indirizzo di bind */
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(portNumber);
    my_addr.sin_addr.s_addr = INADDR_ANY;

    /* Aggancio */
    ret = bind(listener, (struct sockaddr*)&my_addr, sizeof(my_addr));
    if(ret < 0) {
        perror("Bind non riuscita\n");
        exit(0);
        return -1;
    }

    /* Apro la coda */
    listen(listener, 10);

    /* Reset dei descrittori */
    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    // Aggiungo il socket di ascolto 'listener' e 'stdin' (0) ai socket monitorati
    FD_SET(listener, &master);
    FD_SET(0, &master);

    // Tengo traccia del nuovo fdmax
    fdmax = listener;

    // Ciclo principale
    for(;;) {
        // Imposto il set di socket da monitorare in lettura per la select()
        pthread_mutex_lock(&fd_lock);
        read_fds = master;
        pthread_mutex_unlock(&fd_lock);

        // Mi blocco (potenzialmente) in attesa di descrittori pronti
        struct timeval tv = {0, 0}; // timeout = 0
        pthread_mutex_lock(&fd_lock);
        ret = select(fdmax+1, &read_fds, NULL, NULL, &tv); // metto un timeout poiché modifico master
        pthread_mutex_unlock(&fd_lock);
        if(ret < 0) {
            perror("Errore nella select!");
            exit(-1);
        }

        // Scorro ogni descrittore 'i'
        for(i = 0; i <= fdmax; i++) {
            // Il descrittore 'i' è pronto se la select lo ha lasciato nel set "read_fds"
            if(FD_ISSET(i, &read_fds)) {
                // Ci sono tre casi:
                //   - ho ricevuto un comando (stdin);
                //   - ho ricevuto una nuova richiesta di connessione;
                //   - devo gestire una richiesta da un socket già connesso.
                if(i == 0) { // Primo caso: comando da stdin
                    char *serverCommand;
                    scanf(" %[^\n]", buffer); // Lo inserisco nel buffer e poi lo analizzo
                    serverCommand = strtok(buffer, " ");
                    // Ci sono due casi:
                    //   - stat, con i relativi sotto casi;
                    //   - stop.
                    if(strcmp(serverCommand, "stat") == 0) { // Primo caso: stat
                        // Ci sono diversi casi
                        //   - Nessun parametro, restituisce lo stato di tutte le comande giornaliere;
                        //   - "T<nTavolo>"", restituisce tutte le comande relative al tavolo table relative al pasto in corso;
                        //   - "a", restituisce tutte le comande in attesa;
                        //   - "p", restituisce tutte le comande in preparazione;
                        //   - "s", restituisce tutte le comande in servizio.

                        // Prendo il secondo termine del comando
                        serverCommand = strtok(NULL, " ");

                        if(serverCommand == NULL) {
                            // Non necessario, quindi:
                            printf("Comando 'stat' senza parametri inesistente!\n");
                            fflush(stdout);
                        }
                        else if(strncmp(serverCommand, "T", 1) == 0) { // Chiede lo stato di un tavolo
                            // Cerco il numero del tavolo e stampo l'esito
                            serverCommand++; // Escludo la T per l'atoi
                            int tavolo = atoi(serverCommand);

                            if(tavolo > nMaxTd || tavolo == 0) {
                                printf("Tavolo inesistente!\n");
                                fflush(stdout);
                                break;
                            }
                            elencoComandeTavolo(bufferOut, tavolo);

                            printf(bufferOut);
                            fflush(stdout);
                        }
                        else if(strcmp(serverCommand, "a") == 0) { // Chiedo le comande in attesa
                            // Scorro tutto l'array di liste, nel caso sia in attesa, la aggiungo al buffer
                            elencoComande(bufferOut, in_attesa);

                            printf(bufferOut);
                            fflush(stdout);
                        }
                        else if(strcmp(serverCommand, "p") == 0) { // Chiedo le comande in preparazione
                            // Scorro tutto l'array di liste, nel caso sia in preparazione, la aggiungo al buffer
                            elencoComande(bufferOut, in_preparazione);

                            printf(bufferOut);
                            fflush(stdout);
                        }
                        else if(strcmp(serverCommand, "s") == 0) { // Chiedo le comande in servizio
                            // Scorro tutto l'array di liste, nel caso sia in servizio, la aggiungo al buffer
                            elencoComande(bufferOut, in_servizio);

                            printf(bufferOut);
                            fflush(stdout);
                        }
                        else { // Comando non riconosciuto
                            printf("Comando 'stat' con elementi non riconosciuti!\n");
                            fflush(stdout);
                        }
                    }
                    else if(strcmp(serverCommand, "stop") == 0) { // Secondo caso: stop
                        // Se posso stopparmi, mando una notifica a tutti i dispositivi connessi e mi interrompo.
                        // Nel caso io non possa fermarmi (ci sono delle comande in attesa o preparazione), lo comunico e non faccio niente.
                        if(!comandeInSospeso()) { // Mi posso fermare
                            int j;
                            // Comunico l'esecuzione del comando
                            printf("Comunico a tutti la chiusura del server e lo termino.\n");
                            fflush(stdout);

                            // Invio ad ogni dispositivo connesso il messaggio "STOP"
                            strcpy(bufferOut,"STOP\0");
                            for(j = 1; j <= fdmax; j++) {
                                if(j == listener) continue; // Salta il listener
                                if(!FD_ISSET(j, &master)) continue; // Nel caso lo abbia già chiuso, non scrivo
                                ret = invia(j, bufferOut);
                                printf("Comunicata chiusura al sd %d\n", j);
                                fflush(stdout);
                                if(ret < 0) {
                                    perror("Errore invio chiusura server\n");
                                    exit(1);
                                }
                                close(j);
                                pthread_mutex_lock(&fd_lock);
                                FD_CLR(j, &master);
                                pthread_mutex_unlock(&fd_lock);
                            }
                            close(listener);

                            deallocaStrutture();

                            // Termino il server con esito positivo
                            printf("Server chiuso\n");
                            fflush(stdout);
                            return 1;
                        }
                        else { // Non posso fermarmi
                            printf("Sono presenti delle comande in preparazione e in attesa!\nNon è possibile fermare il server!\n");
                            fflush(stdout);
                        }
                    }
                    else { // Comando da stdin non riconosciuto
                        printf("Comando non riconosciuto!\n");
                        fflush(stdout);
                    }
                }
                else if(i == listener) { // Secondo caso: nuova richiesta di connessione
                    // Mi preparo ad accettare la nuova connessione
                    addrlen = sizeof(cl_addr);
                    newfd = accept(listener, (struct sockaddr*)&cl_addr, &addrlen);

                    // Messaggio di errore
                    if(newfd < 0) {
                        perror("Errore accettazione nuova connessione\n");
                    }
                    else {
                        // Lo aggiungo ai descrittori monitorati
                        pthread_mutex_lock(&fd_lock);
                        FD_SET(newfd, &master);
                        pthread_mutex_unlock(&fd_lock);

                        // Aggiorno fdmax
                        if(newfd > fdmax) fdmax = newfd;

                        printf("Nuova connessione da %s sul socket %d\n", inet_ntoa(cl_addr.sin_addr), newfd);
                        fflush(stdout);
                    }
                }
                else { // Terzo caso: richiesta su un socket connesso
                    // Leggo il messaggio
                    ret = ricevi(i, buffer);
                    if(ret <= 0) {
                        // Il client ha chiuso la connessione
                        if(ret == 0) {
                            printf("Socket %d ha chiuso la connessione\n", i);
                            fflush(stdout);
                        }
                        else {
                            perror("Errore ricezione messaggio\n");
                        }
                        close(i);
                        pthread_mutex_lock(&fd_lock);
                        FD_CLR(i, &master);
                        pthread_mutex_unlock(&fd_lock);
                    }
                    else {
                        // Devo interpretare il messaggio ricevuto
                        interpreteMessaggio(buffer, bufferOut, i);
                        // e lo reinoltro
                        ret = invia(i, bufferOut);
                        if(ret <= 0) {
                            // Il client ha chiuso la connessione
                            if(ret == 0) {
                                printf("Socket %d ha chiuso la connessione\n", i);
                                fflush(stdout);
                            }
                            else {
                                perror("Errore invio messaggio\n");
                            }
                            close(i);
                            pthread_mutex_lock(&fd_lock);
                            FD_CLR(i, &master);
                            pthread_mutex_unlock(&fd_lock);
                        }
                    }
                }
            }
        }
    }

    return 0;
}
