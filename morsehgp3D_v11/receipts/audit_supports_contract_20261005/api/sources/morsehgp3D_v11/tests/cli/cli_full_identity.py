"""Porte mhgp11_cli_full_identity (tranche S5) : la sortie full du CLI est, octet pour octet, le dump de la sonde.

    python3 cli_full_identity.py --cli <mhgp11> --bench <mhgp11_cli_full_reference> --bits <18|21|24>
            [--uniform=<n> | --data=<trame>] [--k=<K>] [--fils=<W>] [--min-attempts=<N>]

Sans --uniform ni --data : petits nuages (fixtures du paragraphe 2.9 de la specification, petites boites riches en
cospheriques, position generique, bords du domaine, alignements), K = 1..min(5, n), deux ordres d'entree, W1 et W4,
plus K = n + 1 refuse par les deux programmes ; au moins 400 tentatives (plancher). Avec --uniform (famille
uniform_u18 des bancs) ou --data (trame du dossier MHGP11_DATA_DIR) : un nuage, un K, un W (portes d'echelle et
LiDAR, jouees sur G4).

Chaque tentative appelle la sonde de reference (bench/full_probe.cpp, masque 16379, feuilles de 16 a 256 sites) et le
CLI sur les memes fichiers, et exige : memes codes et memes raisons de refus ; sha256 brut de full.mhgp11ful1 egal a
celui du dump ; dossier conforme (bench/mhgp11_formats.py : manifeste canonique, inventaire exact, tailles, sha256,
decodage strict, comptes) ; empreintes des entrees dans le manifeste ; ligne standard coherente (k, fils, sites,
comptes, sha256 du manifeste). Pour un meme nuage et un meme K : meme fichier et meme tree_k_sha256 quels que soient
l'ordre d'entree et W, et meme manifeste pour un meme ordre d'entree. Codes : 0 conforme, 1 ecart, 2 usage ou donnees
absentes, 3 plancher.
"""
import argparse
import os
import random
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import cli_support as cs  # noqa: E402

mhgp11_gate = cs.mhgp11_gate
formats = cs.formats


def fixtures():
    """Fixtures gravees du paragraphe 2.9 (z = 0 sauf mention), coordonnees entieres positives."""
    return [
        [(0, 0, 0), (2, 0, 0), (2, 2, 0), (0, 2, 0)],
        [(0, 0, 0), (4, 0, 0), (0, 3, 0)],
        [(1, 8, 0), (5, 10, 0), (9, 8, 0), (5, 0, 0)],
        [(0, 0, 0), (2, 2, 0), (4, 0, 0), (8, 0, 0)],
        [(0, 0, 0), (2, 0, 0), (1, 2, 0)],
        [(0, 0, 0), (1, 0, 0), (2, 0, 0)],
        [(0, 0, 0), (2, 2, 0), (2, 0, 2)],
        [(20, 20, 20), (20, 0, 0), (0, 20, 0), (0, 0, 20), (10, 10, 10), (11, 10, 10), (10, 11, 10)],
        [(x, y, z) for x in (0, 2) for y in (0, 2) for z in (0, 2)],
        [(0, 1, 1), (2, 1, 1), (1, 0, 1), (1, 2, 1), (1, 1, 0), (1, 1, 2)],
        [(0, 0, 0), (4, 0, 0), (6, 0, 0), (8, 0, 0), (12, 0, 0)],
    ]


def random_cloud(rng, count, low, high):
    seen = set()
    while len(seen) < count:
        seen.add(tuple(rng.randrange(low, high) for _ in range(3)))
    points = sorted(seen)
    rng.shuffle(points)
    return points


def small_clouds(bits, rng):
    top = (1 << bits) - 1
    clouds = fixtures()
    clouds += [random_cloud(rng, n, 0, side) for n, side in ((5, 4), (9, 6), (14, 7), (22, 9), (35, 11), (48, 12))]
    clouds += [random_cloud(rng, n, 0, 1 << bits) for n in (8, 17, 30, 48, 64, 96)]
    clouds += [random_cloud(rng, n, top - 40, top + 1) for n in (12, 40)]  # bord haut du domaine
    clouds += [[(3 * i, 0, 0) for i in range(6)], [(x, y, 5) for x in (0, 3, 6) for y in (0, 3, 6)]]
    return clouds


