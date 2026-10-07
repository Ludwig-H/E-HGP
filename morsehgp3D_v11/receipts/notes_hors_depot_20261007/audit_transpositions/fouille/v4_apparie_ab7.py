#!/usr/bin/env python3
"""Statistique appariee de la v4 (mediane des rapports par paire, test des signes) appliquee aux prises G4 AB7.

Lecture seule : l'archive `claudeab7/results.tar.gz` est lue par `git show origin/main:...` et decompressee en
memoire (aucun fichier ecrit). Prises : moteur `b87285378` (« new ») contre base `a45daff3a` (« base »), mode 16379,
K = 1..5, u21, cinq prises W48 par trame, base et new jouees l'une apres l'autre, ordre alterne d'une prise a
l'autre (`protocol/ab_g4.py`, boucle `for variant in (variants if rep % 2 == 0 else reversed)`).

Usage : python3 -B v4_apparie_ab7.py   (code 0 si tout est lu)
"""
import io
import json
import math
import statistics
import subprocess
import sys
import tarfile

WORKTREE = '/workspaces/E-HGP/build/v11-claude-20261003'
ARCHIVE = ('origin/main:morsehgp3D_v11/receipts/developpement_20261003/pipeline_g4/sessions/'
           'claudeab7/results.tar.gz')
PREFIX = 'results/cmd/000_ab/files/'
FRAMES = ('lidar_ng00', 'lidar_ng01', 'lidar_ng02')
METRICS = ('wall_ns', 'domain_ns', 'forest_ns')


def load():
    raw = subprocess.run(['git', '-C', WORKTREE, 'show', ARCHIVE], check=True, capture_output=True).stdout
    with tarfile.open(fileobj=io.BytesIO(raw), mode='r:gz') as tar:
        return {m.name: tar.extractfile(m).read().decode() for m in tar.getmembers()
                if m.isfile() and m.name.startswith(PREFIX) and m.name.endswith('.stdout')}


def full_phase(text):
    for line in text.splitlines():
        try:
            record = json.loads(line)
        except ValueError:
            continue
        if record.get('phase') == 'full':
            return record
    raise ValueError('phase full absente')


def sign_test(wins, n):
    """P(X >= wins) pour X ~ Bin(n, 1/2), unilateral."""
    return sum(math.comb(n, k) for k in range(wins, n + 1)) / 2 ** n


def main():
    files = load()
    take = lambda variant, frame, rep: full_phase(files[PREFIX + 't_%s_%s_w48_r%d.stdout' % (variant, frame, rep)])
    pooled = {m: [] for m in METRICS}
    for frame in FRAMES:
        for metric in METRICS:
            base = [take('base', frame, r)[metric] / 1e6 for r in range(5)]
            new = [take('new', frame, r)[metric] / 1e6 for r in range(5)]
            ratios = [n / b for n, b in zip(new, base)]
            pooled[metric] += [math.log(x) for x in ratios]
            wins = sum(1 for x in ratios if x < 1)
            print('%s %-9s base_ms=%s new_ms=%s' % (frame, metric, ' '.join('%.0f' % x for x in base),
                                                   ' '.join('%.0f' % x for x in new)))
            print('    rapports=%s mediane_appariee=%.4f rapport_de_medianes=%.4f victoires_new=%d/5 p=%.4f'
                  % (' '.join('%.3f' % x for x in ratios), statistics.median(ratios),
                     statistics.median(new) / statistics.median(base), wins, sign_test(wins, 5)))
    for metric in METRICS:
        logs = pooled[metric]
        sigma = statistics.stdev(logs)
        # Paires pour detecter 5 % (t unilateral, alpha 0,05, puissance 0,8), approximation normale.
        need = ((1.645 + 0.842) * sigma / math.log(1 / 0.95)) ** 2
        print('global %-9s paires=%d moyenne_log=%+.3f sigma_log=%.3f paires_pour_5pc=%.0f'
              % (metric, len(logs), statistics.mean(logs), sigma, need))
    # Series v4 publiees (receipts/forest_20260817/ADDENDUM_BANC_APPARIE_ET_ORDONNANCEMENT_20260819.md, § 2).
    for name, ratios in (('v4_serie_reference', [0.8797, 0.8459, 0.8963, 0.8615, 0.9139,
                                                 0.8661, 0.8548, 0.8741, 0.8829, 0.8900]),
                         ('v4_serie_avant_reference', [0.8002, 0.9314, 0.8489, 1.0854, 0.7450,
                                                       0.6384, 0.7502, 0.8698, 0.9566, 0.8418])):
        logs = [math.log(x) for x in ratios]
        print('%s mediane_appariee=%.4f sigma_log=%.3f' % (name, statistics.median(ratios), statistics.stdev(logs)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
