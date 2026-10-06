#!/usr/bin/env python3
"""Affiches des vidéos en PNG à palette : 256 couleurs (coupe médiane, sans tramage), quatre fois plus légères.

    python3 Zoltan/demos/tools/palette_png.py IMAGE.png [IMAGE.png ...]

Les images des vidéos n'ont que des aplats, du texte et des points : l'écart moyen reste sous 0,2 niveau sur 255 ;
seuls les bords lissés changent. Appelé par tools/render_duel.cjs quand Pillow est présent.
Les couleurs des deux thèmes (player/duel.js, player/supports.js) sont réservées dans la palette : sans cela, une
couleur rare dans l'image (le rouge d'une pastille de légende quand rien n'est réuni) est fondue dans une voisine.
"""
import sys

from PIL import Image

RESERVED = ['#071b2e', '#0f2c48', '#3a5773', '#1d4466', '#eaf5f7', '#9fb4c4', '#f0606e', '#62b3ff', '#ffc53d',
            '#5ee8c8', '#585864', '#7d8ba0', '#ffffff', '#f4fafb', '#b9cad4', '#d3e0e6', '#082c4c', '#4a5f71',
            '#a8102c', '#1f5fbf', '#c27a00', '#00897b', '#b8b8c7', '#8f9cad']


def main():
    reserved = [int(h[i:i + 2], 16) for h in RESERVED for i in (1, 3, 5)]
    free = 256 - len(RESERVED)
    for path in sys.argv[1:]:
        image = Image.open(path).convert('RGB')
        cut = image.quantize(colors=free, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
        palette = Image.new('P', (1, 1))
        palette.putpalette(cut.getpalette()[:3 * free] + reserved)
        image.quantize(palette=palette, dither=Image.Dither.NONE).save(path, optimize=True)


if __name__ == '__main__':
    main()
