"""Audit L07 : depouille les JSONL de la sonde. Pour chaque (source, K) et chaque configuration de tete : nombre de
cas, defaut declenche (une cohorte laisse un reste non vide sous mcs), stabilites changees, etiquettes changees
(V publie contre C cohortes, contre N cohortes sur dendrogramme normalise). Pour les cas a etiquettes changees :
ARI_s contre la verite du generateur, sans remplissage et avec le remplissage borne b2 du banc (k = max(K, 5)).

  python3 -B analyse_dev.py RUN_DIR SCENES_DIR [--detail]
"""
import glob
import json
import os
import sys
from collections import defaultdict

import numpy as np

BENCH = '/workspaces/E-HGP/build/v11-worktree/morsehgp3D_v10/bench/synthetic'
sys.path.insert(0, BENCH)
import methods  # noqa: E402
import metrics  # noqa: E402


def main():
    run_dir, scenes_dir = sys.argv[1], sys.argv[2]
    detail = '--detail' in sys.argv
    index = {s['name']: s for s in json.load(open(os.path.join(scenes_dir, 'index.json')))}
    agg = defaultdict(lambda: defaultdict(float))
    changed = []
    scenes = 0
    families = defaultdict(int)
    for path in sorted(glob.glob(os.path.join(run_dir, '*.jsonl'))):
        name = os.path.basename(path)[:-6]
        names = json.load(open(os.path.join(run_dir, name + '.cfgnames')))
        rows = [json.loads(l) for l in open(path) if l.startswith('{"source"')]
        if not rows:
            continue
        scenes += 1
        families[index[name]['family']] += 1
        G = T = None
        for r in rows:
            if 'cfg' not in r:
                agg[(r['source'], r['k'], 'REFUS')]['cas'] += 1
                continue
            c = names[r['cfg']]
            key = (r['source'], r['k'], '%s mcs=%d z=%s' % (c['sel'], c['mcs'], c['zname']))
            a = agg[key]
            a['cas'] += 1
            a['port_bad'] += not r['port_ok']
            a['declenche'] += r['trig_c'] > 0
            a['stab'] += r['stab_changed'] > 0
            a['lab_vc'] += r['lab_vc'] > 0
            a['lab_vn'] += r['lab_vn'] > 0
            a['lab_cn'] += r['lab_cn'] > 0
            a['lab_vvn'] += r['lab_vvn'] > 0
            a['norm'] += (r['contracted'] + r['relocated']) > 0
            a['relocated'] += r['relocated'] > 0
            a['feuilles'] += r['leaves']
            a['noeuds_declenches'] += r['trig_c']
            a['leaf_sum_v'] += r['leaf_sum_v'] if np.isfinite(r['leaf_sum_v']) else 0
            a['leaf_sum_c'] += r['leaf_sum_c'] if np.isfinite(r['leaf_sum_c']) else 0
            if r['lab_vn'] > 0:
                if G is None:
                    G = np.fromfile(os.path.join(scenes_dir, name + '.u32le'), dtype='<u4').reshape(-1, 3)
                    T = np.fromfile(os.path.join(scenes_dir, name + '.truth.i32le'), dtype='<i4').astype(np.int64)
                base = os.path.join(run_dir, '%s.%s.k%d.%d' % (name, r['source'], r['k'], r['cfg']))
                V = np.fromfile(base + '.v', dtype='<i4').astype(np.int64)
                N = np.fromfile(base + '.n', dtype='<i4').astype(np.int64)
                kf = max(int(r['k']), 5)
                sv, sn = metrics.scores(T, V), metrics.scores(T, N)
                fv = metrics.scores(T, methods.bounded_fill(G, V, kf, 2.0))
                fn = metrics.scores(T, methods.bounded_fill(G, N, kf, 2.0))
                a['d_ari'] += sn['ari_s'] - sv['ari_s']
                a['d_ari_b2'] += fn['ari_s'] - fv['ari_s']
                a['d_abs'] += abs(sn['ari_s'] - sv['ari_s'])
                changed.append(dict(scene=name, source=r['source'], k=r['k'], cfg=key[2], sel_v=r['sel_v'], sel_n=r['sel_n'],
                                    points_changed=r['lab_vn'], ari_v=round(sv['ari_s'], 4), ari_n=round(sn['ari_s'], 4),
                                    ari_v_b2=round(fv['ari_s'], 4), ari_n_b2=round(fn['ari_s'], 4),
                                    ari_between=round(float(metrics.adjusted_rand_score(metrics.singletons(V), metrics.singletons(N))), 4)))
    print('scenes %d ; familles %s' % (scenes, dict(families)))
    print('%-10s %2s %-22s %4s %4s %4s %4s %4s %4s %4s %5s %8s %8s %9s' % (
        'source', 'K', 'tete', 'cas', 'decl', 'stab', 'V!=C', 'V!=N', 'C!=N', 'norm', 'infl', 'dARI', 'dARI_b2', 'port_bad'))
    tot = defaultdict(lambda: defaultdict(float))
    for key in sorted(agg, key=lambda k: (k[0], k[1], k[2])):
        a = agg[key]
        infl = a['leaf_sum_v'] / a['leaf_sum_c'] if a['leaf_sum_c'] else float('nan')
        if detail or True:
            print('%-10s %2d %-22s %4d %4d %4d %4d %4d %4d %4d %5.2f %+8.4f %+8.4f %9d' % (
                key[0], key[1], key[2], a['cas'], a['declenche'], a['stab'], a['lab_vc'], a['lab_vn'], a['lab_cn'], a['norm'],
                infl, a['d_ari'] / max(a['cas'], 1), a['d_ari_b2'] / max(a['cas'], 1), a['port_bad']))
        t = tot[key[0]]
        for f in ('cas', 'declenche', 'stab', 'lab_vc', 'lab_vn', 'lab_cn', 'port_bad', 'lab_vvn', 'relocated'):
            t[f] += a[f]
    print()
    for s in sorted(tot):
        t = tot[s]
        print('TOTAL %-10s cas %5d declenche %5d stab %5d V!=C %4d V!=N %4d C!=N %4d V!=VN %4d relocalises %4d port_bad %d' % (
            s, t['cas'], t['declenche'], t['stab'], t['lab_vc'], t['lab_vn'], t['lab_cn'], t['lab_vvn'], t['relocated'], t['port_bad']))
    print()
    print('cas a etiquettes changees : %d' % len(changed))
    for c in changed:
        print(json.dumps(c))


if __name__ == '__main__':
    main()
