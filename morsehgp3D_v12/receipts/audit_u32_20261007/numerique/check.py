#!/usr/bin/env python3
"""Oracle Fraction autonome : systemes lineaires, aucune formule de centre du produit importee."""
from fractions import Fraction as F
from pathlib import Path
import argparse, collections, hashlib, itertools, json, random, subprocess
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[3]
PIN='c3de9d73d8999f2f1e31a0f592b829efc6e7a4da'
MAX=(1<<32)-1

def require(ok,message):
    if not ok: raise RuntimeError(message)
def sha(data): return hashlib.sha256(data).hexdigest()
def sub(a,b): return tuple(x-y for x,y in zip(a,b))
def dot(a,b): return sum(x*y for x,y in zip(a,b))
def cross(a,b): return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def sign(x): return (x>0)-(x<0)
def solve(rows,rhs):
    a=[list(map(F,row))+[F(r)] for row,r in zip(rows,rhs)]
    for j in range(3):
        piv=next((i for i in range(j,3) if a[i][j]),None)
        if piv is None:return None
        a[j],a[piv]=a[piv],a[j]
        d=a[j][j];a[j]=[v/d for v in a[j]]
        for i in range(3):
            if i!=j:
                d=a[i][j];a[i]=[x-d*y for x,y in zip(a[i],a[j])]
    return tuple(row[-1] for row in a)
def center(p):
    if len(p)==2:return tuple(F(x+y,2) for x,y in zip(*p)) if p[0]!=p[1] else None
    rows=[sub(v,p[0]) for v in p[1:]]
    rhs=[F(dot(v,v),2) for v in rows]
    if len(rows)==2:rows.append(cross(*rows));rhs.append(F(0))
    v=solve(rows,rhs)
    return None if v is None else tuple(x+y for x,y in zip(p[0],v))
def domain(d,n,orientation=False):
    accepted=[]
    for t in range(42 if orientation else 62):
        pd=124-3*t if orientation else 123-2*t
        pn=124-2*t if orientation else 124-t
        if 0<d<(1<<pd) and all(abs(v)<(1<<pn) for v in n):accepted.append(t)
    return max(accepted,default=-1)
