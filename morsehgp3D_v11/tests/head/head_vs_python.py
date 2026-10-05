#!/usr/bin/env python3
"""Porte mhgp11_head_vs_python (tranche S10, label long, numpy) : la tete plate native contre la tete Python
qualifiee (bench/points_flat.py : condense, select, labels), sur le MEME arbre de points.

    python3 head_vs_python.py --cli <mhgp11> --work <dossier> [--clouds N] [--k=1,2,...] [--mcs=3,5,...]
            [--data=<trame>] [--min-clouds N] [--min-calls N] [--min-clusters N]

Pour chaque nuage (amas tires a graine fixe, PointId non denses, ordre d'entree melange ; ou trame du dossier
MHGP11_DATA_DIR) et chaque K : un appel --sortie=points (W1), relu par le lecteur officiel (structure) ; l'arbre de
points publie (plateaux en valeur exacte, blocs, entrees) est donne a points_flat.flat. Puis, pour chaque mcs et chaque
selection (EOM z = 1, 2, 3 ; feuilles), un appel --sortie=plat (W4) relu par check_directory. Exige :
  - meme partition des points (bijection des clusters, meme bruit) entre MHGP11ET et la tete Python ;
  - chaque etiquette native egale au plus petit PointId de son cluster ;
  - meme tree_k_sha256 que --sortie=points ;
  - aucun refus de la tete native quand Python decide (un refus Python est compte, jamais une egalite supposee).
Codes : 0 conforme ; 1 ecart ; 2 refus d'usage ; 3 plancher. Derniere ligne de verdict :
    head_vs_python_verdict conforme nuages=<c> appels=<a> clusters=<k> retenus=<r> bruit=<b>
"""
import argparse
import os
import random
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, '..', '..'))
sys.path.insert(0, os.path.join(ROOT, 'tests', 'cli'))
import cli_support as cs  # noqa: E402

sys.path.insert(0, os.path.join(ROOT, 'bench'))
import points_flat as pf  # noqa: E402

mhgp11_gate = cs.mhgp11_gate
formats = cs.formats
LINES = (('eom', 1), ('eom', 2), ('eom', 3), ('feuilles', 1))


def clustered(rng, n):
    """Amas : 2 a 6 centres dans une boite de 400, points a +-12 des centres, 10 % de bruit ; positions distinctes."""
    centres = [tuple(rng.randrange(20, 380) for _ in range(3)) for _ in range(rng.randrange(2, 7))]
    seen, points = set(), []
    while len(points) < n:
        if rng.random() < 0.1:
            p = tuple(rng.randrange(0, 400) for _ in range(3))
        else:
            c = rng.choice(centres)
            p = tuple(max(0, c[a] + rng.randrange(-12, 13)) for a in range(3))
        if p not in seen:
            seen.add(p)
            points.append(p)
    return points


def python_tree(f):
    """points_flat.PointTree de l'arbre de points publie (plateaux en valeur exacte, indices de sites natifs)."""
    pt = pf.PointTree(f.n)
    for t, m, q in zip(f.plateau_t, f.plateau_M, f.plateau_Q):
        pt.add_plateau(pf.Level(f.level(t), f.level(m), f.level(q)))
    for b, (plateau, parent) in enumerate(zip(f.block_plateau, f.block_parent)):
        pt.block_plateau.append(plateau)
        pt.block_parent.append(-1 if parent == formats.NONE else parent)
        pt.block_merged.append(False)
    for b, parent in enumerate(pt.block_parent):
        if parent >= 0:
            pt.block_merged[parent] = True
    for s in range(f.n):
        pt.enter(s, f.site_block[s], f.site_plateau[s])
    return pt.finish()


def partitions_agree(gate, where, f, python_labels, native_by_pid):
    """Bijection des clusters, meme bruit ; etiquette native = plus petit PointId du cluster. Rend les clusters."""
    pairs = {}
    reverse = {}
    members = {}
    ok = True
    for s in range(f.n):
        py, nat = int(python_labels[s]), native_by_pid[f.point_id[s]]
        if (py < 0) != (nat < 0):
            ok = False
            break
        if py < 0:
            continue
        if pairs.setdefault(py, nat) != nat or reverse.setdefault(nat, py) != py:
            ok = False
            break
        members.setdefault(nat, []).append(f.point_id[s])
    gate.check(ok, '%s : partitions differentes' % where)
    gate.check(all(label == min(ids) for label, ids in members.items()),
               '%s : etiquette differente du plus petit PointId du cluster' % where)
    return len(members)


