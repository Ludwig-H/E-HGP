# Modele d'instructions GPU (equivalents instructions entieres 32 bits, sm_120) a partir des compteurs deterministes
# des sondes v12 (recu lidar_scaling_local_20260923, identiques a R7b sur les champs communs).
import json, glob, sys
W = {  # instructions par operation elementaire (hypotheses explicites, voir la note D6)
 'node': 60,     # visite de noeud d'index : boite 24 o + enfants, borne entiere i64/i128, pile
 'bound': 40,    # borne de bloc / de noeud sans descente
 'uniform': 30,  # test uniforme du certificat de voie morte : deux formes affines i64 aux coins
 'point': 15,    # test ponctuel d'une forme affine
 'copy': 10,     # site/forme/ID ecrit ou charge dans un tampon
 'sweep_site': 15, 'sweep_event': 150, 'ball_build': 300,
}
def q34_ops(L):
    node = sum(L[k] for k in ['witness_pair_node_visits','witness_rect_node_visits','witness_cache_node_tests','core_cover_node_visits',
               'cover_node_visits','atlas_node_visits','q3_seed_node_visits','q4_cover_decomposition_node_visits','q4_domain_node_visits','q4_seed_node_visits'])
    bound = sum(L[k] for k in ['core_cover_bound_tests','atlas_block_bounds','q3_seed_bound_tests','q3_census_bounds'])
    uni = L['dead_uniform_tests'] + L['dead_core_uniform_tests']
    pt = sum(L[k] for k in ['core_cover_point_tests','dead_point_tests','dead_core_point_tests','atlas_point_tests','q3_seed_point_tests','q3_census_point_tests','q3_leaf_point_tests'])
    cp = sum(L[k] for k in ['core_sites','cover_sites','dead_form_sites','dead_core_form_sites','atlas_ids_copied'])
    sw_s, sw_e = L['q4_sweep_active_sites'], L['q4_sweep_events']
    bb = L['q3_ball_builds']
    ops = dict(node=node, bound=bound, uniform=uni, point=pt, copy=cp, sweep_site=sw_s, sweep_event=sw_e, ball_build=bb)
    return ops
for f in sorted(glob.glob('s0*_k*_w8_r0/s0*_piece_full.json')):
    d = json.load(open(f)); p = d.get('probe', d)
    L = p['ledger']; T = p['times_ms']; tw = p['tower_work']; cat = p['catalogue']
    ops = q34_ops(L)
    instr = sum(ops[k]*W[k] for k in ops)
    elem = sum(ops.values())
    census = cat['census_nodes']*W['node'] + cat['census_leaf_tests']*W['point']
    full_static = tw['intruder_nodes']*W['node'] + tw['meb_pair_distances']*15 + tw['meb_power_tests']*30 + tw['key_lookups']*100
    print(f.split('/')[-1], 'n=%d'%p['input']['sites'], 'K=%d'%p['options']['K'])
    print('   q34 ops elementaires = %.2f G ; instructions = %.0f G  (%s)' % (elem/1e9, instr/1e9, ', '.join('%s %.2fG'%(k,v/1e9) for k,v in ops.items())))
    print('   census %.1f G instr ; FULL statique %.1f G instr ; boules %d ; reps %d' % (census/1e9, full_static/1e9, cat['balls'], tw['representatives']))
    for name, thr in (('pessimiste 0.4 T/s',0.4e12),('central 1.5 T/s',1.5e12),('optimiste 4 T/s',4e12)):
        print('   %-20s q34 %.3f s  census %.3f s  FULL-statique %.3f s' % (name, instr/thr, census/thr, full_static/thr))
