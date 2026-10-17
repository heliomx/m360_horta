"""Acrescenta ao esquemático o header 1x8 do nRF24L01 legado e o seu capacitor de 10 uF.

Por que existe (2026-09-15): as três placas novas levam o RFM95W num mezanino, mas os nós e o
gateway atuais usam nRF24L01+. Cada carrier ganha um pin header 1x8 (CE, CSN, SCK, MO, MI,
IRQ, 3V3, GND) ligado às MESMAS redes do soquete do mezanino: CE = DIO0, CSN = NSS, IRQ = DIO1.
Um rádio por placa — o CSN partilha o NSS e o CE partilha o DIO0, então os dois nunca ficam
montados ao mesmo tempo (ARQUITETURA §2). O 10 uF junto do header é o desacoplamento que os
módulos nRF24 (sobretudo PA+LNA) exigem na ponta do cabo.

O que grava, por texto (schlib.py), sem o MCP:
  - o símbolo Connector_Generic:Conn_01x08 em (lib_symbols), copiado da biblioteca stock do
    KiCad 10, se ainda não existir;
  - o header (in_bom no / in_pos_files no, LCSC C492407, footprint PinHeader_1x08 stock) com
    um fio de stub_len(nome) e label na ponta de cada pino, no padrão do fix_layout.py; pino
    sem rede recebe no_connect;
  - o capacitor Device:C 10 uF 0805 (LCSC C15850, no BOM) com fios para o 3,3 V e o GND.

Antes de gravar confere que nada novo encosta em símbolo, fio ou label existente. Idempotente:
se a referência do header já existe, não faz nada. Depois: kicad-cli sch upgrade --force,
netdiff.py (só DIO0, NSS, SCK, MOSI, MISO, DIO1, 3V3/+3V3 e GND podem mudar), ERC 0,
add nada — o LCSC já vai gravado.

Uso: python add_nrf_header.py <placa.kicad_sch> --placa gw|lp|act
"""
import math
import os
import re
import sys
import uuid

from schlib import block_end, near, parse, top_blocks

G = 1.27
STOCK_SYMBOLS = os.environ.get("KICAD10_SYMBOL_DIR", r"C:\Program Files\KiCad\10.0\share\kicad\symbols")

# pino do header -> rede (None = sem ligação). Ordem pedida: CE CSN SCK MO MI IRQ 3V3 GND.
PLACAS = {
    "gw": dict(ref="J4", cap="C5", at=(203.2, 152.4), cap_at=(215.9, 152.4), rail="+3V3",
               pinos={"1": "DIO0", "2": "NSS", "3": "SCK", "4": "MOSI", "5": "MISO", "6": None, "7": "+3V3", "8": "GND"}),
    "lp": dict(ref="J5", cap="C16", at=(45.72, 226.06), cap_at=(60.96, 226.06), rail="3V3",
               pinos={"1": "DIO0", "2": "NSS", "3": "SCK", "4": "MOSI", "5": "MISO", "6": "DIO1", "7": "3V3", "8": "GND"}),
    "act": dict(ref="J6", cap="C9", at=(45.72, 248.92), cap_at=(88.9, 248.92), rail="3V3",
                pinos={"1": "DIO0", "2": "NSS", "3": "SCK", "4": "MOSI", "5": "MISO", "6": "DIO1", "7": "3V3", "8": "GND"}),
}
HEADER_VALUE = "nRF24 legado"
HEADER_FP = "Connector_PinHeader_2.54mm:PinHeader_1x08_P2.54mm_Vertical"
HEADER_LCSC = "C492407"      # XFCN PZ254V-11-08P, header macho 1x8 2,54 mm (solda manual)
CAP_VALUE, CAP_FP, CAP_LCSC = "10uF", "Capacitor_SMD:C_0805_2012Metric", "C15850"   # basic
LIB_HEADER, LIB_CAP = "Connector_Generic:Conn_01x08", "Device:C"

# direção de saída do pino -> (ângulo do label, justify), como o fix_layout.py grava
LABEL_STYLE = {(-1, 0): (0, "left bottom"), (1, 0): (180, "right bottom"),
               (0, -1): (270, "right bottom"), (0, 1): (90, "left bottom")}


