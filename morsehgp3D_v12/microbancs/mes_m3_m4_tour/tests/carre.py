#!/usr/bin/env python3
"""Vidages synthetiques du carre K1..4 (generateur du recu audit_socle_microbancs_20261007/tour/check_dumps.py).

Quatre sites A=(0,0,0), B=(2,0,0), D=(0,2,0), C=(2,2,0) en identifiants de Morton 0..3 ; Cat_4 : quatre boules
diametrales des cotes au niveau 1 et le cercle au niveau 2 ; FULL attendu : a K1 quatre naissances puis une fusion a
quatre ; a K2 quatre naissances d'aretes puis une fusion a quatre ; a K3 et K4 une naissance du cercle au niveau 2 ;
l'image basse K4 -> K3 est une naissance (branche etendue de LEM-T6). Trame « audit_square », profil 21, K = 4.
Aucune donnee reelle. make(dossier, mode FLOWER, alteration) ecrit cat.bin, ordre_1..4.bin, foret_1..4.bin.
"""
import struct

NONE = (1 << 32) - 1


def section(tag, fmt, rows):
    pack = struct.Struct('<' + fmt)
    return tag, pack.size, b''.join(pack.pack(*row) for row in rows), len(rows)


def center_section(points):
    data = b''.join(int(x).to_bytes(16, 'little', signed=True) for p in points for x in (*p, 1))
    return 'BCENTER', 64, data, len(points)


def write(path, kind, k, n, sections, order=None):
    out = bytearray(struct.pack('<8s6IQ24s', b'MHGP12DP', 1, kind, 21, 4, k if order is None else order,
                                len(sections), n, b'audit_square'))
    for tag, size, data, count in sections:
        out += struct.pack('<8sIIQ', tag.encode(), size, 0, count) + data
        out += b'\0' * (-len(data) % 8)
    path.write_bytes(bytes(out))


def make(directory, flower_mode='valid', mutation=None):
    """Carre K1..4 (generateur de l'auditeur), avec une alteration optionnelle des entrees."""
    points = [(0, 0, 0), (2, 0, 0), (0, 2, 0), (2, 2, 0)]
    supports = [(0, 1), (0, 2), (1, 3), (2, 3), (0, 3)]
    populations = [s for s in supports[:4]] + [(0, 1, 2, 3)]
    off = [0]
    for p in populations:
        off.append(off[-1] + len(p))
    write(directory / 'cat.bin', 1, 0, 4, [
        section('SITEXYZ', '3I', points),
        section('BALLS', '8I', [(1 if i < 4 else 2, 0, 2 if i < 4 else 4, 2, *s, NONE, NONE)
                                for i, s in enumerate(supports)]),
        section('POPOFF', 'Q', [(x,) for x in off]),
        section('POPVAL', 'I', [(x,) for p in populations for x in p]),
        section('NLEVELS', 'Q', [(3,)])])
    birth_rows = {1: [(i, 0, c, 0) for i, c in enumerate([0, 2, 1, 3])],
                  2: [(i, 1, c, 0) for i, c in enumerate([1, 0, 3, 2])],
                  3: [(4, 2, 0, 1)], 4: [(4, 2, 0, 1)]}
    centers = {1: points, 2: [(1, 0, 0), (0, 1, 0), (2, 1, 0), (1, 2, 0)], 3: [(1, 1, 0)], 4: [(1, 1, 0)]}
    cell_rows = {1: [(i, 1, 0, 2, 2, 0) for i in range(4)] + [(4, 2, 0, 4, 2, 1)],
                 2: [(4, 2, 0, 4, 2, 1)], 3: [], 4: []}
    trace_sites = {1: [0, 1, 0, 2, 1, 3, 2, 3, 0, 1, 2, 3], 2: [0, 1, 2, 3], 3: [], 4: []}
    trace_masks = {1: [1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 4, 8], 2: [3, 5, 10, 12], 3: [], 4: []}
    cell_off = {1: [0, 2, 4, 6, 8, 12], 2: [0, 4], 3: [0], 4: [0]}
    canonical = {1: [0, 2, 1, 3], 2: [1, 0, 3, 2]}
    if mutation == 'cle_hors_catalogue':
        birth_rows[3] = [(5, 2, 0, 1)]  # boule 5 : Cat_4 n'en a que 5 (0..4)
    if mutation == 'boule_de_cellule_hors_catalogue':
        cell_rows[2] = [(7, 2, 0, 4, 2, 1)]
    if mutation == 'decalages_decroissants':
        cell_off[1] = [0, 2, 1, 6, 8, 12]
    for k in range(1, 5):
        seeds = [(key, canonical[k][key], 1, 0, 0) for key in trace_sites[k]]
        order_field = k + 1 if (mutation == 'ordre_en_tete_faux' and k == 2) else None
        write(directory / ('ordre_%d.bin' % k), 2, k, 4, [
            section('BIRTHS', '4I', birth_rows[k]), center_section(centers[k]),
            section('CELLS', '6I', cell_rows[k]), section('CELLOFF', 'Q', [(x,) for x in cell_off[k]]),
            section('TRACEA', 'Q', [(m,) for m in trace_masks[k]]), section('SEEDS', '2I2BH', seeds),
            section('PARTOFF', 'Q', [(0,)] * (len(seeds) + 1)),
            section('PARTS', str(k) + 'I', []), section('PARTINF', '4BI', [])], order=order_field)
        if k <= 2:
            rank = k - 1
            keys = [0, 2, 1, 3] if k == 1 else [1, 0, 3, 2]
            nodes = [(rank, 4, key, 0, 0) for key in keys] + [(k, NONE, NONE, 4, 0)]
            edges = [(i,) for i in range(4)]
            meta = [(4,), (4,), (4,)]
        else:
            nodes = [(2, NONE, 4, 0, 0)]
            edges = []
            meta = [(1,), (0,), (0,)]
        sections = [section('FNODES', '4IQ', nodes), section('FEDGES', 'I', edges), section('FMETA', 'Q', meta)]
        absent = flower_mode == 'absent' or (flower_mode == 'absent_ordre_3' and k == 3)
        if k >= 2 and not absent:
            low = ([4] * 5 if k == 2 else [4] if k == 3 else [0])
            if flower_mode == 'wrong' and k == 2:
                low[0] = 0
            sections.append(section('FLOWER', 'I', [(x,) for x in low]))
        kind = 2 if (mutation == 'genre_de_foret_faux' and k == 3) else 3
        write(directory / ('foret_%d.bin' % k), kind, k, 0, sections)
