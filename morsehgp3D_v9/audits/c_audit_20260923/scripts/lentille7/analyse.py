#!/usr/bin/env python3
"""Lentille 7 : agregats a partir de rows.json (produit par table_g4.py depuis les bruts G4)."""
import json
import statistics as st
import sys
from pathlib import Path

rows = json.loads(Path(sys.argv[1]).read_text())
SESS = ['R1', 'R2', 'R3', 'R4b', 'R5', 'R6', 'R7b']
FR = ['000100', '000000', '000200']


def is_default(r):
    lv = r['levers']
    s = r['session']
    if s == 'R1':
        return r['static'] == 0 and r['W'] == 48
    if s == 'R3':
        return lv.get('q34_dead_lanes', True)
    if s == 'R4b':
        return lv.get('q34_witness_cache', True)
    if s == 'R6':
        return lv.get('q34_dead_core', True)
    if s == 'R7b':
        return lv.get('tower_meb_proposal', True)
    return True


def sel(s, fr, K, W=48, default=True):
    return [r for r in rows if r['session'] == s and r['frame'] == fr and r['K'] == K and r['W'] == W and
            (is_default(r) if default else True)]


print('== 1. Chronologie : meilleur cas W48 configuration par defaut, perimetre ANCIEN (digest inclus)')
print('   ses   pkg      | ' + ' | '.join('%s K%d' % (f, k) for k in (5, 10) for f in FR))
first = {}
for s in SESS:
    cells = []
    for k in (5, 10):
        for f in FR:
            c = sel(s, f, k)
            if not c:
                cells.append('   -   ')
                continue
            b = min(c, key=lambda r: r['chain_old'])
            first.setdefault((f, k), b['chain_old'])
            cells.append('%7.2f' % b['chain_old'])
    print('   %-5s %s | %s' % (s, c[0]['commit'] if c else '', ' | '.join(cells)))
print('   facteur R1 -> R7b (perimetre ancien) :')
for k in (5, 10):
    for f in FR:
        b = min(sel('R7b', f, k), key=lambda r: r['chain_old'])
        print('     %s K%d : %.2f -> %.2f  x%.1f' % (f, k, first[(f, k)], b['chain_old'], first[(f, k)] / b['chain_old']))

print('\n== 2. Ventilation R7b (moyenne des 2 repetitions MEB ON), secondes et parts de chain_total')
for k in (5, 10):
    for f in FR:
        c = sel('R7b', f, k)
        m = {key: st.mean(r[key] for r in c) for key in ('q2', 'q34', 'merge', 'census', 'tower', 'chain_total',
                                                          'digest_s', 'wall_ext', 'chain_cpu', 'prepare',
                                                          'gen_index', 'tower_index', 'residual', 'read')}
        other = m['prepare'] + m['gen_index'] + m['tower_index'] + m['residual']
        ct = m['chain_total']
        teardown = m['wall_ext'] - (m['read'] + m['chain_total'] + m['digest_s'])
        print('   %s K%-2d chain %.3f : q2 %.3f (%.1f%%) q34 %.3f (%.1f%%) merge %.3f (%.1f%%) census %.3f (%.1f%%) '
              'tower %.3f (%.1f%%) autres %.3f (%.1f%%) | digest %.3f (+%.1f%%) | mur ext %.3f, hors chronos %.3f s'
              % (f, k, ct, m['q2'], 100 * m['q2'] / ct, m['q34'], 100 * m['q34'] / ct, m['merge'], 100 * m['merge'] / ct,
                 m['census'], 100 * m['census'] / ct, m['tower'], 100 * m['tower'] / ct, other, 100 * other / ct,
                 m['digest_s'], 100 * m['digest_s'] / ct, m['wall_ext'], teardown))

