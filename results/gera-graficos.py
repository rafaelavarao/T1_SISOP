#!/usr/bin/env python3
"""
Gera os graficos do relatorio (SVG) a partir de results/medicoes.csv.
Usa apenas a biblioteca padrao do Python 3.

Uso (a partir da raiz do repositorio): python3 results/gera-graficos.py
"""
import csv
import os
import statistics

AQUI = os.path.dirname(os.path.abspath(__file__))

LARGURA, ALTURA = 720, 400
ESQ, DIR, TOPO, BASE = 64, 656, 64, 336

FUNDO = "#fcfcfb"
TEXTO = "#0b0b0b"
TEXTO_SEC = "#52514e"
GRADE = "#e4e3df"
SERIE = "#2a78d6"
REFERENCIA = "#8a8983"


def num(valor, casas):
    return ("%.*f" % (casas, valor)).replace(".", ",")


def le_medicoes():
    """Devolve lista de (rotulo, trabalhadores, [tempos]) ordenada por trabalhadores."""
    grupos = {}
    with open(os.path.join(AQUI, "medicoes.csv"), newline="") as arq:
        for linha in csv.DictReader(arq):
            p = int(linha["trabalhadores"])
            rotulo = "Sequencial" if linha["versao"] == "sequencial" else "Paralela " + linha["grade"]
            grupos.setdefault(p, (rotulo, []))[1].append(float(linha["tempo_ms"]))
    return [(grupos[p][0], p, grupos[p][1]) for p in sorted(grupos)]


def y_de(valor, maximo):
    return BASE - (BASE - TOPO) * valor / maximo


def x_de(p, maximo):
    return ESQ + (DIR - ESQ) * p / maximo


def texto(x, y, conteudo, cor=TEXTO_SEC, ancora="start", tamanho=12, peso="400"):
    return ('<text x="%.1f" y="%.1f" font-size="%d" font-weight="%s" text-anchor="%s" fill="%s">%s</text>'
            % (x, y, tamanho, peso, ancora, cor, conteudo))


def cabecalho(titulo, subtitulo):
    return [
        '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 %d %d" width="%d" height="%d" '
        'font-family="system-ui, -apple-system, Segoe UI, Helvetica, Arial, sans-serif">'
        % (LARGURA, ALTURA, LARGURA, ALTURA),
        '<rect width="%d" height="%d" fill="%s"/>' % (LARGURA, ALTURA, FUNDO),
        texto(ESQ, 28, titulo, TEXTO, tamanho=16, peso="600"),
        texto(ESQ, 46, subtitulo),
    ]


def grade_y(marcas, maximo, casas):
    partes = []
    for m in marcas:
        y = y_de(m, maximo)
        partes.append('<line x1="%d" y1="%.1f" x2="%d" y2="%.1f" stroke="%s" stroke-width="1"/>'
                      % (ESQ, y, DIR, y, GRADE))
        partes.append(texto(ESQ - 8, y + 4, num(m, casas), ancora="end"))
    return partes


def linha_base():
    return ('<line x1="%d" y1="%d" x2="%d" y2="%d" stroke="%s" stroke-width="1"/>'
            % (ESQ, BASE, DIR, BASE, TEXTO_SEC))


def eixo_x_trabalhadores(ps, maximo):
    partes = [texto(x_de(p, maximo), BASE + 20, "%d" % p, ancora="middle") for p in ps]
    partes.append(texto((ESQ + DIR) / 2.0, BASE + 44, "Trabalhadores (threads)", ancora="middle"))
    return partes


def tracejada(x1, y1, x2, y2):
    return ('<line x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f" stroke="%s" stroke-width="2" '
            'stroke-dasharray="6 5"/>' % (x1, y1, x2, y2, REFERENCIA))


def marcador(x, y):
    return ('<circle cx="%.1f" cy="%.1f" r="4.5" fill="%s" stroke="%s" stroke-width="2"/>'
            % (x, y, SERIE, FUNDO))


def linha_serie(pontos):
    caminho = " ".join("%s%.1f,%.1f" % ("M" if i == 0 else "L", x, y)
                       for i, (x, y) in enumerate(pontos))
    partes = ['<path d="%s" fill="none" stroke="%s" stroke-width="2" stroke-linejoin="round"/>'
              % (caminho, SERIE)]
    partes += [marcador(x, y) for x, y in pontos]
    return partes


