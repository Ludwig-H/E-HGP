import csv, collections, ast, statistics as st
B='/tmp/claude-1000/-workspaces-E-HGP/b64b3f68-b0cf-4b1f-9ec0-8ffa4358295a/scratchpad/bench/'
A='/tmp/claude-1000/-workspaces-E-HGP/497c67a8-5e59-434d-a8a7-2dc2144d8af4/scratchpad/audit_v9/L08_clustering_code/'
R='/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/receipts/synthetic_bench_20260928/r1/baselines.csv'
base=list(csv.DictReader(open(R)))
orc={(r['scene'],r['seed']):float(r['ari']) for r in base if r['method']=='hdbscan_oracle'}
z1r={(r['scene'],r['seed']):r for r in csv.DictReader(open(B+'tour_z1.csv'))}
z1br={(r['scene'],r['seed']):r for r in csv.DictReader(open(B+'tour_z1b.csv'))}
# slice n=2000 medium g8 seed0 per family
print('slice n=2000 g8 medium, first seed:')
for k,r in sorted(z1r.items()):
    if r['n']=='2000' and r['level']=='medium' and r['groups']=='8' and r['replicate']=='0':
        print(f"  {r['family']:16s} {r['scene']:40s} z1={float(r['ari']):.3f} z1b={float(z1br[k]['ari']):.3f}")
sph=[k for k,r in z1r.items() if r['family']=='spherical']
d=[float(z1br[k]['ari'])-float(z1r[k]['ari']) for k in sph]
print('spherical z1b-z1: mean',round(st.mean(d),3),'up',sum(x>1e-9 for x in d),'down',sum(x<-1e-9 for x in d),'same',sum(abs(x)<=1e-9 for x in d))
nz=[k for k in sph if float(z1br[k]['ari'])>0]
print('spherical excluding z1b ARI=0:', len(nz), 'z1b', round(st.mean(float(z1br[k]['ari']) for k in nz),3), 'z1', round(st.mean(float(z1r[k]['ari']) for k in nz),3))
ext={(r['scene'],r['seed']):r for r in csv.DictReader(open(A+'oracle_ext.csv'))}
ks=[k for k in ext if z1r[k]['family']=='spherical']
nz=[k for k in ks if float(z1br[k]['ari'])>0]
print('ext spherical excluding z1b ARI0:',len(nz),'z1b',round(st.mean(float(z1br[k]['ari']) for k in nz),3),'ext',round(st.mean(float(ext[k]['oracle_ext']) for k in nz),3))
# ties: shells best
print('shells best:', collections.Counter(ext[k]['best'] for k in ext if z1r[k]['family']=='shells'))
print('best==restricted (ext == restricted) count', sum(abs(float(r['oracle_ext'])-float(r['oracle_restricted']))<1e-9 for r in ext.values()))
# smoke 34 subset
sm={(r['scene'],r['seed']):float(r['ari']) for r in csv.DictReader(open(B+'tour_smoke.csv'))}
ms1={(r['scene'],r['seed']):float(r['ari_ms1_sqrt']) for r in csv.DictReader(open(A+'hdb_ms1_all.csv'))}
ks=list(sm)
print('smoke34: tour',round(st.mean(sm[k] for k in ks),3),'oracle',round(st.mean(orc[k] for k in ks),3),'ms1',round(st.mean(ms1[k] for k in ks),3), 'ms1 vs oracle W', sum(ms1[k]>orc[k]+1e-9 for k in ks),'L',sum(ms1[k]<orc[k]-1e-9 for k in ks))
kse=[k for k in ks if k in ext]
print('smoke34 with ext',len(kse),'tour',round(st.mean(sm[k] for k in kse),3),'ext',round(st.mean(float(ext[k]['oracle_ext']) for k in kse),3),'W',sum(sm[k]>float(ext[k]['oracle_ext'])+1e-9 for k in kse),'L',sum(sm[k]<float(ext[k]['oracle_ext'])-1e-9 for k in kse))
print('smoke vs z1 identical?', sum(abs(sm[k]-float(z1r[k]['ari']))<1e-9 for k in ks), 'of', len(ks))
print('ms1 vs z1 spherical 110', round(st.mean(ms1[k] for k in sph),3), round(st.mean(float(z1r[k]['ari']) for k in sph),3))
