/*
 * Contagem paralela de objetos (componentes conexos com conectividade 8)
 * em uma matriz binaria, usando Pthreads.
 *
 * Estrategia de decomposicao: a matriz e dividida em uma grade de
 * block_linhas x block_colunas blocos retangulares (uma thread por bloco).
 * Cada thread roda flood fill de forma totalmente independente, apenas
 * dentro dos limites do seu proprio bloco, escrevendo unicamente em sua
 * fatia exclusiva da matriz de rotulos - por isso nao ha secao critica
 * nem espera entre threads durante a etapa pesada de processamento.
 *
 * Depois que todas as threads terminam (pthread_join), uma etapa
 * sequencial de baixo custo (proporcional ao perimetro dos blocos, nao a
 * area da matriz) percorre as fronteiras horizontais e verticais entre
 * blocos adjacentes e usa uniao-busca (union-find) para unificar objetos
 * que atravessam essas fronteiras - incluindo o caso de encontro de
 * quatro blocos com conectividade diagonal.
 *
 * Uso: conta-objetos-paralelo <arquivo_matriz> <blocos_linha> <blocos_coluna>
 */
#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include "matriz_io.h"

typedef struct {
    int r;
    int c;
} Ponto;

typedef struct {
    int **matriz;
    int **rotulo;
    int linha_ini, linha_fim;
    int coluna_ini, coluna_fim;
    int rotulo_base;
    int rotulos_criados; /* saida: quantos componentes locais a thread encontrou */
} ArgBloco;

/*
 * Flood fill iterativo (pilha explicita, sem recursao) restrito aos
 * limites [linha_ini,linha_fim) x [coluna_ini,coluna_fim) do bloco.
 */
