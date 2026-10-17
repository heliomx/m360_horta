"""Gera footprints da família M360 em hardware/lib/M360.pretty.

- M360-RF_Soquete_2x05: cópia do PinSocket_2x05 stock (a coluna par já fica em x negativo,
  o mesmo lado do J1 do mezanino, que está na face de baixo) + contorno do mezanino e do SMA
  na F.Fab, para o contorno andar junto com o soquete no posicionamento.
- Arduino_ProMini_DeeKRobot: 2 fileiras de 12 a 15,24 mm + furos internos A4/A5 e A6/A7/GND.
  Posição dos furos internos MEDIDA POR FOTO (promini_medida.py) — medir com paquímetro
  antes de fabricar; ver ARQUITETURA_FAMILIA.md §9.

Uso: python gen_fp.py     (reescreve os dois .kicad_mod)
"""
import os

HW = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(HW, "lib", "M360.pretty")
KICAD_FP = os.environ.get("KICAD10_FOOTPRINT_DIR",
                          r"C:\Program Files\KiCad\10.0\share\kicad\footprints")
STOCK = os.path.join(KICAD_FP, "Connector_PinSocket_2.54mm.pretty",
                     "PinSocket_2x05_P2.54mm_Vertical.kicad_mod")


def line(x1, y1, x2, y2, layer, width=0.1, kind="solid"):
    return (f"\t(fp_line\n\t\t(start {x1:g} {y1:g})\n\t\t(end {x2:g} {y2:g})\n"
            f"\t\t(stroke\n\t\t\t(width {width:g})\n\t\t\t(type {kind})\n\t\t)\n\t\t(layer \"{layer}\")\n\t)\n")


def rect(x1, y1, x2, y2, layer, width=0.1, kind="solid"):
    return (line(x1, y1, x2, y1, layer, width, kind) + line(x2, y1, x2, y2, layer, width, kind) +
            line(x2, y2, x1, y2, layer, width, kind) + line(x1, y2, x1, y1, layer, width, kind))


def text(s, x, y, layer, size=1.0, angle=0):
    return (f"\t(fp_text user \"{s}\"\n\t\t(at {x:g} {y:g} {angle:g})\n\t\t(layer \"{layer}\")\n"
            f"\t\t(effects\n\t\t\t(font\n\t\t\t\t(size {size:g} {size:g})\n\t\t\t\t(thickness {size * 0.15:g})\n\t\t\t)\n\t\t)\n\t)\n")


def pad(num, x, y, shape="circle", size=1.9, drill=1.2):
    return (f"\t(pad \"{num}\" thru_hole {shape}\n\t\t(at {x:g} {y:g})\n\t\t(size {size:g} {size:g})\n"
            f"\t\t(drill {drill:g})\n\t\t(layers \"*.Cu\" \"*.Mask\")\n\t\t(remove_unused_layers no)\n\t)\n")


def prop(name, value, x, y, layer, hide=False):
    return (f"\t(property \"{name}\" \"{value}\"\n\t\t(at {x:g} {y:g} 0)\n\t\t(layer \"{layer}\")\n"
            + ("\t\t(hide yes)\n" if hide else "") +
            "\t\t(effects\n\t\t\t(font\n\t\t\t\t(size 1 1)\n\t\t\t\t(thickness 0.15)\n\t\t\t)\n\t\t)\n\t)\n")


def socket():
    src = open(STOCK, encoding="utf-8").read()
    src = src.replace('(footprint "PinSocket_2x05_P2.54mm_Vertical"', '(footprint "M360-RF_Soquete_2x05"', 1)
    src = src.replace('(property "Value" "PinSocket_2x05_P2.54mm_Vertical"', '(property "Value" "M360-RF_Soquete_2x05"', 1)
    src = src.replace(
        '(descr "Through hole straight socket strip, 2x05, 2.54mm pitch, double rows (from Kicad 4.0.7), script generated")',
        '(descr "Soquete 2x05 2,54 mm do mezanino M360-RF (PinSocket_2x05 stock). F.Fab: contorno do mezanino '
        '34,2 x 23 mm e do SMA de borda. Mezanino em ~11 mm de altura: SMD baixo pode ficar embaixo; nada alto")', 1)
    # contorno do mezanino, relativo ao pino 1 (MISO): pino 1 fica a 7,00 mm da borda esquerda e
    # 6,42 mm da borda de cima do mezanino (ARQUITETURA §3)
    extra = rect(-7.0, -6.42, 27.2, 16.58, "F.Fab", 0.12, "dash")
    extra += text("M360-RF (mezanino)", 10.1, -4.9, "F.Fab")
    # SMA de borda: eixo em y = 18,50 mm do mezanino → 12,08 mm abaixo do pino 1; avança ~10 mm
    extra += rect(27.2, 12.08 - 3.2, 37.2, 12.08 + 3.2, "F.Fab", 0.12, "dash")
    extra += text("SMA", 32.2, 12.08, "F.Fab", 0.8)
    anchor = '\t(fp_text user "${REFERENCE}"'
    src = src.replace(anchor, extra + anchor, 1)
    return src


