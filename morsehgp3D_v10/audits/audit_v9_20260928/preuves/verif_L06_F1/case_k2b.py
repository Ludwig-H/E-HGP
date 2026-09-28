import sys, os, itertools
sys.argv=['x']
exec(open('gamma_check.py').read().replace('\nmain()\n','\n'))
P=[(1758, 3005, 1987), (4937, 83, 281), (3079, 2614, 4600), (312, 3802, 1078), (1139, 2734, 4198), (4436, 240, 3803), (3203, 2240, 4308)]
n=len(P); K=2
meb=meb_all(P,4)
rep=run_export(P,K,'c.u32le')
tr=consumer_tree(rep,'boundary')
levels=sorted(set(v for T,v in meb.items() if len(T) in (K,K+1)))
for a in levels:
    g=gamma_count(meb,n,K,a); c=consumer_count(tr,a); nc=native_count(rep['native'],a)
    if g!=c or g!=nc: print('a=',float(a),'gamma',g,'consumer',c,'native',nc)
cof,gab,size=M.read_export(rep)
print('gabriel 2-sets', {k:float(v) for k,v in gab.items()})
print('cofaces', [(v,float(b)) for v,b in cof])
bad=F(2475515978143748042055,416703371979898)
print('bad a', float(bad))
print('3-sets with meb<=a:', [(U,float(meb[U])) for U in itertools.combinations(range(n),3) if meb[U]<=bad])
print('2-sets with meb<=a:', [(U,float(meb[U])) for U in itertools.combinations(range(n),2) if meb[U]<=bad])
print('consumer items alive', [(it,float(l)) for it,l in tr[0].items() if l<=bad and (it not in tr[1] or tr[0][tr[1][it]]>bad)])
