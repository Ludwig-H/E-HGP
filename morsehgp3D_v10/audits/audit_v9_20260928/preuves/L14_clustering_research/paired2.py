import csv, collections, statistics as st, sys, random
exec(open('analyze.py').read().split("keys = sorted(res)")[0].replace("F = sys.argv[1]","F='L14_heads.csv'"))
keys=sorted(res)
def pair(a,b,sel=lambda k:True,label=''):
    L=[(res[k][a][0],res[k][b][0]) for k in keys if a in res[k] and b in res[k] and sel(k)]
    d=[x-y for x,y in L]; w=sum(1 for x in d if x>1e-9); l=sum(1 for x in d if x<-1e-9)
    rnd=random.Random(1); bs=[]
    for _ in range(2000):
        s=[d[rnd.randrange(len(d))] for _ in d]; bs.append(sum(s)/len(s))
    bs.sort()
    print('%-28s vs %-28s %-10s n=%3d  d=%+.3f [%+.3f,%+.3f]  %d-%d-%d' % (a,b,label,len(d),st.mean(d),bs[50],bs[1949],w,len(d)-w-l,l))
for t in ('tower_prefix','tower_postfix'):
    for b in ('eom_z1_ms2_mcssqrt','eom_z1_ms3_mcssqrt','receipt_hdbscan_oracle','sklearn_default_fill','oracle2d_eom_z1'):
        pair(t,b)
for lvl in ('easy','medium','hard','extreme'):
    pair('tower_prefix','eom_z1_ms2_mcssqrt',lambda k:meta[k]['level']==lvl,lvl)
for fam in ('spherical','shells','hierarchical','filaments','bridge','unbalanced'):
    pair('tower_prefix','eom_z1_ms2_mcssqrt',lambda k:meta[k]['family']==fam,fam)
pair('eom_z3_ms3_mcssqrt_fill','oracle2d_eom_z1_fill')
pair('eom_z3_ms3_mcssqrt_fill','eom_z1_ms3_mcssqrt_fill')
pair('eom_z3_ms3_mcssqrt','eom_z1_ms3_mcssqrt')
pair('eom_z3_ms3_mcssqrt_fill','receipt_hdbscan_oracle')
pair('sklearn_default_fill','sklearn_default')
