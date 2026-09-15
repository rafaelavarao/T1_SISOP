/*
 * Contagem sequencial de objetos (componentes conexos com conectividade 8)
 * em uma matriz binaria. Serve como referencia de correcao e de desempenho
 * para a versao paralela (conta-objetos-paralelo.c).
 *
 * Uso: conta-objetos-sequencial <arquivo_matriz>
 */
#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "matriz_io.h"

typedef struct {
    int r;
    int c;
} Ponto;

/*
 * Preenchimento por inundacao (flood fill) iterativo com pilha explicita,
 * evitando recursao (item 41 do enunciado). `pilha` e um buffer reutilizavel
 * com capacidade para linhas*colunas pontos, suficiente para qualquer
 * componente unico da matriz.
 */
static void inunda(int **matriz, int **rotulo, int linhas, int colunas,
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

            if (nr < 0 || nr >= linhas || nc < 0 || nc >= colunas) {
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

/* Conta os objetos (componentes com conectividade 8) da matriz. */
static int conta_objetos(int **matriz, int **rotulo, int linhas, int colunas)
{
    Ponto *pilha;
    int contador;
    int i, j;

    pilha = (Ponto *) malloc((size_t) linhas * (size_t) colunas * sizeof(Ponto));
    if (pilha == NULL) {
        fprintf(stderr, "Erro: falha ao alocar pilha de flood fill\n");
        exit(EXIT_FAILURE);
    }

    contador = 0;
    for (i = 0; i < linhas; i++) {
        for (j = 0; j < colunas; j++) {
            if (matriz[i][j] == 1 && rotulo[i][j] == 0) {
                contador++;
                inunda(matriz, rotulo, linhas, colunas, i, j, contador, pilha);
            }
        }
    }

    free(pilha);
    return contador;
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
    int objetos;
    struct timespec t_inicio, t_fim;

    if (argc != 2) {
        fprintf(stderr, "Uso: %s <arquivo_matriz>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (carrega_matriz(argv[1], &matriz, &linhas, &colunas) != 0) {
        return EXIT_FAILURE;
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

    if (clock_gettime(CLOCK_MONOTONIC, &t_inicio) != 0) {
        perror("clock_gettime");
        libera_matriz(matriz);
        libera_matriz(rotulo);
        return EXIT_FAILURE;
    }

    objetos = conta_objetos(matriz, rotulo, linhas, colunas);

    if (clock_gettime(CLOCK_MONOTONIC, &t_fim) != 0) {
        perror("clock_gettime");
        libera_matriz(matriz);
        libera_matriz(rotulo);
        return EXIT_FAILURE;
    }

    printf("Dimensoes: %d x %d\n", linhas, colunas);
    printf("Objetos: %d\n", objetos);
    printf("Tempo (s): %.6f\n", decorrido_segundos(t_inicio, t_fim));

    libera_matriz(matriz);
    libera_matriz(rotulo);
    return EXIT_SUCCESS;
}
