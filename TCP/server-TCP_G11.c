#include <stdio.h>      // Libreria standard per l'input/output, usata per funzioni come printf e perror
#include <stdlib.h>     // Libreria standard per funzioni di utilità generale, come atoi ed exit
#include <string.h>     // Libreria per la manipolazione di stringhe, usata per bzero e strcpy
#include <unistd.h>     // Fornisce accesso alle API POSIX, qui usata per read, write e close
#include <arpa/inet.h>  // Definisce la struttura sockaddr_in e le funzioni di manipolazione degli indirizzi IP come htons

// Funzione per la gestione degli errori. Stampa un messaggio di errore e termina il programma.
void error(const char *msg) {
    perror(msg); // Stampa il messaggio di errore personalizzato seguito da una descrizione dell'errore di sistema
    exit(1);     // Termina il programma con un codice di stato che indica un errore
}

int main(int argc, char *argv[]) {
    // Verifica che sia stato fornito il numero di porta come argomento da riga di comando
    if (argc < 2) {
        fprintf(stderr, "Errore: porta non fornita\n"); // Stampa un messaggio di errore sullo standard error
        exit(1);                                       // Termina se la porta non è specificata
    }

    int sockfd, newsockfd, portno;              // Descrittori per i socket e variabile per la porta
    struct sockaddr_in serv_addr, cli_addr;     // Strutture per l'indirizzo del server e del client
    socklen_t clilen;                           // Lunghezza della struttura dell'indirizzo del client
    int n;                                      // Variabile per i valori di ritorno delle chiamate di sistema (read/write)

    // 1. Creazione del socket TCP
    // AF_INET indica che useremo indirizzi IPv4.
    // SOCK_STREAM specifica che il socket sarà di tipo TCP (orientato alla connessione).
    // Il terzo argomento, 0, lascia al sistema la scelta del protocollo appropriato (TCP in questo caso).
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("ERRORE apertura socket"); // Se la creazione fallisce, chiama la funzione di errore
    }

    // 2. Setup dell'indirizzo del server
    bzero((char *) &serv_addr, sizeof(serv_addr)); // Inizializza la struttura dell'indirizzo a zero
    portno = atoi(argv[1]);                        // Converte il numero di porta da stringa a intero

    serv_addr.sin_family = AF_INET;                // Imposta la famiglia di indirizzi a IPv4
    serv_addr.sin_addr.s_addr = INADDR_ANY;        // Accetta connessioni da qualsiasi interfaccia di rete del server
    serv_addr.sin_port = htons(portno);            // Converte il numero di porta in network byte order (big-endian)

    // 3. Binding del socket all'indirizzo e alla porta specificati
    // Associa il socket 'sockfd' all'indirizzo 'serv_addr'.
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        error("ERRORE in binding"); // Se il binding fallisce (es. porta già in uso), chiama la funzione di errore
    }

    // 4. Mette il server in ascolto di connessioni in ingresso
    // Il secondo argomento, 5, è la dimensione della coda di connessioni in attesa.
    listen(sockfd, 5);
    printf("Server TCP avviato sulla porta %d...\n", portno);

    // Ciclo infinito per accettare e gestire le connessioni dei client (server iterativo)
    while (1) {
        clilen = sizeof(cli_addr); // Imposta la dimensione della struttura dell'indirizzo del client

        // 5. Accetta una nuova connessione
        // Estrae la prima richiesta di connessione dalla coda e crea un nuovo socket per comunicare con il client.
        // La chiamata è bloccante: il programma si ferma qui finché un client non si connette.
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) {
            error("ERRORE in accept"); // Se l'accettazione fallisce, chiama la funzione di errore
        }

        // 6. Invia un messaggio di conferma al client
        // Informa il client che la connessione è stata stabilita con successo.
        n = write(newsockfd, "connessione avvenuta", 20);
        if (n < 0) {
            error("ERRORE scrittura socket"); // Gestisce errori di scrittura sul socket
        }

        // 7. Riceve il comando dal client
        char command;
        // Legge un singolo byte (il carattere del comando) dal socket del client.
        n = read(newsockfd, &command, 1);
        if (n < 0) {
            error("ERRORE lettura socket"); // Gestisce errori di lettura dal socket
        }

        char response_msg[100]; // Buffer per il messaggio di risposta
        int valid_op = 1;       // Flag per indicare se l'operazione richiesta è valida

        // 8. Interpreta il comando e prepara una risposta
        switch(command) {
            case 'A': case 'a': strcpy(response_msg, "ADDIZIONE"); break;
            case 'S': case 's': strcpy(response_msg, "SOTTRAZIONE"); break;
            case 'M': case 'm': strcpy(response_msg, "MOLTIPLICAZIONE"); break;
            case 'D': case 'd': strcpy(response_msg, "DIVISIONE"); break;
            default:
                // Se il comando non è uno dei precedenti, il client verrà terminato
                strcpy(response_msg, "TERMINE PROCESSO CLIENT");
                valid_op = 0; // Imposta il flag a 0 per indicare un'operazione non valida
        }

        // Invia la risposta testuale (es. "ADDIZIONE" o "TERMINE PROCESSO CLIENT")
        n = write(newsockfd, response_msg, strlen(response_msg) + 1); // +1 per includere il terminatore nullo

        // Se l'operazione è valida, procede a ricevere i numeri ed eseguire il calcolo
        if (valid_op) {
            int numbers[2];
            // 9. Riceve i due interi dal client
            // Legge 2 * sizeof(int) byte dal socket, che corrispondono ai due numeri.
            n = read(newsockfd, numbers, sizeof(int) * 2);
            if (n < 0) {
                error("ERRORE lettura interi"); // Gestisce errori di lettura
            }

            // Esegue l'operazione richiesta
            int result = 0;
            if (command == 'A' || command == 'a') result = numbers[0] + numbers[1];
            if (command == 'S' || command == 's') result = numbers[0] - numbers[1];
            if (command == 'M' || command == 'm') result = numbers[0] * numbers[1];
            if (command == 'D' || command == 'd') {
                // Controlla la divisione per zero
                if(numbers[1] != 0) {
                    result = numbers[0] / numbers[1];
                } else {
                    result = 0; // In caso di divisione per zero, il risultato è 0
                }
            }

            // 10. Invia il risultato del calcolo al client
            n = write(newsockfd, &result, sizeof(int));
            if (n < 0) {
                error("ERRORE scrittura risultato");
            }
        }

        // 11. Chiude la connessione con il client corrente
        close(newsockfd);
    }

    // 12. Chiude il socket di ascolto (questa parte di codice non viene mai raggiunta a causa del ciclo infinito)
    close(sockfd);
    return 0; // Termina il programma (non raggiungibile)
}