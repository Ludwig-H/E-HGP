import json, sys, os
SCALE = os.environ.get('SCALE', 'cpu')
def load(prefix):
    err = open(prefix + '.err').read().splitlines()
    prof = {}; cal = {}; tsc = None
    for line in err:
        if line.startswith('PROFA '):
            d = json.loads(line[6:]); prof[d['K']] = d
        elif line.startswith('PROFA_CAL '):
            cal = json.loads(line[10:])
        elif line.startswith('PROFA_TSC '):
            tsc = json.loads(line[10:])['tsc_per_ns']
    j = json.loads(open(prefix + '.json').read())
    return prof, cal, tsc, j
def main(prefix):
    prof, cal, tsc, j = load(prefix)
    tp = j['tower_phases_ms']
    K = j['options']['K']
    print(f"== {prefix}  status={j['status']} digest={j['tower_digest']} cat={j['catalogue_digest']} balls={j['catalogue']['balls']}")
    print(f"tsc_per_ns={tsc} rdtsc_pair_min={cal.get('rdtsc_pair_min')} mean={cal.get('rdtsc_pair_mean')}")
    print("chain_total", j['times_ms']['chain_total'], "tower", j['times_ms']['tower'], "static", tp['static'], "lots", tp['lots'])
    print("lots_by_k  ", tp['lots_by_k']); print("static_by_k", tp['static_by_k'])
    ms = lambda t: t / tsc / 1e6
    t0 = min(p['start_ns'] for p in prof.values())
    rows = []
    for k in sorted(prof):
        p = prof[k]
        loop = p['cpu_loop_ns'] / 1e6 if SCALE == 'cpu' else ms(p['t_loop'])
        st = p['s_total'] or 1
        sh = lambda x: x / st
        r = dict(K=k, wall=p['wall_ns']/1e6, cpu=p['cpu_ns']/1e6, start=(p['start_ns']-t0)/1e6, end=(p['end_ns']-t0)/1e6,
                 total=ms(p['t_total']), anchors=ms(p['t_anchors']), k1=ms(p['t_k1_setup']), prep=ms(p['t_prepare']),
                 prep_par=ms(p['t_prepare_par']), reserve=ms(p['t_reserve']), loop=loop, cleanup=ms(p['t_cleanup']),
                 live=ms(p['t_live']), wall_loop=p['wall_loop_ns']/1e6, cpu_loop=p['cpu_loop_ns']/1e6,
                 wall_prep=p['wall_prep_ns']/1e6, cpu_prep=p['cpu_prep_ns']/1e6)
        # loop split estimated from sampled lots
        for name, key in [('disc','s_disc'),('blk','s_blk_total'),('blk_facets','s_blk_facets'),('blk_sort','s_blk_sort'),
                          ('lot','s_lot_total'),('s1_node','s1_node'),('s1_draft','s1_draft'),('s1_anchor','s1_anchor'),
                          ('s1_total','s1_total'),('sg_total','sg_total'),('sg_setup','sg_setup'),('sg_union','sg_union'),
                          ('sg_action','sg_action'),('sg_node','sg_node'),('sg_draft','sg_draft'),('sg_anchor','sg_anchor')]:
            r['e_'+name] = sh(p[key]) * loop if p['stride'] else float('nan')
        r['rdtsc_frac'] = p['s_rdtsc'] * cal.get('rdtsc_pair_min', 0) / 2 / st if p['stride'] else 0
        r['ns_per_facet_blk'] = (p['s_blk_facets'] / max(1, p['s_facets'])) / tsc if p['stride'] else float('nan')
        r['p'] = p
        rows.append(r)
    print("\nK | start | end | wall | thr_cpu | tsc_total | anchors | k1_setup | prepare(par) | reserve | loop (wall/cpu) | cleanup | live")
    for r in rows:
        print(f"{r['K']} | {r['start']:.1f} | {r['end']:.1f} | {r['wall']:.1f} | {r['cpu']:.1f} | {r['total']:.1f} | {r['anchors']:.2f} | {r['k1']:.2f} | {r['prep']:.2f} ({r['prep_par']:.2f}) | {r['reserve']:.2f} | {r['loop']:.1f} ({r['wall_loop']:.1f}/{r['cpu_loop']:.1f}) | {r['cleanup']:.2f} | {r['live']:.2f}")
    print(f"\nLoop split (ms, shares of sampled lots x loop {SCALE} time):")
    print("K | disc | blocks | .facets+root | .sort_uniq | order_lot | single(node/draft/anchor) | grouped(setup/union/action/node/draft/anchor) | rdtsc_frac | ns/facet")
    for r in rows:
        print(f"{r['K']} | {r['e_disc']:.1f} | {r['e_blk']:.1f} | {r['e_blk_facets']:.1f} | {r['e_blk_sort']:.1f} | {r['e_lot']:.1f} | {r['e_s1_total']:.1f} ({r['e_s1_node']:.1f}/{r['e_s1_draft']:.1f}/{r['e_s1_anchor']:.1f}) | {r['e_sg_total']:.1f} ({r['e_sg_setup']:.1f}/{r['e_sg_union']:.1f}/{r['e_sg_action']:.1f}/{r['e_sg_node']:.1f}/{r['e_sg_draft']:.1f}/{r['e_sg_anchor']:.1f}) | {r['rdtsc_frac']:.3f} | {r['ns_per_facet_blk']:.1f}")
    print("\nCounts:")
    keys = ['program','lots','singleton_lots','grouped_lots','grouped_blocks','max_lot','facets','root_calls','root_steps','root_zero','roots_raw','roots_unique','nodes','births','merges','links','singleton_nodes','grouped_nodes','contributions','actions','batches','parent_entries','inert','singleton_draft','grouped_draft','owners','dsu_unions','dsu_find_steps','groups','gparents_raw','gparents_unique','balls','domain','anchors_bytes','s_lots','s_blocks','s_singleton','s_grouped','s_discarded','s_discarded_ticks']
    print('key | ' + ' | '.join(f"K{r['K']}" for r in rows))
    for key in keys:
        print(key + ' | ' + ' | '.join(str(r['p'][key]) for r in rows))
    return rows, j
if __name__ == '__main__':
    main(sys.argv[1])
