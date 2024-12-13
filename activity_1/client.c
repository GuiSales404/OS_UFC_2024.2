#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
#include <fcntl.h>

int main()
{

void inspecionar_e_ajustar_semaforo(sem_t *sem, const char *nome) {
    int valor;
    if (sem_getvalue(sem, &valor) == 0) {
        printf("Semáforo %s está com valor: %d\n", nome, valor);

        // Ajustar o valor do semáforo para 1, se necessário
        if (valor < 1) {
            // Incrementar até chegar a 1
            while (valor < 1) {
                sem_post(sem);
                valor++;
            }
            printf("Semáforo %s ajustado para valor: 1\n", nome);
        }
    } else {
        perror("Erro ao consultar semáforo");
    }
}


    int tempo;
    srand( (unsigned)time(NULL) );
    int x = rand() % 10;

    if (x==0)
        tempo = 15;
    else if (x > 0 && x <= 3)
        tempo = 5;
    else
        tempo = 1;

    // Escreve no arquivo demanda.txt
    FILE* demanda = fopen("demanda.txt", "w+");
    fprintf(demanda, "%d", tempo);
    fclose(demanda);
    printf("CLIENT: Arquivo demanda.txt criado com sucesso\n");

    // printf("CLIENT: Cliente bloqueado\n");
    // raise(SIGSTOP); // Parando aqui
    // printf("CLIENT: Cliente desbloqueado\n");

    printf("CLIENT: Abrindo semáforo sem_atend\n");
    sem_t *sem = sem_open("/sem_atend", O_RDWR);
    printf("CLIENT: Semáforo sem_atend aberto\n");
    inspecionar_e_ajustar_semaforo(sem, "/sem_atend client abriu");

    // printf("CLIENT: Aguardando semáforo sem_atend\n");
    // if(sem != SEM_FAILED) sem_wait(sem);
    // printf("CLIENT: sem_atend...\n");

    usleep(tempo);

    printf("CLIENT: sem_atend liberando\n");
    if(sem != SEM_FAILED) sem_post(sem);
    printf("CLIENT: sem_atend liberado\n");

    return 0;
}