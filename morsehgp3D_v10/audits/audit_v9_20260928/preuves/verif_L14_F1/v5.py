import csv, statistics as st, random
B='/tmp/claude-1000/-workspaces-E-HGP/b64b3f68-b0cf-4b1f-9ec0-8ffa4358295a/scratchpad/bench/'
H='/tmp/claude-1000/-workspaces-E-HGP/497c67a8-5e59-434d-a8a7-2dc2144d8af4/scratchpad/audit_v9/L14_clustering_research/L14_heads.csv'
REC='/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/receipts/synthetic_bench_20260928/r1/baselines.csv'
def load(f): return {(r['scene'],r['seed']):r for r in csv.DictReader(open(f))}
pre=load(B+'tour_z1.csv'); smoke=load(B+'tour_smoke.csv')
orac={(r['scene'],r['seed']):r for r in csv.DictReader(open(REC)) if r['method']=='hdbscan_oracle'}
heads={}
for r in csv.DictReader(open(H)): heads.setdefault(r['method'],{})[(r['scene'],r['seed'])]=r
def boot(d):
    rnd=random.Random(1); bs=sorted(sum(d[rnd.randrange(len(d))] for _ in d)/len(d) for _ in range(2000)); return bs[50],bs[1949]
def pair(A,Bd,keys,label):
    d=[float(A[k]['ari'])-float(Bd[k]['ari']) for k in keys]; w=sum(x>1e-9 for x in d); l=sum(x<-1e-9 for x in d); lo,hi=boot(d)
    print('%-50s n=%3d d=%+.3f [%+.3f,%+.3f] %d-%d-%d'%(label,len(d),st.mean(d),lo,hi,w,len(d)-w-l,l))
sph=sorted(k for k in pre if pre[k]['family']=='spherical'); sk=sorted(smoke)
for m in ('sklearn_default_fill','eom_z1_ms2_mcssqrt_fill','eom_z1_ms3_mcssqrt_fill','oracle2d_eom_z1','oracle2d_eom_z1_fill','leaf_ms2_mcssqrt','leaf_ms2_mcssqrt_fill'):
    pair(pre,heads[m],sph,'prefix spherical110 vs '+m)
    pair(pre,heads[m],sk,'prefix smoke34 vs '+m)
# coverage
print('coverage spherical prefix %.3f oracle %.3f'%(st.mean(float(pre[k]['coverage']) for k in sph),st.mean(float(orac[k]['coverage']) for k in sph)))
# oracle parameter = min_cluster_size (min_samples tied)
import collections
print('oracle mcs on spherical', collections.Counter(orac[k]['parameter'] for k in sph))
