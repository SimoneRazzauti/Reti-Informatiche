/* Contiene le funzioni necessarie al Server */
/* ----------------------------------------- */

#include "strutture.h"
#include "funzioni.h"

// Carica i tavoli dal file e li mette nell'array
void caricaTavoli() {
	FILE *f;
	char buffer[BUFFER_SIZE];
	char* serverCommand;
	int i;
	f = fopen("../txts/tavoli.txt","r");
	
	if (f == NULL) {
       printf("Errore! Apertura file tavoli.txt non riuscita\n");
       exit(-1);
	}

	for(i = 0; i < nTavoli; i++) {
		fgets(buffer, sizeof(buffer), f);
		serverCommand = strtok(buffer, " ");
		tavoli[i].nPosti = atoi(serverCommand);
		serverCommand = strtok(NULL, " ");
		strcpy(tavoli[i].sala, serverCommand);
		serverCommand = strtok(NULL, " ");
		strcpy(tavoli[i].descrizione, serverCommand);

		tavoli[i].numero = i+1;
	}

	fclose(f);
}

// Carica il menu dal file e lo mette nell'array
void caricaMenu() {
	FILE *f;
	char buffer[BUFFER_SIZE];
	char *serverCommand;
	int i;
	f = fopen("../txts/menu.txt","r");
	
	if (f == NULL) {
       printf("Errore! Apertura file menu.txt non riuscita\n");
       exit(-1);
	}

	for(i = 0; i < nPiatti; i++) {
		fgets(buffer, sizeof(buffer), f);
		strcat(menu_text, buffer);
		serverCommand = strtok(buffer, " ");
		strcpy(menu[i].codice, serverCommand);
		serverCommand = strtok(NULL, "€");
		serverCommand = strtok(NULL, "€");
		menu[i].prezzo = atoi(serverCommand);
	}

	strcat(menu_text, "\n");

	fclose(f);
}

// Ritorna 1 nel caso ci siano attualmente delle comande
// "in_preparazione" o "in_servizio"; 0 altrimenti
int comandeInSospeso() {
	int i, ret = 0;
	fflush(stdout);
	for (i = 0; i < nTavoli; i++) {
		struct comanda *c = comande[i];
		while(c != NULL) {
			if(c->stato == in_attesa || c->stato == in_preparazione)
				ret = 1;
			c = c->prossima;
		}
	}
	return ret;
}

// Invia al socket in input il messaggio dentro buffer
int invia(int j, char* buffer) {
	int len = strlen(buffer)+1;
	int lmsg = htons(len);
	int ret;

	// Invio la dimensione del messaggio
	ret = send(j, (void*) &lmsg, sizeof(uint16_t), 0);
	// Invio il messaggio
	ret = send(j, (void*) buffer, len, 0);

	// Comunico l'esito
	return ret;
}

// Ricevi dal socket in input la lunghezza del messaggio e lo mette dentro lmsg
int riceviLunghezza(int j, int *lmsg) {
	int ret;
	ret = recv(j, (void*)lmsg, sizeof(uint16_t), 0);
	return ret;
}

// Riceve dal socket in input il messaggio e lo mette dentro buffer
int ricevi(int j, int lunghezza, char* buffer) {
	int ret;
	ret = recv(j, (void*)buffer, lunghezza, 0);
	return ret;
}

// Prende uno stato_comanda e inserisce dentro il buffer tutte le informazioni
// delle comande in quello stato di qualunque tavolino
void elencoComande(char* buffer, enum stato_comanda stato) {
    char numeroString[BUFFER_SIZE];
    int i, j;
    strcpy(buffer, "\0");
    for (i = 0; i < nTavoli; i++) {
        struct comanda *c;
        c = comande[i];
        while (c != NULL) {
            if (c->stato == stato) {
                strcat(buffer, "com");
                sprintf(numeroString, "%d", c->nComanda);
                strcat(buffer, numeroString);
                strcat(buffer, " T");
                sprintf(numeroString, "%d", i + 1);
                strcat(buffer, numeroString);
                strcat(buffer, "\n");
                for (j = 0; j < nPiatti; j++) {
                    if (c->quantita[j] != 0) {
                        strcat(buffer, menu[j].codice);
                        strcat(buffer, " ");
                        sprintf(numeroString, "%d", c->quantita[j]);
                        strcat(buffer, numeroString);
                        strcat(buffer, "\n");
                    }
                }
            }
            // Vado avanti
            c = c->prossima;
        }
    }
}


