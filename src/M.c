#include "M.h"

static pid_t S_Array[4];              // Array contente i pid dei processi S (S1, S2, S3, S4)
static int fdInputS[2];               // Pipe per leggere i messaggi inviati dai processi S
static int fdOutputS[4][2];           // Array di pipe per scrivere messaggi ai processi S (S1, S2, S3, S4)
static char *buttonTypes[4];          // Array contente il tipo dei bottoni [P -> Pulsante e I -> Interruttore]
static pthread_t threadPipeS;         // Thread per la lettura dalla Pipe
static pthread_t threadProcessiP;     // Thread per la gestione dei processi P
static pthread_t treadComunicazioneQ; // Thread per la comunicazione tra Q e M
static volatile int primoProcessoPid; // PID DEL PROCESSO P0

static int fdInputP[2];  // Pipe per comunicare con i processi in lettura
static int fdOutputP[2]; // Pipe per comunicare con i processi in scrittura

// Thread che si occupa della comunicazione con il programma "Q"
void *comunicazioneConQThread()
{
    int fdQ;   // Descrittore comunicazione tra M e Q
    int index; // Posizione del processo P ricercato

    // FIFO file path
    char *fifoLettura = "/tmp/SO/fifoMLettura";
    char *fifoScrittura = "/tmp/SO/fifoMScrittura";

    // Creo la fifo per comunicazione tra M e Q
    mkfifo(fifoLettura, 0666);
    mkfifo(fifoScrittura, 0666);

    char str1[30]; // Buffer lettura da pipe
    char str2[30]; // Buffer scrittura sulla pipe

    while (1)
    {
        // Apro il sola lettura se non c'è il processo Q aperto in scrittura si blocca qui e aspetta
        fdQ = open(fifoLettura, O_RDONLY);
        read(fdQ, str1, 30);
        close(fdQ);

        if (strcmp(str1, "ALL") == 0)
        {
            // Restituisco il numero totale di processi P
            fdQ = open(fifoScrittura, O_WRONLY);
            snprintf(str2, sizeof(str2), "%d", lista.currentIndex);
            write(fdQ, str2, strlen(str2) + 1);
            close(fdQ);
        }
        else
        {
            index = atoi(str1);
            if (index < lista.currentIndex)
            {
                // Restituisco il pid del processo P che si trova nella posizione che ho ricevuto da Q
                fdQ = open(fifoScrittura, O_WRONLY);
                snprintf(str2, sizeof(str2), "%d", lista.processiPList[index].pid);
                write(fdQ, str2, strlen(str2) + 1);
                close(fdQ);
            }
            else
            {
                // Restituisco errore perchè non c'è nessun processo P che si trova nella posizione che ho ricevuto da Q
                fdQ = open(fifoScrittura, O_WRONLY);
                snprintf(str2, sizeof(str2), "ERROR");
                write(fdQ, str2, strlen(str2) + 1);
                close(fdQ);
            }
        }
    }
    return 0;
}

// Thread che si occupa di leggere i messaggi ricevuti dai processi S.
void *readingFromPipeThread()
{
    char concat_str[100];
    char *ptr;
    int numeroP;
    int doSomething;
    while (1)
    {
        if (read(fdInputS[0], concat_str, 100) != -1)
        {
            ptr = strtok(concat_str, "|");
            numeroP = atoi(ptr);
            ptr = strtok(NULL, "|");
            if (strcmp(ptr, "ERR") == 0)
            {
                ptr = strtok(NULL, "|");
                write(1, ptr, strlen(ptr));
            }
            else if (strcmp(ptr, "OUT") == 0)
            {
                ptr = strtok(NULL, "|");
                doSomething = atoi(ptr);
                ptr = strtok(NULL, "|");
                write(1, ptr, strlen(ptr));
                if (doSomething == 1)
                {
                    gestisciConfermaPulsanti(numeroP);
                }
            }
        }
        else
        {
            write(2, "Errore nella lettura della Pipe di input dai processi S.\n", 59);
            esci(3);
        }
    }
}

// Thread che si occupa di leggere i messaggi ricevuti dai processi P.
void *leggiDallaPipeDaiP()
{
    char concat_str[100];
    int identificatore;
    int pid;
    char *pointer;

    while (1)
    {
        if (read(fdInputP[0], concat_str, 100) != -1)
        {
            if (strcmp(concat_str, "ERROR") != 0)
            {
                pointer = strtok(concat_str, " ");
                pid = atoi(pointer);
                pointer = strtok(NULL, " ");
                identificatore = atoi(pointer);

                if (listAdd(pid, identificatore) == 0)
                    printf("Aggiunto processo P. PID: %d --- ID: %d\n", lista.processiPList[lista.currentIndex - 1].pid, lista.processiPList[lista.currentIndex - 1].id);
                else
                    printf("Impossibile aggiungere la memoria è piena\n");
            }
            else
            {
                printf("Errore forking P. Non posso più creare processi P\n");
            }
        }
    }
}

