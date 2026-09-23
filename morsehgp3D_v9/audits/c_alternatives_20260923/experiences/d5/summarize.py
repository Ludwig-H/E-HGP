import json,sys
d=json.load(open(sys.argv[1]))
p=d.get('product',{})
print('n',d['n'],'K',d['kmax'],'balls',d['balls'],'tower_ms_1thread',p.get('tower_ms_1thread'),'prog_ms',d['programs_ms'],'plateaus',d['plateaus'])
if p:
  b=p['bytes']; tot=sum(v for k,v in b.items() if not k.startswith('sizeof') and k!='population_rows')
  print(' product reps',p['representatives'],'meb',p['meb_calls'],'hits',p['anchor_hits'],'iq',p['intruder_queries'],'inodes',p['intruder_nodes'],'births',p['births'],'merges',p['merges'],'single',p['singleton_lots'],'grouped',p['grouped_lots'])
  print(' explicit bytes (sans surcout malloc)',tot, {k:v for k,v in b.items()})
T=dict(facets=0,unique=0,seed0=0,hit0=0,hit0i=0,chains=0,meb=0,iq=0,inodes=0,cat=0,tseed=0,thit=0,thiti=0,res=0,lean=0,blocks=0,nodes=0)
hist=[0]*8
print('K blocks facets unique seed0 hit0(idx) chains meb iq inodes/iq catIntr thit(idx) tseed maxd res_ms lean_ms lean_ns/block nodes merges singleton grouped')
for o in d['orders']:
  l=o['lean']
  print(o['K'],o['blocks'],o['facets'],o['unique'],o['seed0'],'%d(%d)'%(o['hit0'],o['hit0_in_index']),o['chains'],o['meb_calls'],o['intruder_queries'],
        round(o['intruder_nodes']/max(1,o['intruder_queries']),1),o['catalogued_intruder_steps'],'%d(%d)'%(o['terminal_hit'],o['terminal_hit_in_index']),o['terminal_seed_after_step'],o['max_depth'],
        o['resolve_ms'],l['ms'],round(1e6*l['ms']/max(1,o['blocks']),0),l['nodes'],l['merges'],l['singleton_lots'],l['grouped_lots'])
  for i in range(8): hist[i]+=o['depth_hist'][i]
  T['facets']+=o['facets'];T['unique']+=o['unique'];T['seed0']+=o['seed0'];T['hit0']+=o['hit0'];T['hit0i']+=o['hit0_in_index'];T['chains']+=o['chains']
  T['meb']+=o['meb_calls'];T['iq']+=o['intruder_queries'];T['inodes']+=o['intruder_nodes'];T['cat']+=o['catalogued_intruder_steps'];T['tseed']+=o['terminal_seed_after_step']
  T['thit']+=o['terminal_hit'];T['thiti']+=o['terminal_hit_in_index'];T['res']+=o['resolve_ms'];T['lean']+=l['ms'];T['blocks']+=o['blocks'];T['nodes']+=l['nodes']
print('TOTAL',T)
print('depth hist (unique facets, K>=2)',hist, 'share depth0 %.3f'%(hist[0]/max(1,sum(hist))))
u=T['unique']
if u:
  print('unique/facets %.3f ; seed0 %.3f ; hit0 %.3f ; chains %.3f'%(u/T['facets'] if T['facets'] else 0, T['seed0']/u, T['hit0']/u, T['chains']/u))
  print('MEB saved by hit index (terminal hits in index) %d of %d = %.3f'%(T['thiti'],T['meb'],T['thiti']/max(1,T['meb'])))
  print('intruder searches saved by catalogued interior %d of %d = %.3f'%(T['cat'],T['iq'],T['cat']/max(1,T['iq'])))
print('compact_bytes',d['compact_bytes'],'total_nodes',d['total_nodes'],'lean_ms_total',d['lean_ms_total'],'resolve_ms_total',d['resolve_ms_total'])
