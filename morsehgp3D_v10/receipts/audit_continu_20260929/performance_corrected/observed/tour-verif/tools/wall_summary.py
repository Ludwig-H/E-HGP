"""A/B du mur (verificateur) : medianes et quartiles du mur de build_tower, t_resolve+t_kruskal, par variante."""
import json
import statistics as st
import sys

out, name = sys.argv[1], sys.argv[2]
vs = sys.argv[3:]
rows = {v: [json.loads(l) for l in open(f"{out}/{name}.{v}.jsonl") if l.startswith('{')] for v in vs}
print(f"== {name} : passes {({v: len(rows[v]) for v in vs})} ; empreintes {sorted({str(r.get('digest')) for v in vs for r in rows[v]})}")
for w in sorted({r['threads'] for r in rows[vs[0]]}):
    for label, fn in [('mur build_tower', lambda r: r['wall']), ('CPU processus', lambda r: r['cpu']),
                      ('resolve+kruskal', lambda r: r['t_resolve'] + r['t_kruskal']), ('t_local', lambda r: r['t_local']),
                      ('t_points', lambda r: r['t_points'])]:
        cells = []
        base = None
        for v in vs:
            xs = sorted(fn(r) for r in rows[v] if r['threads'] == w)
            m = st.median(xs)
            base = m if base is None else base
            cells.append(f"{v}={m:.4f} [{xs[len(xs)//4]:.4f}-{xs[(3*len(xs))//4]:.4f}] x{m/base:.3f}")
        print(f"  W={w} {label:16s} " + '  '.join(cells))
