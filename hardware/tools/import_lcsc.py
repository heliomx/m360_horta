"""Traz os footprints do LCSC (easyeda2kicad) para hardware/lib e, opcionalmente, atribui os
footprints das peças manuais no esquemático da M360-LP.

Antes de rodar, baixar as peças com o easyeda2kicad (num venv qualquer):

    pip install easyeda2kicad
    easyeda2kicad --full --lcsc_id=C385834 --output <dir>/M360_LCSC

O --output cria <dir>/M360_LCSC.pretty e <dir>/M360_LCSC.3dshapes; passe <dir> aqui.

Uso: python import_lcsc.py <dir_do_easyeda2kicad> [--sch]
     --sch também grava o campo Footprint das peças manuais no esquemático da LP.
"""
import os
import re
import shutil
import sys

from schlib import block_end, top_blocks

HW = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LIB = os.path.join(HW, "lib")
SCH = os.path.join(HW, "m360-lp", "M360-LP.kicad_sch")

PARTS = [
    # (arquivo do easyeda2kicad, nome novo, modelo 3D)
    ("RJ45-TH_R-RJ45R08P-A004", "RJ45_R-RJ45R08P-A004", "RJ45-TH_L15.9-W15.1-H12.6"),
    ("SENSOR-SMD_59170-1-S-00-D", "Reed_Littelfuse_59170", "SENSOR-SMD_L11.4-W2.3-H2.3-59170-1-S-00-D"),
    # borne de entrada da ACT: o nome do easyeda diz P5.00, mas os furos estão a 5,08 mm
    ("CONN-TH_2P-P5.00_WJ500V-5.08-2P", "TerminalBlock_WJ500V-5.08-2P", "CONN-TH_2P-P5.00_WJ500V-5.08-2P"),
]

FOOTPRINTS = {
    "A1": "M360:Arduino_ProMini_DeeKRobot",
    "J1": "M360:RJ45_R-RJ45R08P-A004",
    "J2": "M360:RJ45_R-RJ45R08P-A004",
    "J4": "M360:M360-RF_Soquete_2x05",
    "SW1": "M360:Reed_Littelfuse_59170",   # reed de volta em 2026-09-13 (o botão durou um dia)
}


def import_parts(e2k):
    os.makedirs(os.path.join(LIB, "M360.pretty"), exist_ok=True)
    os.makedirs(os.path.join(LIB, "M360.3dshapes"), exist_ok=True)
    for src, name, model in PARTS:
        t = open(os.path.join(e2k, "M360_LCSC.pretty", src + ".kicad_mod"), encoding="utf-8").read()
        t = t.replace(f"(module easyeda2kicad:{src}", f"(module {name}", 1)
        t = t.replace(f"(fp_text value {src}", f"(fp_text value {name}", 1)
        # pinos-guia de plástico: furo não metalizado (o easyeda2kicad gera PTH sem anel)
        t = re.sub(r'\(pad "" thru_hole circle', '(pad "" np_thru_hole circle', t)
        t = re.sub(r'\(model "[^"]*"', f'(model "${{KIPRJMOD}}/../lib/M360.3dshapes/{model}.step"', t)
        open(os.path.join(LIB, "M360.pretty", name + ".kicad_mod"), "w", encoding="utf-8", newline="\n").write(t)
        shutil.copy(os.path.join(e2k, "M360_LCSC.3dshapes", model + ".step"), os.path.join(LIB, "M360.3dshapes"))
        print("footprint", name)
    print("o modelo 3D do RJ45 vem centrado no plano da placa: rodar fix_rj45_3d.py em seguida")


def assign_footprints():
    text = open(SCH, encoding="utf-8").read()
    ls = text.index("(lib_symbols")
    le = block_end(text, ls)
    edits = []
    for s, e in top_blocks(text, "symbol", le):
        blk = text[s:e]
        ref = re.search(r'\(property "Reference" "([^"]+)"', blk).group(1)
        if ref in FOOTPRINTS:
            new = re.sub(r'\(property "Footprint" "[^"]*"', f'(property "Footprint" "{FOOTPRINTS[ref]}"', blk, count=1)
            edits.append((s, e, new))
            print("footprint de", ref, "=", FOOTPRINTS[ref])
    for s, e, new in sorted(edits, reverse=True):
        text = text[:s] + new + text[e:]
    open(SCH, "w", encoding="utf-8", newline="\n").write(text)


if __name__ == "__main__":
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    if not args:
        sys.exit(__doc__)
    import_parts(args[0])
    # A atribuição de footprints mexe no esquemático da LP: só roda quando pedida.
    if "--sch" in sys.argv:
        assign_footprints()