def grafico_tempo(dados):
    maximo = 125.0
    largura = 56
    passo = (DIR - ESQ) / float(len(dados))
    svg = cabecalho("Tempo de execução por configuração",
                    "Tempo (ms): mediana de 3 repetições; hastes indicam mínimo e máximo")
    svg += grade_y([0, 25, 50, 75, 100, 125], maximo, 0)
    for i, (rotulo, p, tempos) in enumerate(dados):
        centro = ESQ + passo * (i + 0.5)
        mediana = statistics.median(tempos)
        topo = y_de(mediana, maximo)
        x0, x1 = centro - largura / 2.0, centro + largura / 2.0
        svg.append('<path d="M%.1f,%d V%.1f Q%.1f,%.1f %.1f,%.1f H%.1f Q%.1f,%.1f %.1f,%.1f V%d Z" fill="%s"/>'
                   % (x0, BASE, topo + 4, x0, topo, x0 + 4, topo, x1 - 4, x1, topo, x1, topo + 4, BASE, SERIE))
        y_min, y_max = y_de(min(tempos), maximo), y_de(max(tempos), maximo)
        svg.append('<path d="M%.1f,%.1f V%.1f M%.1f,%.1f H%.1f M%.1f,%.1f H%.1f" fill="none" '
                   'stroke="%s" stroke-width="1.5"/>'
                   % (centro, y_max, y_min, centro - 6, y_max, centro + 6,
                      centro - 6, y_min, centro + 6, TEXTO))
        svg.append(texto(centro, y_max - 8, num(mediana, 1), TEXTO, "middle", peso="600"))
        svg.append(texto(centro, BASE + 20, rotulo, TEXTO, "middle"))
        svg.append(texto(centro, BASE + 37, "%d thread%s" % (p, "" if p == 1 else "s"), ancora="middle"))
    svg.append(linha_base())
    svg.append("</svg>")
    return "\n".join(svg)


def grafico_aceleracao(dados):
    maximo = 16.0
    base = statistics.median(dados[0][2])
    ps = [p for _, p, _ in dados]
    acel = [base / statistics.median(t) for _, _, t in dados]
    svg = cabecalho("Aceleração por quantidade de trabalhadores", "Aceleração S(p)")
    svg += grade_y([0, 4, 8, 12, 16], maximo, 0)
    svg += eixo_x_trabalhadores(ps, maximo)
    svg.append(tracejada(x_de(1, maximo), y_de(1, maximo), x_de(16, maximo), y_de(16, maximo)))
    svg.append(texto(x_de(12.3, maximo), y_de(13.6, maximo), "Ideal: S(p) = p", ancora="end"))
    pontos = [(x_de(p, maximo), y_de(s, maximo)) for p, s in zip(ps, acel)]
    svg += linha_serie(pontos)
    svg.append(texto(pontos[-1][0], pontos[-1][1] - 12, num(acel[-1], 2), TEXTO, "middle", peso="600"))
    lx, ly = ESQ + 16, TOPO + 14
    svg.append('<line x1="%d" y1="%d" x2="%d" y2="%d" stroke="%s" stroke-width="2"/>'
               % (lx, ly, lx + 24, ly, SERIE))
    svg.append(marcador(lx + 12, ly))
    svg.append(texto(lx + 32, ly + 4, "Observada", TEXTO))
    svg.append(tracejada(lx, ly + 20, lx + 24, ly + 20))
    svg.append(texto(lx + 32, ly + 24, "Ideal", TEXTO))
    svg.append(linha_base())
    svg.append("</svg>")
    return "\n".join(svg)


def grafico_eficiencia(dados):
    maximo = 1.25
    eixo_p = 16.0
    base = statistics.median(dados[0][2])
    ps = [p for _, p, _ in dados]
    efic = [base / statistics.median(t) / p for _, p, t in dados]
    svg = cabecalho("Eficiência por quantidade de trabalhadores", "Eficiência E(p) = S(p) / p")
    svg += grade_y([0, 0.25, 0.5, 0.75, 1.0, 1.25], maximo, 2)
    svg += eixo_x_trabalhadores(ps, eixo_p)
    svg.append(tracejada(ESQ, y_de(1, maximo), DIR, y_de(1, maximo)))
    svg.append(texto(DIR, y_de(1, maximo) - 8, "Ideal: E(p) = 1", ancora="end"))
    pontos = [(x_de(p, eixo_p), y_de(e, maximo)) for p, e in zip(ps, efic)]
    svg += linha_serie(pontos)
    for i in range(2, len(pontos)):
        svg.append(texto(pontos[i][0], pontos[i][1] - 12, num(efic[i], 2), TEXTO, "middle", peso="600"))
    svg.append(linha_base())
    svg.append("</svg>")
    return "\n".join(svg)


def main():
    dados = le_medicoes()
    for nome, funcao in (("grafico-tempo.svg", grafico_tempo),
                         ("grafico-aceleracao.svg", grafico_aceleracao),
                         ("grafico-eficiencia.svg", grafico_eficiencia)):
        with open(os.path.join(AQUI, nome), "w", encoding="utf-8", newline="\n") as arq:
            arq.write(funcao(dados) + "\n")
        print("Gerado results/%s" % nome)


if __name__ == "__main__":
    main()