def fmt(v):
    s = f"{v:.4f}".rstrip("0").rstrip(".")
    return "0" if s in ("-0", "") else s


def stub_len(name):
    return max(5.08, round(math.ceil((len(name) * 1.0 + 2.5) / G - 1e-9) * G, 2))


def prop(name, value, x, y, ang=0, hide=False):
    h = "\n\t\t\t(hide yes)" if hide else ""
    return (f'\t\t(property "{name}" "{value}"\n\t\t\t(at {fmt(x)} {fmt(y)} {fmt(ang)}){h}\n'
            f"\t\t\t(show_name no)\n\t\t\t(do_not_autoplace no)\n\t\t\t(effects\n\t\t\t\t(font\n"
            f"\t\t\t\t\t(size 1.27 1.27)\n\t\t\t\t)\n\t\t\t)\n\t\t)\n")


def symbol_block(lib_id, ref, value, at, props, pins, project, sheet_uuid, in_bom, in_pos):
    x, y = at
    out = [f'(symbol\n\t\t(lib_id "{lib_id}")\n\t\t(at {fmt(x)} {fmt(y)} 0)\n\t\t(unit 1)\n\t\t(body_style 1)\n'
           f"\t\t(exclude_from_sim no)\n\t\t(in_bom {'yes' if in_bom else 'no'})\n\t\t(on_board yes)\n"
           f"\t\t(in_pos_files {'yes' if in_pos else 'no'})\n\t\t(dnp no)\n\t\t(uuid \"{uuid.uuid4()}\")\n"]
    out.append(props)
    for p in pins:
        out.append(f'\t\t(pin "{p}"\n\t\t\t(uuid "{uuid.uuid4()}")\n\t\t)\n')
    out.append(f'\t\t(instances\n\t\t\t(project "{project}"\n\t\t\t\t(path "/{sheet_uuid}"\n'
               f'\t\t\t\t\t(reference "{ref}")\n\t\t\t\t\t(unit 1)\n\t\t\t\t)\n\t\t\t)\n\t\t)\n\t)')
    return "".join(out)


def wire_block(a, b):
    return (f"(wire\n\t\t(pts\n\t\t\t(xy {fmt(a[0])} {fmt(a[1])}) (xy {fmt(b[0])} {fmt(b[1])})\n\t\t)\n"
            f"\t\t(stroke\n\t\t\t(width 0)\n\t\t\t(type default)\n\t\t)\n\t\t(uuid \"{uuid.uuid4()}\")\n\t)")


def label_block(name, p, ang, justify):
    return (f'(label "{name}"\n\t\t(at {fmt(p[0])} {fmt(p[1])} {fmt(ang)})\n\t\t(effects\n\t\t\t(font\n'
            f"\t\t\t\t(size 1.27 1.27)\n\t\t\t)\n\t\t\t(justify {justify})\n\t\t)\n\t\t(uuid \"{uuid.uuid4()}\")\n\t)")


def no_connect_block(p):
    return f'(no_connect\n\t\t(at {fmt(p[0])} {fmt(p[1])})\n\t\t(uuid "{uuid.uuid4()}")\n\t)'


def stock_symbol(lib_id):
    """Bloco do símbolo na biblioteca stock, já com o nome qualificado e a indentação de (lib_symbols)."""
    lib, name = lib_id.split(":")
    text = open(os.path.join(STOCK_SYMBOLS, lib + ".kicad_sym"), encoding="utf-8").read()
    i = text.index(f'(symbol "{name}"')
    blk = text[i:block_end(text, i)]
    blk = blk.replace(f'(symbol "{name}"', f'(symbol "{lib_id}"', 1)
    linhas = blk.split("\n")
    return "\n".join([linhas[0]] + [("\t" + ln if ln else ln) for ln in linhas[1:]])