// Prende un tavolo e inserisce dentro il buffer tutte le informazioni
// delle comande inerenti a quel tavolino
void elencoComandeTavolo(char* buffer, int tavolo) {
    char numeroString[BUFFER_SIZE];
    int i;
    tavolo--;
    strcpy(buffer, "\0");

    struct comanda *c;
    c = comande[tavolo];
    while (c != NULL) {
        strcat(buffer, "com");
        sprintf(numeroString, "%d", c->nComanda);
        strcat(buffer, numeroString);
        strcat(buffer, " ");
        switch (c->stato) {
            case in_attesa:
                strcpy(numeroString, "<in attesa>");
                break;
            case in_preparazione:
                strcpy(numeroString, "<in preparazione>");
                break;
            case in_servizio:
                strcpy(numeroString, "<in servizio>");
                break;
            default:
                strcpy(numeroString, "Sconosciuto");
        }
        strcat(buffer, numeroString);
        strcat(buffer, "\n");
        for (i = 0; i < nPiatti; i++) {
            if (c->quantita[i] != 0) {
                strcat(buffer, menu[i].codice);
                strcat(buffer, " ");
                sprintf(numeroString, "%d", c->quantita[i]);
                strcat(buffer, numeroString);
                strcat(buffer, "\n");
            }
        }
        // Vado avanti
        c = c->prossima;
    }
}


// Inserisce in base alla lettera c, il socket id nell'array relativo
int inserisci(int i, char *c) {
    int j = 0;
    int ret = -1;

    switch (c[0])
    {
    case 'c': // Client
        for (; j < nMaxClient; j++) {
            if (socket_client[j] == -1) {
                socket_client[j] = i;
                ret = 0;
                break;
            }
        }
        break;
    case 'k': // Kitchen device
        for (; j < nMaxKd; j++) {
            if (socket_kd[j] == -1) {
                socket_kd[j] = i;
                ret = 1;
                break;
            }
        }
        break;
    case 't': // Table device
        for (; j < nMaxTd; j++) { // Ipotizzo l'accensione ordinata
            if (socket_td[j] == -1) {
                socket_td[j] = i;
                ret = 2;
                break;
            }
        }
        break;
    
    default:
        ret = -1;
        break;
    }

    return ret;
}


// Prende i parametri della find ed inserisce nel buffer le disponibilità
int cercaDisponibilita(int nPers, char* dataora, char* buffer, char* disponibilita) {
    char numeroString[BUFFER_SIZE];
    int index;
    strcpy(buffer, "\0");

    int numero = 0;
    for (index = 0; index < nTavoli; index++) {
        disponibilita[index] = 0;
        if (tavoli[index].nPosti < nPers)
            continue;
        struct prenotazione* punta = prenotazioni[index];
        char esito = 1; // Non esiste bool
        while (punta != NULL) {
            if (strcmp(punta->data_ora, dataora) == 0) {
                esito = 0;
                break;
            }
            punta = punta->prossima;
        }
        // Tavolo non buono
        if (!esito)
            continue;

        // Tavolo buono
        disponibilita[index] = 1;

        sprintf(numeroString, "%d", numero + 1);
        strcat(buffer, numeroString);
        strcat(buffer, ") T");
        sprintf(numeroString, "%d", index + 1);
        strcat(buffer, numeroString);
        strcat(buffer, " ");
        strcat(buffer, tavoli[index].sala);
        strcat(buffer, " ");
        strcat(buffer, tavoli[index].descrizione);
        strcat(buffer, "\n");
        numero++;
    }
    strcat(buffer, "\n");
    return numero;
}


