#!/usr/bin/env python3
"""Porte d'export du rattachement (tranche S3) : les incidences fortes tirees de WindowAttachment sont identiques,
octet pour octet, au bloc d'incidences de MHGP11PH ecrit par mhgp11_points_export (kmax = K, ordres = K).

    attach_export_gate.py --probe <mhgp11_tower_attach_probe> --export <mhgp11_points_export> --work <dossier>
                          [--data <nom> --k <K>] [--workers W] [--min-cases N] [--min-incidences N]

Sans --data : temoins D2 et E5 du contrat et nuages bornes tires a graine fixe (bibliotheque standard), K = 1..5,
petites boites (coquilles cospheriques, coquilles etendues) et grappes. Avec --data : la trame <nom>.u32le / <nom>.ids.u32le du dossier de la
variable MHGP11_DATA_DIR, a l'ordre K. Pour chaque cas, la sonde ecrit le bloc de foret (k, N, naissances, racine,
N x (parent, rang)) et le bloc d'incidences (T, decalages n+1, T mots) ; l'export ecrit MHGP11PH. Les deux blocs
doivent etre egaux a l'octet (le bloc core de l'export, n x (noeud, D_k), est saute). L'egalite du bloc de foret est
aussi I10 entre deux outils (pipeline 16379 de l'export, ordre K seul de la sonde).

Codes : 0 conforme ; 1 ecart ; 2 refus (usage, programme en echec) ; 3 plancher. Derniere ligne :
    attach_export_verdict <conforme|ecart|refus|plancher> cas=<c> noeuds=<N> incidences=<T> octets=<o>
Python 3.10 nu, aucun assert.
"""
import argparse
import os
import random
import struct
import subprocess
import sys
import tempfile

MAGIC = b'MHGP11PH'


class Refusal(Exception):
    def __init__(self, code, message):
        Exception.__init__(self, message)
        self.code = code


def words(data, start, count):
    end = start + 8 * count
    if end > len(data):
        raise Refusal(1, 'MHGP11PH tronque')
    return list(struct.unpack_from('<%dQ' % count, data, start)), end


def order_blocks(data, k):
    """(bloc de foret, bloc d'incidences) de l'ordre k d'un fichier MHGP11PH a un seul ordre."""
    if data[:8] != MAGIC:
        raise Refusal(1, 'magie MHGP11PH absente')
    head, at = words(data, 8, 6)
    version, _bits, kmax, n, levels, count = head
    if version not in (1, 2) or count != 1 or kmax != k:
        raise Refusal(1, 'en-tete MHGP11PH inattendu : %r' % (head,))
    orders, at = words(data, at, count)
    if orders != [k]:
        raise Refusal(1, 'ordres MHGP11PH : %r' % (orders,))
    width = 3 if version == 1 else 4
    at += 8 * (4 * n + 2 * width * levels)
    prefix, _ = words(data, at, 4)
    if prefix[0] != k:
        raise Refusal(1, 'bloc d ordre : k=%d' % prefix[0])
    nodes = prefix[1]
    forest_end = at + 8 * (4 + 2 * nodes)
    incidence_start = forest_end + 8 * 2 * n
    total, _ = words(data, incidence_start, 1)
    end = incidence_start + 8 * (1 + n + 1 + total[0])
    if end != len(data):
        raise Refusal(1, 'longueur MHGP11PH : %d au lieu de %d' % (len(data), end))
    return data[at:forest_end], data[incidence_start:end], nodes, total[0]


def run(argv, timeout):
    try:
        done = subprocess.run(argv, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                              timeout=timeout, check=False)
    except (OSError, subprocess.TimeoutExpired) as error:
        raise Refusal(2, 'execution impossible : %s' % error)
    if done.returncode != 0 or done.stderr:
        raise Refusal(2, 'code %d : %s %s' % (done.returncode, done.stdout.decode('utf-8', 'replace')[-400:],
                                               done.stderr.decode('utf-8', 'replace')[-400:]))
    return done


def write_cloud(folder, name, points):
    xyz, ids = os.path.join(folder, name + '.u32le'), os.path.join(folder, name + '.ids.u32le')
    with open(xyz, 'wb') as handle:
        for p in points:
            handle.write(struct.pack('<III', *p))
    with open(ids, 'wb') as handle:
        for i in range(len(points)):
            handle.write(struct.pack('<I', (2654435761 * (i + 1)) & 0xFFFFFFFF))
    return xyz, ids