print('\n== 3. Ecart au contrat, R7b (4 essais par trame et K : 2 rep x MEB ON/OFF)')
for k in (5, 10):
    for f in FR:
        c = sel('R7b', f, k, default=False)
        on = [r for r in c if is_default(r)]
        mn = min(r['chain_total'] for r in c)
        mon = st.mean(r['chain_total'] for r in on)
        mx = max(r['chain_total'] for r in c)
        cpu_on = st.mean(r['chain_cpu'] for r in on)
        par = st.mean(r['chain_cpu'] / r['chain_total'] for r in on)
        print('   %s K%-2d chain min %.3f / moy ON %.3f / max %.3f | x%.1f (1 s)  x%.0f (100 ms) sur min ; '
              'CPU ON %.1f s -> CPU/48 = %.2f s (x%.1f de 1 s, x%.0f de 100 ms) ; CPU/mur = %.1f fils occupes'
              % (f, k, mn, mon, mx, mn, mn / 0.1, cpu_on, cpu_on / 48, cpu_on / 48, cpu_on / 48 / 0.1, par))

print('\n== 4. Plancher CPU/48 par session (meilleur CPU W48 defaut, perimetre de la session)')
for s in SESS:
    cells = []
    for k in (5, 10):
        for f in FR:
            c = sel(s, f, k)
            if not c:
                cells.append('  -  ')
                continue
            cells.append('%5.2f' % (min(r['chain_cpu'] for r in c) / 48))
    print('   %-5s %s' % (s, ' | '.join(cells)))

print('\n== 5. W24 -> W48 (000000, memes objets)')
for s in SESS:
    w24 = [r for r in rows if r['session'] == s and r['W'] == 24]
    for a in w24:
        b = [r for r in rows if r['session'] == s and r['W'] == 48 and r['frame'] == a['frame'] and r['K'] == a['K'] and
             is_default(r) and (r['static'] == a['static'] or (a['static'] == 24 and r['static'] == 48))]
        if s == 'R1':
            b = [r for r in b if r['static'] == 0]
        if not b:
            continue
        m48 = {key: st.mean(r[key] for r in b) for key in ('chain_total', 'q34', 'tower', 'chain_cpu', 'q2', 'census')}
        print('   %s %s K%d st%d/%d n48=%d : chain %.2f -> %.2f (x%.3f) ; q34 %.2f -> %.2f (x%.3f) ; tower %.2f -> %.2f '
              '(x%.3f) ; q2 x%.2f census x%.2f ; CPU %.0f -> %.0f (+%.0f%%)'
              % (s, a['frame'], a['K'], a['static'], b[0]['static'], len(b), a['chain_total'], m48['chain_total'],
                 a['chain_total'] / m48['chain_total'], a['q34'], m48['q34'], a['q34'] / m48['q34'], a['tower'],
                 m48['tower'], a['tower'] / m48['tower'], a['q2'] / m48['q2'], a['census'] / m48['census'],
                 a['chain_cpu'], m48['chain_cpu'], 100 * (m48['chain_cpu'] / a['chain_cpu'] - 1)))

print('\n== 6. Tailles (R7b ON rep 0) : catalogue, sortie, par site ; octets de sortie (formule A : 80 noeuds + 8 parents + 80 contrib)')
for k in (5, 10):
    for f in FR:
        r = [x for x in sel('R7b', f, k) if x['repeat'] == 0][0]
        n = r['n']
        out_bytes = 80 * r['nodes'] + 8 * r['parents'] + 80 * r['contributions']
        print('   %s K%-2d n=%d : boules %d (%.2f/site, %.3f Go cat.) ; presentations %d (doublons %d) ; noeuds %d '
              '(%.2f/site) ; ordre K seul %d (%.2f/site) ; contrib %d (%.2f/site) ; sortie >= %.1f Mio ; '
              'RSS %.2f Gio ; paires dev. %d (%.1f/site)'
              % (f, k, n, r['balls'], r['balls'] / n, r['cat_bytes'] / 1e9, r['presentations'],
                 r['presentations'] - r['balls'], r['nodes'], r['nodes'] / n, r['nodes_topK'], r['nodes_topK'] / n,
                 r['contributions'], r['contributions'] / n, out_bytes / 2**20, r['gt_rss'] / 2**20,
                 r['expanded_pairs'], r['expanded_pairs'] / n))

