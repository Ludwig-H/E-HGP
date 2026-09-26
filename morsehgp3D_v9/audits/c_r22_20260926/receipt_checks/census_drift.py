import json
R="/workspaces/E-HGP/build/v9-audit-c-publish/morsehgp3D_v9/receipts/"
V=[json.loads(open(f"{R}g4_tower_r22_20260926/vm/probe_{i}.stdout").read()) for i in range(36)]
W=[json.loads(open(f"{R}g4_tower_r21_20260925/vm/probe_{i}.stdout").read()) for i in range(34)]
def key(v): 
    lv=v['options']['levers']; return (v['input']['hash'],v['options']['K'],lv['device_session'])
out=[]
for i,w in enumerate(W):
    lv=w['options']['levers']
    if not all(lv[k] for k in ('tower_pipelined_tail','tower_hash_grouping','tower_persistent_pool')): continue
    if lv['device_session']!=lv['q34_batch_filter']: continue
    m=[j for j in range(36) if key(V[j])==key(w)]
    # R22 matches: for gpu: prefer gpu_r21 arm (new levers off) else full gpu; engine sealed
    for j in m:
        L=V[j]['options']['levers']
        tag='eng' if not L['device_session'] else ('r21arm' if not (L['tower_sealed_catalogue'] or L['q2_early_census'] or L['q34_lanes_pinned']) else ('gpu' if all((L['tower_sealed_catalogue'],L['q2_early_census'],L['q34_lanes_pinned'])) else 'abl'))
        out.append((w['input']['sites'],w['options']['K'],'eng' if not lv['device_session'] else 'gpu',i,round(w['times_ms']['census'],1),tag,j,round(V[j]['times_ms']['census'],1),round(V[j]['times_ms']['census']-w['times_ms']['census'],1)))
seen=set()
for o in out:
    if (o[3],o[6]) in seen: continue
    seen.add((o[3],o[6])); print(o)
