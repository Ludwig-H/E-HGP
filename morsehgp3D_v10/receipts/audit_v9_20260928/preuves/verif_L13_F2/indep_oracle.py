# Oracle independant (Fractions, barycentriques, sans primitives de cble) :
# boules bien centrees, q_min, p + q_min <= K+1, comptes par (q_min, p) et coquilles etendues.
import itertools, json, random, subprocess, struct, sys
from fractions import Fraction as F
def solve(A, b):
    n=len(A); M=[row[:]+[bb] for row,bb in zip(A,b)]
    for c in range(n):
        piv=next((r for r in range(c,n) if M[r][c]!=0),None)
        if piv is None: return None
        M[c],M[piv]=M[piv],M[c]
        for r in range(n):
            if r!=c and M[r][c]!=0:
                f=M[r][c]/M[c][c]; M[r]=[x-f*y for x,y in zip(M[r],M[c])]
    return [M[i][n]/M[i][i] for i in range(n)]
def ball_of(S):
    s0=S[0]; D=[[F(S[i][k]-s0[k]) for k in range(3)] for i in range(1,len(S))]
    G=[[sum(a*b for a,b in zip(D[i],D[j])) for j in range(len(D))] for i in range(len(D))]
    rhs=[sum(a*a for a in D[i])/2 for i in range(len(D))]
    lam=solve(G,rhs)
    if lam is None: return None
    bary=[1-sum(lam)]+lam
    if any(x<=0 for x in bary): return None
    c=tuple(s0[k]+sum(lam[i]*D[i][k] for i in range(len(D))) for k in range(3))
    r2=sum((c[k]-s0[k])**2 for k in range(3))
    return c,r2
def oracle(P,K):
    best={}
    for m in (2,3,4):
        for S in itertools.combinations(P,m):
            b=ball_of(S)
            if b is None: continue
            if b not in best or best[b]>m: best[b]=m
    byqp={}; ext=0; tot=0
    for (c,r2),q in best.items():
        p=0; sh=0
        for z in P:
            d=sum((z[k]-c[k])**2 for k in range(3))
            if d<r2: p+=1
            elif d==r2: sh+=1
        if p+q<=K+1:
            byqp[(q,p)]=byqp.get((q,p),0)+1; tot+=1
            if sh>q: ext+=1
    return byqp,tot,ext
def gen(n,seed,fam,rng):
    random.seed(seed); used=set(); P=[]
    while len(P)<n:
        x,y,z=random.randrange(rng),random.randrange(rng),random.randrange(rng)
        if fam=='plane': z=(x+2*y)%3
        if fam=='grid': x,y,z=random.randrange(5),random.randrange(5),random.randrange(5)
        if fam=='big': x,y,z=random.randrange(1<<18),random.randrange(1<<18),random.randrange(1<<18)
        if fam=='bigsphere':
            # points entiers sur une grande sphere (cospheriques) + bruit
            pass
        if (x,y,z) not in used: used.add((x,y,z)); P.append((x,y,z))
    return P
fails=0; tot=0
for fam,n,rng in (('uniform',22,32),('plane',22,8),('grid',22,5),('big',18,1),('uniform',25,10)):
    for seed in (1,2,3):
        P=gen(n,seed,fam,rng)
        fn=f'pts_{fam}_{n}_{seed}.u32le'
        open(fn,'wb').write(b''.join(struct.pack('<3I',*p) for p in P))
        for K in (2,4,6):
            byqp,t,e=oracle(P,K)
            out=json.loads(subprocess.run(['nice','-n','19','./cble',fn,f'--K={K}',f'--M={max(16,2*K+4)}','--dom=2'],capture_output=True,text=True,check=True).stdout)
            cb={}
            for q in (2,3,4):
                for p,v in enumerate(out['by_q_p'][f'q{q}']):
                    if v: cb[(q,p)]=v
            ok = cb==byqp and out['extra_shell']==e and out['balls']==t
            tot+=1
            if not ok:
                fails+=1; print('MISMATCH',fam,n,seed,K,sorted(byqp.items()),sorted(cb.items()),e,out['extra_shell'])
            else:
                print('ok',fam,n,seed,K,t,e)
print('total',tot,'fails',fails)
