#!/usr/bin/env python3
"""Lentille 7 : lois d'echelle locales (recu lidar_scaling_local_20260923, origin/main 1f73b40d,
extrait en lecture seule par git show) et partiel (lidar_scaling_local_partial_20260923).
Exposants p = log2(m(2n)/m(n)) ; densites par site (boules, noeuds de sortie, contributions)."""
import json
import math
import sys
from pathlib import Path

D = Path(sys.argv[1])
PHASES = ('q2', 'q34', 'merge', 'census', 'tower', 'digest')


def load(case_dir):
    rows = {}
    for f in sorted(case_dir.glob('*.json')):
        if f.name.startswith('SUMMARY'):
            continue
        v = json.loads(f.read_text())
        p = v['probe']
        name = f.stem.split('_r0_')[-1]
        orders = p['orders']
        t = p['times_ms']
        rows[name] = dict(n=p['input']['sites'], chain=t['chain_total'] / 1e3, cpu=p['chain_cpu_s'],
                          ext=v['external_wall_s'],
                          balls=p['catalogue']['balls'], nodes=sum(o['nodes'] for o in orders),
                          contrib=sum(o['contributions'] for o in orders), topK=orders[-1]['nodes'],
                          rss=p['peak_rss_kb'], **{ph: t[ph] / 1e3 for ph in PHASES},
                          pairs=p['generator']['q34_expanded_pairs'],
                          q3e=p['generator']['q3_emitted'], q4e=p['generator']['q4_emitted'],
                          q3seeds=p['ledger']['q3_seeds'], q4seeds=p['ledger']['q4_seeds'],
                          atlas_pt=p['ledger']['atlas_point_tests'],
                          core_sites=p['ledger']['core_sites'], cover_sites=p['ledger']['cover_sites'],
                          meb=p['tower_work']['meb_calls'], intr=p['tower_work']['intruder_nodes'])
    return rows


def p2(a, b, na, nb):
    if a <= 0 or b <= 0:
        return float('nan')
    return math.log(b / a) / math.log(nb / na)


def main():
    out = {}
    for case_dir in sorted((D / 'out').iterdir()):
        rows = load(case_dir)
        tag = case_dir.name
        nest = [rows['nested_8000'], rows['nested_16000'], rows['nested_32000']]
        full = rows['piece_full']
        print('\n==', tag, 'full n =', full['n'])
        print('  size   n     chain   cpu    q34    tower  q2    census merge digest balls/n nodes/n contrib/n topK/n rssMiB')
        for lab, r in (('8k', nest[0]), ('16k', nest[1]), ('32k', nest[2]), ('full', full)):
            print('  %-5s %6d %7.2f %6.1f %6.2f %6.2f %5.2f %5.2f %5.2f %5.2f %7.2f %7.2f %7.2f %6.2f %6.0f' % (
                lab, r['n'], r['chain'], r['cpu'], r['q34'], r['tower'], r['q2'], r['census'], r['merge'], r['digest'],
                r['balls'] / r['n'], r['nodes'] / r['n'], r['contrib'] / r['n'], r['topK'] / r['n'], r['rss'] / 1024))
        keys = ('chain', 'cpu', 'q34', 'tower', 'q2', 'census', 'merge', 'digest', 'balls', 'nodes', 'contrib', 'pairs',
                'atlas_pt', 'core_sites', 'cover_sites', 'meb', 'intr', 'q3seeds', 'q4seeds')
        exps = {}
        for k in keys:
            exps[k] = [round(p2(nest[i][k], nest[i + 1][k], nest[i]['n'], nest[i + 1]['n']), 2) for i in range(2)]
            # 32k -> entier (rapport de tailles non 2)
            exps[k].append(round(p2(nest[2][k], full[k], nest[2]['n'], full['n']), 2))
        print('  exposants (8k->16k, 16k->32k, 32k->full):')
        for k in keys:
            print('    %-12s %s' % (k, exps[k]))
        # travail par sortie
        print('  travail / sortie (entier) : paires/(q3e+q4e)=%.1f  graines q3/q3e=%.1f  graines q4/q4e=%.1f  '
              'tests atlas/(q3e+q4e)=%.0f  MEB/boule=%.2f  noeuds intrus/boule=%.1f' % (
                  full['pairs'] / (full['q3e'] + full['q4e']), full['q3seeds'] / full['q3e'],
                  full['q4seeds'] / max(1, full['q4e']), full['atlas_pt'] / (full['q3e'] + full['q4e']),
                  full['meb'] / full['balls'], full['intr'] / full['balls']))
        out[tag] = dict(rows=rows, exps=exps)
    (D.parent / 'scaling_summary.json').write_text(json.dumps(out, indent=1))


if __name__ == '__main__':
    main()
