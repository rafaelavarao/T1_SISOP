CC = cc
CFLAGS = -std=c89 -Wall -Wextra -pedantic
SRC = src

.PHONY: all clean test

all: conta-objetos-sequencial conta-objetos-paralelo gera-matriz

conta-objetos-sequencial: $(SRC)/conta-objetos-sequencial.c $(SRC)/matriz_io.c $(SRC)/matriz_io.h
	$(CC) $(CFLAGS) $(SRC)/conta-objetos-sequencial.c $(SRC)/matriz_io.c -o $@

conta-objetos-paralelo: $(SRC)/conta-objetos-paralelo.c $(SRC)/matriz_io.c $(SRC)/matriz_io.h
	$(CC) $(CFLAGS) -pthread $(SRC)/conta-objetos-paralelo.c $(SRC)/matriz_io.c -o $@

gera-matriz: $(SRC)/gera-matriz.c $(SRC)/matriz_io.c $(SRC)/matriz_io.h
	$(CC) $(CFLAGS) $(SRC)/gera-matriz.c $(SRC)/matriz_io.c -o $@

test: all
	@for f in tests/exemplo*.txt; do \
		echo "== $$f =="; \
		echo "-- sequencial --"; ./conta-objetos-sequencial $$f; \
		echo "-- paralelo 2x2 --"; ./conta-objetos-paralelo $$f 2 2; \
	done

clean:
	rm -f conta-objetos-sequencial conta-objetos-paralelo gera-matriz
