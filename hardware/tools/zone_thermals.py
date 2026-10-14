"""Ajusta os parâmetros de alívio térmico das zonas de uma placa.

O preenchimento dos planos de terra costuma acusar `starved_thermal` em ilha PTH de conector:
o DRC exige 2 raios por ilha e o padrão do KiCad (haste 0,5 mm, folga 0,5 mm) não cabe no anel
apertado dessas ilhas. Hastes mais finas com folga menor normalmente resolvem — e é o que se
deve tentar ANTES de partir para ligação sólida (`pad_zone_connect.py`), porque é o alívio
térmico que torna a solda manual do conector viável.

Valores usados na família M360: haste 0,35 mm, folga 0,3 mm (ver ARQUITETURA §11.4).

Trabalha no texto do arquivo: não usa o pcbnew. Age em TODAS as zonas da placa — os blocos
`(zone ...)` de nível superior —, mas ignora zonas de keep-out, que não têm preenchimento
(o footprint do D1 mini traz uma, sobre a antena).

Idempotente: reescreve os valores se já existirem.

Uso: python zone_thermals.py <placa.kicad_pcb> [haste] [folga]
     sem os números, aplica 0.35 e 0.3
"""
import re
import sys

from schlib import block_end


def main(path, haste, folga):
    texto = open(path, encoding="utf-8", newline="").read()

    # zonas de nível superior: "\n\t(zone" — as de dentro de footprint têm mais indentação
    spans = []
    for m in re.finditer(r"\n\t\(zone\b", texto):
        ini = m.start() + 1
        spans.append((ini, block_end(texto, ini)))

    if not spans:
        sys.exit("nenhuma zona de nível superior encontrada")

    saida, pos, mexidas, puladas = [], 0, 0, 0
    for ini, fim in spans:
        bloco = texto[ini:fim]
        if "(keepout" in bloco:
            puladas += 1
            continue
        # as zonas gravam ora (net_name "GND"), ora (net "GND") — aceitar as duas formas,
        # senão o relato sai com "?" no lugar da rede e convida a leitura errada
        rede = re.search(r'\(net(?:_name)? "([^"]*)"\)', bloco)
        camada = re.search(r'\(layer "([^"]*)"\)', bloco)
        novo, n_g = re.subn(r"\(thermal_gap [\d.]+\)", f"(thermal_gap {folga})", bloco)
        novo, n_b = re.subn(r"\(thermal_bridge_width [\d.]+\)", f"(thermal_bridge_width {haste})", novo)
        if n_g or n_b:
            saida.append(texto[pos:ini])
            saida.append(novo)
            pos = fim
            mexidas += 1
            print(f"zona [{rede.group(1) if rede else '?'}] em {camada.group(1) if camada else '?'}: "
                  f"haste {haste} mm, folga {folga} mm")
    saida.append(texto[pos:])

    if not mexidas:
        sys.exit("nenhuma zona alterada — os campos térmicos não foram encontrados")
    open(path, "w", encoding="utf-8", newline="").write("".join(saida))
    print(f"{mexidas} zona(s) alterada(s), {puladas} de keep-out ignorada(s)")
    print("preencher de novo: kicad-cli pcb drc --refill-zones --save-board ...")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    main(sys.argv[1],
         sys.argv[2] if len(sys.argv) > 2 else "0.35",
         sys.argv[3] if len(sys.argv) > 3 else "0.3")