print('\n== 7. Debits requis a 1 s et 100 ms (R7b ON rep 0, K10) : sortie et catalogue par seconde')
for f in FR:
    r = [x for x in sel('R7b', f, 10) if x['repeat'] == 0][0]
    out_bytes = 80 * r['nodes'] + 8 * r['parents'] + 80 * r['contributions']
    print('   %s : noeuds/s requis %.1f M (1 s) / %.0f M (100 ms) ; octets sortie+catalogue %.2f Go -> %.1f Go/s a 100 ms'
          % (f, r['nodes'] / 1e6, r['nodes'] / 1e5, (out_bytes + r['cat_bytes']) / 1e9,
             (out_bytes + r['cat_bytes']) / 1e8))

print('\n== 8. Defauts de page et temps systeme (R7b)')
for k in (5, 10):
    for f in FR:
        c = sel('R7b', f, k)
        print('   %s K%-2d : defauts mineurs %s ; sys %.2f s / user %.1f s (%.1f%%)' % (
            f, k, [r['minflt'] for r in c], st.mean(r['gt_sys'] for r in c), st.mean(r['gt_user'] for r in c),
            100 * st.mean(r['gt_sys'] / (r['gt_user'] + r['gt_sys']) for r in c)))

print('\n== 9. Dispersion entre repetitions (config. par defaut, W48) : ecart relatif max-min / moyenne')
for s in SESS:
    out = []
    for k in (5, 10):
        for f in FR:
            c = sel(s, f, k)
            if len(c) >= 2:
                v = [r['chain_total'] for r in c]
                out.append('%s K%d %.1f%%' % (f, k, 100 * (max(v) - min(v)) / st.mean(v)))
    if out:
        print('   %-5s %s' % (s, ' ; '.join(out)))

print('\n== 10. Residu hors phases nommees (chain_total - somme des 8 sous-chronos), config. par defaut W48')
for s in SESS:
    out = []
    for k in (5, 10):
        vals = [r['residual'] for r in rows if r['session'] == s and r['K'] == k and r['W'] == 48 and is_default(r)]
        if vals:
            out.append('K%d %.3f-%.3f' % (k, min(vals), max(vals)))
    print('   %-5s %s (digest %s)' % (s, ' ; '.join(out), 'dedans' if s != 'R7b' else 'separe'))

print('\n== 11. Par trame et K : meilleur cas par session (defaut, W48, perimetre ancien), phases et occupation')
for f in FR:
    for k in (5, 10):
        print('   %s K%d : ses | chain_old | q2 | q34 (%%) | tower (%%) | merge | census | autres+digest | CPU·s | CPU/48 | fils occ. | RSS GiB' % (f, k))
        for s in SESS:
            c = sel(s, f, k)
            b = min(c, key=lambda r: r['chain_old'])
            oth = b['chain_old'] - (b['q2'] + b['q34'] + b['tower'] + b['merge'] + b['census'])
            print('     %-4s | %7.2f | %5.2f | %6.2f (%4.1f) | %6.2f (%4.1f) | %5.2f | %5.2f | %5.2f | %7.1f | %6.2f | %5.1f | %5.2f' % (
                s, b['chain_old'], b['q2'], b['q34'], 100 * b['q34'] / b['chain_old'], b['tower'],
                100 * b['tower'] / b['chain_old'], b['merge'], b['census'], oth, b['chain_cpu'], b['chain_cpu'] / 48,
                b['chain_cpu'] / b['chain_total'], b['gt_rss'] / 2**20))

print('\n== 12. Temps systeme et commutations volontaires : R3 apparie voies mortes ON/OFF')
for r in rows:
    if r['session'] == 'R3' and r['W'] == 48:
        print('   %s K%-2d dead_lanes=%s rep%d : sys %.2f s, user %.1f s, defauts mineurs %d' % (
            r['frame'], r['K'], r['levers'].get('q34_dead_lanes'), r['repeat'], r['gt_sys'], r['gt_user'], r['minflt']))
