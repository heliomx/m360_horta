"""Grava a propriedade LCSC em cada símbolo colocado de um .kicad_sch (KiCad 10).

O mapa abaixo é o da M360-LP. Ver as candidatas e o estado do catálogo em
ARQUITETURA_FAMILIA.md §9.1.

Uso: python add_lcsc.py <arquivo.kicad_sch>
Idempotente: se o símbolo já tem LCSC, o valor é substituído.
"""
import re
import sys

from schlib import block_end

LCSC = {
    # capacitores
    **{r: "C15849" for r in ("C1", "C2", "C6")},            # 1 uF 0603 X5R
    "C3": "C15850",                                         # 10 uF 0805 X5R
    **{r: "C14663" for r in ["C4", "C7"] + [f"C{i}" for i in range(8, 16)]},  # 100 nF 0603 X7R
    "C5": "C57112",                                         # 10 nF 0603 X7R
    # resistores
    "R1": "C22976",                                         # 2 M
    "R2": "C22935",                                         # 1 M
    **{r: "C25804" for r in ["R3", "R5", "R23"] + [f"R{i}" for i in range(6, 14)]},  # 10 k
    "R4": "C25803",                                         # 100 k
    **{f"R{i}": "C21190" for i in range(14, 22)},           # 1 k
    "R22": "C23162",                                        # 4,7 k
    **{r: "C22978" for r in ("R24", "R25")},                # 3,3 k
    # semicondutores
    **{r: "C15127" for r in ("Q1", "Q2")},                  # AO3401A
    "U1": "C39051",                                         # MCP1700T-3302E/TT
    "U2": "C9386",                                          # 74HC4051D,653
    **{r: "C558418" for r in ("U3", "U4", "U5")},           # SRV05-4 (TECH PUBLIC)
    # conectores e chave
    **{r: "C385834" for r in ("J1", "J2")},                 # RJ45 8P8C sem blindagem
    "J3": "C158012",                                        # JST B2B-XH-A
    "J4": "C492399",                                        # soquete fêmea 2x5 2,54 mm
    "SW1": "C315909",                                       # reed SMD Littelfuse 59170
}


def main(path):
    text = open(path, encoding="utf-8").read()
    lib = text.index("(lib_symbols")
    pos = block_end(text, lib)
    out = [text[:pos]]
    done = []
    while True:
        m = re.compile(r"\n(\t)\(symbol\b").search(text, pos)
        if not m:
            break
        start = m.start(0) + 1
        end = block_end(text, start + 1)
        out.append(text[pos:start])
        block = text[start:end]
        ref = re.search(r'\(property "Reference" "([^"]+)"', block).group(1)
        if ref in LCSC:
            block = re.sub(r'\n\t\t\(property "LCSC" "[^"]*".*?\n\t\t\)', "", block, flags=re.S)
            at = re.search(r"\(at ([-\d.]+) ([-\d.]+)", block)
            prop = (
                f'\t\t(property "LCSC" "{LCSC[ref]}"\n'
                f"\t\t\t(at {at.group(1)} {at.group(2)} 0)\n"
                f"\t\t\t(hide yes)\n"
                f"\t\t\t(effects\n\t\t\t\t(font\n\t\t\t\t\t(size 1.27 1.27)\n\t\t\t\t)\n\t\t\t)\n"
                f"\t\t)\n"
            )
            idx = block.index('\t\t(property "Datasheet"')
            block = block[:idx] + prop + block[idx:]
            done.append(ref)
        out.append(block)
        pos = end
    out.append(text[pos:])
    open(path, "w", encoding="utf-8", newline="\n").write("".join(out))
    missing = sorted(set(LCSC) - set(done))
    print(f"LCSC gravado em {len(done)} símbolos")
    if missing:
        print("não encontrados no esquemático:", ", ".join(missing))


if __name__ == "__main__":
    main(sys.argv[1])
