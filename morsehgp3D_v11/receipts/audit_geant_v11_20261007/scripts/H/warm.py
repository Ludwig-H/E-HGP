"""Regime a chaud : dispersion intra-processus (passes 2..P) contre dispersion entre processus (A/A explicite o1place3)."""
import json, math, statistics, glob, os
R = '/workspaces/E-HGP/morsehgp3D_v11/receipts/'
def within(row, key):
    later = [p[key] / 1e6 for p in row['passes'] if p.get('pass', 0) >= 2 and p.get('status') == 'ok' and key in p]
    if len(later) < 3:
        return None
    lg = [math.log(v) for v in later]
    return statistics.stdev(lg), statistics.median(later)
# 1) intra-processus sur toutes les sessions K5
acc = {'wall_ns': [], 'domain_ns': [], 'forest_ns': []}
for p in sorted(glob.glob(R + 'developpement_2026100[67]/**/gpu_ab_report*.json', recursive=True)):
    r = json.load(open(p))
    if r.get('kmax') != 5:
        continue
    for row in r['warm']:
        for k in acc:
            w = within(row, k)
            if w:
                acc[k].append(w[0])
for k, v in acc.items():
    print('intra-processus %s : n=%d processus, sd log mediane=%.4f, moyenne=%.4f, max=%.4f' % (k, len(v), statistics.median(v), statistics.mean(v), max(v)))
# 2) entre processus : A/A explicite a chaud (un processus par mode)
for f, a, b in (('developpement_20261006/o1_placement/claudeo1place3/gpu_ab_report_k5_16_cpu.json', 'cpu', 'cpu_aa'),
                ('developpement_20261006/o1_placement/claudeo1place3/gpu_ab_report_k5_24_gpu.json', 'gpu', 'gpu_aa')):
    r = json.load(open(R + f))
    wm = r['warm_medians_ms']
    for fr in ('lidar_ng00', 'lidar_ng01', 'lidar_ng02'):
        x, y = wm['warm|%s|w48|%s' % (fr, a)], wm['warm|%s|w48|%s' % (fr, b)]
        rows = {row['mode']: row for row in r['warm'] if row['frame'] == fr}
        wa = within(rows[a], 'forest_ns'); wb = within(rows[b], 'forest_ns')
        print('A/A chaud %s %s : wall %.3f domain %.3f forest %.3f | sd intra forest %.4f / %.4f' % (
            a, fr, y['wall_ms'] / x['wall_ms'], y['domain_ms'] / x['domain_ms'], y['forest_ms'] / x['forest_ms'], wa[0], wb[0]))
