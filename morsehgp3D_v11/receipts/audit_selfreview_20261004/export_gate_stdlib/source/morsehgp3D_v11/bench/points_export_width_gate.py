#!/usr/bin/env python3
"""Porte de largeur de l'export POINTS (audit P2 du 4 octobre 2026) : le tetraedre regulier au bord du profil.

    python3 bench/points_export_width_gate.py --export BUILD/mhgp11_points_export --work DIR

Le profil B du binaire est lu dans l'en-tete d'un premier export minuscule. Puis le tetraedre regulier
(0,0,0), (L,L,0), (L,0,L), (0,L,L), L = 2^B - 1, est exporte a K = 4 : son niveau de boule circonscrite vaut
3 L^2 / 4, sous la forme non reduite 12 L^8 / 16 L^6 du moteur (196/148 bits en u24, que trois mots refusaient).
Controles : version du format (2 et quatre mots par niveau en u24, 1 et trois mots sinon), presence du niveau exact,
et, en u24, 196 et 148 bits pour ses numerateur et denominateur non reduits. Bibliotheque standard seule (la matrice
G4 tourne en Python nu) : l'export est relu par struct. Codes : 0 conforme ; 1 ecart ; 2 refus d'entree (binaire
absent ou export en echec).
"""
import argparse
from fractions import Fraction
from pathlib import Path
import struct
import subprocess
import sys


def read_levels(path):
    """En-tete et niveaux exacts d'un export MHGP11PH, bibliotheque standard seule (Python nu de la VM)."""
    data = Path(path).read_bytes()
    if data[:8] != b'MHGP11PH':
        raise ValueError('magie')
    version, bits, _kmax, n, nlevels, norders = struct.unpack_from('<6Q', data, 8)
    if version not in (1, 2):
        raise ValueError('version %d' % version)
    words = 3 if version == 1 else 4
    offset = 8 + 8 * (6 + norders + 4 * n)
    levels = []
    for _ in range(nlevels):
        limbs = struct.unpack_from('<%dQ' % (2 * words), data, offset)
        offset += 16 * words
        levels.append((sum(v << (64 * j) for j, v in enumerate(limbs[:words])),
                       sum(v << (64 * j) for j, v in enumerate(limbs[words:]))))
    return dict(version=version, bits=bits, words=words, levels=levels)


def export(binary, points, work, name, kmax):
    work.mkdir(parents=True, exist_ok=True)
    xyz, ids, out = work / (name + '.xyz'), work / (name + '.ids'), work / (name + '.ph')
    for path in (xyz, ids, out):
        if path.exists():
            path.unlink()
    xyz.write_bytes(struct.pack('<%dI' % (3 * len(points)), *[c for p in points for c in p]))
    ids.write_bytes(struct.pack('<%dI' % len(points), *range(len(points))))
    orders = ','.join(str(k) for k in range(1, kmax + 1))
    done = subprocess.run([str(binary), str(xyz), str(ids), str(out), str(kmax), orders, '1', str(1 << 30)],
                          capture_output=True, text=True, timeout=60)
    if done.returncode != 0:
        return None, done.stdout[-400:] + done.stderr[-400:]
    return read_levels(out), ''


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
        if got['words'] != words:
            problems.append('mots par niveau %d' % got['words'])
        target = Fraction(3 * side * side, 4)
        hits = [(num, den) for num, den in got['levels'] if den and Fraction(num, den) == target]
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
