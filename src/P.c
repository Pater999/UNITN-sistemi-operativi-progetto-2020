#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>

static volatile int id;             // Identificatore del processo P
static volatile int pipeOutputToM;  // Pipe utilizzata per la comunicazione in scrittura con M
static volatile int pipeInputFromM; // Pipe utilizzata per la comunicazione in scrittura con M
static volatile int pidFiglio;      // Pid del eventuale figlio
static int pidProcessoM;

static const char *fifoPipe = "/tmp/SO/fifoP_Q"; // Fifo di comunicazione tra i processi P e Q

static void leggiDallaPipe(int sig, siginfo_t *info, void *context);

void creaUnNuovofiglio(int indice)
{
    char stringaTmp[100];
    char nomeProcesso[20];

    pidFiglio = fork();
    if (pidFiglio == -1)
    {
        // RITORNO ERRORE NELLA CREAZIONE AD M
        snprintf(stringaTmp, sizeof(stringaTmp), "ERROR");
        write(pipeOutputToM, stringaTmp, strlen(stringaTmp) + 1);
    }
    else if (pidFiglio == 0)
    {
        // CAMBIO I NOMI DEI PROCESSI
        snprintf(nomeProcesso, sizeof(nomeProcesso), "P%d", indice);
        pidFiglio = 0;
        id = indice;
        struct sigaction sa;
        sa.sa_flags = SA_SIGINFO;
        sa.sa_sigaction = leggiDallaPipe;
        sigaction(SIGUSR1, &sa, NULL);
        pthread_setname_np(pthread_self(), nomeProcesso);
    }
    else
    {
        // RITORNO IL PID AD M
        snprintf(stringaTmp, sizeof(stringaTmp), "%d %d", pidFiglio, indice);
        write(pipeOutputToM, stringaTmp, strlen(stringaTmp) + 1);
        int rit = waitpid(pidFiglio, 0, 0);
        if (rit == pidFiglio)
            pidFiglio = 0;
    }
}

// Handler segnale ricevuto da M
static void leggiDallaPipe(int sig, siginfo_t *info, void *context)
{
    if (info->si_pid == pidProcessoM)
    {
        char concat_str[20];
        int idTmp;

        // SCARTO SE ha già un figlio
        if (pidFiglio == 0)
        {
            if (read(pipeInputFromM, concat_str, 20) != -1)
            {
                idTmp = atoi(concat_str);
                creaUnNuovofiglio(idTmp);
            }
            else
            {
                fprintf(stderr, "Impossibile leggere dalla pipe (processo P%d)", id);
                exit(1);
            }
        }
    }
}

// Handler segnale ricevuto da Q
static void scriviNellaPipeQ(int sig)
{
    int fdQ;
    char str[30];

    // Scrive nella pipe il proprio identificatore
    fdQ = open(fifoPipe, O_WRONLY);
    snprintf(str, sizeof(str), "%d ID", id);
    write(fdQ, str, strlen(str) + 1);
    close(fdQ);
}

int main(int argc, char *argv[])
{
    if (getppid() != 1)
    {
        pidProcessoM = getppid();
        // Cambio gli handler dei segnali
        struct sigaction sa;
        sa.sa_flags = SA_SIGINFO;
        sa.sa_sigaction = leggiDallaPipe;
        sigaction(SIGUSR1, &sa, NULL);

        signal(SIGUSR2, scriviNellaPipeQ);

        mkfifo(fifoPipe, 0666);

        char stringaTmp[10];

        id = atoi(argv[1]);
        pipeOutputToM = atoi(argv[2]);
        pipeInputFromM = atoi(argv[3]);

        char nomeProcessoString[15];
        snprintf(nomeProcessoString, sizeof(nomeProcessoString), "P%d", id);
        pthread_setname_np(pthread_self(), nomeProcessoString);

        // INVIO AD M CONFERMA DI ESSERE STATO INIZIALIZZATO
        snprintf(stringaTmp, sizeof(stringaTmp), "OK");
        write(pipeOutputToM, stringaTmp, strlen(stringaTmp) + 1);

        while (1)
            pause();
    }
    else
    {
        fprintf(stderr, "Errore il processo P deve essere creato da M. Non si può avviare questo programma singolarmente!\n");
        exit(1);
    }
}