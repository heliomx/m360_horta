"""Costura o plano de terra: põe uma via de GND dentro de cada ilha preenchida que esteja solta.

⚠️ **NÃO CONFIE NO JULGAMENTO DESTE SCRIPT — só na parte que coloca a via.**
O critério de "ilha já ligada" aqui é *conter uma ilha de componente ou uma via de GND*, e isso
**não prova ligação ao plano principal**. Na M360-LP, depois de 11 ilhas receberem ligação
sólida, ele passou a relatar "20 ilhas já ligadas, 0 a costurar" enquanto o DRC acusava 13 zonas
soltas. Serve para **colocar** vias com verificação de colisão nas duas faces; não serve para
**decidir** onde elas são necessárias — para isso, leia o DRC.

E a lição maior (ARQUITETURA §11.6): se o plano fragmentou porque o posicionamento mudou, o
remédio é **rerrotear**, não costurar. Vias de costura, refill, ligação sólida e remoção de
ilhas foram tentados em sequência na LP e os desconectados não saíram de 15.

Por que é preciso: o Freerouting trata o GND como **plano** (o DSN declara `plane GND`), não como
rede a rotear — o SES não traz um fio de terra sequer. Isso funciona enquanto o preenchimento
sai contínuo. Quando o posicionamento muda e o plano se parte em ilhas, os pedaços que não
encostam em nenhuma ilha de componente ficam eletricamente soltos, e o DRC acusa
`unconnected_items` com "Zona [GND] ... / Ilha [GND] de ..." e `starved_thermal` em série.
Refill e parâmetros térmicos NÃO resolvem: o problema é topológico, não de preenchimento.

A costura liga cada ilha órfã de uma face ao plano da outra por uma via, que é o remédio
clássico para plano de terra em placa de duas camadas.

⚠️ Uma via PTH atravessa TODAS as camadas. Checar a colisão em apenas uma face é a maneira
clássica de criar um curto silencioso. Aqui cada candidata é testada contra ilhas, trilhas e
vias de **outras redes em ambas as faces**; encostar em cobre do próprio GND é permitido e
desejado. O teste usa caixa envolvente, que é conservador: recusa alguns pontos válidos, mas
nunca aprova um inválido.

Uso: python gnd_stitch.py <placa.kicad_pcb> [--dry] [--rede GND]
"""
import sys

import pcbnew

MM = 1e6
VIA_TAM = 0.6       # diâmetro da ilha da via, mm
VIA_FURO = 0.3      # furo, mm
ISOLAMENTO = 0.2    # isolamento da placa, mm
MARGEM = 0.15       # folga extra sobre o isolamento, mm
BORDA = 0.6         # distância mínima da borda da placa, mm
PASSO = 0.5         # passo da varredura de candidatas, mm


def mm(v):
    return v / MM


def ui(v):
    return int(round(v * MM))


def dist_ponto_caixa(px, py, caixa):
    """Distância de um ponto à caixa envolvente (0 se dentro), em mm."""
    x1, y1 = mm(caixa.GetX()), mm(caixa.GetY())
    x2, y2 = x1 + mm(caixa.GetWidth()), y1 + mm(caixa.GetHeight())
    dx = max(x1 - px, 0.0, px - x2)
    dy = max(y1 - py, 0.0, py - y2)
    return (dx * dx + dy * dy) ** 0.5