// Gestisce UNA richiesta da parte di UN client
void *gestisciClient(void *arg) {
    int socketId = *(int *)arg;
    char buffer[BUFFER_SIZE];

    printf("Avviato thread client\n");
    fflush(stdout);

    // Ricevi il messaggio
    int ret, indice;
    int lmsg = 0;
    ret = riceviLunghezza(socketId, &lmsg);
    if (ret == 0) {
        printf("Client disconnesso\n");
        fflush(stdout);
        close(socketId);
        for (indice = 0; indice < nMaxClient; indice++)
            if (socketId == socket_client[indice])
                socket_client[indice] = -1;
        printf("Terminato thread client\n");
        fflush(stdout);
        return NULL;
    }
    ret = ricevi(socketId, lmsg, buffer);
    if (ret == 0) {
        printf("Client disconnesso\n");
        fflush(stdout);
        close(socketId);
        for (indice = 0; indice < nMaxClient; indice++)
            if (socketId == socket_client[indice])
                socket_client[indice] = -1;
        printf("Terminato thread client\n");
        fflush(stdout);
        return NULL;
    }

    // Gestisce i tipi di comandi:
    //   - find, e, di conseguenza, dopo;
    //       - book, e termina il thread;
    //       - disconnessione, cancella il socketId da socket_client e termina il thread.
    char* token;
    token = strtok(buffer, " ");
    if (strcmp(token, "find") == 0) { // Primo caso
        // Parsa la stringa e cerca i tavoli liberi
        char cognome[64];
        int nPers;
        char dataora[12];
        char disponibilita[nTavoli];

        token = strtok(NULL, " ");
        strcpy(cognome, token);

        token = strtok(NULL, " ");
        nPers = atoi(token);

        token = strtok(NULL, " ");
        strcpy(dataora, token);

        // Salvo l'orario che perdevo con l'operazione antecedente
        token = strtok(NULL, " ");
        strcpy(dataora + 8, " ");
        strcpy(dataora + 9, token);

        for (;;) {
            usleep(50000); // 50 ms di pausa, sennò nascono problemi sull'invio
            int massimo;
            // Invia il buffer con le possibilità
            massimo = cercaDisponibilita(nPers, dataora, buffer, disponibilita);
            ret = invia(socketId, buffer);

            printf("Mandate le disponibilità\n");
            fflush(stdout);

            // Aspetta una book o una disconnessione
            ret = riceviLunghezza(socketId, &lmsg);
            if (ret == 0) {
                printf("Client disconnesso\n");
                fflush(stdout);
                close(socketId);
                for (indice = 0; indice < nMaxClient; indice++)
                    if (socketId == socket_client[indice])
                        socket_client[indice] = -1;
                printf("Terminato thread client\n");
                fflush(stdout);
                return NULL;
            }
            ret = ricevi(socketId, lmsg, buffer);
            if (ret == 0) {
                printf("Client disconnesso\n");
                fflush(stdout);
                close(socketId);
                for (indice = 0; indice < nMaxClient; indice++)
                    if (socketId == socket_client[indice])
                        socket_client[indice] = -1;
                printf("Terminato thread client\n");
                fflush(stdout);
                return NULL;
            }

            token = strtok(buffer, " ");

            if (strcmp(token, "book") == 0) { // Caso book
                token = strtok(NULL, " ");

                // Converto l'indice in tavolo
                int tavolo;
                int v = atoi(token);
                if (v > massimo) {
                    printf("Opzione non disponibile\n");
                    strcpy(buffer, "Opzione non disponibile\n");
                    invia(socketId, buffer);
                    fflush(stdout);
                    continue;
                }
                for (tavolo = 0; tavolo <= nTavoli && v > 0; tavolo++)
                    if (disponibilita[tavolo] == 1)
                        v--;
                tavolo--;

                printf("Provo a prenotare il tavolo %d per %s\n", tavolo + 1, dataora);
                fflush(stdout);

                // Cerco le disponibilità attuali
                cercaDisponibilita(nPers, dataora, buffer, disponibilita);

                if (disponibilita[tavolo] == 0) {
                    // Caso in cui non sia più disponibile l'opzione
                    printf("Tavolo già prenotato\n");
                    fflush(stdout);
                    strcpy(buffer, "Tavolo già prenotato\n");
                    invia(socketId, buffer);
                    continue;
                }
                // Salvo la prenotazione
                struct prenotazione* p = malloc(sizeof(struct prenotazione));
                if (p == NULL) {
                    printf("Errore di allocazione per la nuova prenotazione\n");
                    fflush(stdout);
                    return NULL;
                }
                strcpy(p->cognome, cognome);
                strcpy(p->data_ora, dataora);
                for (indice = 0; indice < 5; indice++)
                    p->pwd[indice] = 'A' + (rand() % 26);
                p->prossima = NULL;

                // Inserisco in lista prenotazioni
                if (prenotazioni[tavolo] == NULL) {
                    prenotazioni[tavolo] = p;
                }
                else {
                    struct prenotazione* punta = prenotazioni[tavolo];
                    while (punta->prossima != NULL) {
                        punta = punta->prossima;
                    }
                    punta->prossima = p;
                }

                printf("Un client ha effettuato una prenotazione %s\n", prenotazioni[tavolo]->pwd);
                fflush(stdout);

                strcpy(buffer, "PRENOTAZIONE EFFETTUATA\nCodice: ");
                strcat(buffer, p->pwd);
                strcat(buffer, "\n");
                invia(socketId, buffer);
                break;
            }
            else if (strcmp(token, "disconnessione") == 0) {
                printf("Client disconnesso per richiesta di disconnessione\n");
                fflush(stdout);
                close(socketId);
                for (indice = 0; indice < nMaxClient; indice++)
                    if (socketId == socket_client[indice])
                        socket_client[indice] = -1;
                printf("Terminato thread client\n");
                fflush(stdout);
                return NULL;
            }
        }
    }
    else if (strcmp(token, "book") == 0) {
        // Errore, non sono state fatte precedenti find
        strcpy(buffer, "Errore, non sono state fatte precedenti find\n");
        invia(socketId, buffer);
    }
    else {
        // Errore, comando non riconosciuto
        printf("Errore comando Client!\n");
        fflush(stdout);
    }

    printf("Terminato thread client\n");
    fflush(stdout);

    return NULL;
}


