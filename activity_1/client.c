#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
#include <fcntl.h>

int main()
{
    int tempo;
    srand( (unsigned)time(NULL) );
    int x = rand() % 10;

    if (x==0)
        tempo = 15;
    else if (x > 0 && x <= 3)
        tempo = 5;
    else
        tempo = 1;

    sem_t *sem = sem_open("/sem_atend", O_CREAT, 0644, 0);
    if (sem == SEM_FAILED) {
        perror("Cliente: Erro ao abrir semáforo \\sem_atend");
        exit(1);
    }

    // Escreve no arquivo demanda.txt
    FILE *demanda = fopen("demanda.txt", "w+");
    if (demanda == NULL) {
        perror("Cliente: Erro ao criar demanda.txt");
        exit(1);
    }
    fprintf(demanda, "%d", tempo);
    fclose(demanda);
    printf("Cliente: Arquivo demanda.txt criado\n");

    // Libera o semáforo \sem_atend
    sem_post(sem);
    printf("Cliente: Semáforo \\sem_atend liberado\n");
    return 0;
}