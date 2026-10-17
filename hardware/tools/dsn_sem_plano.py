"""Tira do DSN as declarações `plane`, para o Freerouting rotear o GND como net comum.

Placa que já tem zona de GND exporta o DSN com `(plane GND (polygon ...))` nas duas faces, e o
Freerouting então NÃO desenha um fio de terra sequer — confia no plano. Só que o plano, sozinho,
se parte em ilhas: a LP rerroteada assim saiu com 16 `unconnected_items`, todos "Zona [GND]".

O primeiro roteamento das placas funcionou porque foi feito antes de as zonas existirem: o GND
foi roteado como qualquer outra net (88 segmentos na LP em `bf701dc`) e o plano preencheu por
cima. Este script reproduz essa condição sem mexer nas zonas da placa: a net GND continua na
seção `network` do DSN, e só a declaração de plano sai.

Uso: python dsn_sem_plano.py <entrada.dsn> <saida.dsn>
"""
import re
import sys


def main(entrada, saida):
    texto = open(entrada, encoding="utf-8", newline="").read()
    novo, n = re.subn(r"\r?\n[ \t]*\(plane [^\s()]+ \(polygon [^)]*\)\)", "", texto)
    if n == 0:
        sys.exit("nenhuma declaração plane encontrada — o DSN já roteia todas as nets")
    if "(plane" in novo:
        sys.exit("sobrou declaração plane em formato não reconhecido — nada foi gravado")
    open(saida, "w", encoding="utf-8", newline="").write(novo)
    print(f"{n} declaração(ões) plane removida(s) -> {saida}")


if __name__ == "__main__":
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    main(sys.argv[1], sys.argv[2])
