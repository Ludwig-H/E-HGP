import csv, collections, ast, statistics as st
B='/tmp/claude-1000/-workspaces-E-HGP/b64b3f68-b0cf-4b1f-9ec0-8ffa4358295a/scratchpad/bench/'
A='/tmp/claude-1000/-workspaces-E-HGP/497c67a8-5e59-434d-a8a7-2dc2144d8af4/scratchpad/audit_v9/L08_clustering_code/'
R='/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/receipts/synthetic_bench_20260928/r1/baselines.csv'
base=list(csv.DictReader(open(R)))
orc={(r['scene'],r['seed']):float(r['ari']) for r in base if r['method']=='hdbscan_oracle'}
dfl={(r['scene'],r['seed']):float(r['ari']) for r in base if r['method']=='hdbscan_default'}
fam={(r['scene'],r['seed']):r['family'] for r in base}
print('baseline rows', len(base), collections.Counter(r['method'] for r in base))
ms1=list(csv.DictReader(open(A+'hdb_ms1_all.csv')))
print('ms1 rows',len(ms1))
def cmp(ch, ref, keys, label):
    ks=[k for k in keys if k in ref and k in ch]
    d=[ch[k]-ref[k] for k in ks]
    w=sum(x>1e-9 for x in d); l=sum(x<-1e-9 for x in d); t=len(d)-w-l
    print(f'{label}: pairs={len(ks)} mean_ch={st.mean(ch[k] for k in ks):.3f} mean_ref={st.mean(ref[k] for k in ks):.3f} delta={st.mean(d):+.3f} W-L-T={w}-{l}-{t}')
m1={(r['scene'],r['seed']):float(r['ari_ms1_sqrt']) for r in ms1}
cmp(m1,orc,list(m1),'ms1 vs oracle all')
for f in sorted(set(fam.values())):
    cmp(m1,orc,[k for k in m1 if fam.get(k)==f],f'ms1 vs oracle {f}')
z1={(r['scene'],r['seed']):float(r['ari']) for r in csv.DictReader(open(B+'tour_z1.csv'))}
z1b={(r['scene'],r['seed']):float(r['ari']) for r in csv.DictReader(open(B+'tour_z1b.csv'))}
cmp(z1,orc,list(z1),'tour_z1 vs oracle all')
cmp(z1b,orc,list(z1b),'tour_z1b vs oracle all')
cmp(m1,z1,list(z1),'ms1 vs tour_z1 all')
cmp(m1,z1b,list(z1b),'ms1 vs tour_z1b all')
for f in sorted(set(fam.values())):
    ks=[k for k in z1 if fam.get(k)==f]
    print(f, f'z1={st.mean(z1[k] for k in ks):.3f} z1b={st.mean(z1b[k] for k in ks):.3f} n={len(ks)}', 'root-ARI0 z1b:', sum(z1b[k]==0 for k in ks), 'z1:', sum(z1[k]==0 for k in ks))
ext=list(csv.DictReader(open(A+'oracle_ext.csv')))
E={(r['scene'],r['seed']):float(r['oracle_ext']) for r in ext}
Rr={(r['scene'],r['seed']):float(r['oracle_restricted']) for r in ext}
# check restricted matches receipt oracle
mism=[k for k in Rr if abs(Rr[k]-orc[k])>1e-3]
print('ext rows',len(ext),'restricted mismatch vs receipt',len(mism))
print('ext families', collections.Counter(fam[k] for k in E), collections.Counter(r['n'] for r in ext))
for f in sorted(set(fam[k] for k in E)):
    ks=[k for k in E if fam[k]==f]
    print(f'ext {f}: n={len(ks)} ext={st.mean(E[k] for k in ks):.3f} restr={st.mean(Rr[k] for k in ks):.3f}')
    cmp(z1,E,ks,f'  tour_z1 vs ext {f}')
    cmp(z1b,E,ks,f'  tour_z1b vs ext {f}')
    cmp(m1,E,ks,f'  ms1 vs ext {f}')
ms=collections.Counter(ast.literal_eval(r['best'])[1] for r in ext)
sel=collections.Counter(ast.literal_eval(r['best'])[2] for r in ext)
print('best min_samples', ms, 'best sel', sel)
