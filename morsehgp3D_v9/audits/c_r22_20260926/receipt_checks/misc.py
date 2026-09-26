import json
R="/workspaces/E-HGP/build/v9-audit-c-publish/morsehgp3D_v9/receipts/g4_tower_r22_20260926"
V=[json.loads(open(f"{R}/vm/probe_{i}.stdout").read()) for i in range(36)]
C=[json.load(open(f"{R}/vm/probe_{i}.command.json")) for i in range(36)]
eu=set()
for i,v in enumerate(V):
    e=v['catalogue']['euler']; K=v['options']['K']
    ok = e['checkable_max_k']==K-2 and all(x==1 for x in e['by_k'][:e['checkable_max_k']]) and len(e['by_k'])==K
    eu.add((K,e['checkable_max_k'],ok,e['status']))
print('euler (K, checkable_max_k, first checkable all ==1, status):', eu)
m=[]
for i,v in enumerate(V):
    t=v['times_ms']; s=v['device_session']
    inner=(t['read']+t['chain_total']+t['digest']+t['catalogue_digest']+s.get('context_ms',0)+s.get('reserve_ms',0)+s.get('pinned_ms',0))/1000
    m.append((round(C[i]['elapsed_seconds']-inner,3),i, round(C[i]['elapsed_seconds'],3)))
print('external wall margin min/max (s):', min(m), max(m))
print('negative margins:', [x for x in m if x[0]<0])
print('K_effective == K:', all(v['options']['K_effective']==v['options']['K'] for v in V))
print('grid:', {v['input']['grid'] for v in V})
print('watchdog/group_closed:', {(c.get('per_command_watchdog'), c.get('group_closed')) for c in C})
# chronological order
order=sorted(range(36), key=lambda i:C[i]['started_utc'])
print('chronological order:', order)
print('first start', C[order[0]]['started_utc'], 'last end', max(c['ended_utc'] for c in C))
# overlap check: any two cases overlapping in time?
iv=sorted((C[i]['started_utc'],C[i]['ended_utc'],i) for i in range(36))
ov=[(a[2],b[2]) for a,b in zip(iv,iv[1:]) if b[0]<a[1]]
print('overlapping consecutive cases:', ov)
v=V[8]; t=v['times_ms']; qb=v['q34_batch']
print('case8 chain',t['chain_total'],'tower',t['tower'],'q34',t['q34'],'front',qb['front_ms'],'filter',qb['filter_ms'],'cert',qb['certificate_ms'],'lanes',qb['lanes_ms'],'census',t['census'],'merge',t['merge'],'q2',t['q2'],'gen_index',t['gen_index'],'read',t['read'],'prepare',t['prepare'],'tower_index',t['tower_index'])
print(' sum tower+q34+census+merge+gen_index+prepare', t['tower']+t['q34']+t['census']+t['merge']+t['gen_index']+t['prepare'])
v=V[0]; tp=v['tower_phases_ms']
print('case0 tower', v['times_ms']['tower'], 'validate', tp['validate'], 'static5', tp['static_by_k'][4], 'lots5', tp['lots_by_k'][4], 'tail pop+img+bank+enc', tp['populations']+tp['images']+tp['bank']+tp['encode'], 'img5+enc5+pop5', tp['images_by_k'][4]+tp['encode_by_k'][4]+v['tower_detail']['populations_by_k'][4])
print(' residual', v['times_ms']['tower']-tp['validate']-tp['static_by_k'][4]-tp['lots_by_k'][4]-(tp['populations']+tp['images']+tp['bank']+tp['encode']))
for i in (13,):
    v=V[i]; tp=v['tower_phases_ms']; print(i,'tower',v['times_ms']['tower'],'validate',tp['validate'],'static5',tp['static_by_k'][4],'lots5',tp['lots_by_k'][4],'tail',tp['populations']+tp['images']+tp['bank']+tp['encode'])
