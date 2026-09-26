import json
R="/workspaces/E-HGP/build/v9-audit-c-publish/morsehgp3D_v9/receipts/g4_tower_r21_20260925"
V=[json.loads(open(f"{R}/vm/probe_{i}.stdout").read()) for i in range(34)]
C=[json.load(open(f"{R}/vm/probe_{i}.command.json")) for i in range(34)]
print('schemas', {v['schema'] for v in V}, 'status', {v['status'] for v in V}, 'reason', {v['reason'] for v in V})
eu=set()
for i,v in enumerate(V):
    e=v['catalogue']['euler']; K=v['options']['K']
    ok = e['checkable_max_k']==K-2 and all(x==1 for x in e['by_k'][:e['checkable_max_k']]) and len(e['by_k'])==K
    eu.add((K,e['checkable_max_k'],ok))
print('euler (K, checkable_max_k, first checkable all ==1):', eu)
m=[]
for i,v in enumerate(V):
    t=v['times_ms']; s=v['device_session']
    inner=(t['read']+t['chain_total']+t['digest']+t['catalogue_digest']+s['context_ms']+s['reserve_ms'])/1000
    m.append((round(C[i]['elapsed_seconds']-inner,3),i))
print('external wall margin min/max (s):', min(m), max(m))
print('K_effective == K:', all(v['options']['K_effective']==v['options']['K'] for v in V))
print('workers/static:', {(v['options']['workers'],v['options']['tower_static_threads'],v['options']['s']) for v in V})
print('grid:', {v['input']['grid'] for v in V})
# tower_detail null without levers
for i in (18,19,1,3):
    print(i, V[i]['tower_detail'])
