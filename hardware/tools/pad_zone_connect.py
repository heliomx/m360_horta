"""Define a forma de ligação de um pad às zonas (alívio térmico ou ligação sólida).

zone_connect: 0 = sem ligação, 1 = alívio térmico, 2 = sólido.
Usado no pino 8 dos RJ45 (GND do chicote), onde o campo de pinos escalonado de 1,02 mm não
deixa espaço para as duas hastes térmicas que o DRC exige.

Trabalha no texto do arquivo e age só no footprint pedido: um sed global pelo pad "8"
atingiria todos os conectores da placa.

Uso: python pad_zone_connect.py <placa.kicad_pcb> <REF> <PAD> <MODO>
"""
import re
import sys

from schlib import block_end


def main(path, ref, pad, modo):
    texto = open(path, encoding="utf-8").read()

    # bloco do footprint com a referência pedida
    alvo = None
    for m in re.finditer(r"\n\t\(footprint ", texto):
        ini = m.start() + 1
        fim = block_end(texto, ini)
        if re.search(r'\(property "Reference" "' + re.escape(ref) + r'"', texto[ini:fim]):
            alvo = (ini, fim)
            break
    if alvo is None:
        sys.exit(f"{ref}: footprint não encontrado")
    ini, fim = alvo
    bloco = texto[ini:fim]

    # bloco do pad dentro dele
    mp = re.search(r'\n\t\t\(pad "' + re.escape(pad) + r'"[^\n]*', bloco)
    if mp is None:
        sys.exit(f"{ref}: pad {pad} não encontrado")
    p_ini = mp.start() + 1
    p_fim = block_end(bloco, p_ini)
    pad_txt = bloco[p_ini:p_fim]

    if "(zone_connect" in pad_txt:
        novo = re.sub(r"\(zone_connect \d+\)", f"(zone_connect {modo})", pad_txt)
        acao = "atualizado"
    else:
        corte = mp.end() - p_ini            # fim da primeira linha do bloco do pad
        novo = pad_txt[:corte] + f"\n\t\t\t(zone_connect {modo})" + pad_txt[corte:]
        acao = "inserido"

    bloco = bloco[:p_ini] + novo + bloco[p_fim:]
    texto = texto[:ini] + bloco + texto[fim:]
    open(path, "w", encoding="utf-8", newline="\n").write(texto)
    nomes = {0: "sem ligação", 1: "alívio térmico", 2: "sólido"}
    print(f"{ref} pad {pad}: zone_connect {acao} = {modo} ({nomes.get(int(modo), '?')})")


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4])
