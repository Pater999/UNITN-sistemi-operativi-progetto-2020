#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>

// Testo del menù del programma Q
#define HELP_COMANDI \
    "\n\033[1;35m-------------------------- Comandi disponibili M ---------------------------\033[0m\n\
\033[1;35m| \033[0;32m premi <numero_interruttore>\033[0m           Accendi o spegni l'interruttore   \033[1;35m|\n\
\033[1;35m| \033[0;32m premi <numero_pulsante> <secondi>\033[0m     Premi un pulsante per tot secondi \033[1;35m|\n\
\033[1;35m| \033[0;34m status\033[0m                                Mostra lo stato di led e pulsanti \033[1;35m|\n\
\033[1;35m| \033[0;34m list\033[0m                                  Mostra la lista dei processi P    \033[1;35m|\n\
\033[1;35m| \033[0;33m help\033[0m                                  Mostra i comandi disponibili      \033[1;35m|\n\
\033[1;35m| \033[0;31m quit\033[0m                                  Esci dal programma                \033[1;35m|\n\
\033[1;35m----------------------------------------------------------------------------\033[0m\n\n"

// Struct che rappresenta un processo P
struct processoP
{
    int pid;
    int id;
};

// Struct che permette di gestire la coda dei processi P
struct dynamicArray
{
    struct processoP *processiPList;
    int sizeArray;
    int currentIndex;
    int counterId;
    int processoSelezionato;
};

// Lista dinamica contente i processi P
struct dynamicArray lista;

// Inizializza la lista dei processiP
void inizializzaLista();

// Rimuove un elemento dalla lista dei processiP
int listDelete(int pos);

// Aggiunge un elemento alla lista dei processiP
int listAdd(int pid, int id);

// Metodo per gestire il Menu (input con utente - comandi)
void checkInputUtente();

// Metodo di comodo per creazione pipe
void creaPipe(int pipeIn[2], int flags);

// Metodo per creare i processi S
void creaProcessoS(int i);

// Metodo di comodo per l'uscita dal programma
void esci(int codiceUscita);

// Handler per il segnale Segmentation Fault (UTILE PER DEBUG)
void sigSegmentation(int signum);

// Handler per il segnale SIGTERM
void sigTermHandler(int signum);

// Si occupa della gestione della coda dei processi P una volta ricevuta la conferma di pulsante/interruttore premuto
void gestisciConfermaPulsanti(int T);

// Ricrea tutti i processi in caso di un eliminazione in testa
void ricreaProcessoP0(int identificatore);

// Metodo di comodo per l'eliminazione in testa (Processi P)
void eliminazioneInTesta();

// Metodo di comodo per l'eliminazione processo selezionato (Processi P)
void eliminazioneSelezionato();

// Metodo di comodo per la creazione dei primo processo P all'avvio del programma
void creaProcessoP0();