def lib_bboxes(text, libs_span):
    """Caixa (em coordenadas locais, Y para cima) de cada símbolo de biblioteca, pelos seus desenhos e pinos."""
    ls, le = libs_span
    boxes = {}
    for s, e in top_blocks(text, "symbol", ls, le, indent="\t\t"):
        blk = text[s:e]
        name = re.match(r'\(symbol "([^"]+)"', blk).group(1)
        xs, ys = [], []
        for m in re.finditer(r"\((?:start|end|xy|center|at) (-?[\d.]+) (-?[\d.]+)", blk):
            xs.append(float(m.group(1)))
            ys.append(float(m.group(2)))
        boxes[name] = (min(xs), min(ys), max(xs), max(ys)) if xs else (-1, -1, 1, 1)
    return boxes


def cruza(a, b, folga=0.0):
    return a[0] - folga < b[2] and b[0] - folga < a[2] and a[1] - folga < b[3] and b[1] - folga < a[3]


def main(path, placa):
    cfg = PLACAS[placa]
    text, libs, syms, labels, wires = parse(path)
    if any(s["ref"] == cfg["ref"] for s in syms):
        print(f"{cfg['ref']} já existe em {path}: nada a fazer")
        return 0
    if any(s["ref"] == cfg["cap"] for s in syms):
        sys.exit(f"{cfg['cap']} já existe com outro papel: escolher outra referência para o capacitor")
    project = re.search(r'\(project "([^"]+)"', text).group(1)
    sheet_uuid = re.search(r'\(uuid "([^"]+)"', text).group(1)
    ls = text.index("(lib_symbols")
    le = block_end(text, ls)

    # --- símbolos de biblioteca que faltam ------------------------------------------------
    lib_add = "".join("\n\t\t" + stock_symbol(lid) for lid in (LIB_HEADER, LIB_CAP) if lid not in libs)
    text2 = text[:le - 1] + lib_add + text[le - 1:] if lib_add else text
    # re-parse (as posições mudaram com a inserção)
    open_tmp = text2
    libs_span = (open_tmp.index("(lib_symbols"), block_end(open_tmp, open_tmp.index("(lib_symbols")))
    boxes = lib_bboxes(open_tmp, libs_span)

    # --- geometria do header (rot 0): pino k em (x-5.08, y-7.62+2.54(k-1)), sai para -x ----
    hx, hy = cfg["at"]
    novos = []           # blocos a inserir antes de (sheet_instances)
    caixas_novas = []    # (x0,y0,x1,y1) na folha, para a checagem de colisão
    hb = boxes[LIB_HEADER]
    caixas_novas.append((hx + hb[0], hy - hb[3], hx + hb[2], hy - hb[1]))
    for k in range(1, 9):
        px, py = hx - 5.08, hy - 7.62 + 2.54 * (k - 1)
        net = cfg["pinos"][str(k)]
        if net is None:
            novos.append(no_connect_block((px, py)))
            continue
        L = stub_len(net)
        end = (px - L, py)
        ang, just = LABEL_STYLE[(-1, 0)]
        novos.append(wire_block((px, py), end))
        novos.append(label_block(net, end, ang, just))
        caixas_novas.append((end[0], py - 1.6, px, py + 0.3))          # fio + texto do label acima dele
    # --- capacitor (rot 0): pino 1 em (cx, cy-3.81) sai para cima; pino 2 em (cx, cy+3.81) para baixo
    cx, cy = cfg["cap_at"]
    cb = boxes[LIB_CAP]
    caixas_novas.append((cx + cb[0] - 3.5, cy - cb[3], cx + cb[2] + 3.5, cy - cb[1]))   # inclui Ref/Value ao lado
    for pin, (dy, net) in (("1", (-1, cfg["rail"])), ("2", (1, "GND"))):
        p0 = (cx, cy + 3.81 * dy)
        L = stub_len(net)
        end = (cx, p0[1] + L * dy)
        ang, just = LABEL_STYLE[(0, dy)]
        novos.append(wire_block(p0, end))
        novos.append(label_block(net, end, ang, just))
        caixas_novas.append((cx - 0.3, min(p0[1], end[1]), cx + 1.6, max(p0[1], end[1])))

    # --- colisão com o que já existe (símbolos, fios, labels) ----------------------------------
    existentes = []
    for sy in syms:
        b = boxes.get(sy["lib"])
        if b:
            xs = [sy["x"] + b[0], sy["x"] + b[2]]
            ys = [sy["y"] - b[3], sy["y"] - b[1]]
            if sy["rot"] in (90, 270):
                w, h = xs[1] - xs[0], ys[1] - ys[0]
                xs = [sy["x"] - h / 2, sy["x"] + h / 2]
                ys = [sy["y"] - w / 2, sy["y"] + w / 2]
            existentes.append((sy["ref"], (xs[0], ys[0], xs[1], ys[1])))
    for w in wires:
        existentes.append(("fio", (min(w["a"][0], w["b"][0]), min(w["a"][1], w["b"][1]),
                                   max(w["a"][0], w["b"][0]), max(w["a"][1], w["b"][1]))))
    for l in labels:
        n = len(l["name"]) * G
        existentes.append((f"label {l['name']}", (l["x"] - n, l["y"] - 1.6, l["x"] + n, l["y"] + 1.6)))
    conflitos = [(nome, c, e) for c in caixas_novas for nome, e in existentes if cruza(c, e, 1.27)]
    if conflitos:
        for nome, c, e in conflitos[:10]:
            print("colisão:", nome, "novo", tuple(round(v, 2) for v in c), "existente", tuple(round(v, 2) for v in e))
        sys.exit(f"{len(conflitos)} colisões: mudar 'at'/'cap_at' de {placa} em PLACAS")

    # --- blocos dos símbolos colocados -----------------------------------------------------------
    hprops = (prop("Reference", cfg["ref"], hx, hy - 10.16) + prop("Value", HEADER_VALUE, hx, hy + 12.7)
              + prop("Footprint", HEADER_FP, hx, hy, hide=True) + prop("Datasheet", "", hx, hy, hide=True)
              + prop("Description", "Generic connector, single row, 01x08, script generated (kicad-library-utils/schlib/autogen/connector/)", hx, hy, hide=True)
              + prop("LCSC", HEADER_LCSC, hx, hy, hide=True))
    novos.insert(0, symbol_block(LIB_HEADER, cfg["ref"], HEADER_VALUE, (hx, hy), hprops,
                                 [str(k) for k in range(1, 9)], project, sheet_uuid, in_bom=False, in_pos=False))
    cprops = (prop("Reference", cfg["cap"], cx - 3.175, cy, 90) + prop("Value", CAP_VALUE, cx + 3.175, cy, 90)
              + prop("Footprint", CAP_FP, cx + 0.965, cy - 3.81, hide=True) + prop("Datasheet", "", cx, cy, hide=True)
              + prop("Description", "Unpolarized capacitor", cx, cy, hide=True) + prop("LCSC", CAP_LCSC, cx, cy, hide=True))
    novos.insert(1, symbol_block(LIB_CAP, cfg["cap"], CAP_VALUE, (cx, cy), cprops, ["1", "2"],
                                 project, sheet_uuid, in_bom=True, in_pos=True))

    anchor = text2.index("\n\t(sheet_instances")
    text3 = text2[:anchor] + "".join("\n\t" + b for b in novos) + text2[anchor:]
    open(path, "w", encoding="utf-8", newline="\n").write(text3)
    print(f"{cfg['ref']} ({HEADER_LCSC}) em {cfg['at']} e {cfg['cap']} ({CAP_LCSC}) em {cfg['cap_at']}: "
          f"{sum(1 for b in novos if b.startswith('(wire'))} fios, {sum(1 for b in novos if b.startswith('(label'))} labels, "
          f"{sum(1 for b in novos if b.startswith('(no_connect'))} no_connect; símbolos de biblioteca acrescentados: "
          f"{[lid for lid in (LIB_HEADER, LIB_CAP) if lid not in libs] or 'nenhum'}")
    return 0


if __name__ == "__main__":
    if len(sys.argv) != 4 or sys.argv[2] != "--placa" or sys.argv[3] not in PLACAS:
        sys.exit(__doc__)
    sys.exit(main(sys.argv[1], sys.argv[3]))
