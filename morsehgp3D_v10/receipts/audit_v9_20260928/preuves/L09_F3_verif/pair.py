import csv, collections, sys
B='/tmp/claude-1000/-workspaces-E-HGP/b64b3f68-b0cf-4b1f-9ec0-8ffa4358295a/scratchpad/bench/'
def load(p, filt=None):
    d={}
    for r in csv.DictReader(open(p)):
        d[(r['method'], r['scene'], int(r['seed']))]=r
    return d
rep=load('repro.csv'); base=load(B+'baselines_v2.csv')
aud=load('/tmp/claude-1000/-workspaces-E-HGP/497c67a8-5e59-434d-a8a7-2dc2144d8af4/scratchpad/audit_v9/L09_bench_method/matched.csv')
# check auditor matched_eom == my m_sqrt_ms3
diff=0; cnt=0
for (m,s,sd),r in rep.items():
    if m=='m_sqrt_ms3':
        a=aud.get(('hdbscan_matched_eom',s,sd))
        if a: cnt+=1; diff=max(diff,abs(float(a['ari'])-float(r['ari'])))
print('repro vs auditor matched_eom: pairs',cnt,'maxdiff',diff)
diff=0;cnt=0
for (m,s,sd),r in rep.items():
    if m=='m20_ms20':
        a=base.get(('hdbscan_default',s,sd))
        if a: cnt+=1; diff=max(diff,abs(float(a['ari'])-float(r['ari'])))
print('repro m20_ms20 vs receipt hdbscan_default: pairs',cnt,'maxdiff',diff)
refs={}
for (m,s,sd),r in rep.items(): refs.setdefault(m,{})[(s,sd)]=r
for (m,s,sd),r in base.items(): refs.setdefault('B_'+m,{})[(s,sd)]=r
for tag,f in (('PRE (tour_z1)','tour_z1.csv'),('POST (tour_z1b)','tour_z1b.csv')):
    t={(r['scene'],int(r['seed'])):r for r in csv.DictReader(open(B+f))}
    print('==',tag,'n=',len(t),'mean tower',sum(float(r['ari']) for r in t.values())/len(t))
    for m in sorted(refs):
        ds=[];fam=collections.defaultdict(list);seed=collections.defaultdict(list)
        for k,r in t.items():
            o=refs[m].get(k)
            if o is None: continue
            d=float(r['ari'])-float(o['ari']); ds.append(d); fam[r['family']].append(d); seed[k[1]].append(d)
        w=sum(d>1e-9 for d in ds); l=sum(d<-1e-9 for d in ds)
        sm=[sum(v)/len(v) for v in seed.values()]
        inv = len({x>1e-9 for x in sm})>1
        mr=sum(float(refs[m][k]['ari']) for k in t if k in refs[m])/len(ds)
        print('  vs %-16s %+.4f %d-%d-%d refmean=%.3f inv=%s | '%(m,sum(ds)/len(ds),w,l,len(ds)-w-l,mr,inv)+' '.join('%s:%+.3f'%(f[:5],sum(v)/len(v)) for f,v in sorted(fam.items())))
