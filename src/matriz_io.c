#include <stdio.h>
#include <stdlib.h>

#include "matriz_io.h"

int **aloca_matriz(int linhas, int colunas)
{
    int **matriz;
    int *dados;
    int i;

    if (linhas <= 0 || colunas <= 0) {
        return NULL;
    }

    dados = (int *) malloc((size_t) linhas * (size_t) colunas * sizeof(int));
    if (dados == NULL) {
        return NULL;
    }

    matriz = (int **) malloc((size_t) linhas * sizeof(int *));
    if (matriz == NULL) {
        free(dados);
        return NULL;
    }

    for (i = 0; i < linhas; i++) {
        matriz[i] = dados + ((size_t) i * (size_t) colunas);
    }

    return matriz;
}

void libera_matriz(int **matriz)
{
    if (matriz == NULL) {
        return;
    }
    /* matriz[0] aponta para o bloco contiguo alocado em aloca_matriz */
    free(matriz[0]);
    free(matriz);
}

int carrega_matriz(const char *caminho, int ***matriz, int *linhas, int *colunas)
{
    FILE *arquivo;
    int **m;
    int i, j;
    int valor;
    int total_lidos;

    arquivo = fopen(caminho, "r");
    if (arquivo == NULL) {
        fprintf(stderr, "Erro: nao foi possivel abrir o arquivo '%s'\n", caminho);
        return -1;
    }

    if (fscanf(arquivo, "%d %d", linhas, colunas) != 2) {
        fprintf(stderr, "Erro: cabecalho invalido em '%s' (esperado: linhas colunas)\n", caminho);
        fclose(arquivo);
        return -1;
    }

    if (*linhas < 3 || *colunas < 3) {
        fprintf(stderr, "Erro: dimensoes minimas sao 3x3 (recebido %dx%d)\n", *linhas, *colunas);
        fclose(arquivo);
        return -1;
    }

    m = aloca_matriz(*linhas, *colunas);
    if (m == NULL) {
        fprintf(stderr, "Erro: falha ao alocar matriz %dx%d\n", *linhas, *colunas);
        fclose(arquivo);
        return -1;
    }

    total_lidos = 0;
    for (i = 0; i < *linhas; i++) {
        for (j = 0; j < *colunas; j++) {
            if (fscanf(arquivo, "%d", &valor) != 1) {
                fprintf(stderr, "Erro: dados insuficientes em '%s' (lidos %d de %d valores)\n",
                        caminho, total_lidos, (*linhas) * (*colunas));
                libera_matriz(m);
                fclose(arquivo);
                return -1;
            }
            if (valor != 0 && valor != 1) {
                fprintf(stderr, "Erro: valor invalido '%d' em (%d,%d), esperado 0 ou 1\n", valor, i, j);
                libera_matriz(m);
                fclose(arquivo);
                return -1;
            }
            m[i][j] = valor;
            total_lidos++;
        }
    }

    fclose(arquivo);
    *matriz = m;
    return 0;
}

int **gera_matriz_aleatoria(int linhas, int colunas, double probabilidade, unsigned int semente)
{
    int **m;
    int i, j;

    m = aloca_matriz(linhas, colunas);
    if (m == NULL) {
        return NULL;
    }

    srand(semente);
    for (i = 0; i < linhas; i++) {
        for (j = 0; j < colunas; j++) {
            double sorteio = (double) rand() / ((double) RAND_MAX + 1.0);
            m[i][j] = (sorteio < probabilidade) ? 1 : 0;
        }
    }

    return m;
}
