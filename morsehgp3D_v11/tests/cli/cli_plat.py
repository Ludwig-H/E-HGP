#!/usr/bin/env python3
"""Portes de --sortie=plat (tranche S10) : mhgp11_cli_plat (temoins et petits nuages) et mhgp11_plat_scale<n>,
mhgp11_plat_lidar_<trame>_k<K> (tailles d'interet et trames).

    python3 cli_plat.py --cli <mhgp11> --bits <18|21|24> [--uniform=<n> | --data=<trame>] [--k=<K>] [--fils=<W,...>]
            [--min-selected=N]

Sans --uniform ni --data :
  - temoins exacts de bench/points_flat_gate.py rejoues par l'executable, groupes attendus graves (indices d'entree) :
    F1 (deux triangles, K = 2), F4 et F4b (neuf sites, K = 2, mcs 3 : EOM z = 1, 2, 3 et feuilles ; seule F4b
    distingue z = 2 de z = 1), F5 (t = -1, 0, +1 : egalite certifiee 1/4 a t = 0), F6 (egalite certifiee sqrt(2)/8),
    F8 (droite, K = 1 : une egalite certifiee sur l'arbre de la tour, S(A u B) = S(A) + S(B)) ; les egalites sont lues dans les comptes du manifeste ;
  - nuages en amas a PointId non denses (0 et 0xFFFFFFFF compris), K = 1..4 : chaque appel relu par le lecteur
    officiel (check_directory) ; etiquette = plus petit PointId de son cluster ; tree_k_sha256 egal a --sortie=points ;
    W1 et W4 : fichier et manifeste identiques ; ordre d'entree permute : meme etiquette par PointId ;
  - refus K = n a K >= 2 (parameter_out_of_range, etape compute, ni D ni D.pending) ; K = 1 admis a n = 1.
Avec --uniform (uniform_u18 de n points) ou --data (trame du dossier MHGP11_DATA_DIR, jamais copiee) : EOM z = 1 mcs
20 (ligne LiDAR publiee), un appel par W de --fils puis un ordre permute ; fichiers identiques entre W, meme etiquette
par PointId sous permutation, etiquettes = plus petit PointId, au moins --min-selected clusters retenus.
Codes : 0 conforme ; 1 ecart ; 2 refus d'usage ; 3 plancher. Derniere ligne de verdict :
    cli_plat_verdict conforme cas=<c> appels=<a> refus=<r> egalites=<e> retenus=<s>
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

TRIANGLES = [(1, 1, 2), (1, 2, 1), (2, 2, 2), (3, 3, 2), (4, 4, 2), (4, 3, 3)]
NINE = [(x, 0, 0) for x in (0, 2, 4, 7, 9, 11, 17, 19, 21)]
NINE_B = [(x, 0, 0) for x in (0, 2, 4, 7, 9, 11, 16, 18, 20)]
LINE1D = [(x, 0, 0) for x in (0, 3, 7, 16, 22, 27, 99, 107, 114)]
ABC_DEF = [[0, 1, 2], [3, 4, 5]]
TWO = [[0, 1, 2, 3, 4, 5], [6, 7, 8]]
THREE = [[0, 1, 2], [3, 4, 5], [6, 7, 8]]


def f5_points(t, embed=False):
    xs = [0, 6, 12] + [x + t for x in (22, 28, 34, 52, 58, 64)]
    return [(x, x, 0) if embed else (x, 0, 0) for x in xs]


def groups(labels):
    out = {}
    for i, c in enumerate(labels):
        if c >= 0:
            out.setdefault(c, []).append(i)
    return sorted(out.values())


class Runner:
    def __init__(self, args, gate, work):
        self.args, self.gate, self.work = args, gate, work
        self.calls = self.refusals = self.equalities = self.selected = 0

    def call(self, where, points, ids, k, workers=1, order=None, extra=(), output='plat'):
        """Un appel ; rend (lecture de check_directory, octets du fichier, octets du manifeste, PointId en entree) ou
        None."""
        folder = tempfile.mkdtemp(prefix='p', dir=self.work)
        order = list(range(len(points))) if order is None else list(order)
        xyz, names = cs.write_inputs(folder, points, ids, order)
        d = os.path.join(folder, 'D')
        result, rows = cs.run_cli(cs.cli_argv(self.args.cli, xyz, names, d, k, workers, extra=extra, output=output),
                                  timeout=3600)
        self.calls += 1
        line = rows[0] if len(rows) == 1 and rows[0] else {}
        if not self.gate.check(result.code == 0 and line.get('status') == 'ok' and
                               line.get('output') == output,
                               '%s %s K%d W%d : %s %r %s' % (where, output, k, workers, result.describe(), line,
                                                              (result.stderr or '')[-300:])):
            shutil.rmtree(folder, ignore_errors=True)
            return None
        try:
            read = formats.check_directory(d, self.args.bits, exact=False)
        except ValueError as error:
            self.gate.check(False, '%s : lecteur : %s' % (where, error))
            shutil.rmtree(folder, ignore_errors=True)
            return None
        manifest = read['manifest']
        if output == 'plat':
            counts = manifest['counts']
            self.gate.check(line.get('counts') == {key: counts[key] for key in
                                                   ('nodes', 'clusters', 'selected', 'noise', 'exact')},
                            '%s : comptes de la ligne et du manifeste' % where)
            self.equalities += counts['equalities']
            self.selected += counts['selected']
        name = manifest['files'][0]['name']
        with open(os.path.join(d, name), 'rb') as handle:
            data = handle.read()
        with open(os.path.join(d, formats.MANIFEST), 'rb') as handle:
            raw = handle.read()
        shutil.rmtree(folder, ignore_errors=True)
        return read, data, raw, [ids[i] for i in order]

    def refuse(self, where, points, ids, k, reason, stage, extra=()):
        folder = tempfile.mkdtemp(prefix='r', dir=self.work)
        xyz, names = cs.write_inputs(folder, points, ids)
        d = os.path.join(folder, 'D')
        result, rows = cs.run_cli(cs.cli_argv(self.args.cli, xyz, names, d, k, 1, extra=extra, output='plat'))
        line = rows[0] if len(rows) == 1 and rows[0] else {}
        self.gate.check(result.code == 2 and line.get('reason') == reason and line.get('stage') == stage and
                        not os.path.lexists(d) and not os.path.lexists(d + '.pending'),
                        '%s : %s %r' % (where, result.describe(), line))
        self.refusals += 1
        shutil.rmtree(folder, ignore_errors=True)


def labels_by_pid(got):
    read, _, _, input_pids = got
    return dict(zip(input_pids, read['decoded']['labels']))


def check_min_pid(gate, where, by_pid):
    members = {}
    for pid, label in by_pid.items():
        if label >= 0:
            members.setdefault(label, []).append(pid)
    return gate.check(all(label == min(pids) for label, pids in members.items()),
                      '%s : etiquette differente du plus petit PointId du cluster' % where)


def witness(runner, name, points, k, mcs, z, method, expected, min_equalities=0):
    extra = ['--mcs=%d' % mcs, '--z=%d' % z, '--selection=' + method]
    got = runner.call(name, points, list(range(len(points))), k, extra=extra)
    if got is None:
        return 0
    labels = got[0]['decoded']['labels']
    runner.gate.check_eq(groups(labels), sorted(expected), '%s : groupes' % name)
    equalities = got[0]['manifest']['counts']['equalities']
    runner.gate.check(equalities >= min_equalities, '%s : %d egalites certifiees, au moins %d' %
                      (name, equalities, min_equalities))
    return 1


def clustered(rng, n):
    centres = [tuple(rng.randrange(20, 380) for _ in range(3)) for _ in range(rng.randrange(2, 6))]
    seen, points = set(), []
    while len(points) < n:
        c = rng.choice(centres)
        p = tuple(max(0, c[a] + rng.randrange(-10, 11)) for a in range(3)) if rng.random() > 0.1 else \
            tuple(rng.randrange(0, 400) for _ in range(3))
        if p not in seen:
            seen.add(p)
            points.append(p)
    return points


def small(runner):
    gate = runner.gate
    cases = 0
    for mcs in (2, 3):
        cases += witness(runner, 'F1 mcs%d' % mcs, TRIANGLES, 2, mcs, 1, 'eom', ABC_DEF)
    cases += witness(runner, 'F1 mcs4', TRIANGLES, 2, 4, 1, 'eom', [])
    cases += witness(runner, 'F4 z1', NINE, 2, 3, 1, 'eom', TWO)
    cases += witness(runner, 'F4 z2', NINE, 2, 3, 2, 'eom', TWO)
    cases += witness(runner, 'F4 z3', NINE, 2, 3, 3, 'eom', THREE)
    cases += witness(runner, 'F4 feuilles', NINE, 2, 3, 1, 'feuilles', THREE)
    cases += witness(runner, 'F4 mcs4', NINE, 2, 4, 1, 'eom', [])
    cases += witness(runner, 'F4b z1', NINE_B, 2, 3, 1, 'eom', TWO)
    cases += witness(runner, 'F4b z2', NINE_B, 2, 3, 2, 'eom', THREE)
    for t, expected, equal in ((-1, TWO, 0), (0, TWO, 1), (1, THREE, 0)):
        cases += witness(runner, 'F5 t%+d' % t, f5_points(t), 2, 3, 1, 'eom', expected, equal)
    cases += witness(runner, 'F6 racine 2 sur 8', f5_points(0, True), 2, 3, 1, 'eom', TWO, 1)
    # F8 : la porte Python exige deux egalites sur deux arbres (tour et lien simple) ; une par arbre.
    cases += witness(runner, 'F8 droite K1', LINE1D, 1, 3, 1, 'eom', TWO, 1)
    rng = random.Random(1005)
    for index in range(6):
        n = rng.randrange(30, 160)
        points = clustered(rng, n)
        ids = cs.distinct_ids(n, rng)
        for k in (1, 2, 3, 4):
            where = 'amas%d K%d' % (index, k)
            extra = ['--mcs=%d' % (3 + index % 3)]
            base = runner.call(where, points, ids, k, 1, extra=extra)
            if base is None:
                continue
            cases += 1
            by_pid = labels_by_pid(base)
            check_min_pid(gate, where, by_pid)
            twin = runner.call(where, points, ids, k, 4, extra=extra)
            if twin is not None:
                gate.check(twin[1] == base[1] and twin[2] == base[2], '%s : W1 et W4 differents' % where)
            order = list(range(n))
            random.Random(index * 7 + k).shuffle(order)
            moved = runner.call(where, points, ids, k, 1, order=order, extra=extra)
            if moved is not None:
                gate.check(labels_by_pid(moved) == by_pid, '%s : etiquettes par PointId sous permutation' % where)
            points_run = runner.call(where, points, ids, k, 1, output='points')
            if points_run is not None:
                gate.check_eq(base[0]['manifest']['tree_k_sha256'], points_run[0]['manifest']['tree_k_sha256'],
                              '%s : tree_k_sha256 de plat et de points' % where)
    # Decision K = n (docs/SORTIES.md, paragraphe 3) : refus a K >= 2, K = 1 admis a n = 1 (bruit).
    for n in (2, 5):
        pts = [(3 * i, i * i % 5, 0) for i in range(n)]
        runner.refuse('K = n = %d' % n, pts, list(range(n)), n, 'parameter_out_of_range', 'compute')
    one = runner.call('n=1 K=1', [(0, 0, 0)], [7], 1)
    if one is not None:
        gate.check_eq(one[0]['decoded']['labels'], [-1], 'n=1 K=1 : bruit')
        cases += 1
    gate.check(cases >= 30, 'cas admis : %d' % cases)
    return cases


def scale(runner, args):
    if args.data:
        points, ids = cs.lidar(args.data)
        name = args.data
    else:
        points, ids = cs.uniform_u18(args.uniform)
        name = 'uniform_u18_n%d' % args.uniform
    gate = runner.gate
    workers = [int(w) for w in args.fils.split(',')]
    first = None
    for w in workers:
        got = runner.call('%s W%d' % (name, w), points, ids, args.k, w)
        if got is None:
            return 0
        if first is None:
            first = got
            check_min_pid(gate, name, labels_by_pid(got))
        else:
            gate.check(got[1] == first[1] and got[2] == first[2], '%s : W%d different de W%d' % (name, w, workers[0]))
    order = list(range(len(points)))
    random.Random(len(points)).shuffle(order)
    moved = runner.call('%s permute' % name, points, ids, args.k, workers[-1], order=order)
    if moved is not None:
        gate.check(labels_by_pid(moved) == labels_by_pid(first), '%s : etiquettes par PointId sous permutation' % name)
    selected = first[0]['manifest']['counts']['selected']
    if selected < args.min_selected:
        print('PLANCHER retenus=%d, au moins %d' % (selected, args.min_selected))
        return -1
    return 1


def main():
    parser = argparse.ArgumentParser(description='--sortie=plat : temoins, lecteur, invariance, refus.')
    parser.add_argument('--cli', required=True)
    parser.add_argument('--bits', type=int, choices=(18, 21, 24), required=True)
    parser.add_argument('--uniform', type=int, default=0)
    parser.add_argument('--data', default='')
    parser.add_argument('--k', type=int, default=5)
    parser.add_argument('--fils', default='1,4')
    parser.add_argument('--min-selected', type=int, default=0)
    args = parser.parse_args()
    gate = mhgp11_gate.Gate('cli_plat')
    with tempfile.TemporaryDirectory(prefix='mhgp11-cli-plat-') as work:
        runner = Runner(args, gate, work)
        cases = scale(runner, args) if (args.uniform or args.data) else small(runner)
    if cases < 0:
        return mhgp11_gate.FLOOR
    if gate.failures == 0:
        print('cli_plat_verdict conforme cas=%d appels=%d refus=%d egalites=%d retenus=%d'
              % (cases, runner.calls, runner.refusals, runner.equalities, runner.selected))
    return gate.finish(floor=1)


if __name__ == '__main__':
    sys.exit(main())