// Gestisce UNA richiesta da parte di UN table device
void *gestisciTd(void *arg) {
    int socketId = *(int *)arg;
    char buffer[BUFFER_SIZE];
    char numeroString[BUFFER_SIZE];

    printf("Avviato thread table device\n");
    fflush(stdout);
    
    // Trovo il tavolo collegato al TD
    int tavolo;
    for (tavolo = 0; tavolo < nTavoli; tavolo++)
        if (socketId == socket_td[tavolo])
            break;

    printf("Richiesta da tavolo %d\n", tavoli[tavolo].numero); // Enumerazione non 0-based
    fflush(stdout);

    // Ricevi il messaggio
    int ret, indice;
    int lmsg = 0;
    ret = riceviLunghezza(socketId, &lmsg);
    if (ret == 0) {
        printf("TD disconnesso\n");
        fflush(stdout);
        close(socketId);
        return NULL;
    }
    ret = ricevi(socketId, lmsg, buffer);
    if (ret == 0) {
        printf("TD disconnesso\n");
        fflush(stdout);
        close(socketId);
        return NULL;
    }

    char* token;
    token = strtok(buffer, " ");

    printf("Prima del controllo\n");
    fflush(stdout);

    if (tavoli_logged[tavolo] == 0) { // Se non è loggato
        // Controllo se ho una prenotazione con questo codice:
        //  - Nel caso affermativo, continuo;
        //  - Nel caso negativo, termino il thread.
        struct prenotazione* punta = prenotazioni[tavolo];
        while (punta != NULL) {
            if (strcmp(punta->pwd, token) == 0) {
                invia(socketId, "accesso");
                tavoli_logged[tavolo] = 1;
                break;
            }
            punta = punta->prossima;
        }
        if (punta == NULL) {
            strcpy(buffer, "Codice prenotazione errato o inserito nel tavolo sbagliato\nInserisci il codice prenotazione: ");
            invia(socketId, buffer);
        }
        printf("Terminato thread table device\n");
        fflush(stdout);
        return NULL;
    }

    printf("Passato controllo\n");
    fflush(stdout);

    // In questo caso è loggato

    // Gestisce i tipi di comandi:
    //   - menu;
    //   - comanda;
    //   - conto.
    
    if (strcmp(token, "menu") == 0) { // Primo caso
        // Invio il menu
        strcpy(buffer, menu_text);
        invia(socketId, buffer);
        printf("Invio il menu al tavolo %d\n", tavoli[tavolo].numero); // Enumerazione non 0-based
        fflush(stdout);
    }
    else if (strcmp(token, "comanda") == 0) { // Secondo caso
        int i;

        printf("Nuova comanda dal tavolo %d\n", tavoli[tavolo].numero); // Enumerazione non 0-based
        fflush(stdout);

        // Parso la comanda ed inserisco
        struct comanda* com = malloc(sizeof(struct comanda));
        
        if (com == NULL) {
            printf("Errore di allocazione per la nuova comanda\n");
            fflush(stdout);
            return NULL;
        }
        
        // Simulazione della lettura della comanda
        token = strtok(NULL, " -");
        while (token != NULL) {
            for (i = 0; i < nPiatti && token != NULL; i++) {
                if (strcmp(token, menu[i].codice) == 0) {
                    token = strtok(NULL, " -");
                    com->quantita[i] = atoi(token);
                    token = strtok(NULL, " -");
                    printf("comanda: %s %d\n", menu[i].codice, com->quantita[i]);
                    fflush(stdout);
                }
            }
        }

        com->prossima = NULL;
        com->timestamp = time(NULL);
        com->kd = socketId;
        com->nComanda = numeroComanda++;
        com->stato = in_attesa;

        // Avviso che la comanda è stata ricevuta correttamente
        strcpy(buffer, "COMANDA RICEVUTA\n");
        invia(socketId, buffer);

        // Notifico tutti i KD
        strcpy(buffer, "*\n");
        // Simulazione della notifica
        printf("Notificato kitchen device\n"); // Enumerazione non 0-based
        fflush(stdout);
    }
    else if (strcmp(token, "conto") == 0) { // Terzo caso

        tavoli_logged[tavolo] = 0;

        printf("Conto richiesto dal tavolo %d\n", tavoli[tavolo].numero); // Enumerazione non 0-based
        fflush(stdout);

        // Scorro l'array comande ed invio
        struct comanda* punta = comande[tavolo];
        int totale = 0;
        strcpy(buffer, "\0");
        while (punta != NULL) {
            for (indice = 0; indice < nPiatti; indice++) {
                if (punta->quantita[indice] == 0) 
                    continue;

                strcat(buffer, menu[indice].codice);
                strcat(buffer, " ");
                sprintf(numeroString, "%d", punta->quantita[indice]);
                strcat(buffer, numeroString);
                strcat(buffer, " ");
                sprintf(numeroString, "%d", punta->quantita[indice] * menu[indice].prezzo);
                strcat(buffer, numeroString);
                strcat(buffer, "\n");
                totale += punta->quantita[indice] * menu[indice].prezzo;
            }
            struct comanda* puntaVecchia = punta;
            punta = punta->prossima;
            free(puntaVecchia);
        }
        comande[tavolo] = NULL;
        strcat(buffer, "Totale: ");
        sprintf(numeroString, "%d", totale);
        strcat(buffer, numeroString);
        strcat(buffer, "\nInserisci il codice prenotazione: ");
        invia(socketId, buffer);
    }
    else {
        // Errore, comando non riconosciuto
        printf("Errore comando Table Device!\n");
        fflush(stdout);
    }

    printf("Terminato thread table device\n");
    fflush(stdout);

    return NULL;
}


