"""Resume du rejeu : medianes du CPU du fil de Kruskal (kr_cpu, plus grand ordre), du mur et du CPU processus."""
import json
import statistics as st
import sys

out, name = sys.argv[1], sys.argv[2]
vs = sys.argv[3:]
rows = {v: [json.loads(l) for l in open(f"{out}/{name}.{v}.jsonl") if l.startswith('{')] for v in vs}
digs = {r.get('digest') for v in vs for r in rows[v]}
print(f"== {name} : passes {({v: len(rows[v]) for v in vs})} ; empreintes {sorted(map(str, digs))}")
for w in sorted({r['threads'] for r in rows[vs[0]]}):
    print(f"  W={w}")
    for label, fn in [('CPU fil Kruskal', lambda r: max(r['orders'], key=lambda o: o['nodes'])['kr_cpu']),
                      ('mur build_tower', lambda r: r['wall']), ('CPU processus', lambda r: r['cpu']),
                      ('mur etage kruskal', lambda r: r['t_kruskal']),
                      ('mur local_info_resize', lambda r: r['sub'].get('local_info_resize', 0)),
                      ('mur points_need+assign', lambda r: r['sub'].get('points_need', 0) + r['sub'].get('points_assign', 0))]:
        meds, qs = [], []
        for v in vs:
            xs = sorted(fn(r) for r in rows[v] if r['threads'] == w)
            meds.append(st.median(xs))
            qs.append((xs[len(xs) // 4], xs[(3 * len(xs)) // 4], len(xs)))
        ratio = meds[1] / meds[0] if meds[0] else float('nan')
        print(f"    {label:24s} " + '  '.join(f"{v}={m:.4f} [q1 {q[0]:.4f} q3 {q[1]:.4f} n={q[2]}]" for v, m, q in zip(vs, meds, qs)) + f"  ratio={ratio:.3f}")