int main(int argc, char *argv[])
{

    char bufComando[100]; // Buffer per l'input utente

    // Creo pipe input
    creaPipe(fdInputS, __O_DIRECT);

    // Creo 4 pipe output (Una per ogni S)
    creaPipe(fdOutputS[0], __O_DIRECT);
    creaPipe(fdOutputS[1], __O_DIRECT);
    creaPipe(fdOutputS[2], __O_DIRECT);
    creaPipe(fdOutputS[3], __O_DIRECT);

    // Creo il primo processo P1 e poi rimane in ascolto sulla pipe
    creaProcessoP0();

    // Creo un thread per la comunicazione tra M e Q
    pthread_create(&treadComunicazioneQ, NULL, comunicazioneConQThread, NULL);

    signal(SIGINT, sigTermHandler);
    signal(SIGTERM, sigTermHandler);
    signal(SIGSEGV, sigSegmentation);

    int uscita = 0; // Condizione per rompere il while
    int i;          // Indice per il ciclo for

    // L'utente decide il tipo pulsante/interruttore
    for (i = 0; i < 4; i++)
    {
        printf("Creazione di %c, specificarne il tipo:\n\
        I     -  Interruttore\n\
        P     -  Pulsante\n\
        quit  -  Esci\n",
               (65 + i));

        while ((uscita == 0))
        {
            fgets(bufComando, 100, stdin);
            if (strcmp(bufComando, "quit\n") == 0 || strcmp(bufComando, "Quit\n") == 0)
            {
                esci(0);
            }
            else if (strcmp(bufComando, "i\n") == 0 || strcmp(bufComando, "I\n") == 0)
            {
                buttonTypes[i] = "I";
                uscita = 1;
            }
            else if (strcmp(bufComando, "P\n") == 0 || strcmp(bufComando, "p\n") == 0)
            {
                buttonTypes[i] = "P";
                uscita = 1;
            }
            else
            {
                fprintf(stderr, "Formato input errato. Puoi usare SOLO quit - i - p!\nRiprova.\n");
            }
        }
        uscita = 0;
    }

    // Creazione processi S  (S1, S2, S3, S4)
    for (i = 0; i < 4; i++)
    {
        creaProcessoS(i);
    }

    // Chiudo endpoint non utilizzati delle pipe
    close(fdInputS[1]);
    close(fdOutputS[0][0]);
    close(fdOutputS[1][0]);
    close(fdOutputS[2][0]);
    close(fdOutputS[3][0]);

    // Creo un thread che leggerà dalla pipe i messaggi ricevuti dai processi S.
    pthread_create(&threadPipeS, NULL, readingFromPipeThread, NULL);

    // Entro nel metodo per leggere l'input dell'utente (Menù)
    checkInputUtente();
}

