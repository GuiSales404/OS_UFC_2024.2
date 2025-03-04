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

#define TAM_FILA 1000
#define LNG_FILE "LNG.txt"
#define SEM_BLOCK_NAME "/sem_block"

// Variáveis globais para métricas
int total_clientes = 0;
int clientes_satisfeitos = 0;
int clientes_insatisfeitos = 0;
long inicio_execucao;
sem_t sem_metrics; // Semáforo para proteger acesso às métricas

sem_t *sem_analyst_ready;

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

    FILE *arquivo;
    while ((arquivo = fopen("demanda.txt", "r")) == NULL) {
        printf("SERVICE: Arquivo demanda.txt não encontrado. Tentando novamente em 1 segundo...\n");
        sleep(0.8); // Espera por 1 segundo antes de tentar novamente
    }


    int tempo_paciencia;
    fscanf(arquivo, "%d", &tempo_paciencia); // Lê o tempo gerado pelo cliente
    fclose(arquivo);
    remove("demanda.txt"); // Remove o arquivo após a leitura

    return tempo_paciencia;
}


// Thread 1 - Atendente
void *atendimento(void *args) {
    pid_t pid_analista = *((pid_t *)args);

    while (1) {
        printf("SERVICE: Atendimento: Aguardando cliente...\n");
        sem_wait(&sem_clientes); // Espera até que haja clientes na fila

        printf("SERVICE: Atendimento: Aguardando liberação da fila\n");
        sem_wait(&sem_fila);

        Cliente cliente = remove_fila(); // Remove o cliente da fila
        printf("SERVICE: Atendimento: Cliente %d removido da fila\n", cliente.pid);
        sem_post(&sem_fila);

        sem_wait(sem_atend); // Sincronização de atendimento
        sem_wait(sem_block); // Bloqueia o acesso ao arquivo

        long tempo_atual = timestamp_ms();
        int satisfeito = (tempo_atual - cliente.chegada) <= cliente.tempo_para_atendimento;

        // Atualiza métricas protegidas pelo semáforo
        sem_wait(&sem_metrics);
        total_clientes++;
        if (satisfeito) {
            clientes_satisfeitos++;
        } else {
            clientes_insatisfeitos++;
        }

        // Calcula a taxa de satisfação e o tempo total de execução
        float taxa_satisfacao = (total_clientes > 0) ? 
                                ((float)clientes_satisfeitos / total_clientes) * 100 : 0.0;
        long tempo_total_execucao = timestamp_ms() - inicio_execucao;
        printf("--");
        printf("Tempo atual: %ld\n", timestamp_ms());
        printf("Tempo de chegada: %ld\n", inicio_execucao);
        printf("--");
        // Atualiza o arquivo de métricas
        FILE *metrics_file = fopen("metrics.txt", "w");
        if (metrics_file != NULL) {
            fprintf(metrics_file, "Total de clientes: %d\n", total_clientes);
            fprintf(metrics_file, "Clientes satisfeitos: %d\n", clientes_satisfeitos);
            fprintf(metrics_file, "Clientes insatisfeitos: %d\n", clientes_insatisfeitos);
            fprintf(metrics_file, "Taxa de satisfação: %.2f%%\n", taxa_satisfacao);
            fprintf(metrics_file, "Tempo total de execução: %ld ms\n", tempo_total_execucao);
            fclose(metrics_file);
        }
        sem_post(&sem_metrics);

        // Escreve os dados do cliente no arquivo
        FILE *arquivo = fopen(LNG_FILE, "a");
        if (arquivo != NULL) {
            fprintf(arquivo, "[                                                                            ] Cliente %d, Satisfeito: %d\n", cliente.pid, satisfeito);
            fclose(arquivo);
        }

        sem_post(sem_block); // Libera o acesso ao arquivo
        printf("SERVICE: Atendimento do cliente %d concluído.\n", cliente.pid);

        // Acorda o analista imediatamente após concluir o atendimento
        printf("SERVICE: Atendimento: Acordando analista de PID: %d...\n", pid_analista);
        sem_post(sem_analyst_ready); // Libera o analista
        kill(pid_analista, SIGCONT); // Envia o sinal SIGCONT para o analista
        printf("SERVICE: Atendimento: Enviando sinal SIGCONT para analista de PID %d\n", pid_analista);
    }
}


