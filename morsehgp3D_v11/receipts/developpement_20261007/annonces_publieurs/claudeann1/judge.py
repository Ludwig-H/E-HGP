"""Regle de claudeann1 (plan.json, ecrite avant les donnees) appliquee aux rapports gpu_ab.

Usage : python3 judge.py <dossier cmd des resultats>
Garde si gm(publish_cpu_ms de l'ordre 5) <= 0,90 et gm(forest_ms) <= 1,00 sur les 6 rapports new/base a froid.
"""
import glob
import json
import math
import statistics
import sys

FRAMES = ('lidar_ng00', 'lidar_ng01', 'lidar_ng02')


def publisher(summary, k):
    return [o for o in summary['pipeline']['orders'] if o['k'] == k][0]['publish_cpu_ms']


def main():
    root = sys.argv[1]
    pub, forest = [], []
    for name, mode in (('ab_k5_16_cpu', 'cpu'), ('ab_k5_24_gpu', 'gpu')):
        report = json.load(open(glob.glob(root + '/*_' + name + '/files/' + name + '/gpu_ab_report.json')[0]))
        if report.get('verdict') != 'conforme':
            print('banc non conforme', name, report.get('verdict'))
        for frame in FRAMES:
            med = {}
            for variant in ('new', 'base'):
                takes = [c['summary'] for c in report['cold']
                         if c['frame'] == frame and c['mode'] == variant + ':' + mode and c['code'] == 0]
                med[variant] = (statistics.median(publisher(s, 5) for s in takes),
                                statistics.median(s['forest_ms'] for s in takes),
                                [statistics.median(publisher(s, k) for s in takes) for k in (3, 4)])
            rp = med['new'][0] / med['base'][0]
            rf = med['new'][1] / med['base'][1]
            pub.append(rp)
            forest.append(rf)
            print('%s %s publieur5 %.1f -> %.1f (%.3f) forest %.1f -> %.1f (%.3f) publieurs3-4 %s -> %s' % (
                name, frame, med['base'][0], med['new'][0], rp, med['base'][1], med['new'][1], rf,
                ['%.1f' % v for v in med['base'][2]], ['%.1f' % v for v in med['new'][2]]))
    gp = math.exp(sum(map(math.log, pub)) / len(pub))
    gf = math.exp(sum(map(math.log, forest)) / len(forest))
    keep = gp <= 0.90 and gf <= 1.00
    print('gm publieur5 %.3f (<= 0,90 : %s) ; gm forest %.3f (<= 1,00 : %s) ; verdict %s'
          % (gp, gp <= 0.90, gf, gf <= 1.00, 'garde' if keep else 'retire'))


if __name__ == '__main__':
    main()
