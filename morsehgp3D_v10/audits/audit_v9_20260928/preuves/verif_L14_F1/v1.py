import csv, statistics as st, random, collections
B='/tmp/claude-1000/-workspaces-E-HGP/b64b3f68-b0cf-4b1f-9ec0-8ffa4358295a/scratchpad/bench/'
REC='/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/receipts/synthetic_bench_20260928/r1/baselines.csv'
H='/tmp/claude-1000/-workspaces-E-HGP/497c67a8-5e59-434d-a8a7-2dc2144d8af4/scratchpad/audit_v9/L14_clustering_research/L14_heads.csv'
def load(f, meth=None):
    d={}
    for r in csv.DictReader(open(f)):
        if meth and r['method']!=meth: continue
        d[(r['scene'],r['seed'])]=r
    return d
smoke=load(B+'tour_smoke.csv'); pre=load(B+'tour_z1.csv'); post=load(B+'tour_z1b.csv')
orac=load(REC,'hdbscan_oracle'); dflt=load(REC,'hdbscan_default')
heads={}
for r in csv.DictReader(open(H)):
    heads.setdefault(r['method'],{})[(r['scene'],r['seed'])]=r
def boot(d):
    rnd=random.Random(1); bs=[]
    for _ in range(2000):
        s=[d[rnd.randrange(len(d))] for _ in d]; bs.append(sum(s)/len(s))
    bs.sort(); return bs[50],bs[1949]
def pair(A,Bd,keys,label):
    d=[float(A[k]['ari'])-float(Bd[k]['ari']) for k in keys]
    w=sum(x>1e-9 for x in d); l=sum(x<-1e-9 for x in d)
    lo,hi=boot(d)
    print('%-45s n=%3d d=%+.3f [%+.3f,%+.3f] %d-%d-%d meanA=%.3f meanB=%.3f'%(label,len(d),st.mean(d),lo,hi,w,len(d)-w-l,l,st.mean(float(A[k]['ari']) for k in keys),st.mean(float(Bd[k]['ari']) for k in keys)))
# smoke vs prefix identity
diff=[abs(float(smoke[k]['ari'])-float(pre[k]['ari'])) for k in smoke]
print('smoke vs tour_z1 same keys:',len(smoke), 'max|dARI|=%.3g'%max(diff))
sk=sorted(smoke)
pair(smoke,orac,sk,'smoke34 vs receipt oracle')
pair(pre,orac,sk,'tour_z1(prefix) on smoke34 vs oracle')
pair(post,orac,sk,'tour_z1b(postfix) on smoke34 vs oracle')
sph=sorted(k for k in pre if pre[k]['family']=='spherical')
pair(pre,orac,sph,'prefix spherical all vs oracle')
pair(post,orac,sph,'postfix spherical all vs oracle')
pair(pre,dflt,sph,'prefix spherical all vs default')
m2=heads['eom_z1_ms2_mcssqrt']; m3=heads['eom_z1_ms3_mcssqrt']
pair(pre,m2,sk,'prefix smoke34 vs eom ms2 sqrt')
pair(pre,m3,sk,'prefix smoke34 vs eom ms3 sqrt')
pair(pre,m2,sph,'prefix spherical vs eom ms2 sqrt')
pair(pre,m3,sph,'prefix spherical vs eom ms3 sqrt')
pair(post,m2,sph,'postfix spherical vs eom ms2 sqrt')
allk=sorted(pre)
pair(pre,orac,allk,'prefix all vs oracle')
pair(post,orac,allk,'postfix all vs oracle')
pair(pre,m2,allk,'prefix all vs ms2')
pair(post,m2,allk,'postfix all vs ms2')
# spherical mean pre/post
print('spherical mean prefix %.3f postfix %.3f n=%d'%(st.mean(float(pre[k]['ari']) for k in sph),st.mean(float(post[k]['ari']) for k in sph),len(sph)))
print('single-cluster outputs prefix',sum(int(pre[k]['clusters'])==1 for k in allk),'postfix',sum(int(post[k]['clusters'])==1 for k in allk))
print('single-cluster spherical postfix',sum(int(post[k]['clusters'])==1 for k in sph))
# over-split ratio median
import statistics
r=[int(pre[k]['clusters'])/int(pre[k]['groups']) for k in allk]; print('median k/g prefix %.2f'%statistics.median(r))
r=[int(post[k]['clusters'])/int(post[k]['groups']) for k in allk]; print('median k/g postfix %.2f'%statistics.median(r))
# per family means pre/post
fam=collections.defaultdict(list)
for k in allk: fam[pre[k]['family']].append((float(pre[k]['ari']),float(post[k]['ari']),int(post[k]['clusters'])==1))
for f,v in sorted(fam.items()): print('%-16s n=%3d pre=%.3f post=%.3f single_post=%d'%(f,len(v),st.mean(a for a,_,_ in v),st.mean(b for _,b,_ in v),sum(c for *_,c in v)))
# default+fill vs oracle
sf=heads['sklearn_default_fill']
print('sklearn_default_fill mean %.3f  receipt oracle mean %.3f  n=%d'%(st.mean(float(sf[k]['ari']) for k in allk),st.mean(float(orac[k]['ari']) for k in allk),len(allk)))
vv=heads.get('verify_vs_sklearn',{})
print('verify_vs_sklearn n=%d min=%.4f'%(len(vv),min(float(r['ari']) for r in vv.values())))
