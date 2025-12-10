#include <stdio.h>      // Libreria standard per l'input/output (printf, scanf, perror)
#include <stdlib.h>     // Libreria per funzioni di utilità generale (atoi, exit)
#include <string.h>     // Libreria per la manipolazione di stringhe (bzero, bcopy, strstr)
#include <unistd.h>     // Fornisce accesso alle API POSIX (read, write, close)
#include <netdb.h>      // Definizioni per le operazioni di network database (gethostbyname)
#include <arpa/inet.h>  // Definizioni per le operazioni su indirizzi Internet (htons)

// Funzione per la gestione degli errori. Stampa un messaggio e termina il programma.
void error(const char *msg) {
    perror(msg); // Stampa il messaggio di errore personalizzato e la descrizione dell'errore di sistema
    exit(0);     // Termina il programma
}

int main(int argc, char *argv[]) {
    // Controlla che siano stati forniti hostname e porta del server come argomenti
    if (argc < 3) {
        fprintf(stderr, "Uso: %s hostname porta\n", argv[0]); // Stampa il corretto utilizzo del programma
        exit(0);                                             // Termina se gli argomenti sono insufficienti
    }

    int sockfd, portno, n;                  // Descrittore del socket, numero di porta, variabile per valori di ritorno
    struct sockaddr_in serv_addr;           // Struttura per l'indirizzo del server
    struct hostent *server;                 // Struttura per memorizzare informazioni sull'host (come l'indirizzo IP)
    char buffer[256];                       // Buffer per la comunicazione

    portno = atoi(argv[2]); // Converte il numero di porta da stringa a intero

    // 1. Creazione del socket TCP
    // AF_INET per indirizzi IPv4, SOCK_STREAM per TCP.
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("ERRORE apertura socket"); // Gestisce l'errore se la creazione fallisce
    }

    // 2. Risoluzione del nome host
    // Ottiene le informazioni sul server (incluso l'indirizzo IP) a partire dal suo nome.
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr, "ERRORE, host non trovato\n"); // Stampa errore se l'host non esiste
        exit(0);
    }

    // 3. Setup dell'indirizzo del server a cui connettersi
    bzero((char *) &serv_addr, sizeof(serv_addr)); // Azzera la struttura dell'indirizzo
    serv_addr.sin_family = AF_INET;                // Imposta la famiglia di indirizzi a IPv4

    // Copia l'indirizzo IP del server (ottenuto da gethostbyname) nella struttura serv_addr
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);

    serv_addr.sin_port = htons(portno); // Converte la porta in network byte order

    // 4. Connessione al server
    // Tenta di stabilire una connessione TCP con il server all'indirizzo specificato.
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        error("ERRORE connessione"); // Gestisce l'errore se la connessione fallisce
    }

    // 5. Riceve il messaggio di conferma dal server
    bzero(buffer, 256); // Pulisce il buffer
    n = read(sockfd, buffer, 255); // Legge il messaggio inviato dal server
    if (n < 0) {
        error("ERRORE lettura da socket");
    }
    printf("Server: %s\n", buffer); // Stampa il messaggio di conferma

    // 6. Chiede all'utente di inserire un comando
    char command;
    printf("Inserisci operazione (A, S, M, D) o altro per terminare: ");
    scanf(" %c", &command); // Legge un carattere da standard input (lo spazio prima di %c ignora eventuali whitespace)

    // 7. Invia il comando al server
    n = write(sockfd, &command, 1); // Scrive il singolo carattere del comando sul socket
    if (n < 0) {
        error("ERRORE scrittura su socket");
    }

    // 8. Riceve la risposta del server al comando
    bzero(buffer, 256); // Pulisce il buffer
    n = read(sockfd, buffer, 255); // Legge la risposta (es. "ADDIZIONE" o "TERMINE...")
    if (n < 0) {
        error("ERRORE lettura stato dal socket");
    }
    printf("Stato Server: %s\n", buffer); // Stampa lo stato inviato dal server

    // Controlla se il server ha inviato un messaggio di terminazione
    if (strstr(buffer, "TERMINE") != NULL) {
        printf("Processo terminato come richiesto.\n");
        close(sockfd); // Chiude il socket
        return 0;      // Termina il client
    }

    // Se il comando è valido, chiede i numeri all'utente
    int numbers[2];
    printf("Inserisci primo intero: ");
    scanf("%d", &numbers[0]);
    printf("Inserisci secondo intero: ");
    scanf("%d", &numbers[1]);

    // 9. Invia i due interi al server
    // Scrive sul socket l'array di interi.
    n = write(sockfd, numbers, sizeof(int) * 2);
    if (n < 0) {
        error("ERRORE scrittura interi su socket");
    }

    // 10. Riceve il risultato del calcolo
    int result;
    n = read(sockfd, &result, sizeof(int)); // Legge il risultato intero inviato dal server
    if (n < 0) {
        error("ERRORE lettura risultato da socket");
    }
    printf("Risultato ricevuto dal server: %d\n", result); // Stampa il risultato

    // 11. Chiude la connessione
    close(sockfd);
    return 0; // Termina il programma con successo
}