class Identity:
    """Tentatives appariees CLI / sonde, avec les empreintes attendues par (nuage, K)."""

    def __init__(self, args, gate, work):
        self.args, self.gate, self.work = args, gate, work
        self.attempts = self.refusals = 0
        self.expected = {}

    def run_pair(self, points, ids, k, order, workers):
        folder = tempfile.mkdtemp(prefix='a', dir=self.work)
        xyz, names = cs.write_inputs(folder, points, ids, order)
        dump, directory = os.path.join(folder, 'dump.ful1'), os.path.join(folder, 'D')
        bench, bench_rows = cs.run_bench(self.args.bench, xyz, names, dump, k, workers)
        cli, cli_rows = cs.run_cli(cs.cli_argv(self.args.cli, xyz, names, directory, k, workers))
        self.attempts += 1
        return folder, xyz, names, dump, directory, bench, bench_rows, cli, cli_rows

    def refusal(self, points, ids, k):
        _, _, _, dump, directory, bench, bench_rows, cli, cli_rows = self.run_pair(points, ids, k, None, 1)
        self.refusals += 1
        want = 'parameter_out_of_range'
        self.gate.check(bench.code == 2 and bench_rows and bench_rows[-1] and bench_rows[-1].get('reason') == want,
                        'sonde, K > n : %s %r' % (bench.describe(), bench_rows[-1:]))
        self.gate.check(cli.code == 2 and len(cli_rows) == 1 and cli_rows[0] and
                        (cli_rows[0].get('reason'), cli_rows[0].get('stage')) == (want, 'compute'),
                        'CLI, K > n : %s %r' % (cli.describe(), cli_rows))
        self.gate.check(not os.path.lexists(directory) and not os.path.lexists(directory + '.pending'),
                        'refus sans dossier')

    def success(self, key, points, ids, k, order, workers):
        folder, xyz, names, dump, directory, bench, _, cli, cli_rows = self.run_pair(points, ids, k, order, workers)
        what = '%s K=%d W=%d n=%d' % (key, k, workers, len(points))
        if not self.gate.check(bench.code == 0 and cli.code == 0, '%s : codes sonde %s, CLI %s %s' %
                               (what, bench.describe(), cli.describe(), (cli.stderr or '')[-400:])):
            return
        try:
            report = formats.check_directory(directory, self.args.bits)
        except (OSError, ValueError) as error:
            self.gate.check(False, '%s : dossier non conforme : %s' % (what, error))
            return
        manifest, decoded = report['manifest'], report['decoded']
        raw = manifest['files'][0]['sha256']
        self.gate.check_eq(raw, cs.file_sha(dump), '%s : sha256 brut contre le dump' % what)
        self.gate.check_eq([manifest['inputs'][0]['sha256'], manifest['inputs'][1]['sha256']],
                           [cs.sha256_of(xyz), cs.sha256_of(names)], '%s : empreintes des entrees' % what)
        line = cli_rows[0] if len(cli_rows) == 1 and cli_rows[0] else {}
        counts = line.get('counts', {})
        self.gate.check((line.get('status'), line.get('k'), line.get('workers'), line.get('sites'),
                         counts.get('nodes'), counts.get('births'), counts.get('edges'), line.get('manifest_sha256')) ==
                        ('ok', k, workers, len(points), decoded['nodes'], decoded['births'], decoded['edges'],
                         report['manifest_sha256']), '%s : ligne standard %r' % (what, line))
        # Meme nuage, meme K : meme fichier, meme arbre, meme manifeste hors entrees ; meme ordre d'entree : meme
        # manifeste a l'octet quel que soit W.
        free = dict(manifest, inputs=None)
        first = self.expected.setdefault((key, k), dict(raw=raw, tree=manifest['tree_k_sha256'], free=free, by={}))
        self.gate.check(first['raw'] == raw and first['tree'] == manifest['tree_k_sha256'],
                        '%s : fichier ou arbre change avec l\'ordre d\'entree ou W' % what)
        self.gate.check(first['free'] == free, '%s : manifeste change hors des entrees' % what)
        previous = first['by'].setdefault(tuple(order) if order else None, report['manifest_sha256'])
        self.gate.check(previous == report['manifest_sha256'], '%s : manifeste change avec W' % what)


def small_campaign(identity, args):
    rng = random.Random(20261004)
    for index, points in enumerate(small_clouds(args.bits, rng)):
        ids = cs.distinct_ids(len(points), rng, include_extremes=index % 2 == 0)
        shuffled = list(range(len(points)))
        rng.shuffle(shuffled)
        for k in range(1, min(5, len(points)) + 1):
            for order in (None, shuffled):
                for workers in (1, 4):
                    identity.success('nuage%d' % index, points, ids, k, order, workers)
        if len(points) < 5:
            identity.refusal(points, ids, len(points) + 1)


def main():
    parser = argparse.ArgumentParser(description='Identite du CLI mhgp11 --sortie=full avec la sonde FULL.')
    parser.add_argument('--cli', required=True)
    parser.add_argument('--bench', required=True)
    parser.add_argument('--bits', type=int, choices=(18, 21, 24), required=True)
    parser.add_argument('--uniform', type=int, default=0)
    parser.add_argument('--data', default='')
    parser.add_argument('--k', type=int, default=5)
    parser.add_argument('--fils', type=int, default=4)
    parser.add_argument('--min-attempts', type=int, default=400)
    args = parser.parse_args()
    gate = mhgp11_gate.Gate('cli_full_identity')
    with tempfile.TemporaryDirectory(prefix='mhgp11-cli-identity-') as work:
        identity = Identity(args, gate, work)
        if args.uniform or args.data:
            points, ids = cs.uniform_u18(args.uniform) if args.uniform else cs.lidar(args.data)
            order = list(range(len(points)))
            random.Random(7).shuffle(order)
            identity.success('grand', points, ids, args.k, None, args.fils)
            identity.success('grand', points, ids, args.k, order, args.fils)
        else:
            small_campaign(identity, args)
    floor = args.min_attempts
    if gate.failures == 0 and identity.attempts >= floor:
        print('cli_full_identity_verdict conforme attempts%d refusals%d' % (identity.attempts, identity.refusals))
    elif gate.failures == 0:
        print('PLANCHER cli_full_identity : %d tentatives, au moins %d' % (identity.attempts, floor))
        return mhgp11_gate.FLOOR
    return gate.finish(floor=1)


if __name__ == '__main__':
    sys.exit(main())
