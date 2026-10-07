"""Regle de claudepref1 (plan.json, ecrite avant les donnees) appliquee aux rapports gpu_ab.

Usage : python3 judge.py <dossier cmd des resultats>
Garde si gm(publish_cells_est_ms de l'ordre 5) <= 0,85 et gm(forest_ms) <= 1,00 sur les 6 rapports new/base a froid.
"""
import glob
import json
import math
import statistics
import sys

FRAMES = ('lidar_ng00', 'lidar_ng01', 'lidar_ng02')


def order(summary, k, key):
    return [o for o in summary['pipeline']['orders'] if o['k'] == k][0][key]


def main():
    root = sys.argv[1]
    cells, forest = [], []
    for name, mode in (('ab_k5_16_cpu', 'cpu'), ('ab_k5_24_gpu', 'gpu')):
        report = json.load(open(glob.glob(root + '/*_' + name + '/files/' + name + '/gpu_ab_report.json')[0]))
        if report.get('verdict') != 'conforme':
            print('banc non conforme', name, report.get('verdict'))
        for frame in FRAMES:
            med = {}
            for variant in ('new', 'base'):
                takes = [c['summary'] for c in report['cold']
                         if c['frame'] == frame and c['mode'] == variant + ':' + mode and c['code'] == 0]
                med[variant] = (statistics.median(order(s, 5, 'publish_cells_est_ms') for s in takes),
                                statistics.median(s['forest_ms'] for s in takes),
                                statistics.median(order(s, 5, 'publish_cpu_ms') for s in takes))
            rc = med['new'][0] / med['base'][0]
            rf = med['new'][1] / med['base'][1]
            cells.append(rc)
            forest.append(rf)
            print('%s %s cellules5 %.1f -> %.1f (%.3f) forest %.1f -> %.1f (%.3f) publieur5 %.1f -> %.1f' % (
                name, frame, med['base'][0], med['new'][0], rc, med['base'][1], med['new'][1], rf, med['base'][2],
                med['new'][2]))
    gc = math.exp(sum(map(math.log, cells)) / len(cells))
    gf = math.exp(sum(map(math.log, forest)) / len(forest))
    keep = gc <= 0.85 and gf <= 1.00
    print('gm cellules5 %.3f (<= 0,85 : %s) ; gm forest %.3f (<= 1,00 : %s) ; verdict %s'
          % (gc, gc <= 0.85, gf, gf <= 1.00, 'garde' if keep else 'retire'))


if __name__ == '__main__':
    main()
