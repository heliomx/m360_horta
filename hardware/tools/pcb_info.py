"""Lista os footprints de uma placa com o courtyard (ou a caixa envolvente) em mm.

Uso: <KiCad>/bin/python.exe pcb_info.py <placa.kicad_pcb>
"""
import sys

import pcbnew

MM = 1e6


def bbox(fp):
    for layer in (pcbnew.F_CrtYd, pcbnew.B_CrtYd):
        poly = fp.GetCourtyard(layer)
        if poly.OutlineCount():
            b = poly.BBox()
            return b.GetX() / MM, b.GetY() / MM, b.GetWidth() / MM, b.GetHeight() / MM, "crt"
    b = fp.GetBoundingBox(False)
    return b.GetX() / MM, b.GetY() / MM, b.GetWidth() / MM, b.GetHeight() / MM, "bb"


def main(path):
    board = pcbnew.LoadBoard(path)
    fps = sorted(board.GetFootprints(), key=lambda f: f.GetReference())
    for fp in fps:
        p = fp.GetPosition()
        x, y, w, h, kind = bbox(fp)
        print(f"{fp.GetReference():6} {str(fp.GetFPID().GetUniStringLibId()):48} "
              f"pos=({p.x / MM:.2f},{p.y / MM:.2f}) rot={fp.GetOrientationDegrees():.0f} "
              f"{'B' if fp.IsFlipped() else 'F'} {kind} {w:.2f}x{h:.2f} "
              f"(desloc. da origem: {x - p.x / MM:+.2f},{y - p.y / MM:+.2f})")
    print(len(fps), "footprints")
    e = board.GetBoardEdgesBoundingBox()
    print("contorno:", e.GetX() / MM, e.GetY() / MM, e.GetWidth() / MM, e.GetHeight() / MM)


if __name__ == "__main__":
    main(sys.argv[1])