def main(path, dry, rede_gnd):
    board = pcbnew.LoadBoard(path)
    borda = board.GetBoardEdgesBoundingBox()
    bx1, by1 = mm(borda.GetX()), mm(borda.GetY())
    bx2, by2 = bx1 + mm(borda.GetWidth()), by1 + mm(borda.GetHeight())

    cod_gnd = None
    for nome, info in board.GetNetsByName().items():
        if str(nome) in (rede_gnd, "/" + rede_gnd):
            cod_gnd = info.GetNetCode()
            break
    if cod_gnd is None:
        sys.exit(f"rede '{rede_gnd}' não encontrada na placa")

    # obstáculos: tudo que NÃO é da rede de terra
    obstaculos = []
    for fp in board.GetFootprints():
        for p in fp.Pads():
            if p.GetNetCode() != cod_gnd:
                obstaculos.append(p.GetBoundingBox())
    for t in board.GetTracks():
        if t.GetNetCode() != cod_gnd:
            obstaculos.append(t.GetBoundingBox())
    # vias de QUALQUER rede são obstáculo físico (furo ocupa espaço)
    vias = [t.GetBoundingBox() for t in board.GetTracks()
            if isinstance(t, pcbnew.PCB_VIA)]

    folga = VIA_TAM / 2 + ISOLAMENTO + MARGEM

    # ilhas já ligadas: as que contêm ilha de componente do GND ou uma via de GND
    pontos_gnd = []
    for fp in board.GetFootprints():
        for p in fp.Pads():
            if p.GetNetCode() == cod_gnd:
                pontos_gnd.append(p.GetPosition())
    for t in board.GetTracks():
        if isinstance(t, pcbnew.PCB_VIA) and t.GetNetCode() == cod_gnd:
            pontos_gnd.append(t.GetPosition())

    novas, puladas, ja_ligadas = [], 0, 0
    for zona in board.Zones():
        if zona.GetNetCode() != cod_gnd:
            continue
        for camada in zona.GetLayerSet().Seq():
            polys = zona.GetFilledPolysList(camada)
            if not polys:
                continue
            for i in range(polys.OutlineCount()):
                # a ilha já tem ligação própria?
                if any(polys.Contains(pt, i) for pt in pontos_gnd):
                    ja_ligadas += 1
                    continue
                # ATENÇÃO: polys.BBox() devolve a caixa do CONJUNTO de ilhas — na LP isso dava
                # 85 x 61 mm, a placa inteira, e a varredura nunca achava ponto livre. A caixa
                # de cada ilha vem de Outline(i).BBox().
                cx = polys.Outline(i).BBox()
                x1, y1 = mm(cx.GetX()), mm(cx.GetY())
                x2 = x1 + mm(cx.GetWidth())
                y2 = y1 + mm(cx.GetHeight())

                escolhida = None
                py = y1
                while py <= y2 and escolhida is None:
                    px = x1
                    while px <= x2:
                        if (bx1 + BORDA <= px <= bx2 - BORDA
                                and by1 + BORDA <= py <= by2 - BORDA
                                and polys.Contains(pcbnew.VECTOR2I(ui(px), ui(py)), i)
                                and all(dist_ponto_caixa(px, py, c) >= folga for c in obstaculos)
                                and all(dist_ponto_caixa(px, py, c) >= folga for c in vias)):
                            escolhida = (px, py)
                            break
                        px += PASSO
                    py += PASSO

                if escolhida is None:
                    puladas += 1
                    print(f"   ilha {i} em {board.GetLayerName(camada)}: sem ponto livre "
                          f"(caixa {x2 - x1:.1f} x {y2 - y1:.1f} mm) — não costurada")
                    continue
                novas.append((escolhida, board.GetLayerName(camada), i))

    print(f"{ja_ligadas} ilha(s) já ligadas, {len(novas)} a costurar, {puladas} sem espaço")
    for (px, py), cam, i in novas:
        print(f"   via GND em ({px:.2f}, {py:.2f}) — ilha {i} de {cam}")

    if dry:
        print("--dry: nada foi gravado")
        return
    if not novas:
        print("nada a fazer")
        return

    for (px, py), _cam, _i in novas:
        v = pcbnew.PCB_VIA(board)
        v.SetPosition(pcbnew.VECTOR2I(ui(px), ui(py)))
        v.SetWidth(ui(VIA_TAM))
        v.SetDrill(ui(VIA_FURO))
        v.SetLayerPair(pcbnew.F_Cu, pcbnew.B_Cu)
        v.SetNetCode(cod_gnd)
        board.Add(v)

    pcbnew.SaveBoard(path, board)
    print(f"{len(novas)} via(s) de costura gravadas")
    print("refazer o preenchimento: kicad-cli pcb drc --refill-zones --save-board ...")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    rede = "GND"
    if "--rede" in sys.argv:
        rede = sys.argv[sys.argv.index("--rede") + 1]
    main(sys.argv[1], "--dry" in sys.argv, rede)