def run_case(args, gate, work, name, points, ids, k, stats):
    folder = tempfile.mkdtemp(prefix='c', dir=work)
    order = list(range(len(points)))
    random.Random(len(points) * 31 + k).shuffle(order)
    xyz, names = cs.write_inputs(folder, points, ids, order)
    d = os.path.join(folder, 'P')
    result, rows = cs.run_cli(cs.cli_argv(args.cli, xyz, names, d, k, 1, output='points'), timeout=3600)
    if not gate.check(result.code == 0 and rows and rows[0].get('status') == 'ok',
                      '%s K%d points : %s' % (name, k, result.describe())):
        return
    read = formats.check_directory(d, args.bits, exact=False)
    f, tree_sha = read['decoded'], read['manifest']['tree_k_sha256']
    pt = python_tree(f)
    input_order_pids = [ids[i] for i in order]
    for mcs in args.mcs:
        for method, z in LINES:
            where = '%s K%d mcs%d %s z%d' % (name, k, mcs, method, z)
            try:
                labels, _ = pf.flat(pt, mcs, z, 'eom' if method == 'eom' else 'leaf')
            except pf.Refusal:
                stats['python_refusals'] += 1
                continue
            out = os.path.join(folder, 'F%d%s%d' % (mcs, method, z))
            extra = ['--mcs=%d' % mcs, '--z=%d' % z, '--selection=' + method]
            result, rows = cs.run_cli(cs.cli_argv(args.cli, xyz, names, out, k, 4, extra=extra, output='plat'),
                                      timeout=3600)
            stats['calls'] += 1
            if not gate.check(result.code == 0 and rows and rows[0].get('status') == 'ok',
                              '%s : %s %s' % (where, result.describe(), (result.stderr or '')[-300:])):
                continue
            got = formats.check_directory(out, args.bits)
            gate.check_eq(got['manifest']['tree_k_sha256'], tree_sha, '%s : tree_k_sha256' % where)
            native = got['decoded']['labels']
            by_pid = dict(zip(input_order_pids, native))
            stats['clusters'] += partitions_agree(gate, where, f, labels, by_pid)
            stats['selected'] += got['manifest']['counts']['selected']
            stats['noise'] += got['manifest']['counts']['noise']
            stats['exact'] += got['manifest']['counts']['exact']
            shutil.rmtree(out, ignore_errors=True)
    stats['clouds'] += 1
    shutil.rmtree(folder, ignore_errors=True)


def main():
    parser = argparse.ArgumentParser(description='Tete plate native contre points_flat.flat sur le meme arbre.')
    parser.add_argument('--cli', required=True)
    parser.add_argument('--bits', type=int, choices=(18, 21, 24), required=True)
    parser.add_argument('--work', required=True)
    parser.add_argument('--clouds', type=int, default=24)
    parser.add_argument('--k', default='1,2,3,4,5')
    parser.add_argument('--mcs', default='3,5,10')
    parser.add_argument('--data', default='')
    parser.add_argument('--min-clouds', type=int, default=1)
    parser.add_argument('--min-calls', type=int, default=1)
    parser.add_argument('--min-clusters', type=int, default=1)
    args = parser.parse_args()
    try:
        ks = [int(x) for x in args.k.split(',')]
        args.mcs = [int(x) for x in args.mcs.split(',')]
    except ValueError:
        print('usage : --k et --mcs entiers separes par des virgules')
        return mhgp11_gate.REFUSAL
    gate = mhgp11_gate.Gate('head_vs_python')
    stats = dict(clouds=0, calls=0, clusters=0, selected=0, noise=0, exact=0, python_refusals=0)
    os.makedirs(args.work, exist_ok=True)
    work = tempfile.mkdtemp(prefix='head-vs-python-', dir=args.work)
    try:
        if args.data:
            points, ids = cs.lidar(args.data)
            for k in ks:
                run_case(args, gate, work, args.data, points, ids, k, stats)
        else:
            rng = random.Random(20261005)
            for index in range(args.clouds):
                n = rng.randrange(40, 400)
                points = clustered(rng, n)
                ids = cs.distinct_ids(n, rng)
                for k in ks:
                    if k >= 2 and k >= n:
                        continue
                    run_case(args, gate, work, 'amas%d' % index, points, ids, k, stats)
    finally:
        shutil.rmtree(work, ignore_errors=True)
    print('{"clouds":%d,"calls":%d,"clusters":%d,"selected":%d,"noise":%d,"exact":%d,"python_refusals":%d}' %
          (stats['clouds'], stats['calls'], stats['clusters'], stats['selected'], stats['noise'], stats['exact'],
           stats['python_refusals']))
    if stats['clouds'] < args.min_clouds or stats['calls'] < args.min_calls or stats['clusters'] < args.min_clusters:
        print('PLANCHER nuages=%d appels=%d clusters=%d' % (stats['clouds'], stats['calls'], stats['clusters']))
        return mhgp11_gate.FLOOR
    if gate.failures == 0:
        print('head_vs_python_verdict conforme nuages=%d appels=%d clusters=%d retenus=%d bruit=%d'
              % (stats['clouds'], stats['calls'], stats['clusters'], stats['selected'], stats['noise']))
    return gate.finish(floor=1)


if __name__ == '__main__':
    sys.exit(main())
