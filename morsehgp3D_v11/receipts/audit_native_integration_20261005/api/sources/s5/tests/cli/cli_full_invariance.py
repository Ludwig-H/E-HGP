"""Portes mhgp11_cli_full_determinism et mhgp11_cli_full_relabel (tranche S5) : sorties du CLI independantes du
nombre de fils, de l'ordre d'entree et des etiquettes.

    python3 cli_full_invariance.py --mode=determinism|relabel --cli <mhgp11> --bits <18|21|24>
            [--uniform=<n>] [--k=<K>] [--fils=<W,...>]

determinism : pour chaque nuage (uniform_u18 de n points, et un nuage de petite boite riche en cospheriques), chaque K
  et chaque W de la liste (defaut 1,2,4), puis une repetition et une permutation de l'entree : dossiers conformes ;
  full.mhgp11ful1 et manifeste identiques a l'octet entre les W et la repetition ; sous permutation, fichier
  identique et manifeste identique hors des empreintes des deux entrees (permutees ensemble) ; la ligne standard
  rapporte le W demande.
relabel : PointId reetiquetes injectivement, non denses, 0 et 0xFFFFFFFF compris : full.mhgp11ful1 ne differe qu'aux
  mots PointId, egaux a l'image des anciens ; le manifeste ne differe qu'aux empreintes de ids.u32le et du fichier ;
  tree_k_sha256 et comptes identiques.
Codes : 0 conforme, 1 ecart, 3 plancher.
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


class Runner:
    def __init__(self, args, gate, work):
        self.args, self.gate, self.work = args, gate, work
        self.runs = 0

    def publish(self, points, ids, k, workers, order=None):
        """Un appel du CLI ; rend (rapport du dossier, octets du fichier full, octets du manifeste) ou None."""
        folder = tempfile.mkdtemp(prefix='r', dir=self.work)
        xyz, names = cs.write_inputs(folder, points, ids, order)
        directory = os.path.join(folder, 'D')
        result, rows = cs.run_cli(cs.cli_argv(self.args.cli, xyz, names, directory, k, workers))
        self.runs += 1
        line = rows[0] if len(rows) == 1 and rows[0] else {}
        if not self.gate.check(result.code == 0 and line.get('status') == 'ok' and line.get('workers') == workers,
                               'CLI K=%d W=%d : %s %r %s' % (k, workers, result.describe(), line,
                                                              (result.stderr or '')[-300:])):
            return None
        try:
            report = formats.check_directory(directory, self.args.bits)
        except (OSError, ValueError) as error:
            self.gate.check(False, 'dossier non conforme : %s' % error)
            return None
        with open(os.path.join(directory, formats.FULL_NAME), 'rb') as handle:
            data = handle.read()
        with open(os.path.join(directory, formats.MANIFEST), 'rb') as handle:
            manifest = handle.read()
        return report, data, manifest


def determinism(runner, clouds, orders, workers_list):
    gate = runner.gate
    for name, points, ids in clouds:
        for k in orders:
            first = runner.publish(points, ids, k, workers_list[0])
            if first is None:
                continue
            for workers in workers_list[1:] + [workers_list[-1]]:
                other = runner.publish(points, ids, k, workers)
                if other is not None:
                    gate.check(other[1] == first[1] and other[2] == first[2],
                               '%s K=%d : sortie differente a W=%d' % (name, k, workers))
            order = list(range(len(points)))
            random.Random(k).shuffle(order)
            permuted = runner.publish(points, ids, k, workers_list[-1], order)
            if permuted is not None:
                a, b = dict(first[0]['manifest']), dict(permuted[0]['manifest'])
                gate.check(permuted[1] == first[1], '%s K=%d : fichier change sous permutation' % (name, k))
                gate.check([entry['bytes'] for entry in a['inputs']] == [entry['bytes'] for entry in b['inputs']],
                           '%s K=%d : tailles des entrees changees sous permutation' % (name, k))
                a['inputs'], b['inputs'] = None, None
                gate.check(a == b, '%s K=%d : manifeste change hors de points.u32le' % (name, k))


def relabel(runner, clouds, orders, workers):
    gate = runner.gate
    for name, points, ids in clouds:
        rng = random.Random(len(points))
        fresh = cs.distinct_ids(len(points), rng)
        mapping = dict(zip(ids, fresh))
        for k in orders:
            first = runner.publish(points, ids, k, workers)
            second = runner.publish(points, fresh, k, workers)
            if first is None or second is None:
                continue
            words_a, words_b = formats.full_point_ids(first[1]), formats.full_point_ids(second[1])
            gate.check([offset for offset, _ in words_a] == [offset for offset, _ in words_b],
                       '%s K=%d : positions des PointId' % (name, k))
            gate.check([mapping[value] for _, value in words_a] == [value for _, value in words_b],
                       '%s K=%d : PointId reetiquetes' % (name, k))
            patched = bytearray(first[1])
            for offset, value in words_b:
                patched[offset:offset + 8] = value.to_bytes(8, 'little')
            gate.check(bytes(patched) == second[1], '%s K=%d : octets hors PointId changes' % (name, k))
            a, b = first[0]['manifest'], second[0]['manifest']
            gate.check(a['tree_k_sha256'] == b['tree_k_sha256'] and a['counts'] == b['counts'] and
                       a['inputs'][0] == b['inputs'][0], '%s K=%d : arbre, comptes ou points changes' % (name, k))
            gate.check(any(old != new for old, new in mapping.items()) and
                       a['inputs'][1]['sha256'] != b['inputs'][1]['sha256'] and
                       a['files'][0]['sha256'] != b['files'][0]['sha256'], '%s K=%d : reetiquetage vide' % (name, k))
            for key in ('inputs', 'files'):
                a, b = dict(a, **{key: None}), dict(b, **{key: None})
            gate.check(a == b, '%s K=%d : manifeste change hors des empreintes' % (name, k))


def main():
    parser = argparse.ArgumentParser(description='Invariance des sorties du CLI mhgp11 --sortie=full.')
    parser.add_argument('--mode', choices=('determinism', 'relabel'), required=True)
    parser.add_argument('--cli', required=True)
    parser.add_argument('--bits', type=int, choices=(18, 21, 24), required=True)
    parser.add_argument('--uniform', type=int, default=1200)
    parser.add_argument('--k', default='1,3,5')
    parser.add_argument('--fils', default='1,2,4')
    args = parser.parse_args()
    orders = [int(k) for k in args.k.split(',')]
    workers = [int(w) for w in args.fils.split(',')]
    points, ids = cs.uniform_u18(args.uniform)
    rng = random.Random(41)
    box = sorted({tuple(rng.randrange(0, 12) for _ in range(3)) for _ in range(400)})
    clouds = [('uniform%d' % args.uniform, points, ids),
              ('boite12', box, cs.distinct_ids(len(box), rng, include_extremes=True))]
    gate = mhgp11_gate.Gate('cli_full_' + args.mode)
    with tempfile.TemporaryDirectory(prefix='mhgp11-cli-invariance-') as work:
        runner = Runner(args, gate, work)
        if args.mode == 'determinism':
            determinism(runner, clouds, orders, workers)
        else:
            relabel(runner, clouds, orders, workers[-1])
    expected = len(clouds) * len(orders) * (len(workers) + 2 if args.mode == 'determinism' else 2)
    if gate.failures == 0 and runner.runs == expected:
        print('cli_full_%s_verdict conforme runs%d' % (args.mode, runner.runs))
    return gate.finish(floor=expected)


if __name__ == '__main__':
    sys.exit(main())