// Gestisce UNA richiesta da parte di UN kitchen device
void *gestisciKd(void* i) {
	int socketId = *(int *)i;
	char buffer[BUFFER_SIZE];
	char numeroString[BUFFER_SIZE];

	printf("Avviato thread kitchen device\n");
	fflush(stdout);

	// Ricevi il messaggio
	int ret, indice;
	int lmsg = 0;
	ret = riceviLunghezza(socketId, &lmsg);
	if(ret == 0) {
		printf("KD disconnesso\n");
		fflush(stdout);
		close(socketId);
		for(indice = 0; indice < nMaxKd; indice++)
			if(socketId == socket_kd[indice])
				socket_kd[indice] = -1;
		printf("Terminato thread kitchen device\n");
		fflush(stdout);
		return NULL;
	}
	ret = ricevi(socketId, lmsg, buffer);
	if(ret == 0) {
		printf("KD disconnesso\n");
		fflush(stdout);
		close(socketId);
		for(indice = 0; indice < nMaxKd; indice++)
			if(socketId == socket_kd[indice])
				socket_kd[indice] = -1;
		printf("Terminato thread kitchen device\n");
		fflush(stdout);
		return NULL;
	}

	// Gestisce i tipi di comandi:
	//   - take, funzionante solo nel caso siano presenti delle comande in_attesa;
	//   - show, comande in_preparazione presso il kd;
	//   - ready <comanda>, solo se presente tra quelle in_preparazione del kd.
	char* token;
	token = strtok(buffer, " ");
	if(strcmp(token, "take") == 0) { // Primo caso
		int indice;

		// Scorro l'array comande ed invio
		struct comanda* com = NULL;
		int nTav = -1;
		for(indice = 0; indice < nTavoli; indice++) {
			struct comanda *punta = comande[indice];
			while(punta != NULL) {
				if((com == NULL || punta->timestamp < com->timestamp) && punta->stato == in_attesa) {
					com = punta;
					nTav = indice;
				}
				punta = punta->prossima;
			}
		}
		if(nTav == -1) {
			invia(socketId, "Non ci sono comande\n");
		}
		else {
			com->kd = socketId;
			com->stato = in_preparazione;

			strcpy(buffer, "com");
			sprintf(numeroString, "%d", com->nComanda);
			strcat(buffer, numeroString);
			strcat(buffer, "\t");
			strcat(buffer, "T");
			sprintf(numeroString, "%d", nTav+1);
			strcat(buffer, numeroString);
			strcat(buffer, "\n");
			for(indice = 0; indice < nPiatti; indice++) {
				if(com->quantita[indice] != 0) {
					strcat(buffer, menu[indice].codice);
					strcat(buffer, "\t");
					sprintf(numeroString, "%d", com->quantita[indice]);
					strcat(buffer, numeroString);
					strcat(buffer, "\n");
				}
			}
			invia(socketId, buffer);
			strcpy(buffer, "IN PREPARAZIONE\n");
			invia(socket_td[nTav], buffer);
		}
	}
	else if(strcmp(token, "show") == 0) { // Secondo caso
		int indice, indice2;
		// Scorro l'array comande ed invio
		strcpy(buffer, "");
		for(indice = 0; indice < nTavoli; indice++) {
			struct comanda *punta = comande[indice];
			while(punta != NULL) {
				if(punta->kd == socketId && punta->stato == in_preparazione) {
					strcat(buffer, "com");
					sprintf(numeroString, "%d", punta->nComanda);
					strcat(buffer, numeroString);
					strcat(buffer, "\t");
					strcat(buffer, "T");
					sprintf(numeroString, "%d", indice);
					strcat(buffer, numeroString);
					strcat(buffer, "\n");
					for(indice2 = 0; indice2 < nPiatti; indice2++) {
						if(punta->quantita[indice2] != 0) {
							strcat(buffer, menu[indice2].codice);
							strcat(buffer, "\t");
							sprintf(numeroString, "%d", punta->quantita[indice2]);
							strcat(buffer, numeroString);
							strcat(buffer, "\n");
						}
					}
				}
				punta = punta->prossima;
			}
		}
		invia(socketId, buffer);
	}
	else if(strcmp(token, "ready") == 0) { // Terzo caso
		// Parso il comando e notifico il td
		int nCom, nTav;
		token = strtok(NULL, " com-T");
		nCom = atoi(token);
		token = strtok(NULL, " com-T");
		nTav = atoi(token);
		nTav--;

		struct comanda* punta = comande[nTav];
		while(punta != NULL) {
			if(punta->nComanda == nCom) break;
			punta = punta->prossima;
		}
		if(punta != NULL) {
			punta->stato = in_servizio;
			invia(socketId, "COMANDA IN SERVIZIO\n");
			invia(socket_td[nTav], "ORDINAZIONE IN ARRIVO\n");
		}
		else {
			printf("Errore nel trovare la comanda\n");
			fflush(stdout);	
		}
	}
	else {
		// Errore, comando non riconosciuto
		printf("Errore comando Kitchen Device!\n");
		fflush(stdout);
	}
	FD_SET(socketId, &master);
	printf("Terminato thread kitchen device\n");
	fflush(stdout);

	return NULL;
}

// Dealloca tutte le strutture
void deallocaStrutture() {
	int i;
	// Comande
	for(i = 0; i < nTavoli; i++) {
		struct comanda *c = comande[i];
		while(c != NULL) {
			struct comanda *c2 = c->prossima;
			free(c);
			c = c2;
		}
	}				
	// Prenotazioni
	for(i = 0; i < nTavoli; i++) {
		struct prenotazione *p = prenotazioni[i];
		while(p != NULL) {
			struct prenotazione *p2 = p->prossima;
			free(p);
			p = p2;
		}
	}
}

// Trova il massimo tra 3 interi
int max(int v1, int v2, int v3) {
    int max_value = v1;

    if (v2 > max_value) {
        max_value = v2;
    }

    if (v3 > max_value) {
        max_value = v3;
    }

    return max_value;
}