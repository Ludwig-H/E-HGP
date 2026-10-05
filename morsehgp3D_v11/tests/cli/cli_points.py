#!/usr/bin/env python3
"""Portes de --sortie=points (tranche S9) : mhgp11_cli_points (petits nuages) et mhgp11_points_scale<n>,
mhgp11_points_lidar_<trame>_k<K> (tailles d'interet et trames, sans juge en O(n^3)).

    python3 cli_points.py --cli <mhgp11> --bits <18|21|24> [--uniform=<n> | --data=<trame>] [--k=<K>] [--fils=<W,...>]
            [--sample=<N>] [--min-delayed=N]

Sans --uniform ni --data : temoins exacts de bench/points_gate.py, nuages aleatoires a ex aequo frequents et une boite
riche en cospheriques, K = 1..4 (K < n a K >= 2) :
  - chaque appel : code 0, ligne de succes (comptes egaux au manifeste), dossier relu par le lecteur officiel
    (check_directory, lecture EXACTE : plancher et drapeau strict certifies, plateaux strictement croissants, entree
    de chaque site a un plateau de sa date) ;
  - tree_k_sha256 egal a celui de --sortie=supports a meme entree et meme K (et de --sortie=full sur les temoins) ;
  - W1 et W4 : fichier et manifeste identiques ; permutation : fichier identique ; reetiquetage non dense
    (0xFFFFFFFF compris) : seule la colonne SITES.point_id change ;
  - refus (code 2, parameter_out_of_range, etape compute, ni D ni D.pending) : K = n a K >= 2 (n = 2, 5, 8) ;
    admis : K = 1 a n = 1 et 2, K = 2 a n = 3 ; --sortie=plat reste refuse (etape options).
Avec --uniform (uniform_u18 de n points) ou --data (trame du dossier MHGP11_DATA_DIR, jamais copiee) : un appel par W
de --fils, lecteur structurel sur tout le fichier et lecture exacte de --sample sites et plateaux tires a graine fixe,
fichiers identiques entre W et sous permutation, tree_k_sha256 egal a --sortie=supports.
Codes : 0 conforme ; 1 ecart ; 2 refus d'usage ; 3 plancher. Derniere ligne de verdict :
    cli_points_verdict conforme cas=<c> appels=<a> refus=<r> retardes=<d> plateaux=<p>
Python 3.10 nu, bibliotheque standard seule, aucun assert. Aucune mesure de temps.
"""
import argparse
import os
import random
import shutil
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import cli_support as cs  # noqa: E402

mhgp11_gate = cs.mhgp11_gate
formats = cs.formats

EQUILATERAL = [tuple(c + 2 for c in p) for p in ((-1, -1, 0), (-1, 0, -1), (0, 0, 0), (1, 1, 0), (2, 2, 0), (2, 1, 1))]
FIVE = [(6, 2, 0), (0, 0, 0), (0, 4, 0), (12, 0, 0), (12, 4, 0)]
EIGHT = [(20, 20, 0), (30, 20, 0), (40, 20, 0), (50, 20, 0), (64, 20, 0), (74, 20, 0), (0, 30, 0), (0, 10, 0)]
PLATEAU = [(4, 4, 4), (6, 6, 4), (0, 0, 4), (8, 8, 4)]