void inspecionar_semaforo(sem_t *sem, const char *nome) {
    int valor;
    if (sem_getvalue(sem, &valor) == 0) {
        printf("Semáforo %s está com valor: %d\n", nome, valor);
    } else {
        perror("Erro ao consultar semáforo");
    }
}

// Thread 2 - Recepção
void *recepcao(void *args) {
    int N = *((int *)args);
    int count = 0;

    printf("SERVICE: Recepção: Criando semáforo /sem_atend\n");
    sem_atend = sem_open("/sem_atend", O_CREAT, 0644, 0);
    if (sem_atend == SEM_FAILED) {
        perror("Erro ao criar semáforo /sem_atend");
        exit(1);
    }

    printf("SERVICE: Recepção: Criando semáforo /sem_block\n");
    sem_block = sem_open("/sem_block", O_CREAT, 0644, 1);
    if (sem_block == SEM_FAILED) {
        perror("Erro ao criar semáforo /sem_block");
        exit(1);
    }
    while (N == 0 || (N > 0 && count < N)) {
        printf("SERVICE: Recepção: Aguardando semáforo /sem_espaco\n");
        sem_wait(&sem_espaco);
        printf("SERVICE: Recepção:  Semáforo /sem_espaco informa que tem espaço\n");
        inspecionar_semaforo(sem_atend, "                      -                                      /sem_atend");
        pid_t pid = fork();
        if (pid > 0) {
            printf("SERVICE: Recepção: Cliente %d criado com PID: %d\n", count+1, pid);
        }
        if (pid == 0) {
            execl("./client", "./client", NULL);
            perror("Erro ao executar client");
            exit(1);
        } else if (pid > 0) {
            printf("SERVICE: Recepção: Esperando semáforo /sem_atend\n");
            sem_wait(sem_atend); 
            printf("SERVICE: Recepção: Semáforo /sem_atend liberado\n");

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
            if (N > 0){
                count++;
            }
            else {
                count = 0;
            }
        }
    }
    pthread_exit(NULL);
}

// Processo Analista
pid_t start_analista() {
    pid_t pid = fork();
    if (pid == 0) {
        execl("./analyst", "./analyst", NULL);
        perror("Erro ao executar analista");
        exit(1);
    }
    return pid;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s <N>\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]);
    srand(time(NULL));

    inicio_execucao = timestamp_ms();

    // Remover semáforos antigos
    sem_unlink("/sem_atend");
    sem_unlink("/sem_block");
    sem_unlink("/sem_analyst_ready");

    // Limpar arquivos
    FILE *file = fopen(LNG_FILE, "w");
    if (file) fclose(file);

    // Inicializar Semáforos
    sem_init(&sem_fila, 0, 1);
    sem_init(&sem_espaco, 0, TAM_FILA);
    sem_init(&sem_clientes, 0, 0);
    sem_init(&sem_metrics, 0, 1);
    sem_analyst_ready = sem_open("/sem_analyst_ready", O_CREAT, 0644, 0);

    // Criar o processo Analista
    pid_t pid_analista = start_analista();

    // Criar threads
    pthread_t th_recepcao, th_atendimento;
    pthread_create(&th_recepcao, NULL, recepcao, &N);
    pthread_create(&th_atendimento, NULL, atendimento, &pid_analista);

    // Aguardar threads
    pthread_join(th_recepcao, NULL);
    pthread_cancel(th_atendimento);

    // Destruir Semáforos
    sem_destroy(&sem_fila);
    sem_destroy(&sem_espaco);
    sem_destroy(&sem_clientes);
    sem_close(sem_atend);
    sem_close(sem_block);
    sem_unlink("/sem_block");
    sem_close(sem_analyst_ready);
    sem_unlink("/sem_analyst_ready");
    sem_destroy(&sem_metrics);


    kill(pid_analista, SIGTERM); // Envia um sinal para terminar o analista
    waitpid(pid_analista, NULL, 0); // Aguarda o processo analista finalizar

    return 0;
}
