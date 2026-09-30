"""Tiny scalar RN/Fraction audit. No native binary, engine import or compilation."""
from fractions import Fraction as F
from functools import reduce
from math import gcd, inf, nextafter, isfinite
from pathlib import Path
import hashlib
import json
import sys

checks = 0
failures = []
def require(ok, label):
    global checks
    checks += 1
    if not ok: failures.append(label)

def sub(a,b): return tuple(x-y for x,y in zip(a,b))
def dot(a,b): return sum(x*y for x,y in zip(a,b))
def cross(a,b): return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def center3(a,b,c):
    u,v=sub(b,a),sub(c,a); w=cross(u,v)
    N=cross(sub(tuple(dot(u,u)*x for x in v),tuple(dot(v,v)*x for x in u)),w)
    return N,2*dot(w,w)
def center4(a,b,c,d):
    u,v,s=sub(b,a),sub(c,a),sub(d,a)
    D=2*dot(u,cross(v,s))
    N=tuple(dot(u,u)*x+dot(v,v)*y+dot(s,s)*z for x,y,z in zip(cross(v,s),cross(s,u),cross(u,v)))
    if D<0: D=-D; N=tuple(-x for x in N)
    return N,D
def approximate(a,N,D): return tuple(float(x)+float(n)/float(D) for x,n in zip(a,N))
def approx_d2(q,x):
    dx,dy,dz=(float(v)-w for v,w in zip(x,q))
    return dx*dx+dy*dy+dz*dz
def side(a,N,D,x):
    v=sub(x,a); return D*dot(v,v)-2*dot(N,v)
def morton(p): return sum(((p[a]>>i)&1)<<(3*i+a) for i in range(32) for a in range(3))

def down(x): return nextafter(x,-inf)
def up(x): return nextafter(x,inf)
def iv_int(n):
    x=float(n); return down(x),up(x)
def iv_exact(n):
    x=float(n); require(F(x)==n,'exact integer endpoint'); return x,x
def iv_sub(a,b): return down(a[0]-b[1]),up(a[1]-b[0])
def iv_div(a,b):
    require(b[0]>0,'positive interval denominator')
    xs=[x/y for x in a for y in b]
    return down(min(xs)),up(max(xs))
def iv_square(a):
    lo=0.0 if a[0]<=0<=a[1] else down(min(a[0]*a[0],a[1]*a[1]))
    hi=up(max(a[0]*a[0],a[1]*a[1]))
    return max(0.0,lo),hi
def iv_sum(xs):
    lo=hi=0.0
    for a,b in xs: lo=down(lo+a); hi=up(hi+b)
    return max(0.0,lo),hi
def relative_center(N,D): return tuple(iv_div(iv_int(n),iv_int(D)) for n in N)
def distance_iv(v,q): return iv_sum(iv_square(iv_sub(iv_exact(x),w)) for x,w in zip(v,q))
def exact_d2(v,N,D): return sum((F(x)-F(n,D))**2 for x,n in zip(v,N))
def encloses(iv,x): return F(iv[0])<=x<=F(iv[1])
def box_lower(lo,hi,q):
    ts=[max(0.0,down(max(0.0,float(a)-w[1],w[0]-float(b)))) for a,b,w in zip(lo,hi,q)]
    out=0.0
    for t in ts: out=max(0.0,down(out+max(0.0,down(t*t))))
    return out

cases=[]
fixtures=[('u24_lost',2_000_333,(0,0,0),1),
          ('u24_false_interior',2_000_222,(0,0,0),-1),
          ('u32_translated_lost',40_001,(4_000_000_000,)*3,1),
          ('u32_translated_false_interior',40_003,(4_000_000_000,)*3,-1)]
