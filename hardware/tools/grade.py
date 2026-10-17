"""Desenha uma grade de pixels sobre a foto e recorta regiões ampliadas, para leitura visual
das coordenadas dos furos. É daqui que saem os quatro pontos de referência do promini_medida.py.

Faz também uma detecção de círculos conservadora, só como conferência.
Precisa de opencv-python-headless e numpy.

Uso: python grade.py <foto.jpg> <prefixo_saida> [x1 y1 x2 y2 nome]...
Sem regiões, gera só a imagem inteira com grade.
"""
import sys

import cv2
import numpy as np

PASSO = 50          # linhas finas a cada 50 px
ROTULO = 100        # linhas grossas e rótulos a cada 100 px


def grade(img):
    out = img.copy()
    h, w = out.shape[:2]
    for x in range(0, w, PASSO):
        forte = x % ROTULO == 0
        cv2.line(out, (x, 0), (x, h), (0, 255, 255) if forte else (0, 128, 128), 2 if forte else 1)
    for y in range(0, h, PASSO):
        forte = y % ROTULO == 0
        cv2.line(out, (0, y), (w, y), (0, 255, 255) if forte else (0, 128, 128), 2 if forte else 1)
    for x in range(0, w, ROTULO):
        for y in range(0, h, ROTULO):
            cv2.putText(out, f"{x},{y}", (x + 3, y + 18), cv2.FONT_HERSHEY_SIMPLEX, 0.45, (0, 0, 255), 1)
    return out


def candidatos(img):
    """Hough conservador mirando o furo (raio ~10–26 px), não o anel de solda."""
    gray = cv2.medianBlur(cv2.cvtColor(img, cv2.COLOR_BGR2GRAY), 5)
    for p2 in (45, 40, 35, 30):
        c = cv2.HoughCircles(gray, cv2.HOUGH_GRADIENT, dp=1, minDist=60,
                             param1=120, param2=p2, minRadius=10, maxRadius=26)
        if c is not None and len(c[0]) >= 24:
            return np.round(c[0]).astype(int), p2
    return (np.round(c[0]).astype(int), p2) if c is not None else (np.empty((0, 3), int), None)


def main():
    path, prefixo = sys.argv[1], sys.argv[2]
    resto = sys.argv[3:]
    img = cv2.imread(path)
    if img is None:
        sys.exit(f"não abriu {path}")
    h, w = img.shape[:2]
    print(f"{path}: {w}x{h}px")

    circ, p2 = candidatos(img)
    print(f"candidatos (param2={p2}): {len(circ)}")
    for i, (x, y, r) in enumerate(sorted(circ.tolist(), key=lambda c: (c[1], c[0]))):
        print(f"   #{i:2d} x={x:4d} y={y:4d} r={r}")

    base = grade(img)
    for x, y, r in circ:
        cv2.circle(base, (x, y), r, (255, 0, 255), 2)

    cv2.imwrite(f"{prefixo}_inteira.png", base)
    print("gravada:", f"{prefixo}_inteira.png")

    for i in range(0, len(resto), 5):
        x1, y1, x2, y2 = (int(v) for v in resto[i:i + 4])
        nome = resto[i + 4]
        rec = base[max(0, y1):min(h, y2), max(0, x1):min(w, x2)]
        rec = cv2.resize(rec, None, fx=2, fy=2, interpolation=cv2.INTER_CUBIC)
        cv2.imwrite(f"{prefixo}_{nome}.png", rec)
        print("gravada:", f"{prefixo}_{nome}.png", f"(região {x1},{y1} a {x2},{y2}, zoom 2x)")


if __name__ == "__main__":
    main()
