#ifndef MATRIZ_IO_H
#define MATRIZ_IO_H

/*
 * Aloca uma matriz de inteiros com `linhas` x `colunas` em um unico bloco
 * contiguo de memoria (facilita alocacao/liberacao e melhora localidade).
 * Retorna NULL em caso de falha de alocacao.
 */
int **aloca_matriz(int linhas, int colunas);

/* Libera uma matriz alocada por aloca_matriz. Aceita NULL. */
void libera_matriz(int **matriz);

/*
 * Le uma matriz binaria de um arquivo texto no formato:
 *   linhas colunas
 *   v v v ... (linhas linhas, colunas valores 0/1 cada, separados por espaco)
 *
 * Preenche *matriz, *linhas e *colunas. Retorna 0 em sucesso, -1 em erro
 * (arquivo inexistente, formato invalido ou falha de alocacao).
 */
int carrega_matriz(const char *caminho, int ***matriz, int *linhas, int *colunas);

/*
 * Gera uma matriz binaria aleatoria (linhas x colunas), usada para os
 * testes de desempenho com matrizes grandes. `probabilidade` e a chance
 * (0.0 a 1.0) de uma celula ser 1. `semente` inicializa o gerador para
 * permitir reproducibilidade entre a versao sequencial e a paralela.
 */
int **gera_matriz_aleatoria(int linhas, int colunas, double probabilidade, unsigned int semente);

#endif
