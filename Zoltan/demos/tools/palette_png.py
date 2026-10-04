#!/usr/bin/env python3
"""Affiches des vidéos en PNG à palette : 256 couleurs (coupe médiane, sans tramage), quatre fois plus légères.

    python3 Zoltan/demos/tools/palette_png.py IMAGE.png [IMAGE.png ...]

Les images des vidéos n'ont que des aplats, du texte et des points : l'écart moyen reste sous 0,2 niveau sur 255 ;
seuls les bords lissés changent. Appelé par tools/render_duel.cjs quand Pillow est présent.
"""
import sys

from PIL import Image


def main():
    for path in sys.argv[1:]:
        image = Image.open(path).convert('RGB')
        image.quantize(colors=256, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE).save(path, optimize=True)


if __name__ == '__main__':
    main()