class Runner:
    def __init__(self, args, gate, work):
        self.args, self.gate, self.work = args, gate, work
        self.calls = self.refusals = self.delayed = self.plateaus = 0

    def call(self, where, points, ids, k, workers, order=None, output='points', read='exact'):
        """Un appel ; rend (manifeste, octets du fichier, octets du manifeste, decodage) ou None."""
        folder = tempfile.mkdtemp(prefix='r', dir=self.work)
        xyz, names = cs.write_inputs(folder, points, ids, order)
        directory = os.path.join(folder, 'D')
        result, rows = cs.run_cli(cs.cli_argv(self.args.cli, xyz, names, directory, k, workers, output=output),
                                  timeout=3600)
        self.calls += 1
        line = rows[0] if len(rows) == 1 and rows[0] else {}
        if not self.gate.check(result.code == 0 and line.get('status') == 'ok' and line.get('output') == output,
                               '%s %s K%d W%d : %s %r %s' % (where, output, k, workers, result.describe(), line,
                                                              (result.stderr or '')[-300:])):
            shutil.rmtree(folder, ignore_errors=True)
            return None
        name = formats.OUTPUT_FILES[output][0]
        with open(os.path.join(directory, formats.MANIFEST), 'rb') as handle:
            raw = handle.read()
        with open(os.path.join(directory, name), 'rb') as handle:
            data = handle.read() if output == 'points' else b''
        decoded = None
        try:
            if output == 'points' and read:
                report = formats.check_directory(directory, self.args.bits, exact=read == 'exact')
                manifest, decoded = report['manifest'], report['decoded']
            else:
                manifest = formats.read_manifest(raw)
        except (OSError, ValueError) as error:
            self.gate.check(False, '%s %s K%d W%d : dossier non conforme : %s' % (where, output, k, workers, error))
            shutil.rmtree(folder, ignore_errors=True)
            return None
        if output == 'points':
            c = manifest['counts']
            self.gate.check_eq(line.get('counts'), dict(nodes=c['nodes'], levels=c['levels'], plateaus=c['plateaus'],
                                                         blocks=c['blocks'], delayed=c['delayed']),
                               '%s K%d W%d : comptes de la ligne' % (where, k, workers))
        shutil.rmtree(folder, ignore_errors=True)
        return manifest, data, raw, decoded

    def refuse(self, where, points, ids, k, reason, stage, output='points'):
        folder = tempfile.mkdtemp(prefix='x', dir=self.work)
        xyz, names = cs.write_inputs(folder, points, ids)
        directory = os.path.join(folder, 'D')
        result, rows = cs.run_cli(cs.cli_argv(self.args.cli, xyz, names, directory, k, 1, output=output))
        self.calls += 1
        self.refusals += 1
        line = rows[0] if len(rows) == 1 and rows[0] else {}
        self.gate.check(result.code == 2 and line.get('reason') == reason and line.get('stage') == stage and
                        line.get('publication') == 'none' and not os.path.lexists(directory) and
                        not os.path.lexists(directory + '.pending'),
                        '%s : refus %s/%s attendu, obtenu %s %r' % (where, reason, stage, result.describe(), line))
        shutil.rmtree(folder, ignore_errors=True)


def sample_exact(gate, where, pt, count, seed):
    """Lecture exacte d'un echantillon : plancher et drapeau strict de `count` sites retardes, ordre de `count`
    paires de plateaux consecutifs, egalite de date des sites stricts tires."""
    rng = random.Random(seed)
    delayed = [s for s in range(pt.n) if pt.M[s] != 0]
    for s in rng.sample(delayed, min(count, len(delayed))):
        sign = formats.radical_sign([(1, pt.level(pt.t[s])), (1, pt.level(pt.M[s])), (-1, pt.level(pt.Q[s])),
                                     (-1, pt.level(pt.floor[s]))])
        gate.check(sign >= 0 and (sign > 0) == (pt.strict[s] == 1), '%s : plancher du site %d' % (where, s))
        if pt.strict[s]:
            gate.check(formats.compare_dates(pt.plateau(pt.site_plateau[s]), pt.date(s)) == 0,
                       '%s : plateau du site strict %d' % (where, s))
    for p in rng.sample(range(1, pt.P), min(count, pt.P - 1)):
        gate.check(formats.compare_dates(pt.plateau(p - 1), pt.plateau(p)) < 0, '%s : plateaux %d' % (where, p))


def point_id_column(data):
    """(debut, fin) de la colonne SITES.point_id d'un MHGP11PT."""
    n = int.from_bytes(data[8 + 5 * 8:8 + 6 * 8], 'little')
    start = int.from_bytes(data[8 + 11 * 8:8 + 12 * 8], 'little') + 3 * ((4 * n + 7) & ~7)
    return start, start + 4 * n


def check_cloud(runner, where, points, ids, k, workers, read='exact', full=False):
    gate = runner.gate
    first = runner.call(where, points, ids, k, workers[0], read=read)
    if first is None:
        return None
    runner.delayed += first[0]['counts']['delayed']
    runner.plateaus += first[0]['counts']['plateaus']
    for w in workers[1:]:
        other = runner.call(where, points, ids, k, w, read=None)
        if other is not None:
            gate.check(other[1] == first[1] and other[2] == first[2], '%s K%d : fichier ou manifeste a W%d'
                       % (where, k, w))
    permuted = runner.call(where, points, ids, k, workers[-1], order=list(reversed(range(len(points)))), read=None)
    if permuted is not None:
        gate.check(permuted[1] == first[1], '%s K%d : fichier change sous permutation' % (where, k))
    fresh = cs.distinct_ids(len(points), random.Random(len(points) * 7 + k))
    mapping = dict(zip(ids, fresh))
    relabeled = runner.call(where, points, fresh, k, workers[-1], read=None)
    if relabeled is not None:
        start, end = point_id_column(first[1])
        old, new = first[1][start:end], relabeled[1][start:end]
        gate.check(first[1][:start] == relabeled[1][:start] and first[1][end:] == relabeled[1][end:] and
                   [mapping[int.from_bytes(old[i:i + 4], 'little')] for i in range(0, len(old), 4)] ==
                   [int.from_bytes(new[i:i + 4], 'little') for i in range(0, len(new), 4)],
                   '%s K%d : reetiquetage hors de SITES.point_id' % (where, k))
        gate.check(relabeled[0]['tree_k_sha256'] == first[0]['tree_k_sha256'] and
                   relabeled[0]['counts'] == first[0]['counts'], '%s K%d : arbre ou comptes reetiquetes' % (where, k))
    for output in ('supports', 'full') if full else ('supports',):
        other = runner.call(where, points, ids, k, workers[-1], output=output, read=None)
        if other is not None:
            gate.check_eq(other[0]['tree_k_sha256'], first[0]['tree_k_sha256'],
                          '%s K%d : tree_k_sha256 de %s et de points' % (where, k, output))
    return first


