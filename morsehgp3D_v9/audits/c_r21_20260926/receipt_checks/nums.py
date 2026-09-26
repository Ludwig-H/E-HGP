import json
R="/workspaces/E-HGP/build/v9-audit-c-publish/morsehgp3D_v9/receipts/g4_tower_r21_20260925"
S=json.load(open(R+"/SUMMARY.json"))
V=[json.loads(open(f"{R}/vm/probe_{i}.stdout").read()) for i in range(34)]
rows=S['rows']
print("chain_s per row:", [(i,r['chain_s']) for i,r in enumerate(rows)])
print("tower_s:", [(i,r['tower_s']) for i,r in enumerate(rows)])
print()
for i in (0,13,18,20):
    tp=V[i]['tower_phases_ms']; td=V[i]['tower_detail']
    print(i, 'tower',V[i]['times_ms']['tower'],'validate',tp['validate'],'static',tp['static'],[round(x,1) for x in tp['static_by_k']],
          'sort5',tp['static_sort_by_k'][4],'lots',tp['lots'],'pop',tp['populations'],'img',tp['images'],'bank',tp['bank'],'enc',tp['encode'],
          'helpers',td.get('helper_threads'),'pool_jobs',td.get('pool_jobs'),'pool_threads',td.get('pool_threads'),'runner',td.get('runner_threads'))
for i in (2,15,19,21,14,17):
    td=V[i]['tower_detail']; tp=V[i]['tower_phases_ms']
    print(i,'K10 tower',V[i]['times_ms']['tower'],'static',tp['static'],'lots',tp['lots'],'pool_jobs',td.get('pool_jobs'),'helpers',td.get('helper_threads'))
print()
t=lambda i:V[i]['times_ms']['tower']
print('K5 tower gains: 18-0',t(18)-t(0),'20-13',t(20)-t(13),'mean', (t(18)+t(20))/2-(t(0)+t(13))/2, '18-13',t(18)-t(13),'20-0',t(20)-t(0))
print('K10 tower gains: 19-2',t(19)-t(2),'21-15',t(21)-t(15),'19-15',t(19)-t(15),'21-2',t(21)-t(2))
print('phase0 ratio', V[18]['tower_phases_ms']['static']/V[0]['tower_phases_ms']['static'], V[20]['tower_phases_ms']['static']/V[13]['tower_phases_ms']['static'])
c=lambda i:V[i]['times_ms']['chain_total']
print('cold/warm K5: 12-0',c(12)-c(0),'12-13',c(12)-c(13),'16-0',c(16)-c(0),'16-13',c(16)-c(13))
print('cold/warm K10: 14-2',c(14)-c(2),'14-15',c(14)-c(15),'17-2',c(17)-c(2),'17-15',c(17)-c(15))
print('prepare wait:', [(i,V[i]['q34_batch'].get('gpu_prepare_wait_ms'),V[i]['q34_batch'].get('gpu_prepare_ms'), V[i]['options']['levers']['device_session']) for i in range(34) if V[i]['q34_batch'].get('used')])
ds=[(i,V[i]['device_session']) for i in range(34) if V[i]['device_session'].get('opened')]
print('ds context min/max', min(d['context_ms'] for _,d in ds), max(d['context_ms'] for _,d in ds), 'reserve min/max', min(d['reserve_ms'] for _,d in ds), max(d['reserve_ms'] for _,d in ds), 'n', len(ds))
print('ds not opened:', [(i,V[i]['device_session']) for i in range(34) if not V[i]['device_session'].get('opened')][:3])
