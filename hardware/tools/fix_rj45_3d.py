"""Corrige o deslocamento Z do modelo 3D do RJ45 C385834 importado pelo easyeda2kicad.

O modelo vem centrado no plano da placa (corpo 6,3 mm acima e 6,3 mm abaixo); pelo datasheet
o corpo de 12,6 mm assenta sobre a placa e só os pinos atravessam. Sobe o modelo 6,3 mm.

Varre a biblioteca E todas as placas da família. O footprint guarda o deslocamento, mas cada
placa carrega a sua própria cópia dele: corrigir só a biblioteca não conserta as placas já
montadas, e corrigir só uma placa deixa as outras erradas — foi o que aconteceu entre a LP e
a ACT. Rodar sempre depois do import_lcsc.py, que regrava o footprint a partir do
easyeda2kicad e traz o deslocamento errado de volta.

Só grava quando o conteúdo muda de fato, para não trocar as quebras de linha à toa.

Uso: python fix_rj45_3d.py
"""
import glob
import os
import re

HW = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
Z = "6.3"

ALVOS = sorted(glob.glob(os.path.join(HW, "lib", "M360.pretty", "*.kicad_mod")) +
               glob.glob(os.path.join(HW, "*", "*.kicad_pcb")))

# o \s* cobre as duas formas: (offset (xyz ...)) numa linha só, e o bloco multilinha do .kicad_pcb
PAT = re.compile(r'(\(model "[^"]*RJ45-TH_L15\.9-W15\.1-H12\.6\.step"\s*\(offset\s*\(xyz )'
                 r'[-\d.]+ [-\d.]+ [-\d.]+(\))')

corrigidos = 0
for path in ALVOS:
    texto = open(path, encoding="utf-8", newline="").read()
    novo, n = PAT.subn(rf"\g<1>0 0 {Z}\g<2>", texto)
    if not n:
        continue
    rel = os.path.relpath(path, HW)
    if novo == texto:
        print(f"{rel}: {n} modelo(s) já em z={Z}")
        continue
    open(path, "w", encoding="utf-8", newline="").write(novo)
    print(f"{rel}: {n} modelo(s) corrigido(s) para z={Z}")
    corrigidos += n

print(f"{corrigidos} modelo(s) alterado(s) em {len(ALVOS)} arquivo(s) examinado(s)")
