"""Mede a posição dos furos livres do Pro Mini (A4/A5 e GND/A6/A7) a partir de uma foto.

Os furos abertos são escuros e sem solda: saem bem num Hough conservador — e é justamente a
detecção MAIS seletiva que acerta. Afrouxar o param2 para "achar mais furos" enche a imagem
de falsos positivos nas bolhas de solda; a escala vem de quatro pinos de canto das fileiras
laterais, informados em pixels de forma aproximada (use o grade.py para lê-los), que o script
refina pelo centroide da bolha de solda numa janela em volta do ponto dado.

    pino 1  (TXO)  -> x=0      y=0
    pino 12 (D9)   -> x=0      y=27.94
    pino 24 (RAW)  -> x=15.24  y=0
    pino 13 (D10)  -> x=15.24  y=27.94

Precisa de opencv-python-headless e numpy.

Uso:
    python promini_medida.py <foto.jpg> --ref x1,y1 x12,y12 x24,y24 x13,y13 [--janela 30] [debug.png]
"""
import sys

import cv2
import numpy as np

DST = np.array([(0.0, 0.0), (0.0, 27.94), (15.24, 0.0), (15.24, 27.94)], np.float32)
NOMES = ("pino 1 (TXO)", "pino 12 (D9)", "pino 24 (RAW)", "pino 13 (D10)")


def refina(gray, x, y, janela):
    """Centroide da região clara (bolha de solda) em volta de (x,y)."""
    h, w = gray.shape
    x0, x1 = max(0, int(x - janela)), min(w, int(x + janela))
    y0, y1 = max(0, int(y - janela)), min(h, int(y + janela))
    win = gray[y0:y1, x0:x1].astype(float)
    if win.size == 0:
        return (x, y), 0.0
    thr = np.percentile(win, 75)
    peso = np.clip(win - thr, 0, None)
    if peso.sum() < 1e-6:
        return (x, y), 0.0
    ys, xs = np.mgrid[y0:y1, x0:x1]
    cx = (peso * xs).sum() / peso.sum()
    cy = (peso * ys).sum() / peso.sum()
    return (cx, cy), float(np.hypot(cx - x, cy - y))


def holes(img):
    gray = cv2.medianBlur(cv2.cvtColor(img, cv2.COLOR_BGR2GRAY), 5)
    melhor = (np.empty((0, 3), int), None)
    for p2 in (45, 40, 35, 30, 25):
        c = cv2.HoughCircles(gray, cv2.HOUGH_GRADIENT, dp=1, minDist=60, param1=120,
                             param2=p2, minRadius=10, maxRadius=26)
        if c is None:
            continue
        melhor = (np.round(c[0]).astype(int), p2)
        if len(c[0]) >= 5:
            break
    return melhor


def main():
    path = sys.argv[1]
    if "--ref" not in sys.argv:
        sys.exit(__doc__)
    i = sys.argv.index("--ref")
    ref = [tuple(float(v) for v in a.split(",")) for a in sys.argv[i + 1:i + 5]]
    args = sys.argv[i + 5:]
    janela = 30
    if "--janela" in args:
        j = args.index("--janela")
        janela = int(args[j + 1])
        args = args[:j] + args[j + 2:]
    debug = args[0] if args else None

    img = cv2.imread(path)
    if img is None:
        sys.exit(f"não abriu {path}")
    h, w = img.shape[:2]
    gray = cv2.GaussianBlur(cv2.cvtColor(img, cv2.COLOR_BGR2GRAY), (5, 5), 0)
    print(f"{path}: {w}x{h}px; janela de refino ±{janela}px")

    src = []
    for (x, y), nome in zip(ref, NOMES):
        (cx, cy), d = refina(gray, x, y, janela)
        print(f"   {nome:14} informado ({x:.0f},{y:.0f}) -> refinado ({cx:.1f},{cy:.1f})  desloc. {d:.1f}px")
        src.append((cx, cy))
    src = np.array(src, np.float32)
    Hm = cv2.getPerspectiveTransform(src, DST)

    # escala implícita: distância entre os pinos de canto em px por mm
    esc = [np.hypot(*(src[a] - src[b])) / d for a, b, d in ((0, 1, 27.94), (2, 3, 27.94), (0, 2, 15.24), (1, 3, 15.24))]
    print("   escala px/mm nas quatro bordas: " + ", ".join(f"{e:.2f}" for e in esc))

    hs, p2 = holes(img)
    print(f"furos abertos: {len(hs)} (param2={p2})")
    if not len(hs):
        return
    mm = cv2.perspectiveTransform(hs[:, :2].astype(np.float32).reshape(-1, 1, 2), Hm).reshape(-1, 2)
    for (x, y), c in sorted(zip(mm, hs.tolist()), key=lambda t: (t[0][1], t[0][0])):
        dentro = -3 < x < 18.5 and -6 < y < 33
        print(f"   px({c[0]:4d},{c[1]:4d}) r={c[2]:2d}  ->  x={x:6.2f}  y={y:6.2f} mm"
              f"   grade: x={x / 2.54:5.2f}  y={y / 2.54:5.2f} passos{'' if dentro else '   (fora)'}")

    if debug:
        vis = img.copy()
        for (x, y), nome in zip(src, NOMES):
            cv2.drawMarker(vis, (int(x), int(y)), (0, 255, 0), cv2.MARKER_CROSS, 40, 4)
            cv2.putText(vis, nome.split()[1], (int(x) - 25, int(y) - 28), cv2.FONT_HERSHEY_SIMPLEX,
                        0.7, (0, 255, 0), 2)
        for (x, y, r), (mx, my) in zip(hs.tolist(), mm):
            cv2.circle(vis, (x, y), r, (255, 0, 255), 3)
            cv2.putText(vis, f"{mx:.1f},{my:.1f}", (x - 70, y - r - 10), cv2.FONT_HERSHEY_SIMPLEX,
                        0.6, (255, 0, 255), 2)
        cv2.imwrite(debug, vis)
        print("debug:", debug)


if __name__ == "__main__":
    main()
