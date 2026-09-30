import csv
B='/tmp/claude-1000/-workspaces-E-HGP/b64b3f68-b0cf-4b1f-9ec0-8ffa4358295a/scratchpad/bench/'
def load(f): return {(r['scene'],r['seed']):r for r in csv.DictReader(open(f))}
pre=load(B+'tour_z1.csv'); post=load(B+'tour_z1b.csv')
a=load('rerun_cda636b5e.csv'); b=load('rerun_ce8a649dd.csv')
mp=max(abs(float(a[k]['ari'])-float(pre[k]['ari'])) for k in a)
mq=max(abs(float(b[k]['ari'])-float(post[k]['ari'])) for k in b)
ca=sum(a[k]['clusters']==pre[k]['clusters'] for k in a); cb=sum(b[k]['clusters']==post[k]['clusters'] for k in b)
print('rerun cda vs tour_z1: n=%d max|dARI|=%.3g clusters equal %d'%(len(a),mp,ca))
print('rerun ce8 vs tour_z1b: n=%d max|dARI|=%.3g clusters equal %d'%(len(b),mq,cb))
for k in sorted(a):
    print(k[0],k[1][-2:],'cda=%.3f(%s) ce8=%.3f(%s)'%(float(a[k]['ari']),a[k]['clusters'],float(b[k]['ari']),b[k]['clusters']))
