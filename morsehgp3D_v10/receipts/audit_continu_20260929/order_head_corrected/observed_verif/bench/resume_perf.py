"""Resume du rejeu relatif : medianes par variante et rapports aux medianes de base (appariement par tour aussi)."""
import json
import statistics as st
import sys
from collections import defaultdict

rows = []
for line in open(sys.argv[1]).read().splitlines()[1:]:
    t = line.split('\t')
    if len(t) < 7:
        continue
    try:
        j = json.loads(t[6])
    except Exception:
        continue
    rows.append(dict(tour=int(t[0]), v=t[1], charge=float(t[2]), flt=int(t[3]), rss=int(t[4]), wall=float(t[5]), j=j))
mode = 'cat' if 'catalogue_stages' in rows[0]['j'] else 'cluster'
keys = (['t_order', 't_assemble', 'ordre_assemblage', 't_collect', 't_sort', 't_bands', 't_compare', 't_ranks', 't_copy',
         't_boxes', 'catalogue_s'] if mode == 'cat' else ['catalogue_s', 'tower_s', 'head_s'])
by = defaultdict(list)
for r in rows:
    d = dict(r['j'].get('catalogue_stages', {}))
    d.update({k: r['j'][k] for k in ('catalogue_s', 'tower_s', 'head_s') if k in r['j']})
    if mode == 'cat':
        d['ordre_assemblage'] = d['t_order'] + d['t_assemble']
    d.update(flt=r['flt'], rss=r['rss'], charge=r['charge'], tour=r['tour'])
    by[r['v']].append(d)
vs = list(by)
print('variantes', vs, 'mode', mode, 'n', {v: len(by[v]) for v in vs})
print('%-18s' % 'mesure' + ''.join('%14s' % v for v in vs))
for k in keys + ['flt', 'rss', 'charge']:
    print('%-18s' % k + ''.join('%14.4f' % st.median(x[k] for x in by[v]) for v in vs))
base = vs[0]
for v in vs[1:]:
    for k in (['ordre_assemblage', 't_order', 't_assemble', 't_sort', 't_bands', 't_copy', 'catalogue_s', 'flt', 'rss']
              if mode == 'cat' else ['head_s', 'tower_s', 'catalogue_s']):
        med = st.median(x[k] for x in by[v]) / st.median(x[k] for x in by[base])
        pairs = [a[k] / b[k] for a in by[v] for b in by[base] if a['tour'] == b['tour'] and b[k] > 0]
        q = sorted(pairs)
        print('rapport %s/%s %-16s medianes %.3f ; apparie par tour : mediane %.3f [q1 %.3f, q3 %.3f] n=%d' % (
            v, base, k, med, st.median(pairs), q[len(q) // 4], q[(3 * len(q)) // 4], len(q)))
