#!/usr/bin/env python3
"""Portes mhgp11_cli_supports_scale<n> et mhgp11_cli_supports_lidar_<trame>_k<K> (tranche S7) : --sortie=supports aux
tailles d'interet (8 000, 16 000, 32 000 sites, uniform_u18) et sur les trames LiDAR sans sol, sans juge en O(n^3) :
le lecteur officiel et ses invariants, le determinisme, la permutation, le reetiquetage et la signature commune avec
--sortie=full.

    python3 cli_supports_scale.py --cli <mhgp11> --bits <18|21|24> (--uniform=<n> | --data=<trame>) [--k=<K>]
            [--fils=<W,...>] [--min-balls=N] [--min-extended=N]

Pour le nuage (uniform_u18 de n points, PointId 0..n-1 ; ou trame du dossier MHGP11_DATA_DIR, jamais copiee) et pour
un petit nuage de boite (400 points dans [0, 12)^3, riche en cospheriques, PointId non denses) :
  - un appel par W de --fils (defaut 1,4), plus une repetition au dernier W : code 0, ligne d'etat de succes (W
    rapporte, comptes egaux au manifeste) ; le premier dossier est relu par le lecteur (check_directory : decodage
    strict et controles de read_supports, agregats du manifeste recomptes, tree_k_sha256 egal a la signature
    version 2 recalculee depuis le fichier) ; supports.mhgp11sp et manifeste identiques a l'octet entre tous les
    appels ;
  - permutation de l'entree (ordre inverse) : fichier identique ; manifeste identique hors des empreintes des
    entrees (tailles egales) ;
  - reetiquetage injectif non dense (0 et 0xFFFFFFFF compris) : seule la colonne SITES.point_id change, egale a
    l'image des anciens PointId ; tree_k_sha256 et comptes identiques ; manifeste identique hors des empreintes de
    ids.u32le et du fichier ;
  - --sortie=full sur la meme entree au dernier W : meme tree_k_sha256 (signature commune, docs/SORTIES.md,
    paragraphe 8).
Codes : 0 conforme ; 1 ecart ; 2 refus d'usage (donnees absentes) ; 3 plancher. Dernieres lignes :
    cli_supports_scale_verdict conforme sites=<n> k=<K> noeuds=<N> boules=<B> supports=<S> etendues=<e>
        fusions=<f> branches=<A> appels=<a>
(MHGP11SP 2, arbre couvrant d'ordre K : un S* par boule, naissances et fusions seulement.)
    cli_supports_scale_ok controles=<n>
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


class Runner:
    def __init__(self, args, gate, work):
        self.args, self.gate, self.work = args, gate, work
        self.calls = 0

    def publish(self, where, points, ids, k, workers, order=None, output='supports', read=False):
        """Un appel ; rend (manifeste decode ou None, octets du fichier, octets du manifeste, rapport du lecteur)."""
        folder = tempfile.mkdtemp(prefix='r', dir=self.work)
        xyz, names = cs.write_inputs(folder, points, ids, order)
        directory = os.path.join(folder, 'D')
        result, rows = cs.run_cli(cs.cli_argv(self.args.cli, xyz, names, directory, k, workers, output=output),
                                  timeout=3600)
        self.calls += 1
        line = rows[0] if len(rows) == 1 and rows[0] else {}
        ok = self.gate.check(result.code == 0 and line.get('status') == 'ok' and line.get('workers') == workers and
                             line.get('output') == output, '%s %s W%d : %s %r %s'
                             % (where, output, workers, result.describe(), line, (result.stderr or '')[-300:]))
        if not ok:
            shutil.rmtree(folder, ignore_errors=True)
            return None
        name = formats.SUPPORTS_NAME if output == 'supports' else formats.FULL_NAME
        with open(os.path.join(directory, formats.MANIFEST), 'rb') as handle:
            raw = handle.read()
        with open(os.path.join(directory, name), 'rb') as handle:
            data = handle.read() if output == 'supports' else b''
        report = None
        try:
            report = formats.check_directory(directory, self.args.bits) if read else None
            manifest = report['manifest'] if read else formats.read_manifest(raw)
        except (OSError, ValueError) as error:
            self.gate.check(False, '%s %s W%d : dossier non conforme : %s' % (where, output, workers, error))
            shutil.rmtree(folder, ignore_errors=True)
            return None
        if output == 'supports':
            c = manifest['counts']
            self.gate.check_eq(line.get('counts'), dict(nodes=c['nodes'], balls=c['balls'], supports=c['supports'],
                                                         prior=c['prior']), '%s W%d : comptes de la ligne' % (where,
                                                                                                             workers))
        shutil.rmtree(folder, ignore_errors=True)
        return manifest, data, raw, report


def point_id_column(data):
    """(debut, fin) de la colonne SITES.point_id d'un MHGP11SP."""
    n = int.from_bytes(data[8 + 3 * 8:8 + 4 * 8], 'little')
    start = int.from_bytes(data[8 + 10 * 8:8 + 11 * 8], 'little') + 3 * ((4 * n + 7) & ~7)
    return start, start + 4 * n


