/*
 * Utilitario para gerar uma matriz binaria aleatoria em arquivo texto,
 * usada nos testes de desempenho com matrizes grandes (item 8 do
 * enunciado). Gerar uma unica vez e salvar em arquivo garante que a
 * versao sequencial e a paralela processem exatamente os mesmos dados.
 *
 * Uso: gera-matriz <linhas> <colunas> <probabilidade_de_1> <semente> <arquivo_saida>
 */
#include <stdio.h>
#include <stdlib.h>

#include "matriz_io.h"

int main(int argc, char **argv)
{
    int linhas, colunas;
    double probabilidade;
    unsigned int semente;
    int **matriz;
    FILE *saida;
    int i, j;

    if (argc != 6) {
        fprintf(stderr, "Uso: %s <linhas> <colunas> <probabilidade_de_1> <semente> <arquivo_saida>\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    linhas = atoi(argv[1]);
    colunas = atoi(argv[2]);
    probabilidade = atof(argv[3]);
    semente = (unsigned int) atoi(argv[4]);

    if (linhas < 3 || colunas < 3) {
        fprintf(stderr, "Erro: dimensoes minimas sao 3x3\n");
        return EXIT_FAILURE;
    }
    if (probabilidade < 0.0 || probabilidade > 1.0) {
        fprintf(stderr, "Erro: probabilidade deve estar entre 0.0 e 1.0\n");
        return EXIT_FAILURE;
    }

    matriz = gera_matriz_aleatoria(linhas, colunas, probabilidade, semente);
    if (matriz == NULL) {
        fprintf(stderr, "Erro: falha ao gerar matriz aleatoria\n");
        return EXIT_FAILURE;
    }

    saida = fopen(argv[5], "w");
    if (saida == NULL) {
        fprintf(stderr, "Erro: nao foi possivel criar o arquivo '%s'\n", argv[5]);
        libera_matriz(matriz);
        return EXIT_FAILURE;
    }

    if (fprintf(saida, "%d %d\n", linhas, colunas) < 0) {
        fprintf(stderr, "Erro: falha ao escrever cabecalho em '%s'\n", argv[5]);
        fclose(saida);
        libera_matriz(matriz);
        return EXIT_FAILURE;
    }

    for (i = 0; i < linhas; i++) {
        for (j = 0; j < colunas; j++) {
            if (fprintf(saida, "%d%c", matriz[i][j], (j == colunas - 1) ? '\n' : ' ') < 0) {
                fprintf(stderr, "Erro: falha ao escrever dados em '%s'\n", argv[5]);
                fclose(saida);
                libera_matriz(matriz);
                return EXIT_FAILURE;
            }
        }
    }

    fclose(saida);
    libera_matriz(matriz);

    printf("Matriz %dx%d gerada em '%s' (p=%.3f, semente=%u)\n",
           linhas, colunas, argv[5], probabilidade, semente);
    return EXIT_SUCCESS;
}
