import csv, statistics as st, collections
B='/tmp/claude-1000/-workspaces-E-HGP/b64b3f68-b0cf-4b1f-9ec0-8ffa4358295a/scratchpad/bench/'
def load(f): return {(r['scene'],r['seed']):r for r in csv.DictReader(open(f))}
pre=load(B+'tour_z1.csv'); post=load(B+'tour_z1b.csv')
sph=[k for k in pre if pre[k]['family']=='spherical']
print('spherical median k/g pre %.2f post %.2f'%(st.median(int(pre[k]['clusters'])/int(pre[k]['groups']) for k in sph),st.median(int(post[k]['clusters'])/int(post[k]['groups']) for k in sph)))
drop=[k for k in sph if float(post[k]['ari'])<float(pre[k]['ari'])-1e-9]
print('spherical runs worse post:',len(drop),' of which single-cluster post:',sum(int(post[k]['clusters'])==1 for k in drop))
print('loss mass from single-cluster:',sum(float(pre[k]['ari'])-float(post[k]['ari']) for k in drop if int(post[k]['clusters'])==1)/len(sph),' total drop per run:',sum(float(pre[k]['ari'])-float(post[k]['ari']) for k in sph)/len(sph))
c=collections.Counter((pre[k]['n'],) for k in drop); print(c)
up=[k for k in sph if float(post[k]['ari'])>float(pre[k]['ari'])+1e-9]; print('spherical better post',len(up))
# 'everything else unchanged' across 250: count changed runs per family
ch=collections.Counter(pre[k]['family'] for k in pre if abs(float(pre[k]['ari'])-float(post[k]['ari']))>1e-9)
print('changed runs per family', dict(ch), 'total', sum(ch.values()))
