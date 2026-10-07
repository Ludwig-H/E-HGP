#!/usr/bin/env python3
"""Assemble le rapport L02 : substitue les nombres mesures dans les parties redigees."""
import glob
import json
import os
import re
import subprocess
import sys

W = '/tmp/v11-audit/l02_math_tour'
OUT = sys.argv[1]


def fr(n):
    s = '%d' % abs(n)
    g = []
    while s:
        g.append(s[-3:])
        s = s[:-3]
    return ('−' if n < 0 else '') + ' '.join(reversed(g))


def final_json(path):
    for line in open(path):
        if line.startswith('{"arity'):
            return json.loads(line)
    raise SystemExit('pas de bilan dans ' + path)


camp = {s: final_json('%s/camp/%s' % (W, f)) for s, f in ((11, 'ref_seed11.log'), (21, 'big_seed21.log'), (22, 'big_seed22.log'))}
for s, d in camp.items():
    if d['fails'] or d['refus']:
        raise SystemExit('campagne %d : ecarts' % s)
tot = {k: sum(d[k] for d in camp.values()) for k in ('clouds', 'births', 'merges', 'arity_ge3', 'verticals', 'points', 'euler',
                                                    'cover_growth_no_merge', 'births_pop_gt_k', 'levels_with_2plus_events')}
cov = json.loads(open(W + '/camp/coverage.json').read())
if cov['merges'] != tot['merges'] or cov['arity_ge3'] != tot['arity_ge3'] or cov['clouds'] != tot['clouds']:
    raise SystemExit('couverture incoherente avec les campagnes')

big_rows = []
for s in (21, 22):
    d = camp[s]
    big_rows.append('| graine %d : $n$ de 10 à 12, $K = 8$ (80 %%) ou 10 | %s | %s ordres jugés | %s | %s (%s) | %s | %s | %s | 0 |' % (
        s, fr(d['clouds']), fr(d['orders']), fr(d['births']), fr(d['merges']), fr(d['arity_ge3']), fr(d['verticals']),
        fr(d['points']), fr(d['euler'])))

# fixtures
fx = dict(n=0, births=0, merges=0, ar3=0, vert=0, pts=0, euler=0, bad=0)
for line in open(W + '/fixtures.log'):
    m = re.search(r': (CONFORME|ECART.*?) \| naissances (\d+) \(pop > k : \d+\) fusions (\d+) \(>=3 parents : (\d+)\) verticales (\d+) attaches (\d+) Euler (\d+)', line)
    if m:
        fx['n'] += 1
        fx['bad'] += m.group(1) != 'CONFORME'
        for key, g in (('births', 2), ('merges', 3), ('ar3', 4), ('vert', 5), ('pts', 6), ('euler', 7)):
            fx[key] += int(m.group(g))
if fx['n'] != 15 or fx['bad']:
    raise SystemExit('fixtures : %r' % fx)
fix_row = ('| 15 fixtures gravées (E5, carré, rectangle et centre, cube, cube et centre, octaèdre, octaèdre et centre, cinq points de '
           '`BALL_ANCHORS`, gain de couverture, cercle de 12 points, cercle et centre, triangle rectangle, cinq alignés, '
           'contre-exemple du Th. 5, coins u18), tous les ordres $\\leq \\min(n, 10)$ | 15 | — | %s | %s (%s) | %s | %s | %s | 0 |' % (
               fr(fx['births']), fr(fx['merges']), fr(fx['ar3']), fr(fx['vert']), fr(fx['pts']), fr(fx['euler'])))

hist = cov['ext_m_hist']
coverage = ('%s boules de catalogue, dont %s à coquille étendue ($m = 3$ : %s ; 4 : %s ; 5 : %s ; 6 : %s ; 7 : %s ; 8 : %s ; 9 : %s ; '
            '10 : %s ; 11 : %s ; 12 : %s) ; %s naissances de population supérieure à $k$ ; arité maximale %d ; %s rangs portant '
            'au moins deux fusions ; ordres maximaux : $K = 5$ pour %s nuages, 7 pour %s, 8 pour %s, 10 pour %s' % (
                fr(cov['balls']), fr(cov['ext_balls']), *[fr(hist.get(str(m), 0)) for m in range(3, 13)],
                fr(cov['births_pop_gt_k']), cov['arity_max'], fr(cov['ranks_with_2plus_merges']),
                fr(cov['K_hist'].get('5', 0)), fr(cov['K_hist'].get('7', 0)), fr(cov['K_hist'].get('8', 0)), fr(cov['K_hist'].get('10', 0))))

