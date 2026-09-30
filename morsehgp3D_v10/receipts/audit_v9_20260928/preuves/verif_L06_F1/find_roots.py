import sys
sys.argv=['x']
exec(open('gamma_check.py').read().replace('\nmain()\n','\n'))
rng=np.random.default_rng(777)
for t in range(40):
    n=int(rng.integers(5,10)); box=int(rng.choice([6,12,5000]))
    s=set()
    while len(s)<n: s.add(tuple(int(v) for v in rng.integers(0,box,3)))
    P=sorted(s); rng.shuffle(P); P=[tuple(p) for p in P]
    for K in (2,3):
        if K+1>n: continue
        rep=run_export(P,K,'fr.u32le')
        if rep is None: continue
        tr=consumer_tree(rep,'boundary')
        if len(tr[2])!=1:
            meb=meb_all(P,min(n,5))
            top=max(meb.values())+1
            print('trial',t,'n',n,'box',box,'K',K,'P',P,'boundary roots',len(tr[2]),'native roots',len(rep['native']['roots']),'gamma final',gamma_count(meb,n,K,top))
            cof,gab,size=M.read_export(rep)
            print(' cofaces',len(cof),'gabriel facets',len(gab))