for name,s,shift,direction in fixtures:
    ps=[tuple(x+y for x,y in zip(v,shift)) for v in ((0,0,0),(5*s,0,0),(s,5*s,0))]
    a,b,c=ps; N,D=center3(a,b,c); q=approximate(a,N,D)
    ds=[approx_d2(q,p) for p in ps]; gap=ds[2]-ds[0]
    require(all(dot(sub(y,x),sub(z,x))>0 for x,y,z in ((a,b,c),(b,a,c),(c,a,b))),name+': strict acute')
    require(all(side(a,N,D,p)==0 for p in ps),name+': exact shell')
    require(max(max(p) for p in ps)<(1<<24 if name.startswith('u24') else 1<<32),name+': domain')
    wrong=ds[2]>ds[0]+0.02 if direction>0 else ds[2]<ds[0]-0.02
    require(wrong,name+': old scalar filter loses shell')
    qr=relative_center(N,D); R=distance_iv((0,0,0),qr)
    for p in ps:
        v=sub(p,a); di=distance_iv(v,qr); de=exact_d2(v,N,D)
        require(encloses(di,de),name+': distance interval')
        require(encloses(R,de),name+': radius interval')
        require(not (di[0]>R[1] or di[1]<R[0]),name+': shell falls back')
        require(F(box_lower(v,v,qr))<=de,name+': singleton box lower bound')
    # The identical local geometry at origin has identical certified intervals.
    local=[sub(p,a) for p in ps]; N0,D0=center3(*local)
    require((N,D)==(N0,D0),name+': center translation invariant')
    require(distance_iv(local[2],qr)==distance_iv(local[2],relative_center(N0,D0)),name+': filter translation invariant')
    cases.append({'name':name,'points':ps,'N':N,'D':D,'q':q,'radius_approx':ds[0],'site_approx':ds[2],
                  'gap':gap,'lost':direction>0,'false_interior':direction<0,'local_radius_interval':R,
                  'local_site_interval':distance_iv(local[2],qr),'side_exact':0,
                  'encoding':'unreduced polynomial; native old side width NOT presumed'})

# Causal principal fixtures use small equivalent centers: exact side intermediates fit i128.
reduced=[('u24_reduced_lost',[(0,0,0),(16_498_930,0,0),(3_299_786,16_498_930,0)],(82_494_650,69_295_506,0),10,1),
         ('u24_reduced_false_interior',[(0,0,0),(15_000_005,15_000_005,0),(15_000_005,0,15_000_005)],(30_000_010,15_000_005,15_000_005),3,-1),
         ('u32_translated_reduced_lost',[(4_000_000_000,)*3,(4_000_200_005,4_000_000_000,4_000_000_000),
                                      (4_000_040_001,4_000_200_005,4_000_000_000)],(1_000_025,840_021,0),10,1)]
for name,ps,N,D,direction in reduced:
    a,b,c=ps; N0,D0=center3(*ps)
    require(all(n*D0==m*D for n,m in zip(N,N0)),name+': same exact center')
    require(all(dot(sub(y,x),sub(z,x))>0 for x,y,z in ((a,b,c),(b,a,c),(c,a,b))),name+': strict acute')
    require(all(side(a,N,D,p)==0 for p in ps),name+': exact shell')
    max_terms=max(max(D*dot(sub(p,a),sub(p,a)),2*sum(abs(n*v) for n,v in zip(N,sub(p,a)))) for p in ps)
    require(max_terms<(1<<127),name+': old side intermediates fit i128')
    q=approximate(a,N,D); rr=approx_d2(q,a); dd=approx_d2(q,c)
    require(dd>rr+0.02 if direction>0 else dd<rr-0.02,name+': incorrect scalar shortcut')
    qr=relative_center(N,D); ri=distance_iv((0,0,0),qr); di=distance_iv(sub(c,a),qr)
    require(encloses(ri,exact_d2((0,0,0),N,D)) and encloses(di,exact_d2(sub(c,a),N,D)),name+': intervals')
    require(not(di[0]>ri[1] or di[1]<ri[0]),name+': shell requires exact')
    for h in (0,1,1000):
        lo=tuple(v-h for v in sub(c,a)); hi=tuple(v+h for v in sub(c,a))
        exact_box=sum(max(F(l)-F(n,D),F(n,D)-F(r),0)**2 for l,r,n in zip(lo,hi,N))
        require(F(box_lower(lo,hi,qr))<=exact_box,name+': nontrivial certified box')
    cases.append({'name':name,'points':ps,'N':N,'D':D,'q':q,'radius_approx':rr,'site_approx':dd,
                  'gap':dd-rr,'side_exact':0,'max_side_intermediate_bits':max_terms.bit_length(),
                  'encoding':'equivalent small center; exact old side width safe',
                  'local_radius_interval':ri,'local_site_interval':di})

