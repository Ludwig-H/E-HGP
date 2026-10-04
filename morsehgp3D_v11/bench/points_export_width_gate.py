#!/usr/bin/env python3
"""Porte de largeur de l'export POINTS (audit P2 du 4 octobre 2026) : le tetraedre regulier au bord du profil.

    python3 bench/points_export_width_gate.py --export BUILD/mhgp11_points_export --work DIR

Le profil B du binaire est lu dans l'en-tete d'un premier export minuscule. Puis le tetraedre regulier
(0,0,0), (L,L,0), (L,0,L), (0,L,L), L = 2^B - 1, est exporte a K = 4 : son niveau de boule circonscrite vaut
3 L^2 / 4, sous la forme non reduite 12 L^8 / 16 L^6 du moteur (196/148 bits en u24, que trois mots refusaient).
Controles : version du format (2 et quatre mots par niveau en u24, 1 et trois mots sinon), presence du niveau exact,
et, en u24, 196 et 148 bits pour ses numerateur et denominateur non reduits. Codes : 0 conforme ; 1 ecart ;
2 refus d'entree (binaire absent ou export en echec).
"""
import argparse
from fractions import Fraction
import os
from pathlib import Path
import subprocess
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import points_hierarchy as ph  # noqa: E402


def export(binary, points, work, name, kmax):
    work.mkdir(parents=True, exist_ok=True)
    xyz, ids, out = work / (name + '.xyz'), work / (name + '.ids'), work / (name + '.ph')
    for path in (xyz, ids, out):
        if path.exists():
            path.unlink()
    np.asarray(points, dtype='<u4').tofile(xyz)
    np.arange(len(points), dtype='<u4').tofile(ids)
    orders = ','.join(str(k) for k in range(1, kmax + 1))
    done = subprocess.run([str(binary), str(xyz), str(ids), str(out), str(kmax), orders, '1', str(1 << 30)],
                          capture_output=True, text=True, timeout=60)
    if done.returncode != 0:
        return None, done.stdout[-400:] + done.stderr[-400:]
    raw = np.fromfile(out, dtype='<u8', offset=8)
    version, bits = int(raw[0]), int(raw[1])
    data = ph.read_export(out)
    return dict(version=version, bits=bits, data=data), ''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--export', required=True, type=Path)
    parser.add_argument('--work', required=True, type=Path)
    args = parser.parse_args()
    if not args.export.is_file():
        print('refus : exportateur absent', file=sys.stderr)
        return 2
    probe, why = export(args.export, [(0, 0, 0), (2, 0, 0), (0, 2, 0), (0, 0, 2)], args.work, 'sonde', 2)
    if probe is None:
        print('refus : export de sonde en echec ' + why, file=sys.stderr)
        return 2
    bits = probe['bits']
    side = (1 << bits) - 1
    tetra = [(0, 0, 0), (side, side, 0), (side, 0, side), (0, side, side)]
    got, why = export(args.export, tetra, args.work, 'tetraedre', 4)
    problems = []
    if got is None:
        problems.append('export du tetraedre refuse : ' + why)
    else:
        words = 4 if bits == 24 else 3
        if got['version'] != (2 if bits == 24 else 1):
            problems.append('version %d' % got['version'])
        levels = got['data']['levels']
        if levels.words != words:
            problems.append('mots par niveau %d' % levels.words)
        target = Fraction(3 * side * side, 4)
        exact = [levels.exact(r) for r in range(len(levels))]
        hits = [(num, den) for num, den in exact if den and Fraction(num, den) == target]
        if not hits:
            problems.append('niveau 3 L^2 / 4 absent')
        if bits == 24 and not any(num.bit_length() == 196 and den.bit_length() == 148 for num, den in hits):
            problems.append('forme non reduite 196/148 bits absente : %s' % [(n.bit_length(), d.bit_length())
                                                                             for n, d in hits])
    verdict = 'conforme' if not problems else 'ecart'
    print('points_export_width coord_bits%d version%s' % (bits, got['version'] if got else '-'))
    print('points_export_width_verdict %s' % verdict)
    for problem in problems:
        print('ecart : ' + problem)
    return 0 if not problems else 1


if __name__ == '__main__':
    raise SystemExit(main())
