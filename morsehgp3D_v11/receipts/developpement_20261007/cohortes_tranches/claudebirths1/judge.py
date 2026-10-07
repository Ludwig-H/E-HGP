"""Regle de claudebirths1 (plan.json, ecrite avant les donnees) appliquee aux rapports gpu_ab.

Usage : python3 judge.py <dossier cmd des resultats>
Statistique : mediane des 6 processus a froid par trame et mode ; 6 rapports new/base (K5 CPU feuilles 16 en 278523,
K5 GPU feuilles 24 en 344059:400, trois trames). Garde si gm(births) <= 0,80 et gm(forest_ms) <= 1,00.
"""
import glob
import json
import math
import statistics
import sys

FRAMES = ('lidar_ng00', 'lidar_ng01', 'lidar_ng02')


def births_ms(summary):
    return summary['pipeline']['phases']['births_ns']


def main():
    root = sys.argv[1]
    births, forest, rows_out = [], [], []
    for name, mode in (('ab_k5_16_cpu', 'cpu'), ('ab_k5_24_gpu', 'gpu')):
        report = json.load(open(glob.glob(root + '/*_' + name + '/files/' + name + '/gpu_ab_report.json')[0]))
        if report.get('verdict') != 'conforme':
            print('banc non conforme', name, report.get('verdict'))
        for frame in FRAMES:
            med = {}
            for variant in ('new', 'base'):
                takes = [c['summary'] for c in report['cold']
                         if c['frame'] == frame and c['mode'] == variant + ':' + mode and c['code'] == 0]
                med[variant] = (statistics.median(births_ms(s) for s in takes),
                                statistics.median(s['forest_ms'] for s in takes), len(takes))
            rb = med['new'][0] / med['base'][0]
            rf = med['new'][1] / med['base'][1]
            births.append(rb)
            forest.append(rf)
            rows_out.append((name, frame, med['base'][0], med['new'][0], rb, med['base'][1], med['new'][1], rf,
                             med['new'][2], med['base'][2]))
    for row in rows_out:
        print('%s %s naissances %.2f -> %.2f (%.3f) forest %.1f -> %.1f (%.3f) prises %d/%d' % row)
    gb = math.exp(sum(map(math.log, births)) / len(births))
    gf = math.exp(sum(map(math.log, forest)) / len(forest))
    keep = gb <= 0.80 and gf <= 1.00
    print('gm naissances %.3f (<= 0,80 : %s) ; gm forest %.3f (<= 1,00 : %s) ; verdict %s'
          % (gb, gb <= 0.80, gf, gf <= 1.00, 'garde' if keep else 'retire'))


if __name__ == '__main__':
    main()
