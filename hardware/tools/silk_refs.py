"""Posiciona as referências da serigrafia automaticamente, longe de pads, de outra serigrafia e da
borda; esconde a que não couber em lugar nenhum.

Por que existe: na M360-ACT refeita (2026-09-14) as 35 peças novas entraram com a referência na
posição padrão do footprint e o DRC acusou 86 avisos de serigrafia. Em fileiras a passo de 1,9 mm
(os 100 ohm atrás dos TVS) não há como acertar texto a texto à mão, como o silk_lp.py faz.

Para cada footprint, na ordem da lista (as maiores primeiro), tenta posições em volta do
courtyard — acima, abaixo, à esquerda, à direita, com o texto a 0° e a 90°, afastando de 0,1 em
0,1 mm — e fica com a primeira cujo retângulo não encosta (com folga) em:
  - pad de qualquer peça (silk_over_copper);
  - desenho de serigrafia de qualquer footprint e texto já posicionado (silk_overlap);
  - borda da placa (silk_edge_clearance);
  - courtyard de outra peça (texto que fica embaixo de um componente não se lê) — só na
    primeira rodada; na segunda, vale por cima de courtyard.
Sem posição livre, a referência fica invisível e o script lista quais.

Textos livres e legendas (silk_*.py) devem ser aplicados ANTES: entram como obstáculo.
Idempotente. Uso: <KiCad>/bin/python.exe silk_refs.py <placa.kicad_pcb> [--pular REF,REF...]
"""
import sys

import pcbnew

ALTURA, ESPESSURA = 1.0, 0.15
FOLGA = 0.15          # mm entre o texto e qualquer obstáculo
FOLGA_BORDA = 0.35
PASSOS = [0.1 * k for k in range(0, 16)]
PULAR_PREFIXO = ("H", "FID")


def mm(v):
    return pcbnew.ToMM(v)


def caixa(bb, folga=0.0):
    return (mm(bb.GetLeft()) - folga, mm(bb.GetTop()) - folga, mm(bb.GetRight()) + folga, mm(bb.GetBottom()) + folga)


def cruza(a, b):
    return a[0] < b[2] and b[0] < a[2] and a[1] < b[3] and b[1] < a[3]


def main(path, pular):
    board = pcbnew.LoadBoard(path)
    borda = caixa(board.GetBoardEdgesBoundingBox())
    fps = [f for f in board.GetFootprints()
           if not f.GetReference().startswith(PULAR_PREFIXO) and f.GetReference() not in pular]

    pads = [caixa(p.GetBoundingBox(), FOLGA) for p in board.GetPads()]
    silk = []
    for f in board.GetFootprints():
        for g in f.GraphicalItems():
            if g.GetLayer() == pcbnew.F_SilkS and g.GetClass() != "PCB_TEXT":
                silk.append(caixa(g.GetBoundingBox(), FOLGA))
        if f.GetReference() in pular or f.GetReference().startswith(PULAR_PREFIXO):
            for t in (f.Reference(), f.Value()):
                if t.IsVisible() and t.GetLayer() == pcbnew.F_SilkS:
                    silk.append(caixa(t.GetBoundingBox(), FOLGA))
        elif f.Value().IsVisible() and f.Value().GetLayer() == pcbnew.F_SilkS:
            silk.append(caixa(f.Value().GetBoundingBox(), FOLGA))   # legendas dos conectores
    for d in board.GetDrawings():
        if d.GetLayer() == pcbnew.F_SilkS:
            silk.append(caixa(d.GetBoundingBox(), FOLGA))
    crt = {f.GetReference(): caixa(f.GetCourtyard(pcbnew.F_CrtYd).BBox()) for f in board.GetFootprints()
           if f.GetCourtyard(pcbnew.F_CrtYd).OutlineCount()}

    fps.sort(key=lambda f: -(crt.get(f.GetReference(), (0, 0, 0, 0))[2] - crt.get(f.GetReference(), (0, 0, 0, 0))[0]))
    escondidas = []
    for f in fps:
        ref = f.GetReference()
        t = f.Reference()
        t.SetLayer(pcbnew.F_SilkS)
        t.SetTextSize(pcbnew.VECTOR2I(pcbnew.FromMM(ALTURA), pcbnew.FromMM(ALTURA)))
        t.SetTextThickness(pcbnew.FromMM(ESPESSURA))
        t.SetHorizJustify(pcbnew.GR_TEXT_H_ALIGN_CENTER)
        t.SetVertJustify(pcbnew.GR_TEXT_V_ALIGN_CENTER)
        c = crt.get(ref) or caixa(f.GetBoundingBox(False))
        cx, cy = (c[0] + c[2]) / 2, (c[1] + c[3]) / 2
        achou = False
        for rodada in (0, 1):
            outros = [v for k, v in crt.items() if k != ref] if rodada == 0 else []
            for d in PASSOS:
                for ang in (0, 90):
                    t.SetTextAngleDegrees(ang)
                    t.SetPosition(pcbnew.VECTOR2I(0, 0))
                    bb = caixa(t.GetBoundingBox())
                    hw, hh = (bb[2] - bb[0]) / 2, (bb[3] - bb[1]) / 2
                    for x, y in ((cx, c[1] - hh - d), (cx, c[3] + hh + d), (c[0] - hw - d, cy), (c[2] + hw + d, cy)):
                        r = (x - hw, y - hh, x + hw, y + hh)
                        if not (borda[0] + FOLGA_BORDA <= r[0] and r[2] <= borda[2] - FOLGA_BORDA and
                                borda[1] + FOLGA_BORDA <= r[1] and r[3] <= borda[3] - FOLGA_BORDA):
                            continue
                        if any(cruza(r, o) for o in pads) or any(cruza(r, o) for o in silk) or \
                                any(cruza(r, o) for o in outros):
                            continue
                        t.SetPosition(pcbnew.VECTOR2I(pcbnew.FromMM(x), pcbnew.FromMM(y)))
                        t.SetVisible(True)
                        silk.append((r[0] - FOLGA, r[1] - FOLGA, r[2] + FOLGA, r[3] + FOLGA))
                        achou = True
                        break
                    if achou:
                        break
                if achou:
                    break
            if achou:
                break
        if not achou:
            t.SetVisible(False)
            escondidas.append(ref)
    pcbnew.SaveBoard(path, board)
    print(f"{len(fps) - len(escondidas)} referência(s) posicionada(s); escondidas: {', '.join(escondidas) or 'nenhuma'}")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    args = sys.argv[1:]
    pular = set()
    if "--pular" in args:
        k = args.index("--pular")
        pular = set(args[k + 1].split(","))
        del args[k:k + 2]
    main(args[0], pular)
