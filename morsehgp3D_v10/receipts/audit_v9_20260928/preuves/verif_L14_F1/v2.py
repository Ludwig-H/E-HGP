import csv
B='/tmp/claude-1000/-workspaces-E-HGP/b64b3f68-b0cf-4b1f-9ec0-8ffa4358295a/scratchpad/bench/'
def load(f):
    return {(r['scene'],r['seed']):r for r in csv.DictReader(open(f))}
pre=load(B+'tour_z1.csv'); post=load(B+'tour_z1b.csv')
for k in sorted(pre):
    if '_n2000_g8_medium_noise0' in k[0] and not k[0].endswith(('p1','p3')):
        a,b=pre[k],post[k]
        print('%-40s %s pre=%.3f(%s) post=%.3f(%s)'%(k[0],k[1][-2:],float(a['ari']),a['clusters'],float(b['ari']),b['clusters']))
