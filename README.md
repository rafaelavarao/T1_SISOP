# Contagem paralela de objetos em uma matriz binária

Trabalho prático de Sistemas Operacionais (PUCRS, 2026/II, Prof. Filipo Mór) — contagem de
componentes conexos (conectividade 8) em uma matriz binária, com uma implementação sequencial
de referência e uma implementação paralela com Pthreads.

## Autoria

- [Seu nome] — <rafaelavarao12@gmail.com>
- Gauterio — <gabrielgauterio2208@gmail.com>

## Compilação

Requer um compilador C com suporte a Pthreads (Linux ou macOS).

```sh
make            # compila conta-objetos-sequencial, conta-objetos-paralelo e gera-matriz
make clean      # remove os binários
```

Comando equivalente manual (referência do enunciado):

```sh
cc -std=c89 -Wall -Wextra -pedantic src/conta-objetos-sequencial.c src/matriz_io.c -o conta-objetos-sequencial
cc -std=c89 -Wall -Wextra -pedantic -pthread src/conta-objetos-paralelo.c src/matriz_io.c -o conta-objetos-paralelo
```

## Execução

Formato do arquivo de matriz (texto simples, ver `tests/exemplo1.txt` a `tests/exemplo5.txt`):

```
<linhas> <colunas>
<linha 0: colunas valores 0/1 separados por espaço>
...
```

Sequencial:

```sh
./conta-objetos-sequencial tests/exemplo3.txt
```

Paralelo (a matriz é dividida em uma grade `<blocos_linha> x <blocos_coluna>`, uma thread por bloco):

```sh
./conta-objetos-paralelo tests/exemplo3.txt 2 2
```

Gerador de matriz aleatória para os testes de desempenho:

```sh
./gera-matriz <linhas> <colunas> <probabilidade_de_1> <semente> <arquivo_saida>
./gera-matriz 2000 2000 0.35 42 tests/desempenho.txt
```

`make test` roda as 5 matrizes obrigatórias nas duas versões (paralelo com grade 2x2).

## Arquitetura

### Versão sequencial (`src/conta-objetos-sequencial.c`)

Varre a matriz em ordem de leitura; ao encontrar uma célula `1` ainda sem rótulo, incrementa o
contador de objetos e roda um flood fill iterativo (pilha explícita, sem recursão) que marca
todas as células conectadas por aresta ou canto (conectividade 8) com o rótulo do objeto atual.
Serve como referência de corretude e de tempo para a versão paralela.

### Versão paralela (`src/conta-objetos-paralelo.c`)

**Decomposição:** a matriz é dividida em uma grade configurável de `block_linhas x block_colunas`
blocos retangulares (uma thread Pthread por bloco), com as linhas/colunas distribuídas o mais
equilibradamente possível quando não dividem exatamente. Essa grade permite reproduzir
diretamente as divisões ilustrativas do enunciado (ex.: 2x2 para os Exemplos 1-3, 3x3 para os
Exemplos 4-5) e também se reduz a faixas de linha (`1 x N`) ou de coluna (`N x 1`) quando
desejado.

**Fase paralela (sem sincronização):** cada thread roda flood fill de forma totalmente
independente, restrito aos limites do seu próprio bloco, escrevendo apenas na fatia exclusiva
da matriz de rótulos correspondente ao seu bloco. Como as regiões de escrita de cada thread são
disjuntas, não há mutex nem espera entre threads durante essa etapa — a parte cara do trabalho
(percorrer todas as células) acontece 100% em paralelo. Cada thread gera identificadores de
rótulo únicos a partir de uma faixa reservada (`rotulo_base`), calculada antes da criação das
threads, evitando qualquer colisão sem precisar de sincronização.

**Consolidação (após `pthread_join`):** um objeto pode se estender por dois ou mais blocos, então
somar as contagens locais não basta. Depois que todas as threads terminam, uma etapa sequencial
de baixo custo — proporcional ao perímetro da grade de blocos, não à área da matriz — percorre:

1. cada fronteira **vertical** entre blocos de colunas adjacentes (todas as linhas, janela de
   ±1 linha), e
2. cada fronteira **horizontal** entre blocos de linhas adjacentes (todas as colunas, janela de
   ±1 coluna),

unificando com uma estrutura de **união-busca (union-find, com compressão de caminho e união por
tamanho)** os rótulos de células vizinhas (aresta ou canto) que pertencem a blocos diferentes.
A combinação dessas duas varreduras cobre automaticamente o encontro diagonal de quatro blocos
(Exemplo 3 do enunciado): a célula no canto de um bloco e a célula no canto diagonalmente oposta
do bloco vizinho aparecem na mesma checagem de fronteira vertical (linha ± 1), então a união
diagonal nunca é perdida.

A contagem final é o número de raízes distintas entre os rótulos locais criados pelas threads.

**Por que essa divisão do trabalho:** a fase mais cara (varrer todas as N x M células) é 100%
paralela e sem espera entre threads; apenas a consolidação nas fronteiras — muito mais barata —
permanece sequencial. Isso é o que permite que o paralelismo realmente compense (threads
trabalhando juntas, não uma esperando a outra).

## Matrizes de teste obrigatórias

| Ex. | Dimensões | Esperado | Sequencial | Paralelo |
|-----|-----------|----------|------------|----------|
| 1   | 5 x 5     | 3        | A preencher | A preencher |
| 2   | 6 x 8     | 4        | A preencher | A preencher |
| 3   | 8 x 8     | 5        | A preencher | A preencher |
| 4   | 9 x 12    | 6        | A preencher | A preencher |
| 5   | 12 x 12   | 7        | A preencher | A preencher |

## Análise de desempenho

A preencher após rodar `gera-matriz` para criar uma matriz grande e medir tempos sequencial x
paralelo com pelo menos duas quantidades de threads (ex.: 2x2 e 4x4). Incluir: tempo sequencial,
tempo paralelo, aceleração (S = Tseq / Tpar) e, caso a versão paralela seja mais lenta em algum
caso pequeno, a explicação (overhead de criação de threads não compensado pelo volume de
trabalho).

## Referências e ferramentas

- Editor de tabelas C (https://filipomor.com/editor-tabelas-c) — auxiliar na criação de matrizes
  de teste adicionais.
