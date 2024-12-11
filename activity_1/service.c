#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>

#define TAM_FILA 100
#define LNG_FILE "LNG.txt"
#define SEM_BLOCK_NAME "/sem_block"

pid_t pid_analista;
long ultimo_analista = 0;

// Estrutura para representar um cliente
typedef struct {
    int pid;
    long chegada;
    int prioridade;
    long tempo_para_atendimento;
} Cliente;

// Fila Circular
Cliente fila[TAM_FILA];
int inicio = 0, fim = 0, tamanho = 0;

// Semáforos
sem_t sem_fila;
sem_t sem_espaco;
sem_t sem_clientes;
sem_t *sem_atend;
sem_t *sem_block;

// Funções Auxiliares
void insere_fila(Cliente c) {
    fila[fim] = c;
    fim = (fim + 1) % TAM_FILA;
    tamanho++;
}

Cliente remove_fila() {
    Cliente c = fila[inicio];
    inicio = (inicio + 1) % TAM_FILA;
    tamanho--;
    return c;
}

long timestamp_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

int ler_demanda() {

    // Aguarda o semáforo \sem_atend antes de acessar o arquivo
    // sem_wait(sem); 

    FILE *arquivo = fopen("demanda.txt", "r");
    if (arquivo == NULL) {
        perror("Erro ao abrir demanda.txt");
        exit(1); // Encerra o programa em caso de erro
    }

    int tempo_paciencia;
    fscanf(arquivo, "%d", &tempo_paciencia); // Lê o tempo gerado pelo cliente
    fclose(arquivo);
    remove("demanda.txt"); // Remove o arquivo após a leitura

    return tempo_paciencia;
}


// Thread 1 - Atendente
void *atendimento(void *args) {
    while (1) {
        sem_wait(&sem_clientes);

        sem_wait(&sem_fila);

        printf("Atendendo cliente...\n");
        Cliente cliente = remove_fila();
        sem_post(&sem_fila);

        kill(cliente.pid, SIGCONT);

        sem_wait(sem_atend);
        sem_wait(sem_block);

        long tempo_atual = timestamp_ms();
        int satisfeito = (tempo_atual - cliente.chegada) <= cliente.tempo_para_atendimento;

        FILE *arquivo = fopen(LNG_FILE, "a");
        if (arquivo != NULL) {
            fprintf(arquivo, "Cliente %d, Satisfeito: %d\n", cliente.pid, satisfeito);
            fclose(arquivo);
        }

        sem_post(sem_block);
        printf("Atendimento do cliente %d concluído.\n", cliente.pid);
        
        // Verifica se já passaram 500ms desde a última vez que o analista foi acordado
        if (timestamp_ms() - ultimo_analista >= 500) {
            printf("Acordando analista...\n");
            kill(pid_analista, SIGCONT); // Envia sinal para o analista
            ultimo_analista = timestamp_ms(); // Atualiza o momento do último acionamento
        }
    }
}

void salvar_fila(const char *filename) {
    FILE *arquivo = fopen(filename, "w");
    if (arquivo == NULL) {
        perror("Erro ao abrir arquivo para salvar a fila");
        return;
    }

    sem_wait(&sem_fila); // Garante acesso exclusivo à fila
    int pos = inicio;
    for (int i = 0; i < tamanho; i++) {
        Cliente cliente = fila[pos];
        fprintf(arquivo, "Cliente PID: %d, Chegada: %ld, Prioridade: %d, Tempo para Atendimento: %ld\n",
                cliente.pid, cliente.chegada, cliente.prioridade, cliente.tempo_para_atendimento);
        pos = (pos + 1) % TAM_FILA;
    }
    sem_post(&sem_fila); // Libera o acesso à fila

    fclose(arquivo);
    printf("Fila salva no arquivo: %s\n", filename);
}

// Thread 2 - Recepção
void *recepcao(void *args) {
    int N = *((int *)args);
    int count = 0;

    while (N == 0 || count < N) {
        sem_wait(&sem_espaco);

        pid_t pid = fork();
        printf("Cliente %d criado com PID: %d\n", count, pid);
        if (pid == 0) {
            execl("./client", "./client", NULL);
            perror("Erro ao executar client");
            exit(1);
        } else if (pid > 0) {
            printf("Recepção: Esperando semáforo \\sem_atend\n");
            sem_wait(sem_atend);
            printf("Recepção: Semáforo \\sem_atend liberado\n");

            int tempo_demanda = ler_demanda();
            printf("Recepção: Tempo lido do arquivo demanda.txt: %d\n", tempo_demanda);
            
            Cliente cliente;
            cliente.pid = pid;
            cliente.chegada = timestamp_ms();
            cliente.prioridade = (rand() % 2 == 0) ? 1 : 0;
            cliente.tempo_para_atendimento = (cliente.prioridade == 1) ? tempo_demanda/2 : tempo_demanda;

            sem_wait(&sem_fila);
            insere_fila(cliente);
            sem_post(&sem_fila);

            sem_post(&sem_clientes);
            count++;
        }
    }
    salvar_fila("fila_final.txt");
    pthread_exit(NULL);
}

// Processo Analista
void start_analista() {
    pid_t pid = fork();
    if (pid == 0) {
        execl("./analista", "./analista", NULL);
        perror("Erro ao executar analista");
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s <N>\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]);
    srand(time(NULL));

    // Inicializar Semáforos
    sem_init(&sem_fila, 0, 1);
    sem_init(&sem_espaco, 0, TAM_FILA);
    sem_init(&sem_clientes, 0, 0);
    sem_atend = sem_open("/sem_atend", O_CREAT, 0644, 1);
    sem_block = sem_open(SEM_BLOCK_NAME, O_CREAT, 0644, 1);

    // Criar o processo Analista
    start_analista();

    // Criar threads
    pthread_t th_recepcao, th_atendimento;
    pthread_create(&th_recepcao, NULL, recepcao, &N);
    pthread_create(&th_atendimento, NULL, atendimento, NULL);

    // Aguardar threads
    pthread_join(th_recepcao, NULL);
    pthread_cancel(th_atendimento);

    // Destruir Semáforos
    sem_destroy(&sem_fila);
    sem_destroy(&sem_espaco);
    sem_destroy(&sem_clientes);
    sem_close(sem_atend);
    sem_close(sem_block);
    sem_unlink(SEM_BLOCK_NAME);

    return 0;
}
