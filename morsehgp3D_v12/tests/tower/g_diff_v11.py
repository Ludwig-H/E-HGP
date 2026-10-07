#!/usr/bin/env python3
"""Differentiel de l'etage G contre la v11 gelee (CONTRAT_TOUR.md, paragraphe 9 ; ac081a06f) : sur une trame, la
cible de chaque representant est dans la MEME composante que la graine de la v11 a la coupe OUVERTE de sa jonction
(les graines peuvent differer : la v12 s'arrete a la premiere cellule de fenetre).

    g_diff_v11.py <dossier v11> <dossier v12> [--k=K] [--case=NOM]
    g_diff_v11.py <dossier v11> --probe=<mhgp12_tower_probe> --data=<nom> [--k=K] [--case=NOM] [--threads=W]

Dossier v11 : vidages de microbancs/mes_m3_m4_tour (mhgp12_vidage) : cat.bin, ordre_<k>.bin (BIRTHS, CELLS, CELLOFF,
TRACEA, SEEDS), foret_<k>.bin (FNODES) ; dossier v12 : export de mhgp12_tower_probe --out (cat.bin, res.bin), ou
produit par la sonde donnee sur <MHGP12_DATA_DIR>/<nom>.u32le et .ids.u32le dans un dossier temporaire neuf. Juge :
memes sites ; bijection des boules par (rang, population I puis U) ; par ordre, memes naissances, memes cellules de
fenetre et MEMES traces (masques, ordre de la v11) ; pour chaque trace, noeud de la cible dans la foret publiee de la
v11 (naissance : son noeud ; cellule c' : la graine v11 de la premiere trace de c', unie a toutes les autres sous le
niveau de c') puis ancetre a la coupe ouverte du rang de la jonction, egal a celui de la graine v11. Compte aussi les
graines identiques (information). Les vidages derivent de KITTI : jamais verses ; seuls les comptes sont publies.
Codes : 0 conforme ; 1 ecart ; 2 usage ; 3 vidage illisible ou hors format. Python 3.10 nu, aucun assert.
"""
import os
import struct
import subprocess
import sys
import tempfile
from array import array

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import g_dump  # noqa: E402

NONE = 0xFFFFFFFF


def read_sections(path, kind):
    with open(path, 'rb') as handle:
        data = handle.read()
    if len(data) < 64 or data[:8] != b'MHGP12DP':
        raise g_dump.Refusal('magie absente : %s' % path)
    _version, got, _bits, _kmax, _order, sections, _sites = struct.unpack_from('<6IQ', data, 8)
    if got != kind:
        raise g_dump.Refusal('genre %d au lieu de %d : %s' % (got, kind, path))
    out, at = {}, 64
    for _ in range(sections):
        tag = data[at:at + 8].rstrip(b'\0').decode('ascii', 'replace')
        size, _reserved, count = struct.unpack_from('<IIQ', data, at + 8)
        at += 24
        out[tag] = (size, count, data[at:at + size * count])
        at += size * count + (-(size * count)) % 8
    if at != len(data):
        raise g_dump.Refusal('octets en trop : %s' % path)
    return out


def column(sections, tag, size, code, stride=1, field=0):
    """Champ `field` (mots de `code`) des enregistrements de `size` octets de la section tag."""
    if tag not in sections or sections[tag][0] != size:
        raise g_dump.Refusal('section %s absente ou de taille inattendue' % tag)
    values = g_dump.words(sections[tag][2], code)
    return values if stride == 1 else values[field::stride]


def ball_map(v11, v12):
    """BallIdx v12 -> BallIdx v11, par rang puis population (I puis U) identique."""
    by_rank = {}
    for b in range(v11.count):
        by_rank.setdefault(v11.rank[b], []).append(b)
    out = array('I', [NONE]) * v12.count
    for b in range(v12.count):
        population = v12.population(b)
        for c in by_rank.get(v12.rank[b], ()):
            if v11.p[c] == v12.p[b] and v11.population(c) == population:
                out[b] = c
                break
        if out[b] == NONE:
            raise g_dump.Refusal('boule v12 %d sans boule v11 de meme rang et meme population' % b)
    return out


