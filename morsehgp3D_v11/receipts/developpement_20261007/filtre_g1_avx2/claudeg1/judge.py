"""Regle de claudeg1 (plan.json, ecrite avant les donnees) appliquee aux rapports gpu_ab.

Usage : python3 judge.py <dossier cmd des resultats>
Garde si gm(single_pass_ms + prefix_ms) <= 0,85 et gm(domain_ms) <= 1,00 sur les 6 rapports new/base a froid.
"""
import glob
import json
import math
import statistics
import sys

FRAMES = ('lidar_ng00', 'lidar_ng01', 'lidar_ng02')


def main():
    root = sys.argv[1]
    filt, domain = [], []
    for name, mode in (('ab_k5_16_cpu', 'cpu'), ('ab_k5_24_gpu', 'gpu')):
        report = json.load(open(glob.glob(root + '/*_' + name + '/files/' + name + '/gpu_ab_report.json')[0]))
        if report.get('verdict') != 'conforme':
            print('banc non conforme', name, report.get('verdict'))
        for frame in FRAMES:
            med = {}
            for variant in ('new', 'base'):
                takes = [c['summary'] for c in report['cold']
                         if c['frame'] == frame and c['mode'] == variant + ':' + mode and c['code'] == 0]
                med[variant] = (statistics.median(s['single_pass_ms'] + s['prefix_ms'] for s in takes),
                                statistics.median(s['domain_ms'] for s in takes))
            rf = med['new'][0] / med['base'][0]
            rd = med['new'][1] / med['base'][1]
            filt.append(rf)
            domain.append(rd)
            print('%s %s passe+frontiere %.1f -> %.1f (%.3f) domain %.1f -> %.1f (%.3f)' % (
                name, frame, med['base'][0], med['new'][0], rf, med['base'][1], med['new'][1], rd))
    gf = math.exp(sum(map(math.log, filt)) / len(filt))
    gd = math.exp(sum(map(math.log, domain)) / len(domain))
    keep = gf <= 0.85 and gd <= 1.00
    print('gm passe+frontiere %.3f (<= 0,85 : %s) ; gm domain %.3f (<= 1,00 : %s) ; verdict %s'
          % (gf, gf <= 0.85, gd, gd <= 1.00, 'garde' if keep else 'retire'))


if __name__ == '__main__':
    main()