void checkInputUtente()
{
    // Stringa d'appoggio
    char input_str[100];

    // Buffer per l'input utente
    char bufComando[100];

    // Variabile per cicli for
    int i;

    printf(HELP_COMANDI);
    char *ptr;
    while (1)
    {
        fgets(bufComando, 100, stdin);

        ptr = strtok(bufComando, " ");

        if (strcmp(ptr, "quit\n") == 0 || strcmp(ptr, "Quit\n") == 0)
        {
            esci(0);
        }
        else if (strcmp(ptr, "help\n") == 0 || strcmp(ptr, "Help\n") == 0)
        {
            printf(HELP_COMANDI);
        }
        else if (strcmp(ptr, "status\n") == 0 || strcmp(ptr, "Status\n") == 0)
        {
            // Contatto i vari S per ottenere lo stato del led e del pulsante
            snprintf(input_str, sizeof(input_str), "STATUS");
            write(fdOutputS[0][1], input_str, strlen(input_str) + 1);
            write(fdOutputS[1][1], input_str, strlen(input_str) + 1);
            write(fdOutputS[2][1], input_str, strlen(input_str) + 1);
            write(fdOutputS[3][1], input_str, strlen(input_str) + 1);
        }
        else if (strcmp(ptr, "list\n") == 0 || strcmp(ptr, "List\n") == 0)
        {
            printf("\n----- SITUAZIONE CODA PROCESSI P -----\n");
            printf("\tPOS\t ID\t PID\n");
            for (i = 0; i < lista.currentIndex; i++)
            {
                printf("\t%d\t %d\t %d\n", i, lista.processiPList[i].id, lista.processiPList[i].pid);
            }
            printf("\n");
        }
        else if (strcmp(ptr, "premi") == 0 || strcmp(ptr, "Premi") == 0)
        {
            ptr = strtok(NULL, " ");
            if (ptr != NULL)
            {
                if (strcmp(ptr, "1") == 0 || strcmp(ptr, "1\n") == 0)
                {
                    ptr = strtok(NULL, " ");

                    if (ptr == NULL)
                    {
                        snprintf(input_str, sizeof(input_str), "toggle");
                    }
                    else
                    {
                        snprintf(input_str, sizeof(input_str), "toggle %s", ptr);
                        input_str[strlen(input_str) - 1] = 0;
                    }
                    write(fdOutputS[0][1], input_str, strlen(input_str) + 1);
                }
                else if (strcmp(ptr, "2") == 0 || strcmp(ptr, "2\n") == 0)
                {
                    ptr = strtok(NULL, " ");

                    if (ptr == NULL)
                    {
                        snprintf(input_str, sizeof(input_str), "toggle");
                    }
                    else
                    {
                        snprintf(input_str, sizeof(input_str), "toggle %s", ptr);
                        input_str[strlen(input_str) - 1] = 0;
                    }
                    write(fdOutputS[1][1], input_str, strlen(input_str) + 1);
                }
                else if (strcmp(ptr, "3") == 0 || strcmp(ptr, "3\n") == 0)
                {
                    ptr = strtok(NULL, " ");

                    if (ptr == NULL)
                    {
                        snprintf(input_str, sizeof(input_str), "toggle");
                    }
                    else
                    {
                        snprintf(input_str, sizeof(input_str), "toggle %s", ptr);
                        input_str[strlen(input_str) - 1] = 0;
                    }
                    write(fdOutputS[2][1], input_str, strlen(input_str) + 1);
                }
                else if (strcmp(ptr, "4") == 0 || strcmp(ptr, "4\n") == 0)
                {
                    ptr = strtok(NULL, " ");

                    if (ptr == NULL)
                    {
                        snprintf(input_str, sizeof(input_str), "toggle");
                    }
                    else
                    {
                        snprintf(input_str, sizeof(input_str), "toggle %s", ptr);
                        input_str[strlen(input_str) - 1] = 0;
                    }
                    write(fdOutputS[3][1], input_str, strlen(input_str) + 1);
                }
                else
                {
                    printf("Numero pulsante non valido. I pulsanti sono 4.\n");
                }
            }
            else
            {
                printf("Formato input non valido. Usa premi <numero_pulsante> [delay_ms]\n");
            }
        }
        else
        {
            printf("Comando non valido. Scrivi help per la lista comandi.\n");
        }
    }
}

void gestisciConfermaPulsanti(int T)
{
    char stringaTmp[100];
    int tmpMaxIndex;

    switch (T)
    {
    // ELIMINAZIONE IN TESTA
    case 1:
        if (lista.currentIndex > 1)
        {
            eliminazioneInTesta();
        }
        else
        {
            snprintf(stringaTmp, sizeof(stringaTmp), "Impossibile rimuovere il primo P. Deve esistere almeno un processo P.\n");
            write(2, stringaTmp, strlen(stringaTmp));
        }
        break;
    // AGGIUNGO NUOVO PROCESSO IN CODA
    case 2:
        snprintf(stringaTmp, sizeof(stringaTmp), "%d", lista.counterId);
        if (write(fdOutputP[1], stringaTmp, strlen(stringaTmp) + 1) != -1)
        {
            kill(lista.processiPList[lista.currentIndex - 1].pid, SIGUSR1);
        }
        break;
    // SELEZIONE
    case 3:
        if (lista.processoSelezionato < lista.currentIndex - 1)
            lista.processoSelezionato++;
        else
            lista.processoSelezionato = 0;
        printf("Processo selezionato posizione %d\n", lista.processoSelezionato);
        break;
    // ELIMINAZIONE SELEZIONATO
    case 4:
        if (lista.currentIndex > 1)
        {
            if (lista.processoSelezionato == 0)
                eliminazioneInTesta();
            else
                eliminazioneSelezionato();
            if (lista.processoSelezionato > lista.currentIndex - 1)
                lista.processoSelezionato = 0;
        }
        else
        {
            snprintf(stringaTmp, sizeof(stringaTmp), "Impossibile rimuovere P. Deve esistere almeno un processo P.\n");
            write(2, stringaTmp, strlen(stringaTmp));
        }
        break;
    }
}

