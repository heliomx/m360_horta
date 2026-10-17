"""Recorta regiões (em mm da folha A3) de um PDF de esquemático e salva PNGs.

Serve para conferir visualmente um trecho do desenho sem abrir o KiCad — é assim que se
verifica o resultado do fix_layout.py folha por folha. Precisa de pymupdf.

Uso: python crop.py <arquivo.pdf> <saida_prefixo> [nome x1 y1 x2 y2]...
Sem regiões, usa as predefinidas da M360-LP.
"""
import sys

import pymupdf

PRESETS = {
    "power": (15, 15, 170, 78),
    "vsw": (180, 10, 300, 72),
    "mcu": (15, 88, 125, 170),
    "mux": (118, 78, 195, 162),
    "soil": (195, 84, 370, 205),
    "bottom": (15, 178, 200, 262),
    "tvs": (188, 200, 385, 262),
}

MM = 72 / 25.4


def main():
    pdf, prefix = sys.argv[1], sys.argv[2]
    rest = sys.argv[3:]
    regions = {}
    if rest:
        for i in range(0, len(rest), 5):
            regions[rest[i]] = tuple(float(v) for v in rest[i + 1:i + 5])
    else:
        regions = PRESETS
    page = pymupdf.open(pdf)[0]
    for name, (x1, y1, x2, y2) in regions.items():
        clip = pymupdf.Rect(x1 * MM, y1 * MM, x2 * MM, y2 * MM)
        zoom = min(6.0, 1800 / clip.width)
        pix = page.get_pixmap(matrix=pymupdf.Matrix(zoom, zoom), clip=clip)
        out = f"{prefix}_{name}.png"
        pix.save(out)
        print(out, pix.width, "x", pix.height)


if __name__ == "__main__":
    main()