# echelle
scale_rows = []
names = dict(uniform='uniforme', clusters='amas', shells='coquilles', filaments='filaments', terrain='terrain')
for fam in ('uniform', 'clusters', 'shells', 'filaments', 'terrain'):
    for x in (1, 2, 4):
        d = json.loads(open('%s/scale/syn_%s_space_x%d_k5_kcat7.json' % (W, fam, x)).read())
        e = json.loads(open('%s/scale/syn_%s_space_x%d_k1_emst.json' % (W, fam, x)).read())
        chi = [v['chi'] for v in d['euler'] if v['in_range']]
        ok = len(chi) == 5 and all(c == 1 for c in chi)
        if not ok or not e['equal_multisets'] or not e['equal_nary_trees'] or 'orders' not in d:
            raise SystemExit('echelle %s x%d' % (fam, x))
        o = d['orders']
        scale_rows.append('| %s | %s | %s | oui | oui, oui | %s (%s) | %s |' % (
            names[fam], fr(d['sites']), fr(d['balls']), fr(sum(v['merges'] for v in o)), fr(sum(v['arity_ge3'] for v in o)),
            fr(sum(v['knn_jumps_join'] for v in o))))

# reference Python
tr = open(W + '/test_ref_head.log').read()
m = re.search(r'Ran (\d+) tests in ([0-9.]+)s\s+(OK|FAILED.*)', tr)
if m:
    testref = '%s tests, %s' % (m.group(1), 'OK' if m.group(3) == 'OK' else m.group(3))
else:
    done = len(re.findall(r'\.\.\. ok', tr))
    testref = '%d tests sur 4 passés au moment de la rédaction, les autres en cours' % done

heure = subprocess.run(['date', '-u', '+%H:%M'], capture_output=True, text=True).stdout.strip()
subs = {
    '@@HEURE@@': heure,
    '@@TOT_CLOUDS@@': fr(tot['clouds']),
    '@@TOT_BIRTHS@@': fr(tot['births'] + fx['births']),
    '@@TOT_MERGES@@': fr(tot['merges'] + fx['merges']),
    '@@TOT_AR3@@': fr(tot['arity_ge3'] + fx['ar3']),
    '@@TOT_VERT@@': fr(tot['verticals'] + fx['vert']),
    '@@TOT_PTS@@': fr(tot['points'] + fx['pts']),
    '@@TOT_EULER@@': fr(tot['euler'] + fx['euler']),
    '@@GROWTH@@': fr(tot['cover_growth_no_merge']),
    '@@BIG_ROWS@@': '\n'.join(big_rows),
    '@@FIX_ROW@@': fix_row,
    '@@COVERAGE@@': coverage,
    '@@JUMPS@@': fr(cov['knn_jumps_join']),
    '@@RESOLVES@@': fr(cov['resolves_join']),
    '@@EXT_BALLS@@': fr(cov['ext_balls']),
    '@@SCALE_ROWS@@': '\n'.join(scale_rows),
    '@@TESTREF@@': testref,
}
text = ''.join(open('%s/report/part%d.md' % (W, i)).read() for i in range(1, 6))
for k, v in subs.items():
    if k not in text:
        raise SystemExit('jeton absent : ' + k)
    text = text.replace(k, v)
left = re.findall(r'@@[A-Z0-9_]+@@', text)
if left:
    raise SystemExit('jetons restants : %r' % left)
open(OUT, 'w').write(text)
print('ecrit', OUT, len(text), 'octets ;', text.count('\n'), 'lignes ; heure', heure, '; totaux', tot, '; fixtures', fx, '; test_ref :', testref)
