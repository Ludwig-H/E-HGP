#!/usr/bin/env python3
"""Lecture appariee d'un rapport de bench/ab_g4.py : rapports par paire, mediane appariee et test des signes.

    python3 bench/ab_summary.py RAPPORT.json --pairs base:q3,q3:new [--metrics wall,single_pass,forest]

Une paire est (trame, fils, prise) : les deux variantes ont tourne dans la meme prise, a positions alternees. Pour
chaque comparaison A:B, chaque trame et chaque nombre de fils : nombre de paires, mediane des rapports B/A, rapport des
medianes, victoires de B (B < A), et p bilaterale exacte du test des signes (binomiale 1/2, ex aequo ecartes). Aucun
seuil n'est decide ici : le protocole du plan (bras A/A, nombre de paires fixe d'avance) s'applique au lecteur.
Ecrit RAPPORT.json.paired.json. Bibliotheque standard seule. Codes : 0 lu ; 2 entree refusee.
"""
import argparse
import json
from math import comb
import sys

FIELDS = {'wall': lambda s: s.get('wall_ns'), 'single_pass': lambda s: (s.get('domain_detail') or {}).get('single_pass_ns'),
          'forest': lambda s: s.get('forest_ns'), 'domain': lambda s: s.get('domain_ns')}


def median(values):
    v = sorted(values)
    n = len(v)
    if n == 0:
        return None
    return v[n // 2] if n % 2 else (v[n // 2 - 1] + v[n // 2]) / 2


def sign_test(wins, losses):
    n = wins + losses
    if n == 0:
        return None
    k = min(wins, losses)
    return min(1.0, 2 * sum(comb(n, i) for i in range(k + 1)) / 2 ** n)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('report')
    ap.add_argument('--pairs', required=True)
    ap.add_argument('--metrics', default='wall,single_pass,forest')
    args = ap.parse_args()
    try:
        report = json.load(open(args.report))
        pairs = [tuple(p.split(':')) for p in args.pairs.split(',')]
        metrics = args.metrics.split(',')
    except (OSError, ValueError) as error:
        print('refus : %s' % error, file=sys.stderr)
        return 2
    if any(len(p) != 2 for p in pairs) or any(m not in FIELDS for m in metrics):
        print('refus : --pairs A:B,... et --metrics parmi %s' % sorted(FIELDS), file=sys.stderr)
        return 2
    takes = {}
    for t in report.get('timings', []):
        if t.get('code') != 0 or (t.get('summary') or {}).get('status') != 'ok':
            continue
        takes[(t['frame'], t['workers'], t['rep'], t['variant'])] = t['summary']
    out = []
    keys = sorted({(f, w) for (f, w, _, _) in takes})
    for a, b in pairs:
        for frame, workers in keys:
            reps = sorted({r for (f, w, r, v) in takes if f == frame and w == workers and v in (a, b)})
            for metric in metrics:
                ratios, wins, losses, va, vb = [], 0, 0, [], []
                for rep in reps:
                    sa, sb = takes.get((frame, workers, rep, a)), takes.get((frame, workers, rep, b))
                    if sa is None or sb is None:
                        continue
                    x, y = FIELDS[metric](sa), FIELDS[metric](sb)
                    if not x or y is None:
                        continue
                    ratios.append(y / x)
                    va.append(x); vb.append(y)
                    wins += y < x
                    losses += y > x
                if not ratios:
                    continue
                row = dict(a=a, b=b, frame=frame, workers=workers, metric=metric, pairs=len(ratios),
                           median_ratio=round(median(ratios), 4),
                           ratio_of_medians=round(median(vb) / median(va), 4) if median(va) else None,
                           median_a_ms=round(median(va) / 1e6, 2), median_b_ms=round(median(vb) / 1e6, 2),
                           b_wins=wins, b_losses=losses, sign_p=sign_test(wins, losses))
                out.append(row)
                print('%-4s -> %-4s %-11s w%-2s %-11s paires %d  mediane B/A %.4f  rapport des medianes %s  '
                      'A %.1f ms B %.1f ms  B gagne %d/%d  p signes %s' % (
                          a, b, frame, workers, metric, row['pairs'], row['median_ratio'], row['ratio_of_medians'],
                          row['median_a_ms'], row['median_b_ms'], wins, wins + losses,
                          None if row['sign_p'] is None else round(row['sign_p'], 4)))
    with open(args.report + '.paired.json', 'w') as handle:  # a cote du rapport lu
        json.dump(dict(schema='ehgp.v11.ab_summary.v1', rows=out), handle, indent=1)
    return 0


if __name__ == '__main__':
    sys.exit(main())
