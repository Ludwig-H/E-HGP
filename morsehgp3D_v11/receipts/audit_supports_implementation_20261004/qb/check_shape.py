#!/usr/bin/env python3
"""C++20 u32/i32 scalar replay of the captured public Shape factory and counts.
No native code or C++ parser/interpreter is run. Guarded source text is pinned.
"""
from math import comb
from pathlib import Path
import hashlib,json,re

ROOT=Path(__file__).resolve().parent
U32=(1<<32)-1
CHECKS=0

def need(condition,message):
    global CHECKS
    CHECKS+=1
    if not condition:raise RuntimeError(message)

def u32(x):return x&U32

def i32(x):
    x=u32(x)
    return x-(1<<32) if x&(1<<31) else x

def binomial(x,y):
    if y<0 or y>x:return 0
    # Captured helper indexes the fixed table after only the first guard.
    # This witness remains within the table, so no out-of-range assumption is used.
    need(0<=x<=35 and 0<=y<=35,'scalar table index is out of range')
    return u32(comb(x,y))

def factory(p,m,q,k):
    if m>24:return 'support_shell_capacity',None
    if k<1 or k>12 or q<2 or q>4 or m<q or u32(p+q)>u32(k+1):
        return 'supports_invariant',None
    return 'ok',{'p':p,'m':m,'q':q,'k':k}

def fixed_factory(p,m,q,k):
    if m>24:return 'support_shell_capacity',None
    if k<1 or k>12 or q<2 or q>4 or m<q or p+q>k+1:
        return 'supports_invariant',None
    return 'ok',{'p':p,'m':m,'q':q,'k':k}

def counts(shape,closure):
    p=i32(shape['p']);m=i32(shape['m']);q=i32(shape['q']);k=shape['k'];t=i32(u32(k-shape['p']))
    def parts(j):return closure[j] if 0<=j<=m else 0
    if len(closure)!=m+1 or parts(q)==0 or parts(m)!=1:return 'supports_invariant',None
    for j in range(m+1):
        if parts(j)>binomial(m,j) or j<q and parts(j)!=0:return 'supports_invariant',None
    result={'kparties_reliees':binomial(p+m,k),'compressed_parts':binomial(m,t)}
    result['strict_traces']=u32(result['compressed_parts']-parts(u32(t)))
    cofaces=sum(binomial(p,k+1-j)*parts(j) for j in range(q,m+1))
    if cofaces>U32:return 'supports_invariant',None
    result['cofaces']=cofaces;result['gabriel_cofaces']=parts(u32(t)+1)
    return 'ok',result

def support_cofaces(shape,arity):
    a=i32(arity)
    return binomial(i32(u32(shape['p']+shape['m']))-a,shape['k']+1-a)

def main():
    paths=['sources/src/supports/counts.cpp','sources/src/supports/counts.hpp','sources/src/supports/supports.hpp']
    expected=['635d9b44f89d6c139265211a3121d687c90348fd719580807890385325daa2c1','980a47bafe579b6874b4c207a5fc9cf63051e21ee22167114e7751677a144ae5','b732a08ea6ba89fa175df639a3f55b5100658bb729d7def8b7b5d0437d7282ec']
    for rel,sha in zip(paths,expected):need(hashlib.sha256((ROOT/rel).read_bytes()).hexdigest()==sha,'source hash changed '+rel)
    cpp=(ROOT/paths[0]).read_text();hpp=(ROOT/paths[1]).read_text()
    need(re.search(r'if \(k < 1 \|\| k > kMaxOrder \|\| q < 2 \|\| q > 4 \|\| m < q \|\| p \+ q > u32\{k\} \+ 1\)',cpp) is not None,'factory guard no longer matches captured vulnerable code')
    for text in ['const i32 p = static_cast<i32>(shape.interior())','counts.kparties_reliees = supports_detail::binomial(p + m, k)','counts.compressed_parts = supports_detail::binomial(m, t)','cofaces += u64{supports_detail::binomial(p, k + 1 - j)}']:
        need(text in cpp,'count replay guard changed '+text)
    need('return u32{k_} - p_;' in hpp,'Shape t replay changed')
    need('if (y < 0 || y > x) return 0;' in hpp,'table guard changed')
    need('static_cast<i32>(s.interior() + s.shell()) - a' in hpp,'support_cofaces unsigned addition changed')

    params={'p':U32,'m':2,'q':2,'k':1}
    status,shape=factory(**params)
    need(status=='ok','witness should be accepted by captured code')
    need(u32(params['p']+params['q'])==1,'native unsigned32 guard wrap')
    fixed_status,_=fixed_factory(**params)
    need(fixed_status=='supports_invariant','widened guard must refuse witness')
    actual_status,actual=counts(shape,[0,0,1])
    need(actual_status=='ok','counts consequence should publish without refusal')
    need(actual=={'kparties_reliees':1,'compressed_parts':1,'strict_traces':0,'cofaces':0,'gabriel_cofaces':0},'captured count consequence')
    exact_parts=comb(params['p']+params['m'],params['k'])
    need(exact_parts==4294967297 and exact_parts!=actual['kparties_reliees'],'mathematical count mismatch')
    need(support_cofaces(shape,2)==0,'captured per-support consequence')
    exact_support=comb(params['p']+params['m']-2,params['k']+1-2)
    need(exact_support==1,'mathematical support count')
    invalid=[]
    for k in (1,2,12):
        for q in (2,3,4):
            for p in range(U32-3,U32+1):
                status,s=factory(p,q,q,k)
                corrected,_=fixed_factory(p,q,q,k)
                need(corrected=='supports_invariant','large p must be rejected')
                if status=='ok':invalid.append({'p':p,'m':q,'q':q,'k':k,'wrapped_sum':u32(p+q)})
    need(len(invalid)>0,'missing invalid-Shape cases')
    valid=0
    for k in range(1,13):
        for p in range(k):
            for q in (2,3,4):
                for m in (q,24):
                    before,_=factory(p,m,q,k);after,_=fixed_factory(p,m,q,k)
                    need(before==after,'widening changes a bounded case')
                    valid+=before=='ok'
    result={'status':'PASS','checks':CHECKS,'head_base':'f98aeed67d4030dd78e11d5faf7d8556c4d17aaf','state':'uncommitted S6 snapshot','native_executed':False,'scope':'Scalar replay on the intended GCC/Clang x86-64 ABI (u32 unsigned arithmetic, i32 signed conversion). Not native execution or a geometric/cloud counterexample.','witness':{'params':params,'factory_status':'ok','wrapped_p_plus_q':1,'Shape_t':u32(1-U32),'counts_status':actual_status,'counts_published':actual,'mathematical_k_parts':exact_parts,'support_cofaces_published':0,'mathematical_support_cofaces':1,'recommended_widened_guard_status':fixed_status},'other_invalid_shapes_accepted':invalid,'bounded_domain_cases_retained':valid,'source_sha256':dict(zip(paths,expected)),'limits':['Protected ball_shape/Catalogue path has p<=K-1 and does not reach this malformed input.','The factory is public and documented to certify the Shape domain.','No huge allocation, C++ build, native test, workflow, fit or G4 was run.']}
    print(json.dumps(result,sort_keys=True,indent=2))

if __name__=='__main__':main()
