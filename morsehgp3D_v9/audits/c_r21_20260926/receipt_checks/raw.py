import json
R="/workspaces/E-HGP/build/v9-audit-c-publish/morsehgp3D_v9/receipts/g4_tower_r21_20260925"
V=[json.loads(open(f"{R}/vm/probe_{i}.stdout").read()) for i in range(34)]
S=json.load(open(R+"/SUMMARY.json"))
for i in range(22,34):
    qb=V[i]['q34_batch']; o=V[i]['options']
    print(i, V[i]['input']['sites'], o['K'], 'used',qb.get('used'), 'def',qb.get('deferred'),'judged',qb.get('judged_edges'),'rebuilt',qb.get('rebuilt_covers'),'ldef',qb.get('lanes_deferred'),'ljudged',qb.get('lanes_judged'),'cap',o['certificate_capacity'],o['lanes_capacity'], 'rss_kb',V[i]['peak_rss_kb'], round(V[i]['peak_rss_kb']/2**20,3),'GiB', round(V[i]['peak_rss_kb']*1024/1e9,2),'GB')
c=lambda i:V[i]['times_ms']['chain_total']
pairs={'00':((22,24),(0,13),(2,15)),'01':((26,28),(4,),(6,)),'02':((30,32),(8,),(10,))}
for f,(raw,k5,k10) in pairs.items():
    print(f,'K5 ratios',[round(c(raw[0])/c(j),3) for j in k5],'K10 ratios',[round(c(raw[1])/c(j),3) for j in k10],
          'sites ratio', round(V[raw[0]]['input']['sites']/V[k5[0]]['input']['sites'],3))
ts=sum(V[i]['input']['sites'] for i in (22,26,30)); tn=sum(V[i]['input']['sites'] for i in (0,4,8))
print('aggregate sites ratio', ts/tn)
print('aggregate time ratio K5', sum(c(i) for i in (22,26,30))/sum(c(i) for i in (0,4,8)), 'K10', sum(c(i) for i in (24,28,32))/sum(c(i) for i in (2,6,10)))
print('max rss overall', max((V[i]['peak_rss_kb'],i) for i in range(34)))
print('max gnu rss', max((r['gnu_time_max_rss_kb'],i) for i,r in enumerate(S['rows'])))
# engine vs gpu speedup quick
