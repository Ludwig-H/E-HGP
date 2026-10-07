#!/usr/bin/env python3
"""Small independent exact check of the committed u32 numerical/index socle."""
from fractions import Fraction as F
from pathlib import Path
import hashlib, itertools, json, random, subprocess, sys, tempfile
ROOT=Path(__file__).resolve().parents[3]
HERE=Path(__file__).resolve().parent
PIN='c3de9d73d8999f2f1e31a0f592b829efc6e7a4da'
MAX=(1<<32)-1

def require(p,msg):
    if not p: raise RuntimeError(msg)
def digest(b): return hashlib.sha256(b).hexdigest()
def morton(p): return sum(((p[a]>>i)&1)<<(3*i+a) for i in range(32) for a in range(3))
def dist(a,b): return sum((x-y)**2 for x,y in zip(a,b))
def model(s):
    if len(s)==1: return tuple(map(F,s[0])),F(0),True
    a=s[0]; v=[tuple(y-x for x,y in zip(a,b)) for b in s[1:]]
    dot=lambda x,y:sum(i*j for i,j in zip(x,y))
    m=[[F(dot(u,w)) for w in v]+[F(dot(u,u),2)] for u in v]
    for i in range(len(v)):
        pivot=next((j for j in range(i,len(v)) if m[j][i]),None)
        if pivot is None: return None
        m[i],m[pivot]=m[pivot],m[i]; d=m[i][i];m[i]=[x/d for x in m[i]]
        for j in range(len(v)):
            if j!=i:
                k=m[j][i];m[j]=[x-k*y for x,y in zip(m[j],m[i])]
    w=[r[-1] for r in m]; c=tuple(F(a[j])+sum(w[i]*v[i][j] for i in range(len(v))) for j in range(3))
    return c,dist(c,a),all(x>0 for x in w) and sum(w)<1

def sign(x):return (x>0)-(x<0)
def bounds(c,r,lo,hi):
    near=[]
    for x,l,h in zip(c,lo,hi):
        f=x.numerator//x.denominator
        near.append(min((max(l,min(h,f)),max(l,min(h,f+1))),key=lambda y:abs(x-y)))
    lower=sign(dist(c,near)-r)
    upper=sign(max(dist(c,p)-r for p in itertools.product(*zip(lo,hi))))
    return lower, 1 if lower>0 else upper

def command(op,values):return op+' '+' '.join(str(v) for v in values)+'\n'
def flat(points):return [v for p in points for v in p]

