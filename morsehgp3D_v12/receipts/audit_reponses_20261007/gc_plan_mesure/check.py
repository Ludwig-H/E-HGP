#!/usr/bin/env python3
"""Ordre des bras T2-c : modèle sans temps mesurés, données ni exécution native."""
import json
import math
from collections import Counter


def check(ok, message):
    if not ok:
        raise RuntimeError(message)


ARMS = ('avant', 'avant_bis', 'sans_gl7', 'apres', 'gl5')


def orders(rounds):
    rows = []
    for t in range(rounds):
        row = [(t + i) % len(ARMS) for i in range(len(ARMS))]
        if t % 2:
            row.reverse()
        rows.append(row)
    return rows


def model(rounds):
    rows = orders(rounds)
    positions = {arm: [row.index(i) for row in rows] for i, arm in enumerate(ARMS)}
    counts = {arm: [Counter(p)[i] for i in range(5)] for arm, p in positions.items()}
    # Modèle synthétique sans unité : bras strictement identiques, seul le premier
    # processus de chaque tour coûte 1,1, les quatre autres 1. Aucun chrono observé.
    cost = lambda position: 1.1 if position == 0 else 1.0
    ratios = {arm: [cost(p) / cost(b) for p, b in zip(pos, positions['avant'])]
              for arm, pos in positions.items()}
    gm = {arm: math.exp(sum(map(math.log, r)) / rounds) for arm, r in ratios.items()}
    check(all(sorted(row) == list(range(5)) for row in rows), 'each round covers arms')
    return {'rounds': rounds, 'positions': counts, 'synthetic_unitless_gm': gm}


def main():
    eight, ten = model(8), model(10)
    check(eight['positions']['apres'] == [0, 2, 2, 2, 2], 'actual eight-round imbalance')
    check(eight['positions']['avant'] == eight['positions']['avant_bis'], 'AA same marginal positions')
    check(abs(eight['synthetic_unitless_gm']['avant_bis'] - 1) < 1e-14, 'AA neutral in model')
    check(eight['synthetic_unitless_gm']['apres'] < 1, 'point estimator biased in model')
    check(all(c == [2] * 5 for c in ten['positions'].values()), 'ten rounds balanced by position')
    check(all(abs(g - 1) < 1e-14 for g in ten['synthetic_unitless_gm'].values()), 'model bias removed')
    print(json.dumps({'scope': 'position_model_not_benchmark_or_false_adoption',
                      'eight': eight, 'ten': ten}, sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
