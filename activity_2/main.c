#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define DISK_FILE "virtual_disk.img"
#define DISK_SIZE (1L << 30) // 1GB
#define MAX_FILENAME_LEN 32
#define MAX_FILES 1000
#define BLOCK_SIZE (512 * 1024) // 512KB por bloco
#define HUGE_PAGE_SIZE (2 * 1024 * 1024) // 2MB
#define PAGE_SIZE (HUGE_PAGE_SIZE / sizeof(uint32_t)) 


typedef struct {
    char name[MAX_FILENAME_LEN];
    uint32_t size;
    uint32_t offset;
    int valid;
} FileEntry;

typedef struct {
    FileEntry files[MAX_FILES];
    uint32_t file_count;
    uint32_t free_offset;
} FileSystem;

FileSystem fs;

void init_fs() {
    FILE *disk = fopen(DISK_FILE, "rb");
    if (!disk) {
        disk = fopen(DISK_FILE, "wb");
        if (!disk || ftruncate(fileno(disk), DISK_SIZE) == -1) {
            perror("Erro ao definir tamanho do disco virtual");
            if (disk) fclose(disk);
            return;
        }
        fclose(disk);
        memset(&fs, 0, sizeof(fs));
    } else {
        if (fread(&fs, sizeof(fs), 1, disk) != 1) {
            perror("Erro ao ler o sistema de arquivos");
        }
        fclose(disk);
    }
}

void save_fs() {
    FILE *disk = fopen(DISK_FILE, "rb+");
    if (disk) {
        fwrite(&fs, sizeof(fs), 1, disk);
        fclose(disk);
    }
}

void create_file(const char *name, uint32_t count) {
    if (fs.file_count >= MAX_FILES) {
        printf("Limite de arquivos atingido.\n");
        return;
    }

    for (uint32_t i = 0; i < fs.file_count; i++) {
        if (fs.files[i].valid && strcmp(fs.files[i].name, name) == 0) {
            printf("Erro: O arquivo '%s' ja  existe.\n", name);
            return;
        }
    }

    uint32_t required_space = count * sizeof(uint32_t);
    uint32_t available_space = DISK_SIZE - sizeof(fs) - fs.free_offset;

    if (required_space > available_space) {
        printf("Espaco insuficiente para criar o arquivo '%s'. Necessario: %u bytes, Disponivel: %u bytes.\n",
               name, required_space, available_space);
        return;
    }

    // ✅ 1️⃣ Verifica se há um espaço "vazio" reutilizável
    for (uint32_t i = 0; i < fs.file_count; i++) {
        if (!fs.files[i].valid && fs.files[i].size >= count) {
            printf("Reutilizando espaco do arquivo deletado...\n");
            strncpy(fs.files[i].name, name, MAX_FILENAME_LEN);
            fs.files[i].valid = 1;
            fs.files[i].size = count;
            save_fs();
            return;
        }
    }

    // ✅ 2️⃣ Se não houver espaço reaproveitável, cria um novo arquivo normalmente
    FILE *disk = fopen(DISK_FILE, "rb+");
    if (!disk) {
        perror("Erro ao abrir disco");
        return;
    }

    fseek(disk, fs.free_offset + sizeof(fs), SEEK_SET);
    for (uint32_t i = 0; i < count; i++) {
        uint32_t num = rand();
        fwrite(&num, sizeof(num), 1, disk);
    }

    FileEntry *file = &fs.files[fs.file_count++];
    strncpy(file->name, name, MAX_FILENAME_LEN);
    file->size = count;
    file->offset = fs.free_offset;
    file->valid = 1;

    fs.free_offset += required_space;
    fclose(disk);
    save_fs();
    printf("Arquivo '%s' criado com %u numeros.\n", name, count);
}


int compare(const void *a, const void *b) {
    return (*(uint32_t*)a - *(uint32_t*)b);
}

void delete_file(const char *name) {
  for (uint32_t i = 0; i < fs.file_count; i++) {
    if (fs.files[i].valid && strcmp(fs.files[i].name, name) == 0) {
      fs.files[i].valid = 0;
      save_fs();
      printf("Arquivo '%s' apagado.\n", name);
      return;
    }
  }
  printf("Arquivo '%s' nao encontrado.\n", name);
}