def cases():
    result=[]
    def add(name,p,queries=None,translate=True):
        queries=queries or [p[0],p[1],p[-1],tuple(max(v[j] for v in p) for j in range(3))]
        allp=p+queries
        shift=tuple(MAX-max(v[j] for v in allp) for j in range(3))
        for tag,t in [('base',(0,0,0))]+([('high',shift)] if translate else []):
            pts=[tuple(x+y for x,y in zip(v,t)) for v in p]
            qs=[tuple(x+y for x,y in zip(v,t)) for v in queries]
            require(all(0<=x<=MAX for v in pts+qs for x in v),'generated domain')
            result.append((name+'_'+tag,pts,qs))
    for s in [1,7,14,16,17,19,20,21,24,25,30,31,32]:
        h=(1<<s)-1
        add(f'q2_s{s}',[(0,0,0),(h,h,h)])
        tri=[(0,0,h),(h,0,0),(0,h,0)]
        add(f'q3_s{s}',tri)
        add(f'q3_reanchor_s{s}',tri[1:]+tri[:1])
        add(f'q4_s{s}',[(0,0,0),(0,h,h),(h,0,h),(h,h,0)])
    h=(1<<20)-1
    add('CST0201_s20',[(0,0,h),(h,0,0),(0,h,0)],[(0,0,h),(h,0,0),(0,h,0),((3<<20)-1,)*3])
    add('near_collinear_u32',[(0,0,0),(MAX,MAX-1,0),(MAX-1,MAX-2,0)],[(0,0,0),(MAX,0,0),(0,MAX,0),(0,0,MAX)])
    add('outside_candidate',[(419,0,0),(435,15,0),(434,14,0)],[(0,479,0),(419,0,0),(435,15,0),(434,14,0)])
    add('midpoint_q3',[(0,0,0),(100,100,0),(0,100,0)])
    for s in [24,25,31]:
        h=(1<<s)-2
        add(f'midpoint_q3_s{s}',[(0,0,0),(h,h,0),(0,h,0)])
        add(f'midpoint_q4_s{s}',[(0,h//2,h//2),(h,h//2,h//2),(h//2,0,h//2),(h//2,h//2,0)])
    add('degenerate3',[(0,0,0),(1,1,1),(2,2,2)])
    add('degenerate4',[(0,0,0),(2,0,0),(0,2,0),(2,2,0)])
    rng=random.Random(20261007)
    for i in range(48):
        s=[16,17,20,24,25,31,32][i%7]; q=3+i%2; h=(1<<s)-1
        p=[tuple(rng.randrange(h+1) for _ in range(3)) for _ in range(q)]
        queries=[p[0],p[1],tuple(rng.randrange(h+1) for _ in range(3)),tuple(rng.randrange(h+1) for _ in range(3))]
        add(f'random_{i}',p,queries)
    return result

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--binary',required=True);parser.add_argument('--output',required=True);args=parser.parse_args()
    rels=sorted([p.relative_to(ROOT).as_posix() for base in ['morsehgp3D_v12/src/num','morsehgp3D_v12/src/core'] for p in (ROOT/base).glob('*') if p.suffix in ['.cpp','.hpp','.def']])
    rels+=['morsehgp3D_v12/docs/CONTRAT_NUMERIQUE.md','morsehgp3D_v12/receipts/developpement_20261007/numerique_socle.md']
    before={p:sha((ROOT/p).read_bytes()) for p in rels}
    for p in rels:
        raw=subprocess.run(['git','show',PIN+':'+p],cwd=ROOT,check=True,capture_output=True).stdout
        require(sha(raw)==before[p],'source differs from pin '+p)
    binary=Path(args.binary);binary_hash=sha(binary.read_bytes())
    data=cases(); inp=''.join(str(len(p))+' '+' '.join(map(str,itertools.chain.from_iterable(p+[(0,0,0)]*(4-len(p))+queries)))+'\n' for _,p,queries in data)
    proc=subprocess.run([str(binary)],input=inp,text=True,capture_output=True,timeout=30)
    require(proc.returncode==0 and not proc.stderr,'probe process')
    lines=proc.stdout.splitlines();require(len(lines)==len(data),'record count')
    aggregate=collections.Counter(); witnesses={};previous=None;degenerate=0
    for (name,p,queries),line in zip(data,lines):
        c=center(p)
        if c is None:
            require(line=='degenerate','degenerate '+name);degenerate+=1;continue
        tok=line.split(); require(tok.pop(0)=='ok','sphere '+name)
        n=[int(tok.pop(0),16) for _ in range(3)];d=int(tok.pop(0),16)
        ln=int(tok.pop(0),16);ld=int(tok.pop(0),16)
        span,powerdom,orientdom,inside=[int(tok.pop(0)) for _ in range(4)]
        def lane(label):
            values=[int(tok.pop(0)) for _ in range(4)]
            for k,v in zip(['native','certified','checked','wide'],values):aggregate[label+'_'+k]+=v
            return values
        cl=lane('construction')
        require(tuple(F(x,y)+o for x,y,o in zip(n,[d]*3,p[0]))==c,'center '+name)
        radius=dot(sub(c,p[0]),sub(c,p[0])); require(F(ln,ld)==radius,'radius '+name)
        expected_span=max(max(v[j] for v in p)-min(v[j] for v in p) for j in range(3)).bit_length()
        require(span==expected_span,'span '+name)
        # The factories only grant domains when coefficients fit in signed i128.
        fits=0<d<(1<<127) and all(abs(v)<(1<<127) for v in n)
        require(powerdom==(domain(d,n) if fits else -1),'power domain '+name)
        require(orientdom==(domain(d,n,True) if fits else -1),'orientation domain '+name)
        if len(p)==4:
            cols=[sub(v,p[0]) for v in p[1:]];bary=solve(list(zip(*cols)),sub(c,p[0]))
            require(bool(inside)==(all(x>0 for x in bary) and sum(bary)<1),'q4 positivity '+name)
        else: require(not inside,'non q4 inside '+name)
        qlanes=[];quadratic=[]
        for query in queries:
            actual=int(tok.pop(0),16);qlanes.append(lane('power'))
            expected=d*(dot(sub(query,c),sub(query,c))-radius)
            require(actual==expected,'power '+name)
            quadratic.append(d*dot(sub(query,p[0]),sub(query,p[0])))
        orient=int(tok.pop(0));ol=lane('orientation')
        require(orient==sign(dot(cross(sub(queries[1],queries[0]),sub(queries[2],queries[0])),sub(c,queries[0]))),'orientation '+name)
        mid=int(tok.pop(0));ml=lane('midpoint')
        require(bool(mid)==all(2*x==a+b for x,a,b in zip(c,queries[0],queries[1])),'midpoint '+name)
        co=int(tok.pop(0));col=lane('center_compare');lo=int(tok.pop(0));lol=lane('level_compare')
        require(co==(sign((c>previous[0])-(c<previous[0])) if previous else 0),'center order '+name)
        require(lo==(sign(radius-previous[1]) if previous else 0),'level order '+name)
        require(not tok,'extra fields')
        if name.startswith(('CST0201','near_collinear','midpoint_q3','q3_s24','q3_s25','q4_s32')):
            witnesses[name]={'support_span':span,'center':[str(x) for x in c],'radius2':str(radius),'domains':[powerdom,orientdom],'construction_lanes':cl,'power_lanes':qlanes,'quadratic_bits':[abs(x).bit_length() for x in quadratic],'orientation_lanes':ol,'midpoint':bool(mid),'midpoint_lanes':ml,'center_order_lanes':col,'level_order_lanes':lol}
        previous=(c,radius)
    require(all(sha((ROOT/p).read_bytes())==h for p,h in before.items()),'sources changed')
    require(sha(binary.read_bytes())==binary_hash,'binary changed')
    witness=witnesses['CST0201_s20_base']
    require(witness['domains'][0]==20 and witness['quadratic_bits'][-1]>127,'counterexample strength')
    require(witness['power_lanes'][-1]==[0,0,0,1],'counterexample must take exact fallback')
    require(abs(F(witnesses['near_collinear_u32_base']['center'][0]))>(1<<63),'wide center witness')
    result={'pin':PIN,'public_status':'not_claimed','profile':32,'source_hashes_before_after':before,'audit_hashes':{p.name:sha(p.read_bytes()) for p in [HERE/'probe.cpp',HERE/'check.py']},'binary_sha256':binary_hash,'input_sha256':sha(inp.encode()),'output_sha256':sha(proc.stdout.encode()),'cases':len(data),'degenerate':degenerate,'valid_spheres':len(data)-degenerate,'power_queries':4*(len(data)-degenerate),'lane_totals':dict(sorted(aggregate.items())),'witnesses':witnesses,'result':'pass'}
    Path(args.output).write_text(json.dumps(result,indent=2,sort_keys=True)+'\n')
    print(json.dumps({'result':'pass','cases':len(data),'degenerate':degenerate,'power_queries':result['power_queries']}))
if __name__=='__main__':main()
