import json
R="/workspaces/E-HGP/build/v9-audit-c-publish/morsehgp3D_v9/receipts/g4_tower_r22_20260926"
V=[json.loads(open(f"{R}/vm/probe_{i}.stdout").read()) for i in range(36)]
C=[json.load(open(f"{R}/vm/probe_{i}.command.json")) for i in range(36)]
rows={r["idx"]:r for r in json.load(open("rows.json"))}
K5=[0,13,12,16,18,20,22]; K10=[2,15,14,17,19,21,23]
lab={0:"gpu r0",13:"gpu r1",12:"r21 r0",16:"r21 r1",18:"-seal",20:"-early",22:"-pinned",2:"gpu r0",15:"gpu r1",14:"r21 r0",17:"r21 r1",19:"-seal",21:"-early",23:"-pinned"}
for grp in (K5,K10):
    print("case lab start chain tower validate vparts | gen_index tower_index census q2 q2_census q2_census_index q2_census_wait q2_wait | lanes_tr dl copy upload host_alloc pinned alloc | lanes_ms q34 merge")
    for i in grp:
        v=V[i]; t=v['times_ms']; tp=v['tower_phases_ms']; qb=v['q34_batch']
        print(i, lab[i], C[i]['started_utc'][11:19], t['chain_total'], t['tower'], tp['validate'], tp['validate_parts'], '|', t['gen_index'], t['tower_index'], t['census'], t['q2'], t.get('q2_census'), t.get('q2_census_index'), t.get('q2_census_wait'), t['q2_wait'], '|',
              qb['lanes_transfer_ms'], qb['lanes_download_ms'], qb['lanes_download_copy_ms'], qb['lanes_upload_ms'], qb['lanes_host_alloc_ms'], qb['lanes_pinned'], qb['lanes_pinned_allocations'], '|', qb['lanes_ms'], t['q34'], t['merge'])
# early census times over all early-census cases
ec=[(i,V[i]['options']['K'],V[i]['times_ms']['q2_census'],V[i]['times_ms']['q2_census_wait']) for i in range(36) if V[i]['options']['levers']['q2_early_census'] and V[i]['options']['levers']['device_session']]
print("early census cases:", ec)
for K in (5,10):
    for fr in ('scene_0','scene_b'):
        xs=[(round(x[2],1),x[0]) for x in ec if x[1]==K and rows[x[0]]['file'].startswith(fr)]
        print(K, fr, "q2_census min/max", min(xs), max(xs))
print("max q2_census_wait:", max(x[3] for x in ec))
# device session
ds=[(i,V[i]['options']['K'],V[i]['device_session']) for i in range(36) if V[i]['device_session'].get('opened')]
print("ds context min/max:", min(d['context_ms'] for _,_,d in ds), max(d['context_ms'] for _,_,d in ds), len(ds))
for K in (5,10):
    p=[d['pinned_ms'] for _,k,d in ds if k==K and d.get('pinned_bytes')]
    b={d.get('pinned_bytes') for _,k,d in ds if k==K}
    print("K",K,"pinned_ms min/max",min(p),max(p),"n",len(p),"bytes",b)
print("ds with pinned lever off:", [(i,d) for i,k,d in ds if not V[i]['options']['levers']['q34_lanes_pinned']])
print("pinned allocations (pinned cases):", {V[i]['q34_batch']['lanes_pinned_allocations'] for i in range(36) if V[i]['options']['levers']['q34_lanes_pinned'] and V[i]['q34_batch'].get('used')})
print("max rss:", max((V[i]['peak_rss_kb'],i) for i in range(36)), max((V[i]['peak_rss_kb'],i) for i in range(36))[0]/2**20, "GiB")
print("rss K10 00 pinned vs unpinned:", V[2]['peak_rss_kb']/2**20, V[15]['peak_rss_kb']/2**20, V[23]['peak_rss_kb']/2**20, V[14]['peak_rss_kb']/2**20)
