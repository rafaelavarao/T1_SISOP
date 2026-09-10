#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define ROWS 6
#define COLS 6
#define NUM_THREADS 2

/* Matriz binaria de exemplo */
int matriz[ROWS][COLS] = {
    {1, 1, 0, 0, 0, 1},
    {1, 0, 0, 1, 1, 0},
    {0, 0, 0, 1, 1, 0},
    {0, 1, 1, 0, 0, 0},
    {0, 1, 1, 0, 1, 1},
    {0, 0, 0, 0, 1, 1}
};

/* Estrutura para diferenciar celulas visitadas das nao visitadas */
int visitado[ROWS][COLS];
pthread_mutex_t mutex_contador;
int total_objetos = 0;

/* Estrutura de dados para passar argumentos para as threads */
typedef struct {
    int thread_id;
    int start_row;
    int end_row;
} thread_arg_t;

/* Fila simples para busca em largura (BFS) */
typedef struct {
    int r;
    int c;
} Ponto;

void bfs(int start_r, int start_c) {
    Ponto fila[ROWS * COLS];
    int inicio = 0;
    int fim = 0;
    int dr[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
    int dc[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
    int i;

    fila[fim].r = start_r;
    fila[fim].c = start_c;
    fim++;
    visitado[start_r][start_c] = 1;

    while (inicio < fim) {
        Ponto atual = fila[inicio];
        inicio++;

        for (i = 0; i < 8; i++) {
            int nr = atual.r + dr[i];
            int nc = atual.c + dc[i];

            if (nr >= 0 && nr < ROWS && nc >= 0 && nc < COLS) {
                if (matriz[nr][nc] == 1 && !visitado[nr][nc]) {
                    visitado[nr][nc] = 1;
                    fila[fim].r = nr;
                    fila[fim].c = nc;
                    fim++;
                }
            }
        }
    }
}

/* Funcao executada por cada Pthread */
void *processar_linhas(void *arg) {
    thread_arg_t *t_arg = (thread_arg_t *)arg;
    int i, j;
    int contador_local = 0;

    for (i = t_arg->start_row; i < t_arg->end_row; i++) {
        for (j = 0; j < COLS; j++) {
            /* Se encontrou parte de um objeto nao visitado */
            if (matriz[i][j] == 1 && !visitado[i][j]) {
                /* Sincronizacao para garantir exclusividade ao marcar e contar componentes */
                pthread_mutex_lock(&mutex_contador);
                if (!visitado[i][j]) {
                    bfs(i, j);
                    contador_local++;
                }
                pthread_mutex_unlock(&mutex_contador);
            }
        }
    }

    pthread_exit(NULL);
}

int main(void) {
    pthread_t threads[NUM_THREADS];
    thread_arg_t args[NUM_THREADS];
    int i, j;
    int rc;
    int linhas_por_thread;

    /* Inicializacao da matriz de visitados */
    for (i = 0; i < ROWS; i++) {
        for (j = 0; j < COLS; j++) {
            visitado[i][j] = 0;
        }
    }

    /* Inicializacao do Mutex */
    if (pthread_mutex_init(&mutex_contador, NULL) != 0) {
        perror("Erro ao inicializar o mutex");
        exit(EXIT_FAILURE);
    }

    linhas_por_thread = ROWS / NUM_THREADS;

    /* Criacao das Pthreads com verificacao de retorno */
    for (i = 0; i < NUM_THREADS; i++) {
        args[i].thread_id = i;
        args[i].start_row = i * linhas_por_thread;
        if (i == NUM_THREADS - 1) {
            args[i].end_row = ROWS;
        } else {
            args[i].end_row = (i + 1) * linhas_por_thread;
        }

        rc = pthread_create(&threads[i], NULL, processar_linhas, (void *)&args[i]);
        if (rc != 0) {
            fprintf(stderr, "Erro critico: falha ao criar a thread %d\n", i);
            exit(EXIT_FAILURE);
        }
    }

    /* Juncao das Pthreads com verificacao de retorno */
    for (i = 0; i < NUM_THREADS; i++) {
        rc = pthread_join(threads[i], NULL);
        if (rc != 0) {
            fprintf(stderr, "Erro critico: falha ao aguardar a thread %d\n", i);
            exit(EXIT_FAILURE);
        }
    }

    /* Destruicao do Mutex */
    pthread_mutex_destroy(&mutex_contador);

    for (i = 0; i < ROWS; i++) {
        for (j = 0; j < COLS; j++) {
            if (matriz[i][j] == 1 && visitado[i][j]) {
            }
        }
    }
    printf("Quantidade total de objetos (componentes com conectividade 8): %d\n", total_objetos);

    return 0;
}