static void inunda_bloco(int **matriz, int **rotulo,
                          int linha_ini, int linha_fim,
                          int coluna_ini, int coluna_fim,
                          int r0, int c0, int rotulo_atual, Ponto *pilha)
{
    static const int dr[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
    static const int dc[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
    int topo;
    int i;

    topo = 0;
    pilha[topo].r = r0;
    pilha[topo].c = c0;
    topo++;
    rotulo[r0][c0] = rotulo_atual;

    while (topo > 0) {
        Ponto atual;
        topo--;
        atual = pilha[topo];

        for (i = 0; i < 8; i++) {
            int nr = atual.r + dr[i];
            int nc = atual.c + dc[i];

            if (nr < linha_ini || nr >= linha_fim || nc < coluna_ini || nc >= coluna_fim) {
                continue;
            }
            if (matriz[nr][nc] == 1 && rotulo[nr][nc] == 0) {
                rotulo[nr][nc] = rotulo_atual;
                pilha[topo].r = nr;
                pilha[topo].c = nc;
                topo++;
            }
        }
    }
}

/* Funcao executada por cada thread: rotula todos os componentes locais do bloco. */
static void *processa_bloco(void *arg)
{
    ArgBloco *b = (ArgBloco *) arg;
    Ponto *pilha;
    int capacidade;
    int i, j;
    int contador_local;

    capacidade = (b->linha_fim - b->linha_ini) * (b->coluna_fim - b->coluna_ini);
    pilha = (Ponto *) malloc((size_t) capacidade * sizeof(Ponto));
    if (pilha == NULL) {
        fprintf(stderr, "Erro: falha ao alocar pilha de flood fill na thread\n");
        b->rotulos_criados = 0;
        return NULL;
    }

    contador_local = 0;
    for (i = b->linha_ini; i < b->linha_fim; i++) {
        for (j = b->coluna_ini; j < b->coluna_fim; j++) {
            if (b->matriz[i][j] == 1 && b->rotulo[i][j] == 0) {
                int rotulo_id = b->rotulo_base + contador_local;
                inunda_bloco(b->matriz, b->rotulo, b->linha_ini, b->linha_fim,
                             b->coluna_ini, b->coluna_fim, i, j, rotulo_id, pilha);
                contador_local++;
            }
        }
    }

    free(pilha);
    b->rotulos_criados = contador_local;
    return NULL;
}

/* Uniao-busca com compressao de caminho (por halving) e uniao por tamanho. */
static int encontra(int *pai, int x)
{
    while (pai[x] != x) {
        pai[x] = pai[pai[x]];
        x = pai[x];
    }
    return x;
}

static void uniao(int *pai, int *tamanho, int a, int b)
{
    int ra, rb;

    ra = encontra(pai, a);
    rb = encontra(pai, b);
    if (ra == rb) {
        return;
    }
    if (tamanho[ra] < tamanho[rb]) {
        int tmp = ra;
        ra = rb;
        rb = tmp;
    }
    pai[rb] = ra;
    tamanho[ra] += tamanho[rb];
}

static double decorrido_segundos(struct timespec inicio, struct timespec fim)
{
    double s = (double) (fim.tv_sec - inicio.tv_sec);
    double ns = (double) (fim.tv_nsec - inicio.tv_nsec);
    return s + ns / 1e9;
}

int main(int argc, char **argv)
{
    int **matriz;
    int **rotulo;
    int linhas, colunas;
    int block_linhas, block_colunas;
    int total_blocos;
    int *linha_ini_arr;
    int *coluna_ini_arr;
    int *base;
    int *pai;
    int *tamanho;
    pthread_t *threads;
    ArgBloco *args;
    int br, bc, idx;
    int objetos;
    struct timespec t_inicio, t_fim;
    int rc;

    if (argc != 4) {
        fprintf(stderr, "Uso: %s <arquivo_matriz> <blocos_linha> <blocos_coluna>\n", argv[0]);
        return EXIT_FAILURE;
    }

    block_linhas = atoi(argv[2]);
    block_colunas = atoi(argv[3]);

    if (block_linhas < 1 || block_colunas < 1) {
        fprintf(stderr, "Erro: numero de blocos deve ser >= 1 em cada dimensao\n");
        return EXIT_FAILURE;
    }

    if (carrega_matriz(argv[1], &matriz, &linhas, &colunas) != 0) {
        return EXIT_FAILURE;
    }

    if (block_linhas > linhas) {
        block_linhas = linhas;
    }
    if (block_colunas > colunas) {
        block_colunas = colunas;
    }

    total_blocos = block_linhas * block_colunas;
    if (total_blocos < 2) {
        fprintf(stderr, "Aviso: apenas %d bloco(s)/thread(s) - paralelismo efetivo requer >= 2\n",
                total_blocos);
    }

    rotulo = aloca_matriz(linhas, colunas);
    if (rotulo == NULL) {
        fprintf(stderr, "Erro: falha ao alocar matriz de rotulos\n");
        libera_matriz(matriz);
        return EXIT_FAILURE;
    }
    {
        int i, j;
        for (i = 0; i < linhas; i++) {
            for (j = 0; j < colunas; j++) {
                rotulo[i][j] = 0;
            }
        }
    }

    linha_ini_arr = (int *) malloc((size_t) (block_linhas + 1) * sizeof(int));
    coluna_ini_arr = (int *) malloc((size_t) (block_colunas + 1) * sizeof(int));
    base = (int *) malloc((size_t) (total_blocos + 1) * sizeof(int));
    threads = (pthread_t *) malloc((size_t) total_blocos * sizeof(pthread_t));
    args = (ArgBloco *) malloc((size_t) total_blocos * sizeof(ArgBloco));

    if (linha_ini_arr == NULL || coluna_ini_arr == NULL || base == NULL ||
        threads == NULL || args == NULL) {
        fprintf(stderr, "Erro: falha ao alocar estruturas auxiliares\n");
        free(linha_ini_arr);
        free(coluna_ini_arr);
        free(base);
        free(threads);
        free(args);
        libera_matriz(matriz);
        libera_matriz(rotulo);
        return EXIT_FAILURE;
    }

    /* Divide as linhas em block_linhas faixas o mais equilibradas possivel. */
    {
        int base_h = linhas / block_linhas;
        int resto_h = linhas % block_linhas;
        int i;
        linha_ini_arr[0] = 0;
        for (i = 0; i < block_linhas; i++) {
            int tam = base_h + (i < resto_h ? 1 : 0);
            linha_ini_arr[i + 1] = linha_ini_arr[i] + tam;
        }
    }
    /* Divide as colunas em block_colunas faixas o mais equilibradas possivel. */
    {
        int base_w = colunas / block_colunas;
        int resto_w = colunas % block_colunas;
        int i;
        coluna_ini_arr[0] = 0;
        for (i = 0; i < block_colunas; i++) {
            int tam = base_w + (i < resto_w ? 1 : 0);
            coluna_ini_arr[i + 1] = coluna_ini_arr[i] + tam;
        }
    }

    /* Reserva uma faixa de rotulos exclusiva por bloco (rotulo_base), para
     * que threads nunca gerem o mesmo id sem qualquer sincronizacao. */
    base[0] = 1; /* rotulo 0 = "sem objeto" */
    for (br = 0; br < block_linhas; br++) {
        for (bc = 0; bc < block_colunas; bc++) {
            int capacidade;
            idx = br * block_colunas + bc;
            args[idx].matriz = matriz;
            args[idx].rotulo = rotulo;
            args[idx].linha_ini = linha_ini_arr[br];
            args[idx].linha_fim = linha_ini_arr[br + 1];
            args[idx].coluna_ini = coluna_ini_arr[bc];
            args[idx].coluna_fim = coluna_ini_arr[bc + 1];
            args[idx].rotulos_criados = 0;
            capacidade = (args[idx].linha_fim - args[idx].linha_ini) *
                         (args[idx].coluna_fim - args[idx].coluna_ini);
            args[idx].rotulo_base = base[idx];
            base[idx + 1] = base[idx] + capacidade;
        }
    }

    if (clock_gettime(CLOCK_MONOTONIC, &t_inicio) != 0) {
        perror("clock_gettime");
    }

    for (idx = 0; idx < total_blocos; idx++) {
        rc = pthread_create(&threads[idx], NULL, processa_bloco, &args[idx]);
        if (rc != 0) {
            fprintf(stderr, "Erro critico: falha ao criar thread %d (codigo %d)\n", idx, rc);
            exit(EXIT_FAILURE);
        }
    }

    for (idx = 0; idx < total_blocos; idx++) {
        rc = pthread_join(threads[idx], NULL);
        if (rc != 0) {
            fprintf(stderr, "Erro critico: falha ao aguardar thread %d (codigo %d)\n", idx, rc);
            exit(EXIT_FAILURE);
        }
    }

    pai = (int *) malloc((size_t) base[total_blocos] * sizeof(int));
    tamanho = (int *) malloc((size_t) base[total_blocos] * sizeof(int));
    if (pai == NULL || tamanho == NULL) {
        fprintf(stderr, "Erro: falha ao alocar estruturas de uniao-busca\n");
        exit(EXIT_FAILURE);
    }

    for (idx = 0; idx < total_blocos; idx++) {
        int k;
        for (k = 0; k < args[idx].rotulos_criados; k++) {
            int id = args[idx].rotulo_base + k;
            pai[id] = id;
            tamanho[id] = 1;
        }
    }

    /* Fronteiras verticais: consolida objetos entre colunas de blocos adjacentes. */
    for (bc = 0; bc < block_colunas - 1; bc++) {
        int col_esq = coluna_ini_arr[bc + 1] - 1;
        int col_dir = coluna_ini_arr[bc + 1];
        int r, dr;
        for (r = 0; r < linhas; r++) {
            for (dr = -1; dr <= 1; dr++) {
                int rr = r + dr;
                if (rr < 0 || rr >= linhas) {
                    continue;
                }
                if (matriz[r][col_esq] == 1 && matriz[rr][col_dir] == 1 &&
                    rotulo[r][col_esq] != 0 && rotulo[rr][col_dir] != 0) {
                    uniao(pai, tamanho, rotulo[r][col_esq], rotulo[rr][col_dir]);
                }
            }
        }
    }

    /* Fronteiras horizontais: consolida objetos entre linhas de blocos adjacentes.
     * A janela de +-1 coluna aqui, combinada com a janela de +-1 linha acima,
     * cobre tambem o encontro diagonal de quatro blocos (ex.: canto inferior
     * direito de um bloco conectado ao canto superior esquerdo do bloco
     * diagonalmente adjacente). */
    for (br = 0; br < block_linhas - 1; br++) {
        int linha_cima = linha_ini_arr[br + 1] - 1;
        int linha_baixo = linha_ini_arr[br + 1];
        int c, dc;
        for (c = 0; c < colunas; c++) {
            for (dc = -1; dc <= 1; dc++) {
                int cc = c + dc;
                if (cc < 0 || cc >= colunas) {
                    continue;
                }
                if (matriz[linha_cima][c] == 1 && matriz[linha_baixo][cc] == 1 &&
                    rotulo[linha_cima][c] != 0 && rotulo[linha_baixo][cc] != 0) {
                    uniao(pai, tamanho, rotulo[linha_cima][c], rotulo[linha_baixo][cc]);
                }
            }
        }
    }

    if (clock_gettime(CLOCK_MONOTONIC, &t_fim) != 0) {
        perror("clock_gettime");
    }

    /* Conta quantas raizes distintas restaram entre os rotulos locais criados. */
    {
        char *visto;
        visto = (char *) calloc((size_t) base[total_blocos], sizeof(char));
        if (visto == NULL) {
            fprintf(stderr, "Erro: falha ao alocar marcador de raizes\n");
            exit(EXIT_FAILURE);
        }
        objetos = 0;
        for (idx = 0; idx < total_blocos; idx++) {
            int k;
            for (k = 0; k < args[idx].rotulos_criados; k++) {
                int id = args[idx].rotulo_base + k;
                int raiz = encontra(pai, id);
                if (!visto[raiz]) {
                    visto[raiz] = 1;
                    objetos++;
                }
            }
        }
        free(visto);
    }

    printf("Dimensoes: %d x %d\n", linhas, colunas);
    printf("Blocos: %d x %d (%d threads)\n", block_linhas, block_colunas, total_blocos);
    printf("Objetos: %d\n", objetos);
    printf("Tempo (s): %.6f\n", decorrido_segundos(t_inicio, t_fim));

    free(pai);
    free(tamanho);
    free(linha_ini_arr);
    free(coluna_ini_arr);
    free(base);
    free(threads);
    free(args);
    libera_matriz(matriz);
    libera_matriz(rotulo);

    return EXIT_SUCCESS;
}