def promini():
    # origem no pino 1 (TXO, canto de cima à esquerda, header FTDI para cima)
    s = ["(footprint \"Arduino_ProMini_DeeKRobot\"\n\t(version 20260206)\n\t(generator \"m360-gen_fp\")\n"
         "\t(layer \"F.Cu\")\n"
         "\t(descr \"Arduino Pro Mini 3,3 V (clone DeeK-Robot) soldado por barra de pinos. Fileiras laterais a 15,24 mm. "
         "Furos A4/A5 (internos) e GND/A6/A7 (borda de baixo) ESTIMADOS por foto: medir antes de fabricar\")\n"
         "\t(tags \"arduino pro mini module m360\")\n"]
    s.append(prop("Reference", "REF**", 7.62, -4.4, "F.SilkS"))
    s.append(prop("Value", "Arduino_ProMini_DeeKRobot", 7.62, 31.2, "F.Fab"))
    s.append("\t(attr through_hole)\n\t(duplicate_pad_numbers_are_jumpers no)\n")
    # placa do módulo: 17,78 x 33,0 mm
    x1, x2, y1, y2 = -1.27, 16.51, -3.17, 29.85
    s.append(rect(x1, y1, x2, y2, "F.SilkS", 0.12))
    s.append(rect(x1, y1, x2, y2, "F.Fab", 0.1))
    s.append(rect(x1 - 0.25, y1 - 0.25, x2 + 0.25, y2 + 0.25, "F.CrtYd", 0.05))
    # header FTDI em ângulo: os pinos avançam ~6 mm além da borda de cima — manter livre
    s.append(rect(1.27 - 1.27, -9.5, 13.97 + 1.27, y1, "F.Fab", 0.1, "dash"))
    s.append(text("FTDI (manter livre)", 7.62, -6.3, "F.Fab", 0.8))
    s.append(text("A4-A7: MEDIR", 7.62, 18.0, "F.Fab", 0.8))
    s.append(text("${REFERENCE}", 7.62, 13.0, "F.Fab", 1.0))
    # marca do pino 1
    s.append(line(-1.27, 1.27, -2.0, 1.27, "F.SilkS", 0.12))
    # fileira esquerda: 1 TXO, 2 RXI, 3 RST, 4 GND, 5..12 = D2..D9 (furo 1,0 mm, pino da barra)
    for n in range(1, 13):
        s.append(pad(n, 0, (n - 1) * 2.54, "rect" if n == 1 else "circle", size=1.7, drill=1.0))
    # fileira direita, de baixo para cima: 13 D10 ... 24 RAW (RAW no topo)
    for n in range(13, 25):
        s.append(pad(n, 15.24, (24 - n) * 2.54, size=1.7, drill=1.0))
    # Furos livres, medidos por foto (homografia pelos 4 pinos de canto, ver ARQUITETURA §9):
    # A5/A4 uma coluna para dentro da fileira direita, a meio passo entre VCC/A3 e A3/A2;
    # GND/A6/A7 na borda de baixo, passo 2,54 confirmado, y = 28,3 ± 0,3.
    # Furo de 1,2 mm (e não 1,0) para absorver a incerteza da medição.
    s.append(pad(26, 12.7, 8.89))
    s.append(pad(25, 12.7, 11.43))
    s.append(pad(29, 7.62, 28.3))
    s.append(pad(27, 10.16, 28.3))
    s.append(pad(28, 12.7, 28.3))
    s.append("\t(embedded_fonts no)\n)\n")
    return "".join(s)


def main():
    os.makedirs(OUT, exist_ok=True)
    for name, body in (("M360-RF_Soquete_2x05", socket()), ("Arduino_ProMini_DeeKRobot", promini())):
        path = os.path.join(OUT, name + ".kicad_mod")
        open(path, "w", encoding="utf-8", newline="\n").write(body)
        print("gravado", path)


if __name__ == "__main__":
    main()
