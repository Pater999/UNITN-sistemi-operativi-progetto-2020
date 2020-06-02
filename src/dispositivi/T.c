#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>

static char nomePulsante;
static volatile int statoPulsante;
static pid_t S;   // PID DEL PARENT S
static char tipo; // Pulsante o interruttore

static void spegniPulsante(int sig, siginfo_t *info, void *context);

static void accendiPulsante(int sig, siginfo_t *info, void *context)
{
    // Solo se è un interruttore posso eseguire il kill da terminale
    if (tipo == 'I' || info->si_pid == S)
    {
        statoPulsante = 1;
        struct sigaction sa;
        sa.sa_flags = SA_SIGINFO;
        sa.sa_sigaction = spegniPulsante;
        sigaction(SIGUSR1, &sa, NULL);
        kill(S, SIGUSR1);
    }
}

static void spegniPulsante(int sig, siginfo_t *info, void *context)
{
    // Solo se è un interruttore posso eseguire il kill da terminale
    if (tipo == 'I' || info->si_pid == S)
    {
        statoPulsante = 0;
        struct sigaction sa;
        sa.sa_flags = SA_SIGINFO;
        sa.sa_sigaction = accendiPulsante;
        sigaction(SIGUSR1, &sa, NULL);
        kill(S, SIGUSR1);
    }
}

int main(int argc, char *argv[])
{
    if (getppid() != 1)
    {
        struct sigaction sa;
        sa.sa_flags = SA_SIGINFO;
        sa.sa_sigaction = accendiPulsante;
        sigaction(SIGUSR1, &sa, NULL);

        // Cambio il nome del processo in T1 o T2 o T3 o T4 _<interruttore o pulsante>
        char nomeProcesso[5];
        snprintf(nomeProcesso, sizeof(nomeProcesso), "T%s_%s", argv[2], argv[1]);
        pthread_setname_np(pthread_self(), nomeProcesso);

        S = getppid();

        statoPulsante = 0;
        if (strcmp(argv[1], "P") == 0)
            tipo = 'P';
        else if (strcmp(argv[1], "I") == 0)
            tipo = 'I';
        else
            printf("Sintassi non valida\n");

        while (1)
            pause();
    }
    else
    {
        fprintf(stderr, "Errore il processo T deve essere creato da M. Non si può avviare questo programma singolarmente!\n");
        exit(1);
    }

    return 0;
}
