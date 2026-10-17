"""Tira da serigrafia do RJ45 C385834 os traços que encostam na borda da placa.

O footprint importado pelo easyeda2kicad desenha, na face da boca do conector, uma linha frontal
em y = 8,60 mm, as laterais descendo até 8,60 e um círculo de 0,03 mm em 8,62. Os RJ45 ficam com a
boca na borda da placa, que passa a 8,66 mm do centro: os três traços ficam colados nela e o DRC
acusa `silk_edge_clearance` (8 avisos na LP, 13 na ACT). Não há o que marcar ali — a borda da
placa já é a borda do conector.

Correção: apaga a linha frontal e o círculo, e encurta as laterais até y = 8,0 mm.

Como o fix_rj45_3d.py, varre a biblioteca E todas as placas da família: cada placa guarda a sua
cópia do footprint. Rodar sempre depois do import_lcsc.py, que traz os traços de volta.
Idempotente; só grava quando o conteúdo muda.

Uso: python fix_rj45_silk.py
"""
import glob
import os
import re

from schlib import block_end

HW = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LIB = os.path.join(HW, "lib", "M360.pretty", "RJ45_R-RJ45R08P-A004.kicad_mod")
PLACAS = sorted(glob.glob(os.path.join(HW, "*", "*.kicad_pcb")))

FRENTE = 8.6    # y da linha frontal e do fim das laterais no footprint original
LIMITE = 8.0    # novo fim das laterais
TOL = 0.05


def perto(v, alvo):
    return abs(v - alvo) < TOL


def corrigir_bloco(fp):
    """Recebe o texto de um footprint RJ45 e devolve (texto novo, nº de mudanças)."""
    saida, pos, n = [], 0, 0
    for m in re.finditer(r"\((fp_line|fp_circle)\b", fp):
        ini = m.start()
        if ini < pos:
            continue
        fim = block_end(fp, ini)
        g = fp[ini:fim]
        if "F.SilkS" not in g:
            continue
        novo = g
        if m.group(1) == "fp_circle":
            c = re.search(r"\(center ([-\d.]+) ([-\d.]+)\)", g)
            if c and perto(float(c.group(2)), FRENTE + 0.02):
                novo = None
        else:
            s = re.search(r"\(start ([-\d.]+) ([-\d.]+)\)", g)
            e = re.search(r"\(end ([-\d.]+) ([-\d.]+)\)", g)
            ys, ye = float(s.group(2)), float(e.group(2))
            if perto(ys, FRENTE) and perto(ye, FRENTE):
                novo = None                                   # linha frontal
            elif perto(ye, FRENTE) and ys < FRENTE:
                novo = g[:e.start()] + f"(end {e.group(1)} {LIMITE:g})" + g[e.end():]
            elif perto(ys, FRENTE) and ye < FRENTE:
                novo = g[:s.start()] + f"(start {s.group(1)} {LIMITE:g})" + g[s.end():]
        if novo == g:
            continue
        # apaga também a quebra de linha e a indentação que antecedem o bloco
        corte = ini
        if novo is None:
            k = fp.rfind("\n", 0, ini)
            if k >= pos and fp[k + 1:ini].strip() == "":
                # num arquivo com CRLF o "\r" fica antes do "\n": sai junto, senão sobra solto
                corte = k - 1 if k > pos and fp[k - 1] == "\r" else k
        saida.append(fp[pos:corte])
        if novo is not None:
            saida.append(novo)
        pos = fim
        n += 1
    saida.append(fp[pos:])
    return "".join(saida), n


def main():
    total = 0
    # biblioteca: o arquivo inteiro é o footprint
    t = open(LIB, encoding="utf-8", newline="").read()
    novo, n = corrigir_bloco(t)
    if n:
        open(LIB, "w", encoding="utf-8", newline="").write(novo)
    print(f"{os.path.basename(LIB)}: {n} traço(s) corrigido(s)")
    total += n

    for p in PLACAS:
        t = open(p, encoding="utf-8", newline="").read()
        saida, pos, n_placa, refs = [], 0, 0, []
        for m in re.finditer(r'\n\t\(footprint "M360:RJ45_R-RJ45R08P-A004"', t):
            ini = m.start() + 1
            fim = block_end(t, ini + 1)
            bloco, n = corrigir_bloco(t[ini:fim])
            if n:
                saida.append(t[pos:ini])
                saida.append(bloco)
                pos = fim
                n_placa += n
                refs.append(re.search(r'\(property "Reference" "([^"]+)"', bloco).group(1))
        if n_placa:
            saida.append(t[pos:])
            open(p, "w", encoding="utf-8", newline="").write("".join(saida))
        print(f"{os.path.basename(p)}: {n_placa} traço(s) em {', '.join(refs) or 'nenhum RJ45 a corrigir'}")
        total += n_placa
    print(f"total: {total}")


if __name__ == "__main__":
    main()
