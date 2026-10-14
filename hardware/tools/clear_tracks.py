"""Apaga todas as trilhas e vias de uma placa, para rerroteá-la do zero.

Necessário sempre que uma peça muda de footprint ou de posição depois de roteada: as trilhas
antigas continuam onde estavam e passam a cruzar ilhas novas. O sintoma no DRC é um amontoado
de `clearance`, `hole_clearance`, `solder_mask_bridge` e até `shorting_items` — todos apontando
para a mesma ilha. Não é defeito de projeto, é cobre obsoleto.

Rerrotear por cima não resolve: o DSN exportado leva a fiação existente na seção `wiring`, e o
Freerouting a preserva. Por isso se limpa antes de exportar.

As **zonas ficam** — elas são refeitas pelo preenchimento:
    kicad-cli pcb drc --refill-zones --save-board ...

Trabalha no texto do arquivo: não usa o pcbnew, onde remover elementos pelo SWIG é caminho
conhecido de falha de segmentação.

Uso: python clear_tracks.py <placa.kicad_pcb>
"""
import re
import sys

from schlib import block_end


def main(path):
    texto = open(path, encoding="utf-8", newline="").read()

    # blocos de nível superior: "\n\t(segment" e "\n\t(via"
    alvos = []
    for m in re.finditer(r"\n\t\((segment|via)\b", texto):
        ini = m.start() + 1
        alvos.append((m.start(), block_end(texto, ini), m.group(1)))

    if not alvos:
        print("nenhuma trilha ou via encontrada — nada a fazer")
        return

    saida, pos = [], 0
    contagem = {"segment": 0, "via": 0}
    for ini, fim, tipo in alvos:
        saida.append(texto[pos:ini])
        pos = fim
        contagem[tipo] += 1
    saida.append(texto[pos:])

    open(path, "w", encoding="utf-8", newline="").write("".join(saida))
    print(f"{contagem['segment']} trilha(s) e {contagem['via']} via(s) removidas")
    print("as zonas foram mantidas; refazer o preenchimento depois de rotear")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    main(sys.argv[1])
