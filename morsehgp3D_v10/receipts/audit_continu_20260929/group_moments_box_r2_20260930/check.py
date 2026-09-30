"""Private Fraction isotropic witness and source-rule scope audit; no engine."""
from fractions import Fraction as F
from itertools import product
import json

CHECKS=0
def require(ok,msg):
    global CHECKS
    CHECKS+=1
    if not ok: raise ValueError(msg)
def sub(a,b):return tuple(x-y for x,y in zip(a,b))
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def norm2(a):return dot(a,a)
def power(z,a,c):return norm2(sub(z,c))-norm2(sub(a,c))
def corners(Q):return tuple(product(*Q))
def det(u,v,w):return u[0]*(v[1]*w[2]-v[2]*w[1])-u[1]*(v[0]*w[2]-v[2]*w[0])+u[2]*(v[0]*w[1]-v[1]*w[0])
def enc(v):
    if isinstance(v,F):return str(v)
    if isinstance(v,dict):return {k:enc(x) for k,x in v.items()}
    if isinstance(v,(tuple,list)):return [enc(x) for x in v]
    return v
def initial_boxes(P):
    # Exact source root/crop rules, in T6; no filtering run is being impersonated.
    X=[tuple(int(x)*64 for x in p) for p in P]
    lo=tuple(min(x[k] for x in X) for k in range(3))
    hi=tuple(max(x[k] for x in X) for k in range(3))
    ext=max(hi[k]-lo[k] for k in range(3));side=1
    while side<=ext:side*=2
    root=tuple((lo[k],lo[k]+side) for k in range(3))
    crop=tuple((max(root[k][0],lo[k]),min(root[k][1],hi[k]+1)) for k in range(3))
    for i,a in enumerate(P):
        require(all(F(l,64)<=a[k]<F(h,64) for k,(l,h) in enumerate(root)),"site outside root")
        require(all(power(z,a,a)>=0 for z in P),"site has universal root dominator")
        require(all(l<=X[i][k]<=h for k,(l,h) in enumerate(crop)),"anchor not in closed root crop")
    return dict(root_T6=root,crop_T6=crop,widths_T6=[h-l for l,h in crop],n=len(P),first_leaf_K5=len(P)<=16)
def certificate(P,ids,a,Q):
    W=len(ids);M=tuple(sum(P[i][k] for i in ids) for k in range(3));V=sum(norm2(P[i]) for i in ids)
    def S(c):return V-W*norm2(a)-2*dot(sub(M,tuple(W*x for x in a)),c)
    sigma=max(S(c) for c in corners(Q));R2=max(norm2(sub(a,c)) for c in corners(Q))
    ratio=-sigma/R2;credit=-((-ratio.numerator)//ratio.denominator) if sigma<0 else 0
    for c in corners(Q)+tuple(tuple(F(l)+F(h-l)*t for l,h in Q) for t in(F(1,7),F(2,5),F(1,2),F(2,3))):
        direct=sum(power(P[i],a,c) for i in ids)
        mass=sum(power(P[i],a,c)<0 for i in ids)
        require(direct==S(c),"moment identity")
        require(direct<=sigma,"affine maximum")
        require(norm2(sub(a,c))<=R2,"radius maximum")
        require(direct>=-R2*mass and mass>=credit,"unsafe moment mass")
    return dict(sigma=sigma,R2=R2,credit=credit,threshold2_slack=sigma+2*R2)
def main():
    raw=[(-4226,0,0),(4224,130,0),(4224,-78,104),(4224,-78,-104)]
    raw.extend((0,sy*y,sz*z) for y,z in((1690,1690),(1689,1691),(1691,1689)) for sy,sz in product((-1,1),repeat=2))
    P=[tuple(F(x+5000) for x in p) for p in raw];ids=tuple(range(4,16));c=(F(5000),)*3;Q=((4180,5820),)*3
    require(len(P)==16 and len(set(P))==16,"sixteen distinct sites")
    require(all(x.denominator==1 and 0<=x<=262143 for p in P for x in p),"u18 input")
    require(len(set(h-l for l,h in Q))==1 and all(l<c[k]<h for k,(l,h) in enumerate(Q)),"not isotropic / centre outsideQ")
    bary=(F(2112,4225),F(6339,33800),F(2113,13520),F(2113,13520))
    require(all(x>0 for x in bary) and sum(bary)==1,"positive support weights")
    require(tuple(sum(bary[i]*P[i][k] for i in range(4)) for k in range(3))==c,"positive barycentric centre")
    determinant=det(sub(P[1],P[0]),sub(P[2],P[0]),sub(P[3],P[0]))
    require(determinant!=0,"degenerate tetrahedron")
    powers=[power(z,P[0],c) for z in P];I=[i for i,v in enumerate(powers) if v<0];U=[i for i,v in enumerate(powers) if v==0]
    require(I==list(ids) and U==list(range(4)),"strict global census/shell")
    require(norm2(sub(P[0],c))==4226**2,"radius2")
    dom=[];certs=[]
    expected_sigma=(-62594816,-60075776,-59052416,-59052416)
    expected_R=(26806916,27016836,27102116,27102116)
    for a in range(4):
        row=[i for i,z in enumerate(P) if max(power(z,P[a],v) for v in corners(Q))<0]
        require(row==[],"old individual dominance already rejects")
        dom.append(row);cert=certificate(P,ids,P[a],Q)
        require(cert['sigma']==expected_sigma[a] and cert['R2']==expected_R[a],"wrong closed-box extrema")
        require(cert['credit']==3 and cert['threshold2_slack']<0,"K5q4 certificate")
        certs.append(cert)
    initial=initial_boxes(P)
    actualQ=tuple((F(l,64),F(h,64)) for l,h in initial['crop_T6'])
    actual=[certificate(P,ids,P[a],actualQ) for a in range(4)]
    require(initial['first_leaf_K5'] and all(x['sigma']>=0 and x['credit']==0 for x in actual),"false default-leaf gain")
    old=[(0,1,9),(24,1,9),(12,19,9),(12,6,29)]+[(12,6+y,9+9*s) for y in(-6,-3,0,3,6) for s in(-1,1)]
    old_box=initial_boxes(old)
    require(old_box['first_leaf_K5'] and old_box['widths_T6']==[1537,1217,1857],"old14 actual root crop")
    line_box=initial_boxes([(1,2,0),(1,2,262143)])
    require(line_box['first_leaf_K5'] and line_box['widths_T6']==[1,1,16777153],"real anisotropic source-rule leaf")
    print(json.dumps(enc(dict(status="PASS",checks=CHECKS,scope="Fraction geometry/source-rule mirror only; zero native/generator/GCP",
         points=P,Q=Q,centre=c,radius2=4226**2,bary=bary,determinant=determinant,I=I,U=U,Dom_supports=dom,certificates=certs,
         default_isotropic_fixture_initial_box=initial,default_initial_certificates=actual,old14_initial_box=old_box,
         two_site_vertical_initial_box=line_box)),sort_keys=True,indent=2))
if __name__=='__main__':main()
