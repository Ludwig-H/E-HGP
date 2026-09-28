import csv, collections, statistics as st, sys
B='/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/receipts/synthetic_bench_20260928/r1/baselines.csv'
base=collections.defaultdict(dict)
meta={}
for r in csv.DictReader(open(B)):
    base[(r['scene'],r['seed'])][r['method']]=float(r['ari'])
    meta[(r['scene'],r['seed'])]=r
def load(f):
    d=collections.defaultdict(dict)
    for r in csv.DictReader(open(f)):
        d[(r['scene'],r['seed'])][r['method']]=(float(r['ari']),float(r['coverage']),int(r['clusters']))
    return d
for f in sys.argv[1:]:
    T=load(f)
    methods=sorted({m for v in T.values() for m in v})
    print('=====',f,len(T),'runs',methods)
    for m in methods:
        groups=collections.defaultdict(list)
        for k,v in T.items():
            if m not in v: continue
            fam=meta[k]['family']; lvl=meta[k]['level']
            a=v[m][0]; groups[fam].append((a,base[k]['hdbscan_default'],base[k]['hdbscan_oracle'],base[k]['single_linkage'],v[m][1]))
            groups['ALL'].append(groups[fam][-1])
            groups['lvl_'+lvl].append(groups[fam][-1])
        print('--',m)
        for g in sorted(groups):
            L=groups[g]
            t=st.mean(x[0] for x in L); d=st.mean(x[1] for x in L); o=st.mean(x[2] for x in L)
            wo=sum(1 for x in L if x[0]>x[2]+1e-9); lo=sum(1 for x in L if x[0]<x[2]-1e-9)
            wd=sum(1 for x in L if x[0]>x[1]+1e-9); ld=sum(1 for x in L if x[0]<x[1]-1e-9)
            print(f'{g:18s} n={len(L):3d} tower={t:.3f} cov={st.mean(x[4] for x in L):.3f} def={d:.3f} orc={o:.3f} | vs def {wd}-{len(L)-wd-ld}-{ld} d={t-d:+.3f} | vs orc {wo}-{len(L)-wo-lo}-{lo} d={t-o:+.3f}')