void creaProcessoS(int i)
{
    // Variabili temporanee per passare il numero del processo ad S.
    char numeroProcessoS[3];
    char pipeOutputTmp0[15];
    char pipeInputTmp1[15];

    char pipeOutputTmp1[15];
    char pipeInputTmp0[15];

    snprintf(numeroProcessoS, sizeof(numeroProcessoS), "%d", (i + 1));
    snprintf(pipeOutputTmp0, sizeof(pipeOutputTmp0), "%d", fdOutputS[i][0]);
    snprintf(pipeInputTmp1, sizeof(pipeInputTmp1), "%d", fdInputS[1]);
    snprintf(pipeOutputTmp1, sizeof(pipeOutputTmp1), "%d", fdOutputS[i][1]);
    snprintf(pipeInputTmp0, sizeof(pipeInputTmp0), "%d", fdInputS[0]);

    S_Array[i] = fork();
    if (S_Array[i] == -1)
    {
        fprintf(stderr, "Errore il forking del processo S%d è fallito.\n", i);
        esci(2);
    }
    else if (S_Array[i] == 0)
    {
        // Sostituisco l'immagine del processo con il programma S
        if (execl("./bin/S", "S", pipeOutputTmp0, pipeInputTmp1, buttonTypes[i], numeroProcessoS, pipeOutputTmp1, pipeInputTmp0, NULL) == -1)
        {
            fprintf(stderr, "Errore l'immagine del processo S%d non può essere sostituita.\n", i);
            esci(3);
        }
    }
}

void creaProcessoP0()
{
    lista.counterId = 0;

    inizializzaLista();

    creaPipe(fdInputP, __O_DIRECT);
    creaPipe(fdOutputP, __O_DIRECT);

    char identificatoreString[15];
    snprintf(identificatoreString, sizeof(identificatoreString), "%d", lista.counterId);
    char pipeInputTmp[15];
    snprintf(pipeInputTmp, sizeof(pipeInputTmp), "%d", fdInputP[1]);
    char pipeOutputTmp[15];
    snprintf(pipeOutputTmp, sizeof(pipeOutputTmp), "%d", fdOutputP[0]);

    primoProcessoPid = fork();
    if (primoProcessoPid == -1)
    {
        fprintf(stderr, "Errore il forking del processo P1 è fallito.\n");
        esci(2);
    }
    else if (primoProcessoPid == 0)
    {
        // Sostituisco l'immagine del processo con il programma P
        if (execl("./bin/P", "P", identificatoreString, pipeInputTmp, pipeOutputTmp, NULL) == -1)
        {
            fprintf(stderr, "Errore l'immagine del processo P1 non può essere sostituita.\n");
            esci(3);
        }
    }
    else
    {
        char stringaTmp[10];
        close(fdInputP[1]);
        close(fdOutputP[0]);
        listAdd(primoProcessoPid, lista.counterId);

        // P0 CONFERMA DI ESSERE STATO CREATO
        if (read(fdInputP[0], stringaTmp, 10) != -1)
        {
            pthread_create(&threadProcessiP, NULL, leggiDallaPipeDaiP, NULL);
        }
    }
}

void ricreaProcessoP0(int identificatore)
{
    // RIFACCIO LE PIPE
    close(fdInputP[0]);
    close(fdOutputP[1]);

    creaPipe(fdInputP, __O_DIRECT);
    creaPipe(fdOutputP, __O_DIRECT);

    char stringaTmp[100];

    char identificatoreString[15];
    snprintf(identificatoreString, sizeof(identificatoreString), "%d", identificatore);
    char pipeInputTmp[15];
    snprintf(pipeInputTmp, sizeof(pipeInputTmp), "%d", fdInputP[1]);
    char pipeOutputTmp[15];
    snprintf(pipeOutputTmp, sizeof(pipeOutputTmp), "%d", fdOutputP[0]);

    primoProcessoPid = fork();
    if (primoProcessoPid == -1)
    {
        fprintf(stderr, "Errore il forking del processo P1 è fallito.\n");
        esci(2);
    }
    else if (primoProcessoPid == 0)
    {
        // Sostituisco l'immagine del processo con il programma P
        if (execl("./bin/P", "P", identificatoreString, pipeInputTmp, pipeOutputTmp, NULL) == -1)
        {
            fprintf(stderr, "Errore l'immagine del processo P1 non può essere sostituita.\n");
            esci(3);
        }
    }
    else
    {
        lista.processiPList[0].pid = primoProcessoPid;
        close(fdInputP[1]);
        close(fdOutputP[0]);
        int i;

        // P0 CONFERMA DI ESSERE STATO CREATO
        if (read(fdInputP[0], stringaTmp, 10) != -1)
        {
            for (i = 1; i < lista.currentIndex; i++)
            {
                snprintf(stringaTmp, sizeof(stringaTmp), "%d", lista.processiPList[i].id);
                if (write(fdOutputP[1], stringaTmp, strlen(stringaTmp) + 1) != -1)
                {
                    kill(lista.processiPList[i - 1].pid, SIGUSR1);
                }
                if (read(fdInputP[0], stringaTmp, 100) != -1)
                {
                    lista.processiPList[i].pid = atoi(stringaTmp);
                }
            }
        }
    }
}

