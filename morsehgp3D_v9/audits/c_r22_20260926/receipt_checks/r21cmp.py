import json
R="/workspaces/E-HGP/build/v9-audit-c-publish/morsehgp3D_v9/receipts/"
V=[json.loads(open(f"{R}g4_tower_r22_20260926/vm/probe_{i}.stdout").read()) for i in range(36)]
W=[json.loads(open(f"{R}g4_tower_r21_20260925/vm/probe_{i}.stdout").read()) for i in range(34)]
def st(v):
    t=v['times_ms']; tp=v['tower_phases_ms']; qb=v['q34_batch']
    return dict(chain=t['chain_total'],tower=t['tower'],validate=tp['validate'],q34=t['q34'],census=t['census'],merge=t['merge'],tower_index=t['tower_index'],q2=t['q2'],lanes=qb.get('lanes_ms'),lanes_tr=qb.get('lanes_transfer_ms'),front=qb.get('front_ms'),filt=qb.get('filter_ms'),cert=qb.get('certificate_ms'))
print("R21 gpu 00/K5 cases 0,13 ; R22 gpu_r21 12,16 ; R22 gpu 0,13")
for lab,v in (("R21#0",W[0]),("R21#13",W[13]),("R22r21#12",V[12]),("R22r21#16",V[16]),("R22gpu#0",V[0]),("R22gpu#13",V[13])):
    print(lab, {k:round(x,1) if x is not None else None for k,x in st(v).items()})
print("K10")
for lab,v in (("R21#2",W[2]),("R21#15",W[15]),("R22r21#14",V[14]),("R22r21#17",V[17]),("R22gpu#2",V[2]),("R22gpu#15",V[15])):
    print(lab, {k:round(x,1) if x is not None else None for k,x in st(v).items()})
# engine R21 vs R22 (sealed engine)
print("engine chain R21 vs R22:")
P21=json.load(open(R+"g4_tower_r21_20260925/PACKAGE.json"))
for i,w in enumerate(W):
    lv=w['options']['levers']
    if not lv['q34_batch_filter'] and not lv['device_session']:
        f=P21['cases'][i]['file'].split('/')[-1]; K=w['options']['K']
        m=[j for j in range(36) if V[j]['input']['hash']==w['input']['hash'] and V[j]['options']['K']==K and not V[j]['options']['levers']['device_session']]
        print(' ',f,K,'R21',i,round(w['times_ms']['chain_total']),'tower',round(w['times_ms']['tower']),'R22',m,[round(V[j]['times_ms']['chain_total']) for j in m],'tower',[round(V[j]['times_ms']['tower']) for j in m])