# q2 shell: exact center and no conversion error. .02 rounds away at this scale.
for m,direction in ((400_000_005,1),(400_000_004,-1)):
    R=5*m; a=(0,R,0); b=(2*R,R,0); x=(R+3*m,R+4*m,0)
    N,D=sub(b,a),2; q=approximate(a,N,D); rr=approx_d2(q,a); dd=approx_d2(q,x)
    require(side(a,N,D,x)==0,'q2 exact shell')
    require(dd>rr+0.02 if direction>0 else dd<rr-0.02,'q2 old wrong decision')
    require(rr+0.02==rr and rr-0.02==rr,'margin rounds away')
    qr=relative_center(N,D); ri=distance_iv((0,0,0),qr); di=distance_iv(sub(x,a),qr)
    require(encloses(ri,F(R*R)) and encloses(di,F(R*R)),'q2 intervals contain shell')
    require(not(di[0]>ri[1] or di[1]<ri[0]),'q2 intervals require exact')
    cases.append({'name':'u32_q2_lost' if direction>0 else 'u32_q2_false_interior',
                  'points':[a,b,x],'N':N,'D':D,'radius_approx':rr,'site_approx':dd,'gap':dd-rr,
                  'side_exact':0,'local_radius_interval':ri,'local_site_interval':di})

# Three cloud sites in actual Morton96/site-ID order. One leaf, no tree-shape caveat.
m=400_000_005; R=5*m; q=(R,R,R); x=(R-3*m,R-4*m,R); a=(2*R,R,R)
ps=sorted([x,q,a],key=morton)
require(ps==[x,q,a],'nearest actual Morton/site order')
ds=[approx_d2(tuple(map(float,q)),p) for p in ps]
exact=[dot(sub(p,q),sub(p,q)) for p in ps]
require(exact[0]==exact[2]==R*R and ds[0]-ds[2]==512,'nearest exact tie but floating separation')
best=[]; cand=[]; bound=1e300
for t,d in enumerate(ds):
    if d>bound: continue
    best=sorted(best+[d])[:2]; cand.append((d,t))
    if len(best)==2: bound=best[-1]+0.02
old=sorted((exact[t],t) for d,t in cand if d<=bound)[:2]
expected=sorted((d,t) for t,d in enumerate(exact))[:2]
require([t for d,t in expected]==[1,0] and [t for d,t in old]==[1,2],'nearest tie failure')
cis=[distance_iv(sub(p,q),((0.0,0.0),)*3) for p in ps]
upper=sorted(b for a,b in cis)[1]
survivors=[t for t,(a,b) in enumerate(cis) if a<=upper]
fixed=sorted((exact[t],t) for t in survivors)[:2]
require(fixed==expected,'upper order statistic/lower retention preserves exact tie')
nearest={'points_site_order':ps,'exact_squared_distances':exact,'approx_squared_distances':ds,
         'old_ids':[t for d,t in old],'expected_ids':[t for d,t in expected],
         'intervals':cis,'interval_upper_bound':upper,'interval_ids':[t for d,t in fixed]}

