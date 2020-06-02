#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>

static int numeroS;          // Numero del processo S (S1, S2, S3, S4)
static char *type;           // Variabile che contiene il tipo pulsante ('P') o interruttore ('I')
static volatile int waiting; // Nel caso sia un pulsante è utilizzata per scartare gli input quando il pulsante è già premuto (Può avere valore 0 oppure 1)
static volatile pid_t L;     // Pid del LED
static volatile pid_t T;     // Pid del BOTTONE/PULSANTE CONTROLLATO

static volatile int statoLed;      // Contiene lo stato del led (0 Spento - 1 acceso)
static volatile int statoPulsante; // Contiene lo stato del pulsante/interruttore (0 non premuto - 1 premuto)

static int fdOutputToM;  // Pipe utilizzata in SCRITTURA per inviare messaggi ad M.
static int fdInputFromM; // Pipe utilizzata in LETTURA per ricevere messaggi da M.

// COLORI per il terminale
static const char *colore = "\e[1;37m";
static const char *endColor = "\e[0m";

// Il pulsante/interruttore conferma di essere stato acceso con un segnale
static void confemaDalPulsante(int sig, siginfo_t *info, void *context)
{
    // Handler eseguito solo se il sender del segnale è il processo T
    if (info->si_pid == T)
    {
        char stringa[100];
        int ritorno;
        if (statoPulsante == 0)
            statoPulsante = 1;
        else
            statoPulsante = 0;

        if (statoPulsante == 1)
        {
            if (strcmp(type, "I") == 0)
                snprintf(stringa, sizeof(stringa), "%d|OUT|1|%sInterruttore %d ON.%s\n", numeroS, colore, numeroS, endColor);
            else
                snprintf(stringa, sizeof(stringa), "%d|OUT|1|%sPulsante %d ON.%s\n", numeroS, colore, numeroS, endColor);
        }
        else
        {
            if (strcmp(type, "I") == 0)
                snprintf(stringa, sizeof(stringa), "%d|OUT|0|%sInterruttore %d OFF.%s\n", numeroS, colore, numeroS, endColor);
            else
                snprintf(stringa, sizeof(stringa), "%d|OUT|0|%sPulsante %d OFF.%s\n", numeroS, colore, numeroS, endColor);
        }
        write(fdOutputToM, stringa, strlen(stringa) + 1);

        kill(L, SIGUSR1);
    }
}

// Il led conferma di essere stato acceso con un segnale
static void confemaDalLed(int sig, siginfo_t *info, void *context)
{
    // Handler eseguito solo se il sender del segnale è il processo L
    if (info->si_pid == L)
    {
        char stringa[100];
        if (statoLed == 0)
            statoLed = 1;
        else
            statoLed = 0;

        if (statoLed == 1)
        {
            snprintf(stringa, sizeof(stringa), "%d|OUT|0|%sLed %d ON.%s\n", numeroS, colore, numeroS, endColor);
        }
        else
        {
            snprintf(stringa, sizeof(stringa), "%d|OUT|0|%sLed %d OFF.%s\n", numeroS, colore, numeroS, endColor);
        }
        write(fdOutputToM, stringa, strlen(stringa) + 1);
    }
}

// Invia lo stato di L e di T ad M
void inviaStatus()
{
    char stringaStatus[200];

    if (statoPulsante == 1)
    {
        if (strcmp(type, "I") == 0)
            snprintf(stringaStatus, sizeof(stringaStatus), "%d|OUT|0|%sInterruttore %d ON.%s\n", numeroS, colore, numeroS, endColor);
        else
            snprintf(stringaStatus, sizeof(stringaStatus), "%d|OUT|0|%sPulsante %d ON.%s\n", numeroS, colore, numeroS, endColor);
    }
    else
    {
        if (strcmp(type, "I") == 0)
            snprintf(stringaStatus, sizeof(stringaStatus), "%d|OUT|0|%sInterruttore %d OFF.%s\n", numeroS, colore, numeroS, endColor);
        else
            snprintf(stringaStatus, sizeof(stringaStatus), "%d|OUT|0|%sPulsante %d OFF.%s\n", numeroS, colore, numeroS, endColor);
    }

    char stringaAppoggio[100];
    if (statoLed == 1)
    {
        snprintf(stringaAppoggio, sizeof(stringaAppoggio), "%sLed %d ON.%s\n", colore, numeroS, endColor);
    }
    else
    {
        snprintf(stringaAppoggio, sizeof(stringaAppoggio), "%sLed %d OFF.%s\n", colore, numeroS, endColor);
    }
    strcat(stringaStatus, stringaAppoggio);

    write(fdOutputToM, stringaStatus, strlen(stringaStatus) + 1);
}

// Spegne il pulsante una volta terminati i secondi ricevuti dall'utente
static void spegniPulsante(int sig, siginfo_t *info, void *context)
{
    // Handler eseguito solo se il sender è il processo stesso
    if (info->si_pid == 0)
    {
        kill(T, SIGUSR1);
        waiting = 0;
    }
}

