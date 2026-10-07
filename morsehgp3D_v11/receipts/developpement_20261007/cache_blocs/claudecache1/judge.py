"""Regle de claudecache1 (plan.json, ecrite avant les donnees) appliquee aux rapports gpu_ab.

Usage : python3 judge.py <dossier cmd des resultats>
Adopte si gm(mur a chaud cache/nu) <= 0,95 et gm(mur a froid cache/nu) <= 1,00 sur les 6 rapports K5.
"""
import glob
import json
import math
import sys

FRAMES = ('lidar_ng00', 'lidar_ng01', 'lidar_ng02')


def main():
    root = sys.argv[1]
    warm, cold = [], []
    for name, plain, cached in (('ab_k5_16_cpu', 'cpu', 'cpu_cache'), ('ab_k5_24_gpu', 'gpu', 'gpu_cache')):
        report = json.load(open(glob.glob(root + '/*_' + name + '/files/' + name + '/gpu_ab_report.json')[0]))
        if report.get('verdict') != 'conforme':
            print('banc non conforme', name, report.get('verdict'))
        w, c = report['warm_medians_ms'], report['cold_medians_ms']
        for frame in FRAMES:
            a, b = w['warm|%s|w48|%s' % (frame, plain)], w['warm|%s|w48|%s' % (frame, cached)]
            ca, cb = c['cold|%s|w48|%s' % (frame, plain)], c['cold|%s|w48|%s' % (frame, cached)]
            warm.append(b['wall_ms'] / a['wall_ms'])
            cold.append(cb['wall_ms'] / ca['wall_ms'])
            print('%s %s chaud %.1f -> %.1f (%.3f) froid %.1f -> %.1f (%.3f)' % (
                name, frame, a['wall_ms'], b['wall_ms'], warm[-1], ca['wall_ms'], cb['wall_ms'], cold[-1]))
    gw = math.exp(sum(map(math.log, warm)) / len(warm))
    gc = math.exp(sum(map(math.log, cold)) / len(cold))
    keep = gw <= 0.95 and gc <= 1.00
    print('gm chaud %.3f (<= 0,95 : %s) ; gm froid %.3f (<= 1,00 : %s) ; verdict %s'
          % (gw, gw <= 0.95, gc, gc <= 1.00, 'adopte' if keep else 'option'))


if __name__ == '__main__':
    main()