def run():
    sourcefiles=sorted(p for d in ['core','num','cloud','index'] for p in (ROOT/'src'/d).glob('*') if p.is_file())
    before={str(p.relative_to(ROOT)):digest(p.read_bytes()) for p in sourcefiles}
    for p in sourcefiles:
        rel='morsehgp3D_v12/'+str(p.relative_to(ROOT))
        require(p.read_bytes()==subprocess.check_output(['git','show',PIN+':'+rel],cwd=ROOT),'source differs from pin '+rel)
    rng=random.Random(20261007)
    inputs=[]; judges=[]; families={}; lanes=[0]*4; guard=[0]*3
    def add(op,values,judge):
        inputs.append(command(op,values));judges.append((op,judge));families[op]=families.get(op,0)+1
    for a,b in [((0,0,0),(MAX,MAX,MAX)),((0,0,0),(MAX,0,0)),((0,0,0),((1<<31)-1,)*3),((MAX,)*3,(MAX,)*3)]+[(tuple(rng.randrange(1<<32) for _ in range(3)),tuple(rng.randrange(1<<32) for _ in range(3))) for _ in range(48)]:
        def jd(t,a=a,b=b):
            require(int(t[0])==dist(a,b),'distance'); la=list(map(int,t[1:]));require(sum(la)==1,'distance lanes');expected=0 if max(abs(x-y) for x,y in zip(a,b))<(1<<31) else 2 if dist(a,b)<(1<<64) else 3;require(la[expected]==1,'distance lane exact');
        add('D',flat([a,b]),jd)
    positions=[tuple((1<<i) if j==a else 0 for j in range(3)) for i in range(32) for a in range(3)] + [(MAX,)*3,(0,0,0)] + [tuple(rng.randrange(1<<32) for _ in range(3)) for _ in range(30)]
    for p in positions: add('M',p,lambda t,p=p:require(int(t[0])==morton(p),'Morton 96 bits'))
    for s in [0,1,16,17,24,25,29,30,31,32,33]:
        h=min(MAX,(1<<s)-1); upper=(1<<32) if s==33 else h
        site=(h,)*3; lo=(0,)*3;hi=(min(1,upper),)*3
        add('R',flat([lo,(upper,)*3,site,lo,hi]),lambda t,s=s,site=site,lo=lo,hi=hi:require(int(t[0])==s and int(t[1])==sum((2*x-l-h)**2 for x,l,h in zip(site,lo,hi)),'reservoir/frame'))
    add('R',flat([(0,0,0),(4,4,4),(5,0,0),(0,0,0),(1,1,1)]),lambda t:require(t[1]=='parameter_out_of_range','uncovered site rejected'))
    add('R',flat([(0,0,0),(4,4,4),(1,0,0),(3,0,0),(2,1,1)]),lambda t:require(t[1]=='parameter_out_of_range','reversed box rejected'))
    clouds=[[(0,0,0,90),(1<<24,0,0,20),(1<<31,0,0,80),(0,0,0,10),(MAX,MAX,MAX,70)],[(0,0,0,5),(1<<21,0,0,4),(0,0,0,3),(1<<21,0,0,2)]]
    for cloud in clouds:
        for _ in range(3):
            cloud=list(cloud);rng.shuffle(cloud)
            expected=[]
            for p,g in itertools.groupby(sorted(cloud,key=lambda q:(morton(q[:3]),q[3])),key=lambda q:q[:3]):
                ids=[x[3] for x in g];expected+=list(p)+[len(ids)]+ids
            count=len(set(x[:3] for x in cloud))
            add('H',[len(cloud)]+flat(cloud),lambda t,e=[count]+expected:require(list(map(int,t))==e,'cloud exact sites and IDs'))
    base=[[(0,0,0)],[(0,0,0),(1,0,0)],[(0,0,0),(8,0,0)],[(0,0,0),(4,4,0),(4,0,4)],[(0,0,0),(6,1,0),(2,5,1)],[(0,0,0),(4,4,0),(4,0,4),(0,4,4)],[(0,0,0),(6,0,0),(0,6,0),(1,1,1)],[(0,0,0),(1,0,0),(0,1,0)],[(419,0,0),(435,15,0),(434,14,0)]]
    supports=[]
    for scale in [1,1<<13,1<<16,1<<21,1<<27]:
        for shape in base:
            raw=[tuple(scale*x for x in p) for p in shape]
            if max(flat(raw))>MAX:continue
            for edge in [False,True]:
                offset=tuple(MAX-max(p[j] for p in raw) if edge else 0 for j in range(3))
                supports.append([tuple(x+y for x,y in zip(p,offset)) for p in raw])
    supports.append([(0,0,0),(MAX,MAX,0),(MAX,0,MAX),(0,MAX,MAX)])
    supports.append([(0,0,0),(MAX,MAX,0),(MAX,0,MAX)])
    certs=uncerts=0
    for si,s in enumerate(supports):
        m=model(s);require(m is not None,'fixture independence');c,r,cert=m;certs+=cert;uncerts+=not cert
        low=tuple(min(p[j] for p in s) for j in range(3));high=tuple(max(p[j] for p in s) for j in range(3))
        center=tuple(max(0,min(MAX,x.numerator//x.denominator)) for x in c)
        pts=list(dict.fromkeys(s+[center,(0,0,0),(MAX,MAX,MAX),tuple(rng.randrange(1<<32) for _ in range(3))]))
        boxes=[(low,high),((0,0,0),(MAX,MAX,MAX)),(center,center),(pts[-1],pts[-1])]
        for p,(lo,hi) in itertools.product(pts,boxes):
            def jq(t,p=p,lo=lo,hi=hi,c=c,r=r,cert=cert,s=s):
                vals=list(map(int,t));expected=sign(dist(c,p)-r)
                require(vals[:2]==[int(cert),expected],'certification/generic point')
                if cert:
                    b=bounds(c,r,lo,hi);require(vals[2:5]==[expected,*b],'guarded side or integer bounds')
                    span=max(max(x[j] for x in s)-min(x[j] for x in s) for j in range(3)).bit_length()
                    require(vals[5]==span,'support span')
                    for i in range(3):guard[i]+=vals[6+i]
                    for i in range(4):lanes[i]+=vals[9+i]
            add('Q',[len(s)]+flat(s+[p,lo,hi]),jq)
        if si%4==0 or si==len(supports)-1:
            cp=list(dict.fromkeys(pts+[tuple(max(0,min(MAX,x.numerator//x.denominator+d)) for x in c) for d in (-1,1)]))
            for threshold,leaf in [(1,1),(2,3),(MAX,1),(MAX,8)]:
                ranked=sorted(enumerate(cp),key=lambda x:morton(x[1]));inside=[i for i,p in ranked if dist(c,p)<r];shell=[i for i,p in ranked if dist(c,p)==r];sat=len(inside)>=threshold
                ei=inside[:threshold];eu=[] if sat else shell
                def jc(t,cert=cert,cp=cp,ei=ei,eu=eu,sat=sat):
                    v=list(map(int,t));require(v[:6]==[len(cp),int(cert),1,int(sat),len(ei),len(eu)],'census shape and borrowed parity');require(v[6:6+len(ei)+len(eu)]==ei+eu,'census exact Morton-ordered populations')
                add('C',[len(s)]+flat(s)+[len(cp),leaf,threshold]+flat([(*p,i) for i,p in enumerate(cp)]),jc)
    units=['num/'+x+'.cpp' for x in ['sphere','predicates','centers','lattice','guard','local','big']]+['core/buffer.cpp','core/ledger.cpp','cloud/cloud.cpp','index/build.cpp','index/census.cpp','index/census_workspace.cpp']
    payload=''.join(inputs)
    with tempfile.TemporaryDirectory(prefix='ehgp-u32-index-audit-') as td:
        exe=Path(td)/'probe'
        argv=['g++','-std=c++20','-O2','-DNDEBUG','-DMHGP12_COORD_BITS=32','-I',str(ROOT/'src'),'-Wall','-Wextra','-Werror',str(HERE/'probe.cpp')]+[str(ROOT/'src'/p) for p in units]+['-o',str(exe)]
        build=subprocess.run(argv,text=True,capture_output=True,timeout=180)
        require(build.returncode==0,'compile failed '+build.stderr)
        run=subprocess.run([str(exe)],input=payload,text=True,capture_output=True,timeout=90)
        require(run.returncode==0 and not run.stderr,'probe failed '+run.stderr)
        lines=run.stdout.splitlines();require(len(lines)==len(judges),'response cardinality')
        for n,(line,(op,judge)) in enumerate(zip(lines,judges)):
            t=line.split();require(t[0]==op,'operation order')
            try:judge(t[1:])
            except Exception as e:raise RuntimeError(f'case {n} ({op}): {e}; input {inputs[n].strip()}; got {line}') from e
        compiler=subprocess.check_output(['g++','-dumpfullversion'],text=True).strip()
        binaryhash=digest(exe.read_bytes())
    after={str(p.relative_to(ROOT)):digest(p.read_bytes()) for p in sourcefiles}
    require(before==after,'source closure')
    require(all(x>0 for x in guard),'guard branch coverage');require(lanes[0]>0 and lanes[3]>0,'native and wide guard coverage')
    return {'pin':PIN,'profile':32,'compile':{'compiler':'g++ '+compiler,'flags':['-std=c++20','-O2','-DNDEBUG','-DMHGP12_COORD_BITS=32','-Wall','-Wextra','-Werror'],'translation_units':units,'binary_sha256':binaryhash},'sources_unchanged':True,'source_sha256':before,'audit_sha256':{p.name:digest(p.read_bytes()) for p in [HERE/'check.py',HERE/'probe.cpp']},'checks':len(judges),'families':families,'supports':{'certified':certs,'uncertified':uncerts},'guard_counts':dict(zip(['disjoint','partial','outside'],guard)),'guard_lane_counts':dict(zip(['native','certified','checked','wide'],lanes)),'input_sha256':digest(payload.encode()),'output_sha256':digest(run.stdout.encode()),'success':True}
if __name__=='__main__':
    print(json.dumps(run(),sort_keys=True,indent=2))