// Metodo per creare il processo figlio L (led)
void collegaLed(char *numeroLed)
{
    L = fork();
    if (L == -1)
    {
        fprintf(stderr, "Errore. Il forking del led è fallito!\n");
        exit(1);
    }
    else if (L == 0)
    {
        if (execl("./bin/L", "L", numeroLed, NULL) == -1)
        {
            fprintf(stderr, "Errore l'immagine del processo L%d non può essere sostituita.\n", numeroS);
            exit(1);
        }
    }
}

// Metodo per creare il processo figlio T (pulsante/interruttore)
void collegaPulsante(char *numeroPulsante)
{
    T = fork();
    if (T == -1)
    {
        fprintf(stderr, "Errore. Il forking del pulsante/interruttore è fallito!\n");
        exit(1);
    }
    else if (T == 0)
    {
        if (execl("./bin/T", "P", type, numeroPulsante, NULL) == -1)
        {
            fprintf(stderr, "Errore l'immagine del processo T%d non può essere sostituita.\n", numeroS);
            exit(1);
        }
    }
}

int main(int argc, char *argv[])
{
    // Controllo che gli argomenti passati siano del numero corretto.
    if (getppid() != 1)
    {
        // Chiudo i versi delle pipe non utilizzati
        close(atoi(argv[5]));
        close(atoi(argv[6]));

        // Setto variabili che mantengono lo stato dei dispositivi (Led e Pulsante/interruttore)
        waiting = 0;
        statoLed = 0;
        statoPulsante = 0;

        // Setto il numero del processo S e il tipo di interruttore o pulsante
        numeroS = atoi(argv[4]);
        type = argv[3];

        // Cambio gli handler dei segnali
        struct sigaction sa;
        sa.sa_flags = SA_SIGINFO;
        sa.sa_sigaction = confemaDalPulsante;
        sigaction(SIGUSR1, &sa, NULL);

        struct sigaction sa1;
        sa1.sa_flags = SA_SIGINFO;
        sa1.sa_sigaction = confemaDalLed;
        sigaction(SIGUSR2, &sa1, NULL);

        // Se è un pulsante c'è anche il sigalrm
        if (strcmp(type, "P") == 0)
        {
            struct sigaction sa1;
            sa1.sa_flags = SA_SIGINFO;
            sa1.sa_sigaction = spegniPulsante;
            sigaction(SIGALRM, &sa1, NULL);
        }

        // Setto le pipe ricevute come parametro
        fdInputFromM = atoi(argv[1]);
        fdOutputToM = atoi(argv[2]);

        // Cambio il nome del processo in S1 o S2 o S3 o S4
        char nomeProcesso[3];
        snprintf(nomeProcesso, sizeof(nomeProcesso), "S%d", numeroS);
        pthread_setname_np(pthread_self(), nomeProcesso);

        // Faccio il fork del led e del pulsante
        collegaLed(argv[4]);
        collegaPulsante(argv[4]);

        int delay;            // Memorizza il tempo per cui un pulsante deve essere premuto
        char *ptr;            // Variabile d'appoggio per split stringa ricevuta da M
        char concat_str[100]; // Buffer per l'input ricevuto dalla pipe da M
        char stringaError[100];

        while (1)
        {
            if (read(fdInputFromM, concat_str, 100) != -1)
            {
                if (strcmp(concat_str, "STATUS") == 0)
                {
                    inviaStatus();
                }
                else
                {
                    ptr = strtok(concat_str, " ");
                    ptr = strtok(NULL, " ");
                    if (ptr == NULL)
                    {
                        if (strcmp(type, "I") == 0)
                        {
                            kill(T, SIGUSR1);
                        }
                        else
                        {
                            snprintf(stringaError, sizeof(stringaError), "%d|ERR|Sintassi errata. T%d non è un interruttore, usa premi <numero> <secondi>.\n", numeroS, numeroS);
                            write(fdOutputToM, stringaError, strlen(stringaError));
                        }
                    }
                    else
                    {
                        if (strcmp(type, "P") == 0)
                        {
                            if (waiting == 0)
                            {
                                delay = atoi(ptr);
                                if (delay > 0)
                                {
                                    waiting = 1;
                                    kill(T, SIGUSR1);
                                    alarm(delay);
                                }
                                else
                                {
                                    snprintf(stringaError, sizeof(stringaError), "%d|ERR|Sintassi errata. Il delay deve essere un numero intero positivo.\n", numeroS);
                                    write(fdOutputToM, stringaError, strlen(stringaError));
                                }
                            }
                            else
                            {
                                snprintf(stringaError, sizeof(stringaError), "%d|ERR|Pulsante già premuto. Attendere la fine del delay.\n", numeroS);
                                write(fdOutputToM, stringaError, strlen(stringaError));
                            }
                        }
                        else
                        {
                            snprintf(stringaError, sizeof(stringaError), "%d|ERR|Sintassi errata. T%d non è un pulsante, usa premi <numero>.\n", numeroS, numeroS);
                            write(fdOutputToM, stringaError, strlen(stringaError));
                        }
                    }
                }
            }
            else
            {
                // EINTR -> La read è stata interrotta da un segnale.
                if (errno != EINTR)
                {
                    perror("C'è stato un errore nella lettura della pipe");
                    exit(1);
                }
            }
        }
    }
    else
    {
        fprintf(stderr, "Errore il processo S deve essere creato da M. Non si può avviare questo programma singolarmente!\n");
        exit(1);
    }
    return 0;
}