void eliminazioneInTesta()
{
    int i;
    pthread_cancel(threadProcessiP);
    for (i = lista.currentIndex - 1; i >= 0; i--)
    {
        kill(lista.processiPList[i].pid, SIGTERM);
    }
    waitpid(primoProcessoPid, NULL, 0);
    listDelete(0);
    ricreaProcessoP0(lista.processiPList[0].id);
    if (lista.processoSelezionato != 0)
        lista.processoSelezionato--;
    printf("Ho rimosso il processo P in testa alla coda\n");
    pthread_create(&threadProcessiP, NULL, leggiDallaPipeDaiP, NULL);
}

void eliminazioneSelezionato()
{
    int i;
    char stringaTmp[100];
    pthread_cancel(threadProcessiP);
    for (i = lista.currentIndex - 1; i >= lista.processoSelezionato; i--)
    {
        kill(lista.processiPList[i].pid, SIGTERM);
    }
    listDelete(lista.processoSelezionato);

    for (i = lista.processoSelezionato; i < lista.currentIndex; i++)
    {
        snprintf(stringaTmp, sizeof(stringaTmp), "%d", lista.processiPList[i].id);
        if (write(fdOutputP[1], stringaTmp, strlen(stringaTmp) + 1) != -1)
        {
            kill(lista.processiPList[i - 1].pid, SIGUSR1);
        }
        if (read(fdInputP[0], stringaTmp, 100) != -1)
        {
            lista.processiPList[i].pid = atoi(stringaTmp);
        }
    }

    printf("Ho rimosso il processo P in posizione %d\n", lista.processoSelezionato);
    pthread_create(&threadProcessiP, NULL, leggiDallaPipeDaiP, NULL);
}

int listAdd(int pid, int id)
{
    if (lista.currentIndex >= lista.sizeArray)
    {
        struct processoP *tmp;
        lista.sizeArray = lista.sizeArray * 2;
        tmp = realloc(lista.processiPList, lista.sizeArray * sizeof(struct processoP));
        if (tmp == NULL)
        {
            return -1;
        }
        else
        {
            lista.processiPList = tmp;
            lista.processiPList[lista.currentIndex].pid = pid;
            lista.processiPList[lista.currentIndex].id = id;
            ++lista.currentIndex;
            ++lista.counterId;
            return 0;
        }
    }
    else
    {
        lista.processiPList[lista.currentIndex].pid = pid;
        lista.processiPList[lista.currentIndex].id = id;
        ++lista.currentIndex;
        ++lista.counterId;
        return 0;
    }
}

int listDelete(int pos)
{
    if (pos < lista.currentIndex)
    {
        int i;
        for (i = pos; i < lista.currentIndex; i++)
        {
            lista.processiPList[i] = lista.processiPList[i + 1];
        }
        --lista.currentIndex;
        return 0;
    }
    else
    {
        return -1;
    }
}

void creaPipe(int pipeIn[2], int flags)
{
    if (pipe2(pipeIn, flags) == -1)
    {
        fprintf(stderr, "Errore nella creazione della pipe\n");
        esci(1);
    }
}

void inizializzaLista()
{
    lista.sizeArray = 10;
    lista.processoSelezionato = 0;
    lista.processiPList = malloc(lista.sizeArray * sizeof(struct processoP));
}

void sigTermHandler(int signum)
{
    printf("\nProcessi eliminati. Programma terminato!\n\n");
    exit(0);
}

void sigSegmentation(int signum)
{
    // printf("ATTENZIONE. Un processo utile al funzionamento è stato eliminato. Chiusura forzata!\n");
    // Termino tutti i sottoprocessi S -> e tutti i sottoprocessi L e T.
    kill(-getpid(), SIGTERM);
}

void esci(int codiceUscita)
{
    // Termino tutti i sottoprocessi S -> e tutti i sottoprocessi L e T.
    kill(-getpid(), SIGTERM);
}
