"""Regle de claudefront1 (plan.json, ecrite avant les donnees) appliquee aux rapports gpu_ab.

Usage : python3 judge.py <dossier cmd des resultats>
Statistique : mediane des 6 processus a froid par trame et mode ; 6 rapports new/base (K5 CPU feuilles 16 en 278523,
K5 GPU feuilles 24 en 344059:400, trois trames). Garde si gm(prefix_ms) <= 0,80 et gm(domain_ms) <= 1,00.
"""
import glob
import json
import math
import statistics
import sys

FRAMES = ('lidar_ng00', 'lidar_ng01', 'lidar_ng02')


def main():
    root = sys.argv[1]
    prefix, domain, rows_out = [], [], []
    for name, mode in (('ab_k5_16_cpu', 'cpu'), ('ab_k5_24_gpu', 'gpu')):
        report = json.load(open(glob.glob(root + '/*_' + name + '/files/' + name + '/gpu_ab_report.json')[0]))
        if report.get('verdict') != 'conforme':
            print('banc non conforme', name, report.get('verdict'))
        for frame in FRAMES:
            med = {}
            for variant in ('new', 'base'):
                takes = [c['summary'] for c in report['cold']
                         if c['frame'] == frame and c['mode'] == variant + ':' + mode and c['code'] == 0]
                med[variant] = (statistics.median(s['prefix_ms'] for s in takes),
                                statistics.median(s['domain_ms'] for s in takes), len(takes))
            rp = med['new'][0] / med['base'][0]
            rd = med['new'][1] / med['base'][1]
            prefix.append(rp)
            domain.append(rd)
            rows_out.append((name, frame, med['base'][0], med['new'][0], rp, med['base'][1], med['new'][1], rd,
                             med['new'][2], med['base'][2]))
    for row in rows_out:
        print('%s %s frontiere %.2f -> %.2f (%.3f) domain %.1f -> %.1f (%.3f) prises %d/%d' % row)
    gp = math.exp(sum(map(math.log, prefix)) / len(prefix))
    gd = math.exp(sum(map(math.log, domain)) / len(domain))
    keep = gp <= 0.80 and gd <= 1.00
    print('gm frontiere %.3f (<= 0,80 : %s) ; gm domain %.3f (<= 1,00 : %s) ; verdict %s'
          % (gp, gp <= 0.80, gd, gd <= 1.00, 'garde' if keep else 'retire'))


if __name__ == '__main__':
    main()
