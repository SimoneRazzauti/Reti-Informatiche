int main(){
    int sockfd, ret;

    // Creazione del socket tramite funzione SOCKET
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        perror("ERRORE: Impossibile creare un nuovo socket");
        exit(1);
    }


    return 0;
}