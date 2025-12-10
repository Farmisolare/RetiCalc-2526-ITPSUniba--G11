#include <stdio.h>      // Libreria standard per l'input/output (printf, perror)
#include <stdlib.h>     // Libreria per funzioni di utilità generale (atoi, exit)
#include <string.h>     // Libreria per la manipolazione di stringhe (bzero, strcpy, strlen)
#include <unistd.h>     // Fornisce accesso alle API POSIX (non usata direttamente qui ma buona norma)
#include <arpa/inet.h>  // Definizioni per le operazioni su indirizzi Internet (sockaddr_in, htons)

// Funzione per la gestione degli errori. Stampa un messaggio e termina il programma.
void error(const char *msg) {
    perror(msg); // Stampa il messaggio di errore personalizzato e la descrizione dell'errore di sistema
    exit(1);     // Termina il programma con un codice di stato di errore
}

int main(int argc, char *argv[]) {
    // Verifica che sia stata fornita la porta come argomento
    if (argc < 2) {
        fprintf(stderr, "Errore: porta non fornita\n");
        exit(1);
    }

    int sockfd, portno, n;                  // Descrittore del socket, porta, valore di ritorno delle funzioni
    struct sockaddr_in serv_addr, cli_addr; // Strutture per l'indirizzo del server e del client
    socklen_t clilen;                       // Lunghezza della struttura dell'indirizzo del client
    char buffer[256];                       // Buffer generico (usato per il primo handshake)

    // 1. Creazione del socket UDP
    // AF_INET per indirizzi IPv4.
    // SOCK_DGRAM specifica che il socket sarà di tipo UDP (datagram, non orientato alla connessione).
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        error("ERRORE apertura socket");
    }

    // 2. Setup dell'indirizzo del server
    bzero((char *) &serv_addr, sizeof(serv_addr)); // Azzera la struttura
    portno = atoi(argv[1]);                        // Converte la porta da stringa a intero

    serv_addr.sin_family = AF_INET;                // Famiglia di indirizzi IPv4
    serv_addr.sin_addr.s_addr = INADDR_ANY;        // Accetta pacchetti da qualsiasi interfaccia di rete
    serv_addr.sin_port = htons(portno);            // Converte la porta in network byte order

    // 3. Binding del socket all'indirizzo e alla porta
    // Associa il socket 'sockfd' all'indirizzo 'serv_addr' per poter ricevere pacchetti su quella porta.
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        error("ERRORE in binding");
    }

    printf("Server UDP avviato sulla porta %d...\n", portno);

    // Ciclo infinito per ricevere e rispondere ai datagrammi dei client
    while (1) {
        clilen = sizeof(cli_addr); // Imposta la dimensione della struttura dell'indirizzo del client

        // 4. Handshake simulato: riceve il primo messaggio dal client
        // recvfrom è una chiamata bloccante: attende l'arrivo di un datagramma.
        // Salva l'indirizzo del mittente in 'cli_addr' per poter rispondere.
        n = recvfrom(sockfd, buffer, 255, 0, (struct sockaddr *) &cli_addr, &clilen);
        if (n < 0) {
            error("ERRORE in recvfrom (handshake)");
        }
        buffer[n] = '\0'; // Aggiunge il terminatore di stringa per sicurezza
        printf("Handshake ricevuto da un client.\n");

        // 5. Invia la conferma di "connessione" (handshake) al client
        char *msg_conn = "connessione avvenuta";
        // Usa sendto per inviare la risposta all'indirizzo del client memorizzato in 'cli_addr'.
        n = sendto(sockfd, msg_conn, strlen(msg_conn) + 1, 0, (struct sockaddr *) &cli_addr, clilen);
        if (n < 0) {
            error("ERRORE in sendto (handshake)");
        }

        // 6. Riceve il comando dal client
        char command;
        n = recvfrom(sockfd, &command, 1, 0, (struct sockaddr *) &cli_addr, &clilen);
        if (n < 0) {
            error("ERRORE in recvfrom (comando)");
        }

        char response_msg[100]; // Buffer per il messaggio di risposta testuale
        int valid_op = 1;       // Flag per operazione valida

        // 7. Interpreta il comando e prepara la risposta
        switch(command) {
            case 'A': case 'a': strcpy(response_msg, "ADDIZIONE"); break;
            case 'S': case 's': strcpy(response_msg, "SOTTRAZIONE"); break;
            case 'M': case 'm': strcpy(response_msg, "MOLTIPLICAZIONE"); break;
            case 'D': case 'd': strcpy(response_msg, "DIVISIONE"); break;
            default:
                strcpy(response_msg, "TERMINE PROCESSO CLIENT");
                valid_op = 0; // Comando non valido
        }

        // Invia la risposta testuale al client
        n = sendto(sockfd, response_msg, strlen(response_msg) + 1, 0, (struct sockaddr *) &cli_addr, clilen);
        if (n < 0) {
            error("ERRORE in sendto (risposta comando)");
        }

        // Se l'operazione è valida, procede a ricevere i numeri e a calcolare
        if (valid_op) {
            int numbers[2];
            // 8. Riceve i due interi dal client
            n = recvfrom(sockfd, numbers, sizeof(int) * 2, 0, (struct sockaddr *) &cli_addr, &clilen);
            if (n < 0) {
                error("ERRORE in recvfrom (numeri)");
            }

            // Esegue l'operazione
            int result = 0;
            if (command == 'A' || command == 'a') result = numbers[0] + numbers[1];
            if (command == 'S' || command == 's') result = numbers[0] - numbers[1];
            if (command == 'M' || command == 'm') result = numbers[0] * numbers[1];
            if (command == 'D' || command == 'd') {
                // Controllo divisione per zero
                if (numbers[1] != 0) {
                    result = numbers[0] / numbers[1];
                } else {
                    result = 0; // Risultato in caso di divisione per zero
                }
            }

            // 9. Invia il risultato del calcolo al client
            n = sendto(sockfd, &result, sizeof(int), 0, (struct sockaddr *) &cli_addr, clilen);
            if (n < 0) {
                error("ERRORE in sendto (risultato)");
            }
        }
        // A differenza di TCP, non c'è una connessione da chiudere per ogni client.
        // Il server resta semplicemente in attesa del prossimo datagramma.
    }

    close(sockfd); // Questa parte non è raggiungibile a causa del ciclo infinito
    return 0;
}