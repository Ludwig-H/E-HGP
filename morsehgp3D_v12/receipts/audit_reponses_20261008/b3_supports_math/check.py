#!/usr/bin/env python3
"""B3 : arithmétique des clés et fixture synthétique ; aucun moteur natif."""
from pathlib import Path
import argparse,bisect,hashlib,itertools,json,subprocess
HERE=Path(__file__).resolve().parent
NONE=(1<<32)-1

def need(x,why):
    if not x: raise ValueError(why)

def sha(raw):return hashlib.sha256(raw).hexdigest()

def key(s,n):
    bits=max(1,n.bit_length())
    digits=(*s,*([n]*(4-len(s))))
    value=sum(c*(1<<(bits*(3-i))) for i,c in enumerate(digits))
    need(0<=value<1<<128,'128-bit key')
    recovered=tuple((value>>(bits*(3-i)))&((1<<bits)-1) for i in range(4))
    need(recovered==digits,'injective representation')
    return (value>>64,value&((1<<64)-1))

def old(s):return (*s,*([NONE]*(4-len(s))))

def valid(s,n):return 2<=len(s)<=4 and all(a<b for a,b in zip(s,s[1:])) and 0<=s[0] and s[-1]<n

def model():
    supports=queries=positive=negative=0
    for n in range(2,11):
        universe=[s for q in (2,3,4) for s in itertools.combinations(range(n),q)]
        need(sorted(universe,key=old)==sorted(universe,key=lambda s:key(s,n)), 'lexicographic order')
        need(len(set(key(s,n) for s in universe))==len(universe),'key collision')
        # A sparse immutable table, leaving real valid misses among all support arities.
        chosen=[s for i,s in enumerate(universe) if i%3!=1]
        indexed=sorted((key(s,n),i) for i,s in enumerate(chosen))
        keys=[k for k,i in indexed]
        for s in universe:
            pos=bisect.bisect_left(keys,key(s,n))
            got=indexed[pos][1] if pos<len(keys) and keys[pos]==key(s,n) else None
            want=chosen.index(s) if s in chosen else None
            need(got==want,'lookup differs from direct support identity')
            queries+=1;positive+=want is not None;negative+=want is None
        supports+=len(universe)
        for bad in [(0,),tuple(range(5)),(0,0),(1,0),(0,n),(0,1,n)]:
            need(not valid(bad,n),'bad support admitted')
    boundaries=[]
    for n in [2,3,4,7,8,15,16,255,256,65535,65536,(1<<21),(1<<31)-1,1<<31,NONE-1,NONE]:
        candidates=[(0,n-1)]
        if n>=3:candidates.append((0,1,n-1))
        if n>=4:candidates.append((0,1,2,n-1))
        ordered=sorted(set(candidates),key=old)
        need(ordered==sorted(ordered,key=lambda s:key(s,n)),'boundary order')
        for s in ordered:key(s,n)
        boundaries.append({'sites':n,'bits':max(1,n.bit_length()),'supports':len(ordered)})
    # Removing the out-of-cloud check would alias an absent digit with a queried site.
    need(key((0,1),3)==key((0,1,3),3),'sentinel alias witness')
    points=[(x,y,z) for x in range(-13,14) for y in range(-13,14) for z in range(-13,14)
            if x*x+y*y+z*z==169]
    representatives=sorted(p for p in points if p<tuple(-v for v in p))
    need(len(representatives)>=32,'64 integral shell points unavailable')
    shell=sorted(p for a in representatives[:32] for p in (a,tuple(-v for v in a)))
    need(len(set(shell))==64 and all(sum(v*v for v in p)==169 for p in shell),'shell geometry')
    anchor=representatives[0];opposite=tuple(-v for v in anchor)
    need(anchor in shell and opposite in shell and all(a+b==0 for a,b in zip(anchor,opposite)), 'diameter support')
    outside=(14,0,0);need(sum(v*v for v in outside)>169,'outside witness')
    extra=next(p for p in points if p not in shell)
    counts=[]
    for k in range(2,13):
        inside=[(i,0,0) for i in range(k-1)]
        need(len(set(inside))==k-1 and all(sum(v*v for v in p)<169 for p in inside),'interior geometry')
        population=inside+shell
        for f in [population[-k:],population[:k],[outside]+population[-(k-1):]]:
            separate=all(p in inside or p in shell for p in f)
            linear=all(p in population for p in f)
            need(separate==linear,'I union U membership')
        counts.append({'K':k,'interior':k-1,'shell':64,'row':k+63,'comparisons_upper':k*(k+63)})
    shift=lambda p:[v+16 for v in p]
    fixture={'center':[16,16,16],'radius_squared':169,'shell_64':[shift(p) for p in shell],
        'diameter_support':[shift(anchor),shift(opposite)],'K5_interior':[shift((i,0,0)) for i in range(4)],
        'interior_for_K':'[(16+i,16,16) for i in range(K-1)], 2 <= K <= 12',
        'outside':shift(outside),'65th_shell_refusal_candidate':shift(extra),
        'smaller_shells':'first 1, 2, 4 pairs of the same lexicographic antipodal representatives',
        'integral_shell_candidates':len(points)}
    return dict(exhaustive_supports=supports,queries=queries,positive=positive,negative=negative,
        boundaries=boundaries,missing_range_guard_alias={'sites':3,'stored':[0,1],'invalid_query':[0,1,3]},
        membership_bounds=counts,fixture=fixture,native_execution=False,performance_measurement=False)

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--repo',type=Path,required=True);p.add_argument('--snapshot',type=Path,required=True)
    p.add_argument('--write',action='store_true');a=p.parse_args()
    capture=json.loads((HERE/'capture.json').read_text())
    for rel,pins in capture['sources'].items():
        before=subprocess.check_output(['git','-C',str(a.repo),'show',capture['base_git']+':morsehgp3D_v12/'+rel])
        after=(a.snapshot/'after/morsehgp3D_v12'/rel).read_bytes()
        need(sha(before)==pins['before'] and sha(after)==pins['after'],'source pin '+rel)
    result=model()
    if a.write:(HERE/'results.json').write_text(json.dumps(result,ensure_ascii=False,separators=(',',':'))+'\n')
    else:need(result==json.loads((HERE/'results.json').read_text()),'model result changed')
    print(json.dumps({k:result[k] for k in ['exhaustive_supports','queries','positive','negative','native_execution']}))

if __name__=='__main__':main()
