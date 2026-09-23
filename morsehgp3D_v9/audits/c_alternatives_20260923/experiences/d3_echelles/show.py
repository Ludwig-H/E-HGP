import json,sys,glob
B=['<=50','<=100','<=200','<=400','<=800','<=1600','<=3200','<=6400','<=12800','>12800']
for f in sys.argv[1:]:
    d=json.load(open(f))
    print('==',f, 'n',d['n'],'K',d['kmax'],'balls',d['balls'],'times',d['times_ms'],'cpu',d['cpu_s'])
    print('   r>h (h mm',d['h_mm'],')',d['balls_r_above_h'], ' q4',d['q4_r_above_h'])
    tot={k:sum(d[k]) for k in ('edges','cpu_ns','core_sites','cover_sites','atlas_pt','q3_census_pt','q3_leaf_pt','q4_sweep_active','witness_node_visits','dead_pt','q3_emitted','q4_emitted','q3_seeds','q4_seeds','cover_builds','rect_cpu_ns','rect_mass','rect_count')}
    print('   totals',tot, 'check core',d['core_sites_total'],'cover',d['cover_sites_total'],'atlas',d['atlas_pt_total'],'exp',d['expanded_pairs_total'],'min |ab|/r',d['min_ab_over_r'])
    keys=['edges','witness_rejected','cover_builds','core_sites','cover_sites','atlas_pt','q3_census_pt','q4_sweep_active','dead_pt','q3_seeds','q4_seeds','q3_emitted','q4_emitted','cpu_ns']
    print('   %-10s'%'|ab| mm'+''.join('%12s'%k[:11] for k in keys))
    for j in range(10):
        if d['edges'][j]==0 and d['rect_count'][j]==0: continue
        print('   %-10s'%B[j]+''.join('%12d'%(d[k][j] if k!='cpu_ns' else d[k][j]//1000000) for k in keys))
    # cumulative share above thresholds
    print('   share of q34 edge work with |ab| > X mm:')
    for j,X in enumerate([50,100,200,400,800,1600,3200,6400]):
        sh=lambda k: sum(d[k][j+1:])/max(1,sum(d[k]))
        print('     >%5d: edges %.3f cpu %.3f core %.3f cover %.3f atlas %.3f q3cens %.3f sweep %.3f emitted q3 %.3f q4 %.3f'%(X,sh('edges'),sh('cpu_ns'),sh('core_sites'),sh('cover_sites'),sh('atlas_pt'),sh('q3_census_pt'),sh('q4_sweep_active'),sh('q3_emitted'),sh('q4_emitted')))
    print('   rect level (by box gap):', 'count',d['rect_count'],'cpu ms',[x//1000000 for x in d['rect_cpu_ns']],'mass',d['rect_mass'],'rejmass',d['rect_rejected_mass'])
    print('   emit hist rows=anchor bucket cols=radius bucket')
    for j in range(10):
        if sum(d['emit_hist'][j]): print('     ',B[j],d['emit_hist'][j])