def check_cloud(runner, where, points, ids, k, workers_list):
    """Tous les controles d'un nuage ; rend le manifeste du premier appel, ou None."""
    gate = runner.gate
    first = runner.publish(where, points, ids, k, workers_list[0], read=True)
    if first is None:
        return None
    for workers in workers_list[1:] + [workers_list[-1]]:
        other = runner.publish(where, points, ids, k, workers)
        if other is not None:
            gate.check(other[1] == first[1] and other[2] == first[2],
                       '%s : fichier ou manifeste different a W%d' % (where, workers))
    last = workers_list[-1]
    permuted = runner.publish(where, points, ids, k, last, order=list(reversed(range(len(points)))))
    if permuted is not None:
        a, b = dict(first[0]), dict(permuted[0])
        gate.check(permuted[1] == first[1], '%s : fichier change sous permutation' % where)
        gate.check([e['bytes'] for e in a['inputs']] == [e['bytes'] for e in b['inputs']],
                   '%s : tailles des entrees changees sous permutation' % where)
        a['inputs'] = b['inputs'] = None
        gate.check(a == b, '%s : manifeste change hors des entrees sous permutation' % where)
    rng = random.Random(len(points) * 31 + k)
    fresh = cs.distinct_ids(len(points), rng)
    mapping = dict(zip(ids, fresh))
    relabeled = runner.publish(where, points, fresh, k, last)
    if relabeled is not None:
        start, end = point_id_column(first[1])
        old, new = first[1][start:end], relabeled[1][start:end]
        gate.check(first[1][:start] == relabeled[1][:start] and first[1][end:] == relabeled[1][end:],
                   '%s : octets hors de SITES.point_id changes par le reetiquetage' % where)
        gate.check([mapping[int.from_bytes(old[i:i + 4], 'little')] for i in range(0, len(old), 4)] ==
                   [int.from_bytes(new[i:i + 4], 'little') for i in range(0, len(new), 4)],
                   '%s : SITES.point_id n\'est pas l\'image des anciens PointId' % where)
        gate.check(cs.NONE in fresh and old != new, '%s : reetiquetage vide' % where)
        a, b = first[0], relabeled[0]
        gate.check(a['tree_k_sha256'] == b['tree_k_sha256'] and a['counts'] == b['counts'] and
                   a['inputs'][0] == b['inputs'][0] and a['inputs'][1]['sha256'] != b['inputs'][1]['sha256'] and
                   a['files'][0]['sha256'] != b['files'][0]['sha256'],
                   '%s : arbre, comptes ou points changes par le reetiquetage' % where)
        a, b = dict(a, inputs=None, files=None), dict(b, inputs=None, files=None)
        gate.check(a == b, '%s : manifeste change hors des empreintes par le reetiquetage' % where)
    full = runner.publish(where, points, ids, k, last, output='full')
    if full is not None:
        gate.check_eq(full[0]['tree_k_sha256'], first[0]['tree_k_sha256'],
                      '%s : tree_k_sha256 de full et de supports' % where)
    return first[0]


def main():
    parser = argparse.ArgumentParser(description='--sortie=supports a l\'echelle (lecteur, invariants, invariance).')
    parser.add_argument('--cli', required=True)
    parser.add_argument('--bits', type=int, choices=(18, 21, 24), required=True)
    parser.add_argument('--uniform', type=int, default=0)
    parser.add_argument('--data', default='')
    parser.add_argument('--k', type=int, default=5)
    parser.add_argument('--fils', default='1,4')
    parser.add_argument('--min-balls', type=int, default=0)
    parser.add_argument('--min-extended', type=int, default=0)
    args = parser.parse_args()
    if (args.uniform > 0) == bool(args.data):
        print('usage : --uniform=<n> ou --data=<trame>')
        return mhgp11_gate.REFUSAL
    workers = [int(w) for w in args.fils.split(',')]
    if args.data:
        points, ids = cs.lidar(args.data)
        name = args.data
    else:
        points, ids = cs.uniform_u18(args.uniform)
        name = 'uniform%d' % args.uniform
    rng = random.Random(41)
    box = sorted({tuple(rng.randrange(0, 12) for _ in range(3)) for _ in range(400)})
    gate = mhgp11_gate.Gate('cli_supports_scale')
    with tempfile.TemporaryDirectory(prefix='mhgp11-cli-supports-scale-') as work:
        runner = Runner(args, gate, work)
        manifest = check_cloud(runner, '%s K%d' % (name, args.k), points, ids, args.k, workers)
        small = check_cloud(runner, 'boite12 K%d' % args.k, box, cs.distinct_ids(len(box), rng), args.k, workers)
    if manifest is None or small is None:
        return gate.finish(floor=1)
    c = manifest['counts']
    gate.check(small['counts']['extended_shells'] > 0 and small['counts']['supports'] == small['counts']['balls'],
               'boite12 : aucune coquille etendue, ou plusieurs supports par boule')
    expected = 2 * (len(workers) + 4)
    gate.check_eq(runner.calls, expected, 'nombre d\'appels')
    if gate.failures == 0 and (c['balls'] < args.min_balls or c['extended_shells'] < args.min_extended):
        print('PLANCHER cli_supports_scale : %d boules (au moins %d), %d coquilles etendues (au moins %d)'
              % (c['balls'], args.min_balls, c['extended_shells'], args.min_extended))
        return mhgp11_gate.FLOOR
    if gate.failures == 0:
        print('cli_supports_scale_verdict conforme sites=%d k=%d noeuds=%d boules=%d supports=%d etendues=%d '
              'fusions=%d branches=%d appels=%d'
              % (c['sites'], manifest['k'], c['nodes'], c['balls'], c['supports'], c['extended_shells'],
                 c['roles']['merge'], c['prior'], runner.calls))
    return gate.finish(floor=expected)


if __name__ == '__main__':
    sys.exit(main())