# Translation before polynomial evaluation cancels absolute coordinate size in catalogue forms.
catalogue_checks=0
for s in (1,40_001,2_000_333):
    points=[(0,0,0),(5*s,0,0),(s,5*s,0),(2*s,s,3*s)]
    for shift in ((0,0,0),(4_000_000_000-5*s,)*3):
        P=[tuple((x+y)*64 for x,y in zip(p,shift)) for p in points]
        lo=tuple(min(p[i] for p in P) for i in range(3)); hi=tuple(max(p[i] for p in P)+1 for i in range(3))
        O=lo; PP=[sub(p,O) for p in P]; ll=sub(lo,O); hh=sub(hi,O)
        for i in range(len(P)):
            for j in range(i+1,len(P)):
                delta=dot(P[j],P[j])-dot(P[i],P[i]); u=sub(P[i],P[j])
                delta0=dot(PP[j],PP[j])-dot(PP[i],PP[i]); u0=sub(PP[i],PP[j])
                for mask in range(8):
                    C=tuple(hi[k] if mask>>k&1 else lo[k] for k in range(3)); CC=sub(C,O)
                    require(delta+2*dot(C,u)==delta0+2*dot(CC,u0),'catalogue affine dominance translation')
                    catalogue_checks+=1
                k=next(k for k in range(len(P)) if k not in (i,j))
                v=sub(P[i],P[k]); v0=sub(PP[i],PP[k]); L=tuple(x+y for x,y in zip(lo,hi)); LL=tuple(x+y for x,y in zip(ll,hh))
                p0=2*(dot(P[i],P[i])-dot(P[j],P[j]))-2*dot(L,u)
                p1=2*(dot(P[i],P[i])-dot(P[k],P[k]))-2*dot(L,v)
                pp0=2*(dot(PP[i],PP[i])-dot(PP[j],PP[j]))-2*dot(LL,u0)
                pp1=2*(dot(PP[i],PP[i])-dot(PP[k],PP[k]))-2*dot(LL,v0)
                require((p0,p1)==(pp0,pp1),'catalogue center-line translation')
                catalogue_checks+=1

# GCD helps some presentations, not the domain: strict regular and two strict u24 controls.
gcd_controls=[]
M=(1<<24)-1
for label,P in [('regular',[(0,0,0),(M,M,0),(M,0,M),(0,M,M)]),
                ('near_regular',[(0,0,0),(M,M-1,1),(M-2,1,M),(1,M,M-2)]),
                ('coprime',[(0,0,0),(M,M-1,1),(2,1,M),(1,M,M-2)])]:
    a,b,c,d=P; N,D=center4(*P); u,v,s=sub(b,a),sub(c,a),sub(d,a); det=dot(u,cross(v,s))
    ws=(F(dot(N,cross(v,s)),D*det),F(dot(u,cross(N,s)),D*det),F(dot(u,cross(v,N)),D*det))
    weights=(1-sum(ws),*ws); g=reduce(gcd,(*N,D)); Nr=tuple(n//g for n in N); Dr=D//g
    nb=sum(n*n for n in Nr).bit_length(); db=(Dr*Dr).bit_length()
    require(all(w>0 for w in weights),label+': strictly interior center')
    require(all(side(a,N,D,p)==0 for p in P),label+': exact support shell')
    require((nb<=192 and db<=128) if label=='regular' else (nb>192 and db>128),label+': GCD scope')
    require(g==1 if label=='coprime' else True,label+': gcd1')
    gcd_controls.append({'name':label,'points':P,'N':N,'D':D,'gcd':g,'reduced_num_bits':nb,
                         'reduced_den_bits':db,'barycentric':[str(w) for w in weights]})

R=Path('/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v10')
source_paths=('src/cloud/site_tree.cpp','src/cloud/site_tree.hpp','src/catalogue/generator.cpp','src/tower/tower.cpp','src/arith/geometry.cpp','src/arith/geometry.hpp')
source_hashes={p:hashlib.sha256((R/p).read_bytes()).hexdigest() for p in source_paths}
result={'status':'PASS' if not failures else 'FAIL','checks':checks,'failures':failures,
        'scope':'scalar binary64 RN + Fraction; no engine/native/GCP qualification',
        'shell_cases':cases,'nearest_tie':nearest,'catalogue_translation_checks':catalogue_checks,
        'gcd_controls':gcd_controls,'source_sha256':source_hashes}
payload=json.dumps(result,sort_keys=True,indent=2)+'\n'
if len(sys.argv)==2: Path(sys.argv[1]).write_text(payload)
else: print(payload,end='')
raise SystemExit(0 if not failures else 1)