def small(runner):
    gate = runner.gate
    rng = random.Random(20261005)
    clouds = [('triangles', EQUILATERAL, True), ('cinq', FIVE, True), ('huit', EIGHT, True),
              ('plateau', PLATEAU, True)]
    for index in range(8):
        n = rng.randint(4, 9)
        pts = sorted({(rng.randrange(6) * 7, rng.randrange(6) * 7, rng.choice([0, rng.randrange(6) * 7]))
                      for _ in range(3 * n)})[:n]
        clouds.append(('alea%d' % index, pts, False))
    clouds.append(('boite', sorted({tuple(rng.randrange(0, 6) for _ in range(3)) for _ in range(60)}), False))
    cases = 0
    for name, pts, full in clouds:
        ids = cs.distinct_ids(len(pts), random.Random(len(pts)), include_extremes=False)
        for k in range(1, 5):
            if k > len(pts) or (k >= 2 and k >= len(pts)):
                continue
            got = check_cloud(runner, name, pts, ids, k, [1, 4], full=full)
            cases += got is not None
    # Decision K = n (docs/SORTIES.md, paragraphe 3) : refus a K >= 2, K = 1 admis.
    for n in (2, 5, 8):
        pts = [(3 * i, i * i % 5, 0) for i in range(n)]
        runner.refuse('K = n = %d' % n, pts, list(range(n)), n, 'parameter_out_of_range', 'compute')
    for pts, k in (([(0, 0, 0)], 1), ([(0, 0, 0), (4, 0, 0)], 1), ([(0, 0, 0), (4, 0, 0), (9, 1, 0)], 2)):
        cases += runner.call('n=%d K=%d' % (len(pts), k), pts, list(range(len(pts))), k, 1) is not None
    runner.refuse('sortie plat', FIVE, list(range(5)), 2, 'parameter_out_of_range', 'options', output='plat')
    gate.check(cases >= 40, 'cas admis : %d' % cases)
    return cases


def scale(runner, args):
    if args.data:
        points, ids = cs.lidar(args.data)
        name = args.data
    else:
        points, ids = cs.uniform_u18(args.uniform)
        name = 'uniform%d' % args.uniform
    workers = [int(w) for w in args.fils.split(',')]
    first = check_cloud(runner, name, points, ids, args.k, workers, read='structure')
    if first is None:
        return 0
    sample_exact(runner.gate, name, first[3], args.sample, 20261005)
    if first[0]['counts']['delayed'] < args.min_delayed:
        print('PLANCHER cli_points : %d sites retardes (au moins %d)' % (first[0]['counts']['delayed'],
                                                                        args.min_delayed))
        return -1
    return 1


def main():
    parser = argparse.ArgumentParser(description='--sortie=points : lecteur, refus K = n, signature, invariance.')
    parser.add_argument('--cli', required=True)
    parser.add_argument('--bits', type=int, choices=(18, 21, 24), required=True)
    parser.add_argument('--uniform', type=int, default=0)
    parser.add_argument('--data', default='')
    parser.add_argument('--k', type=int, default=5)
    parser.add_argument('--fils', default='1,4')
    parser.add_argument('--sample', type=int, default=500)
    parser.add_argument('--min-delayed', type=int, default=0)
    args = parser.parse_args()
    gate = mhgp11_gate.Gate('cli_points')
    with tempfile.TemporaryDirectory(prefix='mhgp11-cli-points-') as work:
        runner = Runner(args, gate, work)
        cases = scale(runner, args) if (args.uniform or args.data) else small(runner)
    if cases < 0:
        return mhgp11_gate.FLOOR
    if gate.failures == 0:
        print('cli_points_verdict conforme cas=%d appels=%d refus=%d retardes=%d plateaux=%d'
              % (cases, runner.calls, runner.refusals, runner.delayed, runner.plateaus))
    return gate.finish(floor=1)


if __name__ == '__main__':
    sys.exit(main())