def judge_order(k, v11dir, order, v12_to_v11, totals, gaps):
    o11 = read_sections(os.path.join(v11dir, 'ordre_%d.bin' % k), 2)
    f11 = read_sections(os.path.join(v11dir, 'foret_%d.bin' % k), 3)
    births = column(o11, 'BIRTHS', 16, 'I')
    birth_node = dict(zip(births[0::4], births[2::4]))
    cells11 = column(o11, 'CELLS', 24, 'I')
    cell_ball11, cell_rank11 = cells11[0::6], cells11[1::6]
    cell_of = dict((b, c) for c, b in enumerate(cell_ball11))
    off11, masks11 = column(o11, 'CELLOFF', 8, 'Q'), column(o11, 'TRACEA', 8, 'Q')
    seed_node = column(o11, 'SEEDS', 12, 'I')[1::3]
    nodes = column(f11, 'FNODES', 24, 'I')
    rank, parent = nodes[0::6], nodes[1::6]
    if len(births) // 4 != len(order.birth_keys) or len(cell_ball11) != len(order.cell_balls):
        gaps.append('ordre %d : %d naissances et %d cellules, v11 %d et %d' % (
            k, len(order.birth_keys), len(order.cell_balls), len(births) // 4, len(cell_ball11)))
        return

    def open_up(node, r):
        while parent[node] != NONE and rank[parent[node]] < r:
            node = parent[node]
        return node

    def target_node(target):
        index = target & g_dump.INDEX_MASK
        if target & g_dump.CELL_BIT:
            c11 = cell_of[v12_to_v11[order.cell_balls[index]]]
            return seed_node[off11[c11]], True
        key = order.birth_keys[index]
        return birth_node[key if k == 1 else v12_to_v11[key]], False

    same = exact = cell_targets = 0
    for c in range(len(order.cell_balls)):
        c11 = cell_of.get(v12_to_v11[order.cell_balls[c]])
        r = order.cell_ranks[c]
        if c11 is None or cell_rank11[c11] != r:
            gaps.append('ordre %d cellule %d : absente de la v11 ou de rang different' % (k, c))
            return
        lo, hi = order.cell_offsets[c], order.cell_offsets[c + 1]
        if masks11[off11[c11]:off11[c11 + 1]] != order.masks[lo:hi]:
            gaps.append('ordre %d cellule %d : traces differentes de la v11' % (k, c))
            return
        for t in range(hi - lo):
            node, is_cell = target_node(order.targets[lo + t])
            seed = seed_node[off11[c11] + t]
            cell_targets += is_cell
            if node == seed:  # graine identique : meme composante sans remonter
                exact += 1
                same += 1
            elif open_up(node, r) == open_up(seed, r):
                same += 1
            elif len(gaps) < 20:
                gaps.append('ordre %d cellule %d trace %d : cible hors de la composante de la graine v11' % (k, c, t))
    reps = len(order.targets)
    print('ordre k=%d naissances=%d cellules=%d representants=%d composantes_egales=%d graines_identiques=%d '
          'cibles_cellule=%d' % (k, len(order.birth_keys), len(order.cell_balls), reps, same, exact, cell_targets))
    if same != reps:
        gaps.append('ordre %d : %d representants hors de la composante de la graine v11' % (k, reps - same))
    totals['reps'] += reps
    totals['same'] += same
    totals['exact'] += exact
    totals['cells'] += cell_targets


def judge(v11dir, v12dir, options):
    """Rend le code de la porte ; ecrit les lignes par ordre puis la ligne finale."""
    try:
        v11 = g_dump.read_catalogue(os.path.join(v11dir, 'cat.bin'))
        v12 = g_dump.read_catalogue(os.path.join(v12dir, 'cat.bin'))
        res = g_dump.read_resolution(os.path.join(v12dir, 'res.bin'))
        if v11.pos != v12.pos:
            print('g_diff_v11_ecart sites differents')
            return 1
        v12_to_v11 = ball_map(v11, v12)
        kmax = int(options.get('k', res['kmax']))
        totals, gaps = {'reps': 0, 'same': 0, 'exact': 0, 'cells': 0}, []
        for k in range(1, min(kmax, len(res['orders'])) + 1):
            judge_order(k, v11dir, res['orders'][k - 1], v12_to_v11, totals, gaps)
    except (OSError, KeyError, ValueError, g_dump.Refusal) as error:
        print('g_diff_v11_refus %s' % error)
        return 3
    for gap in gaps[:20]:
        print('ecart %s' % gap)
    if gaps:
        print('g_diff_v11_ecart ecarts=%d' % len(gaps))
        return 1
    print('g_diff_v11_ok cas=%s ordres=%d representants=%d composantes_egales=%d graines_identiques=%d '
          'cibles_cellule=%d' % (options.get('case', '-'), kmax, totals['reps'], totals['same'], totals['exact'],
                                 totals['cells']))
    return 0


def main(argv):
    args = [a for a in argv[1:] if not a.startswith('--')]
    options = dict(a[2:].split('=', 1) for a in argv[1:] if a.startswith('--') and '=' in a)
    probe = 'probe' in options
    if (len(args) != (1 if probe else 2) or set(options) - {'k', 'case', 'probe', 'data', 'threads'} or
            probe != ('data' in options)):
        print('g_diff_v11_refus usage : g_diff_v11.py <dossier v11> (<dossier v12> | --probe=S --data=N) [--k=K]')
        return 2
    if not probe:
        return judge(args[0], args[1], options)
    stem = os.path.join(os.environ.get('MHGP12_DATA_DIR', ''), options['data'])
    with tempfile.TemporaryDirectory() as folder:
        out = os.path.join(folder, 'v12')
        cmd = [options['probe'], stem + '.u32le', stem + '.ids.u32le', '--k=' + options.get('k', '5'),
               '--threads=' + options.get('threads', '4'), '--out=' + out]
        done = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
        if done.returncode != 0:
            print('g_diff_v11_refus sonde code %d' % done.returncode)
            return 2
        return judge(args[0], out, options)


if __name__ == '__main__':
    sys.exit(main(sys.argv))
