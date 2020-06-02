#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <stdlib.h>

// Testo del menù del programma Q
#define HELP_COMANDI_Q \
    "\n\033[01;33m-------------------------- Comandi disponibili Q ---------------------------\033[0m\n\
\033[01;33m| \033[0;34m pos <posizione>\033[0m       Ricava identificatore processo P data posizione   \033[01;33m|\n\
\033[01;33m| \033[0;34m id <identificatore>\033[0m   Ricava posizione processo P dato identificatore   \033[01;33m|\n\
\033[01;33m| \033[0;35m help\033[0m                  Mostra i comandi disponibili                      \033[01;33m|\n\
\033[01;33m| \033[0;31m quit\033[0m                  Esci dal programma                                \033[01;33m|\n\
\033[01;33m----------------------------------------------------------------------------\033[0m\n\n"

// FIFO file path
static const char *fifoScrittura = "/tmp/SO/fifoMLettura";
static const char *fifoLettura = "/tmp/SO/fifoMScrittura";
static const char *fifoP = "/tmp/SO/fifoP_Q";

// Data una posizione si occupa di contattare M per ricevere il Pid del processo P in tale posizione e poi contatta P per farsi restituire l'identificativo
void handlePosizione(int Posizione);
// Data un identificativo si fa dire da M i pid di tutti i processi e poi tramite una ricerca binaria contatta tutti i P per trovare l'identificativo
void handleId(int id);
// Binary search per la ricerca dell'identificativo
int binarySearchIdP(int l, int r, int x);
// Metodo di comodo per contattare M per farsi restituire il pid del processo P da contattare
int riferimentoP(int posizione);
// Metodo di comodo per contattare un processo P dato il suo pid
int contattaP(int pid);
// Nel caso che la lettura della pipe dia errore il programma Q viene chiuso
void errorePipe();

int main(int argc, char *argv[])
{
    char bufComando[100]; // Buffer per la lettura dei comandi da parte dell'utente
    char *ptr;            // Pointer per fare lo split del comando ricevuto dall'utente
    int posizione;        // Posizione ricercata
    int identificatore;   // Identificatore ricercato

    // Creo la fifo per comunicazione tra M e Q

    mkfifo(fifoP, 0666);
    mkfifo(fifoScrittura, 0666);
    mkfifo(fifoLettura, 0666);

    printf(HELP_COMANDI_Q);
    while (1)
    {
        fgets(bufComando, 100, stdin);
        ptr = strtok(bufComando, " ");

        if (strcmp(ptr, "quit\n") == 0 || strcmp(ptr, "Quit\n") == 0)
            exit(0);
        else if (strcmp(ptr, "help\n") == 0 || strcmp(ptr, "Help\n") == 0)
        {
            printf(HELP_COMANDI_Q);
        }
        else if (strcmp(ptr, "pos") == 0 || strcmp(ptr, "Pos") == 0 || strcmp(ptr, "POS") == 0)
        {
            ptr = strtok(NULL, " ");
            if (ptr != NULL)
            {
                posizione = atoi(ptr);
                if (posizione > 0 || strcmp(ptr, "0\n") == 0)
                    handlePosizione(posizione);
                else
                    printf("Formato input non valido. Posizione deve essere un numero >= 0\n");
            }
            else
                printf("Formato input non valido. Devi utilizzare pos <numero_pos>\n");
        }
        else if (strcmp(ptr, "id") == 0 || strcmp(ptr, "Id") == 0 || strcmp(ptr, "ID") == 0)
        {
            ptr = strtok(NULL, " ");
            if (ptr != NULL)
            {
                identificatore = atoi(ptr);
                if (identificatore > 0 || strcmp(ptr, "0\n") == 0)
                    handleId(identificatore);
                else
                    printf("Formato input non valido. Id deve essere un numero >= 0\n");
            }
            else
            {
                printf("Formato input non valido. Devi utilizzare id <numero_pos>\n");
            }
        }
        else
        {
            fprintf(stderr, "Comando non valido. Scrivi help per la lista comandi.\n");
        }
    }
    return 0;
}