void sort_file(const char *name) {
    clock_t start_time = clock();

    for (uint32_t i = 0; i < fs.file_count; i++) {
        if (fs.files[i].valid && strcmp(fs.files[i].name, name) == 0) {
            uint32_t file_size = fs.files[i].size * sizeof(uint32_t);

            if (file_size == 0) {
                printf("Arquivo '%s' está vazio.\n", name);
                return;
            }

            int fd = open(DISK_FILE, O_RDWR);
            if (fd == -1) {
                perror("Erro ao abrir o disco virtual");
                return;
            }

            uint32_t *buffer = mmap(NULL, HUGE_PAGE_SIZE, PROT_READ | PROT_WRITE,
                                    MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
            if (buffer == MAP_FAILED) {
                perror("Erro ao alocar Huge Page");
                close(fd);
                return;
            }

            printf("Usando Huge Page para ordenação...\n");

            uint32_t swap_offset = fs.free_offset; // Swap começa da última posição usada
            uint32_t required_swap_space = file_size; // Precisamos de espaço para armazenar todo o arquivo ordenado
            uint32_t available_space = DISK_SIZE - sizeof(fs) - fs.free_offset;

            if (required_swap_space > available_space) {
                printf("Erro: Espaço insuficiente para ordenação do arquivo '%s'.\n", name);
                printf("É necessário pelo menos %u bytes livres, mas apenas %u bytes estão disponíveis.\n",
                       required_swap_space, available_space);
                printf("Tente excluir arquivos para liberar espaço antes de ordenar.\n");
                munmap(buffer, HUGE_PAGE_SIZE);
                close(fd);
                return;
            }

            uint32_t num_pages = (file_size + HUGE_PAGE_SIZE - 1) / HUGE_PAGE_SIZE;

            FILE *disk = fdopen(fd, "rb+");
            if (!disk) {
                perror("Erro ao abrir disco");
                munmap(buffer, HUGE_PAGE_SIZE);
                close(fd);
                return;
            }

            // 🔄 Passo 1: Ler, ordenar e salvar no swap
            for (uint32_t page = 0; page < num_pages; page++) {
                uint32_t start = fs.files[i].offset + sizeof(fs) + page * HUGE_PAGE_SIZE;
                fseek(disk, start, SEEK_SET);

                size_t num_elements = fread(buffer, sizeof(uint32_t), PAGE_SIZE, disk);
                if (num_elements == 0) break;

                qsort(buffer, num_elements, sizeof(uint32_t), compare);

                // Escrever no espaço de swap dentro do disco virtual
                fseek(disk, swap_offset + page * HUGE_PAGE_SIZE, SEEK_SET);
                fwrite(buffer, sizeof(uint32_t), num_elements, disk);
            }

            for (uint32_t page = 0; page < num_pages; page++) {
                fseek(disk, swap_offset + page * HUGE_PAGE_SIZE, SEEK_SET);
                size_t num_elements = fread(buffer, sizeof(uint32_t), PAGE_SIZE, disk);
                if (num_elements == 0) break;

                fseek(disk, fs.files[i].offset + sizeof(fs) + page * HUGE_PAGE_SIZE, SEEK_SET);
                fwrite(buffer, sizeof(uint32_t), num_elements, disk);
            }

            munmap(buffer, HUGE_PAGE_SIZE);
            fclose(disk);
            close(fd);

            clock_t end_time = clock();
            double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;

            printf("Arquivo '%s' ordenado com sucesso usando swap interno no disco em %.6f segundos.\n", name, elapsed_time);
            return;
        }
    }
    printf("Arquivo '%s' não encontrado.\n", name);
}

void list_files() {
    uint32_t total_size = 0;
    printf("Arquivos no diretório:\n");
    for (uint32_t i = 0; i < fs.file_count; i++) {
        if (fs.files[i].valid) {
            printf("%s - %u bytes\n", fs.files[i].name, fs.files[i].size * (uint32_t)sizeof(uint32_t));
            total_size += fs.files[i].size * (uint32_t)sizeof(uint32_t);
        }
    }
    printf("Espaco total: %lu bytes\n", (unsigned long)DISK_SIZE);
    printf("Espaco disponivel: %lu bytes\n", (unsigned long)(DISK_SIZE - sizeof(fs) - fs.free_offset)); // Corrigido
}


void read_file(const char *name, uint32_t start, uint32_t end) {
    for (uint32_t i = 0; i < fs.file_count; i++) {
        if (fs.files[i].valid && strcmp(fs.files[i].name, name) == 0) {
            FILE *disk = fopen(DISK_FILE, "rb");
            if (!disk) {
                perror("Erro ao abrir o disco virtual");
                return;
            }
            fseek(disk, fs.files[i].offset + sizeof(fs) + start * sizeof(uint32_t), SEEK_SET);

            for (uint32_t j = start; j <= end && j < fs.files[i].size; j++) {
                uint32_t num;
                if (fread(&num, sizeof(num), 1, disk) != 1) {
                    perror("Erro ao ler do arquivo");
                    break;
                }
                printf("%u ", num);
            }
            printf("\n");
            fclose(disk);
            return;
        }
    }
    printf("Arquivo '%s' nao encontrado.\n", name);
}

void concatenate_files(const char *name1, const char *name2) {
    FileEntry *file1 = NULL, *file2 = NULL;
    for (uint32_t i = 0; i < fs.file_count; i++) {
        if (fs.files[i].valid) {
            if (strcmp(fs.files[i].name, name1) == 0)
                file1 = &fs.files[i];
            if (strcmp(fs.files[i].name, name2) == 0)
                file2 = &fs.files[i];
        }
    }

    if (!file1 || !file2) {
        printf("Erro: Um ou ambos os arquivos não foram encontrados.\n");
        return;
    }

    FILE *disk = fopen(DISK_FILE, "rb+");
    if (!disk) {
        perror("Erro ao abrir o disco");
        return;
    }

    uint32_t buffer[PAGE_SIZE];

    fseek(disk, file1->offset + sizeof(fs) + file1->size * sizeof(uint32_t), SEEK_SET);
    
    fseek(disk, file2->offset + sizeof(fs), SEEK_SET);
    size_t num_elements;
    while ((num_elements = fread(buffer, sizeof(uint32_t), PAGE_SIZE, disk)) > 0) {
        fwrite(buffer, sizeof(uint32_t), num_elements, disk);
    }

    file1->size += file2->size;
    file2->valid = 0;

    fclose(disk);
    save_fs();
    printf("Arquivos '%s' e '%s' concatenados com sucesso.\n", name1, name2);
}

int main() {
    srand(time(NULL));
    init_fs();

    char command[64], name1[MAX_FILENAME_LEN];
    char name2[MAX_FILENAME_LEN];
    uint32_t size, start, end;

    while (1) {
        printf("> ");
        if (scanf("%63s", command) != 1) continue;

        if (strcmp(command, "criar") == 0) {
            if (scanf("%31s %u", name1, &size) == 2)
                create_file(name1, size);
    	} else if (strcmp(command, "apagar") == 0) {
      	    if (scanf("%s", name1) == 0){
		printf("Erro ao ler nome do arquivo. \n");
		continue;
		}
      	    delete_file(name1); 
       } else if (strcmp(command, "listar") == 0) {
            list_files();
	} else if (strcmp(command, "ordenar") == 0) {
	    if (scanf("%31s", name1) == 1)
	    	sort_file(name1);
        } else if (strcmp(command, "ler") == 0) {
            if (scanf("%31s %u %u", name1, &start, &end) == 3)
                read_file(name1, start, end);
    	} else if (strcmp(command, "concatenar") == 0) {
            if (scanf("%31s %31s", name1, name2) == 2)
                concatenate_files(name1, name2);
        } else if (strcmp(command, "sair") == 0) {
            break;
        } else {
            printf("Comando desconhecido.\n");
        }
    }

    return 0;
}