def cloud(count, side, seed, clustered):
    rng = random.Random(seed)
    seen, points = set(), []
    while len(points) < count:
        if clustered:
            centre = rng.randrange(4) * side
            p = (centre + rng.randrange(side // 4), rng.randrange(side // 4), centre + rng.randrange(side // 4))
        else:
            p = (rng.randrange(side), rng.randrange(side), rng.randrange(side))
        if p not in seen:
            seen.add(p)
            points.append(p)
    return points


def cases():
    yield 'carre', [(0, 0, 0), (2, 0, 0), (2, 2, 0), (0, 2, 0), (1, 1, 5)], range(1, 6)
    yield 'ligne', [(2 * i, 0, 0) for i in range(9)], range(1, 6)
    # temoins du contrat L0 (MATHEMATIQUES 10.11) : D2 (trace stricte nee apres l(r_b - 1)) et E5 (lemme W.2)
    yield 'temoin_d2', [(2, 10, 0), (18, 10, 0), (10, 20, 0), (9, 3, 0), (11, 3, 0)], range(1, 6)
    yield 'temoin_e5', [(0, 0, 7), (0, 9, 6), (1, 4, 0), (0, 0, 1), (4, 1, 2)], range(1, 6)
    for name, count, side, seed, clustered in (('u40s6', 40, 6, 3, False), ('u70s16', 70, 16, 5, False),
                                               ('u120s64', 120, 64, 7, False), ('g150', 150, 256, 11, True),
                                               ('u300s4096', 300, 4096, 13, False), ('u60s8', 60, 8, 17, False),
                                               ('u2000s2e18', 2000, 1 << 18, 19, False),
                                               ('g1500', 1500, 1 << 16, 23, True)):
        yield name, cloud(count, side, seed, clustered), range(1, 6)


def compare(args, folder, name, xyz, ids, k, totals):
    probe_out = os.path.join(folder, '%s_k%d.attach' % (name, k))
    export_out = os.path.join(folder, '%s_k%d.mhgp11ph' % (name, k))
    run([args.probe, '--incidences=' + probe_out, '--input=%s,%s' % (xyz, ids), '--k=%d' % k,
         '--workers=%d' % args.workers], args.timeout)
    run([args.export, xyz, ids, export_out, str(k), str(k), str(args.workers), str(32 << 30)], args.timeout)
    with open(probe_out, 'rb') as handle:
        probe = handle.read()
    with open(export_out, 'rb') as handle:
        export = handle.read()
    forest, incidences, nodes, total = order_blocks(export, k)
    totals['cas'] += 1
    totals['noeuds'] += nodes
    totals['incidences'] += total
    totals['octets'] += len(probe)
    if probe != forest + incidences:
        same_forest = probe[:len(forest)] == forest
        print('ecart %s k=%d : foret %s, incidences %s' % (name, k, 'egale' if same_forest else 'differente',
                                                           'egales' if probe[len(forest):] == incidences
                                                           else 'differentes'))
        return False
    os.remove(probe_out)
    os.remove(export_out)
    return True


def main():
    parser = argparse.ArgumentParser(description='Incidences fortes de WindowAttachment contre MHGP11PH.')
    parser.add_argument('--probe', required=True)
    parser.add_argument('--export', required=True)
    parser.add_argument('--work', required=True)
    parser.add_argument('--data')
    parser.add_argument('--k', type=int, default=5)
    parser.add_argument('--workers', type=int, default=2)
    parser.add_argument('--timeout', type=int, default=600)
    parser.add_argument('--min-cases', type=int, default=1)
    parser.add_argument('--min-incidences', type=int, default=1)
    args = parser.parse_args()
    totals = dict(cas=0, noeuds=0, incidences=0, octets=0)
    verdict, code = 'conforme', 0
    try:
        if not 1 <= args.k <= 12 or args.workers < 1:
            raise Refusal(2, 'usage : --k dans 1..12, --workers >= 1')
        os.makedirs(args.work, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='mhgp11-attach-export-', dir=args.work) as folder:
            failures = 0
            if args.data:
                root = os.environ.get('MHGP11_DATA_DIR', '')
                if not root or '/' in args.data:
                    raise Refusal(2, 'usage : MHGP11_DATA_DIR et --data=<nom> requis')
                base = os.path.join(root, args.data)
                failures += not compare(args, folder, args.data, base + '.u32le', base + '.ids.u32le', args.k,
                                        totals)
            else:
                for name, points, orders in cases():
                    xyz, ids = write_cloud(folder, name, points)
                    for k in orders:
                        if k <= len(points):
                            failures += not compare(args, folder, name, xyz, ids, k, totals)
            if failures:
                verdict, code = 'ecart', 1
            elif totals['cas'] < args.min_cases or totals['incidences'] < args.min_incidences:
                verdict, code = 'plancher', 3
    except Refusal as refusal:
        print('refus : %s' % refusal)
        verdict, code = ('ecart', 1) if refusal.code == 1 else ('refus', 2)
    print('attach_export_verdict %s cas=%d noeuds=%d incidences=%d octets=%d' % (
        verdict, totals['cas'], totals['noeuds'], totals['incidences'], totals['octets']))
    return code


if __name__ == '__main__':
    sys.exit(main())
