#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>

static volatile int statoLed; // Stato del Led [1 acceso e 0 spento]

static pid_t S; // PID DEL PARENT S

static void spegniLed(int sig, siginfo_t *info, void *context);

static void accendiLed(int sig, siginfo_t *info, void *context)
{
    // Viene cambiato lo stato solo se il segnale proviene da S altrimenti viene scartato
    if (info->si_pid == S)
    {
        statoLed = 1;
        struct sigaction sa;
        sa.sa_flags = SA_SIGINFO;
        sa.sa_sigaction = spegniLed;
        sigaction(SIGUSR1, &sa, NULL);
        kill(S, SIGUSR2);
    }
}

static void spegniLed(int sig, siginfo_t *info, void *context)
{
    // Viene cambiato lo stato solo se il segnale proviene da S altrimenti viene scartato
    if (info->si_pid == S)
    {
        statoLed = 0;
        struct sigaction sa;
        sa.sa_flags = SA_SIGINFO;
        sa.sa_sigaction = accendiLed;
        sigaction(SIGUSR1, &sa, NULL);
        kill(S, SIGUSR2);
    }
}

int main(int argc, char *argv[])
{
    if (getppid() != 1)
    {
        S = getppid();
        statoLed = 0;

        // Setto segnale per accendere/spegnere il Led
        struct sigaction sa;
        sa.sa_flags = SA_SIGINFO;
        sa.sa_sigaction = accendiLed;
        sigaction(SIGUSR1, &sa, NULL);

        // Cambio il nome del processo in L1 L2 L3 L4
        char nomeProcesso[3];
        snprintf(nomeProcesso, sizeof(nomeProcesso), "L%s", argv[1]);
        pthread_setname_np(pthread_self(), nomeProcesso);

        while (1)
            pause();
    }
    else
    {
        fprintf(stderr, "Errore il processo L deve essere creato da M. Non si può avviare questo programma singolarmente!\n");
        exit(1);
    }
    return 0;
}
