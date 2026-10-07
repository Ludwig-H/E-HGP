import json, sys, statistics as st
f = sys.argv[1]
d = json.load(open(f))
print('modes', d.get('modes'), 'kmax', d.get('kmax'), 'leaf', d.get('leaf'))
rows = {}
for regime in ('cold', 'warm'):
    for c in d.get(regime, []):
        s = c.get('summary') or {}
        p = s.get('pipeline')
        if not p:
            continue
        key = (regime, c['frame'], c['mode'])
        ph = p['phases']; ln = p['lanes']
        K = len(p['orders'])
        oK = p['orders'][-1]
        rows.setdefault(key, []).append((s.get('forest_ms'), ph['classify_ns'], ph['births_ns'], ph['regular_ns'], ph['publish_ns'], ph['verticals_ns'], ln['lanes_cpu_ns'], oK['publish_end_ms'], oK['publish_cpu_ms'], oK['vertical_end_ms'], oK['vertical_cpu_ms'], s.get('pipeline_placement_cores'), oK['publish_cells'], oK['publish_closes'], oK.get('publish_cells_est_ms'), oK.get('publish_closes_est_ms'), s.get('domain_ms'), s.get('wall_ms')))
for key in sorted(rows):
    v = rows[key]
    def med(i):
        xs = [x[i] for x in v if x[i] is not None]
        return round(st.median(xs), 1) if xs else None
    print(key, 'n=%d' % len(v), 'forest', med(0), 'classify', med(1), 'births', med(2), 'R', med(3), 'pub_tail', med(4), 'vert_tail', med(5), 'lanesCPU_ms', med(6), 'PK_end', med(7), 'PK_cpu', med(8), 'VK_end', med(9), 'VK_cpu', med(10), 'place', med(11), 'cellsK', med(12), 'closesK', med(13), 'cells_est', med(14), 'closes_est', med(15), 'domain', med(16), 'wall', med(17))
