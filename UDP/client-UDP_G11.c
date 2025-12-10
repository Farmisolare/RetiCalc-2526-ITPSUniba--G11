#include <stdio.h>      // Libreria standard per l'input/output (printf, scanf, perror)
#include <stdlib.h>     // Libreria per funzioni di utilità generale (atoi, exit)
#include <string.h>     // Libreria per la manipolazione di stringhe (bzero, bcopy, strlen)
#include <unistd.h>     // Fornisce accesso alle API POSIX (close)
#include <netdb.h>      // Definizioni per le operazioni di network database (gethostbyname)
#include <arpa/inet.h>  // Definizioni per le operazioni su indirizzi Internet (sockaddr_in, htons)

// Funzione per la gestione degli errori. Stampa un messaggio e termina il programma.
void error(const char *msg) {
    perror(msg); // Stampa il messaggio di errore personalizzato e la descrizione dell'errore di sistema
    exit(0);     // Termina il programma
}

int main(int argc, char *argv[]) {
    // Controlla che siano stati forniti hostname e porta del server come argomenti
    if (argc < 3) {
        fprintf(stderr, "Uso: %s hostname porta\n", argv[0]);
        exit(0);
    }

    int sockfd, portno, n;                  // Descrittore socket, porta, valore di ritorno
    unsigned int length;                    // Lunghezza della struttura dell'indirizzo (necessaria per recvfrom)
    struct sockaddr_in serv_addr, from;     // Strutture per l'indirizzo del server e per l'indirizzo del mittente
    struct hostent *server;                 // Struttura per memorizzare informazioni sull'host
    char buffer[256];                       // Buffer per la comunicazione

    // 1. Creazione del socket UDP
    // AF_INET per indirizzi IPv4, SOCK_DGRAM per UDP.
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        error("ERRORE apertura socket");
    }

    // 2. Risoluzione del nome host e setup dell'indirizzo del server
    server = gethostbyname(argv[1]); // Risolve il nome dell'host in un indirizzo IP
    if (server == NULL) {
        fprintf(stderr, "ERRORE, host non trovato\n");
        exit(0);
    }
    portno = atoi(argv[2]); // Converte la porta da stringa a intero

    bzero((char *) &serv_addr, sizeof(serv_addr)); // Azzera la struttura dell'indirizzo del server
    serv_addr.sin_family = AF_INET;                // Famiglia di indirizzi IPv4
    // Copia l'indirizzo IP del server nella struttura
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno); // Converte la porta in network byte order

    length = sizeof(struct sockaddr_in); // Lunghezza della struttura dell'indirizzo

    // 3. Handshake simulato: invia un primo messaggio per "iniziare" la comunicazione
    char *handshake = "HELLO";
    // Invia il messaggio "HELLO" al server. Con UDP, ogni messaggio deve specificare l'indirizzo di destinazione.
    n = sendto(sockfd, handshake, strlen(handshake), 0, (struct sockaddr *) &serv_addr, length);
    if (n < 0) {
        error("ERRORE in sendto (handshake)");
    }

    // 4. Riceve la conferma di "connessione" dal server
    bzero(buffer, 256); // Pulisce il buffer
    // Attende di ricevere un messaggio. L'indirizzo del mittente verrà salvato in 'from'.
    n = recvfrom(sockfd, buffer, 255, 0, (struct sockaddr *) &from, &length);
    if (n < 0) {
        error("ERRORE in recvfrom (conferma)");
    }
    printf("Server: %s\n", buffer); // Stampa il messaggio ricevuto

    // 5. Chiede all'utente di inserire un comando
    char command;
    printf("Inserisci operazione (A, S, M, D) o altro per terminare: ");
    scanf(" %c", &command);

    // 6. Invia il comando al server
    n = sendto(sockfd, &command, 1, 0, (struct sockaddr *) &serv_addr, length);
    if (n < 0) {
        error("ERRORE in sendto (comando)");
    }

    // 7. Riceve la risposta del server al comando
    bzero(buffer, 256);
    n = recvfrom(sockfd, buffer, 255, 0, (struct sockaddr *) &from, &length);
    if (n < 0) {
        error("ERRORE in recvfrom (risposta comando)");
    }
    printf("Stato Server: %s\n", buffer);

    // Controlla se il server ha indicato la terminazione del processo
    if (strstr(buffer, "TERMINE") != NULL) {
        printf("Processo terminato come richiesto.\n");
        close(sockfd);
        return 0;
    }

    // Se il comando è valido, chiede i numeri all'utente
    int numbers[2];
    printf("Inserisci primo intero: ");
    scanf("%d", &numbers[0]);
    printf("Inserisci secondo intero: ");
    scanf("%d", &numbers[1]);

    // 8. Invia i due interi al server
    n = sendto(sockfd, numbers, sizeof(int) * 2, 0, (struct sockaddr *) &serv_addr, length);
    if (n < 0) {
        error("ERRORE in sendto (numeri)");
    }

    // 9. Riceve il risultato del calcolo
    int result;
    n = recvfrom(sockfd, &result, sizeof(int), 0, (struct sockaddr *) &from, &length);
    if (n < 0) {
        error("ERRORE in recvfrom (risultato)");
    }

    printf("Risultato ricevuto dal server: %d\n", result);

    // 10. Chiude il socket
    close(sockfd);
    return 0;
}