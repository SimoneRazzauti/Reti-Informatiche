int main(){
    int sockfd, ret;
    struct sockaddr_in server_addr, client_addr;
    // Creazione del socket tramite funzione SOCKET
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        perror("ERRORE: Impossibile creare un nuovo socket");
        exit(1);
    }  

    // BIND sul nuovo socket creato
    if(bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        perror("ERRORE: binding del socket errata");
        exit(1);
    }

    // LISTEN sul nuovo socket creato

}