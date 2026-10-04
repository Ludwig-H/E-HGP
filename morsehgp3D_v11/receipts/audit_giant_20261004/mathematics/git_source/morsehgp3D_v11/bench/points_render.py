#!/usr/bin/env python3
"""Vue de dessus d'un sauvetage : meilleur bloc HDBSCAN contre meilleur bloc de la tour, pour un objet vrai.

    python3 bench/points_render.py --scene DIR/NOM --result NOM.json --key margin:3 --order 10 --out FICHIER.png

Deux panneaux (HDBSCAN a gauche, regle de la tour a droite), recadres sur l'objet avec une marge de 2 m :
vert = point de l'objet dans le bloc, rouge = point hors objet dans le bloc, bleu = point de l'objet hors du
bloc, gris = autres points. PNG ecrit en numpy + zlib ; les coordonnees restent hors depot.
"""
import argparse
import json
import struct
import zlib

import numpy as np

COLORS = dict(other=(200, 200, 200), tp=(30, 160, 60), fp=(220, 40, 40), fn=(40, 90, 230))


def png(path, image):
    h, w, _ = image.shape
    raw = b''.join(b'\x00' + image[y].tobytes() for y in range(h))

    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data) & 0xFFFFFFFF)
    blob = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0))
    blob += chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b'')
    open(path, 'wb').write(blob)


def panel(xy, inside, block, box, size):
    (x0, y0), (x1, y1) = box
    image = np.full((size, size, 3), 255, dtype=np.uint8)
    scale = (size - 1) / max(x1 - x0, y1 - y0)
    px = ((xy[:, 0] - x0) * scale).astype(int)
    py = (size - 1 - (xy[:, 1] - y0) * scale).astype(int)
    keep = (px >= 0) & (px < size) & (py >= 0) & (py < size)
    kind = np.full(len(xy), 'other', dtype=object)
    kind[inside & block] = 'tp'
    kind[~inside & block] = 'fp'
    kind[inside & ~block] = 'fn'
    for name in ('other', 'fn', 'fp', 'tp'):  # l'ordre fixe ce qui est dessine au-dessus
        sel = keep & (kind == name)
        for dx in (0, 1):
            for dy in (0, 1):
                image[np.clip(py[sel] + dy, 0, size - 1), np.clip(px[sel] + dx, 0, size - 1)] = COLORS[name]
    return image


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--scene', required=True, help='prefixe DIR/NOM des fichiers _sites.u32le et _labels.u32le')
    parser.add_argument('--result', required=True)
    parser.add_argument('--key', required=True, help='cle du sauvetage, ex. margin:3')
    parser.add_argument('--order', required=True)
    parser.add_argument('--out', required=True)
    parser.add_argument('--size', type=int, default=700)
    args = parser.parse_args()
    xyz = np.fromfile(args.scene + '_sites.u32le', dtype='<u4').reshape(-1, 3).astype(np.float64) / 1000.0
    raw = np.fromfile(args.scene + '_labels.u32le', dtype='<u4').astype(np.int64)
    result = json.load(open(args.result))
    rescue = result['orders'][args.order]['rescued'][args.key]
    label = result['meta']['objects'][rescue['object']]
    inside = raw == label
    if rescue['members'] is None or rescue['hdbscan_members'] is None:
        raise SystemExit('bloc trop grand pour etre publie en entier')
    tower = np.zeros(len(xyz), dtype=bool)
    tower[rescue['members']] = True
    hdb = np.zeros(len(xyz), dtype=bool)
    hdb[rescue['hdbscan_members']] = True
    lo, hi = xyz[inside, :2].min(axis=0) - 2.0, xyz[inside, :2].max(axis=0) + 2.0
    side = max(hi - lo)
    box = (lo, lo + side)
    left = panel(xyz[:, :2], inside, hdb, box, args.size)
    right = panel(xyz[:, :2], inside, tower, box, args.size)
    gap = np.full((args.size, 12, 3), 255, dtype=np.uint8)
    png(args.out, np.concatenate([left, gap, right], axis=1))
    print(json.dumps(dict(out=args.out, object=int(label), hdbscan_iou=rescue['hdbscan_iou'], iou=rescue['iou'],
                          tower_size=rescue['size'], hdbscan_size=rescue['hdbscan_size'])))


if __name__ == '__main__':
    raise SystemExit(main())
