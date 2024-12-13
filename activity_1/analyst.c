#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
#include <fcntl.h>
#include <string.h>

#define LNG_FILE "LNG.txt"

sem_t *sem_analyst_ready;

void process_lng_file() {
    FILE *file = fopen(LNG_FILE, "r+"); // Abre o arquivo para leitura e escrita
    if (file == NULL) {
        perror("Erro ao abrir LNG");
        return;
    }

    char buffer[1024]; // Buffer para armazenar a leitura do arquivo
    char restante[1024]; // Buffer para armazenar os dados restantes
    int count = 0;

    restante[0] = '\0'; // Inicializa como string vazia

    printf("Valores lidos do arquivo LNG:\n");
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        if (count < 10) {
            // Imprime os primeiros 10 valores no console
            printf("%s", buffer);
            count++;
        } else {
            // Armazena os valores restantes no buffer
            strcat(restante, buffer);
        }
    }

    // Trunca o arquivo e grava os valores restantes
    freopen(LNG_FILE, "w", file); // Reabre o arquivo para truncar
    fprintf(file, "%s", restante); // Escreve apenas os valores restantes
    fclose(file);
}

void handle_signal(int signal) {
    if (signal == SIGCONT) {
        printf("Analista: Recebi sinal SIGCONT e estou acordando.\n");
    }
}

int main() {
    sem_analyst_ready = sem_open("/sem_analyst_ready", O_CREAT, 0644, 0);

    signal(SIGCONT, handle_signal);
    // Escreve o PID do processo analista
    printf("ANALYST: Processo Analista iniciado. PID: %d\n", getpid());

    // Cria ou abre o semáforo \sem_block
    sem_t *sem_block = sem_open("/sem_block", O_CREAT, 0644, 1);
    if (sem_block == SEM_FAILED) {
        perror("Erro ao criar o semáforo sem_block");
        exit(1);
    }
    sem_t *sem_clientes = sem_open("/sem_clientes", O_CREAT, 0644, 0);
    if (sem_clientes == SEM_FAILED) {
        perror("Erro ao criar o semáforo sem_clientes");
        exit(1);
    }
    while (1) {
        // Dorme até ser acordado
        printf("ANALYST: Analista dormindo...\n");

        raise(SIGSTOP); // Parando aqui
        printf("ANALYST: Analista recebeu um sinal. Desbloqueando...\n");

        printf("ANALYST: Antes de bloquear sem_block\n");
        sem_wait(sem_block);
        printf("ANALYST: Semáforo sem_block bloqueado\n");

        process_lng_file();

        sem_post(sem_block);
        printf("ANALYST: Semáforo sem_block liberado\n");


        // Volta a dormir
        printf("ANALYST: Analista voltou a dormir...\n");
    }

    // Fecha e remove o semáforo (nunca será alcançado neste caso)
    sem_close(sem_block);
    sem_unlink("/sem_block");
    sem_close(sem_analyst_ready);
    sem_unlink("/sem_analyst_ready");

    return 0;
}