void handlePosizione(int Posizione)
{
    int fdTMP;  // Descrittore temporaneo
    int pidP;   // Pid del processo P da contattare
    int result; // Valore di ritorno della funzione contattaP

    printf("In attesa di M...\n");
    pidP = riferimentoP(Posizione);

    // Gestisco le stampe
    if (pidP == -1)
        printf("Non esiste un processo P in posizione %d. Prova con altra posizione.\n", Posizione);
    else
    {
        result = contattaP(pidP);
        if (result != -1)
            printf("Il processo P in posizione %d ha identificatore: %d\n", Posizione, result);
        else
            printf("Errore. Non è stato possibile contattare il processo P in posizione %d. Riprova!\n", Posizione);
    }
}

void handleId(int id)
{
    int fdTMP;    // Descrittore temporaneo
    char tmp[20]; // Buffer per leggere dalla pipe

    printf("In attesa di M...\n");
    fdTMP = open(fifoScrittura, O_WRONLY); // Leggo dalla pipe. Finchè non viene aperta da M il programma si ferma qua. (Nel caso M non sia stato avviato dall'utente)
    if (fdTMP != -1)
    {
        // Scrivo ad M di inviarmi il numero totale di processi P
        snprintf(tmp, sizeof(tmp), "ALL");
        write(fdTMP, tmp, strlen(tmp) + 1);
        close(fdTMP);

        // Leggo dalla pipe il numero totale di processi P
        fdTMP = open(fifoLettura, O_RDONLY);
        if (fdTMP != -1)
        {
            read(fdTMP, tmp, sizeof(tmp));
            close(fdTMP);

            int result = binarySearchIdP(0, atoi(tmp) - 1, id);
            if (result == -1)
                printf("Il processo P con identificatore %d non è presenta nella coda dei processi P\n", id);
            else if (result == -2)
                printf("Errore. La coda dei processi P è cambiata durante la ricerca. Riprova.\n");
            else
                printf("Il processo P con identificatore %d si trova in posizione %d\n", id, result);
        }
        else
            errorePipe();
    }
    else
        errorePipe();
}

int binarySearchIdP(int l, int r, int x)
{
    int mid, pid, valore;
    while (l <= r)
    {
        mid = l + (r - l) / 2;

        pid = riferimentoP(mid);
        if (pid != -1)
        {
            valore = contattaP(pid);
            if (valore == -1)
                return -2;

            if (valore == x)
                return mid;

            if (valore < x)
                l = mid + 1;
            else
                r = mid - 1;
        }
        else
        {
            return -2;
        }
    }
    return -1;
}

int riferimentoP(int posizione)
{
    int fdM;      // Descrittore temporaneo
    char tmp[30]; // Buffer per leggere dalla pipe

    // Scrivo ad M di inviarmi la posizione del processo P in posizione data
    fdM = open(fifoScrittura, O_WRONLY); // Leggo dalla pipe. Finchè non viene aperta da M il programma si ferma qua. (Nel caso M non sia stato avviato dall'utente)
    if (fdM != -1)
    {
        snprintf(tmp, sizeof(tmp), "%d", posizione);
        write(fdM, tmp, strlen(tmp) + 1);
        close(fdM);

        // Leggo dalla pipe il pid (o Error in caso che non esiste) del processo P in posizione data
        fdM = open(fifoLettura, O_RDONLY);
        if (fdM != -1)
        {
            read(fdM, tmp, sizeof(tmp));
            close(fdM);

            if (strcmp(tmp, "ERROR") == 0)
                return -1;
            else
                return atoi(tmp);
        }
        else
            errorePipe();
    }
    else
        errorePipe();
}

int contattaP(int pid)
{
    int fd;
    char tmpS[30]; // Buffer per leggere dalla pipe

    int ritornoKill = kill(pid, SIGUSR2);
    if (ritornoKill != -1)
    {
        fd = open(fifoP, O_RDONLY);
        if (fd != -1)
        {
            read(fd, tmpS, sizeof(tmpS));
            close(fd);
            return atoi(tmpS);
        }
        else
            errorePipe();
    }
    else
        return -1;
}

void errorePipe()
{
    printf("Non sono riuscito a leggere/scrivere dalla FIFO. Probabilmente è stata elimata il file oppure la cartella.\nPotrebbe essere necessaria una nuova ricompilazione utilizzando make build\n\n");
    exit(1);
}