import sys; sys.path.insert(0,'.')
from extract import load
def ends(sk, lk, top_div=1.0, all_div=1.0, s_div=1.0):
    K=len(sk); out=[]
    for k in range(1,K+1):
        ready=sum(sk[j-1] for j in range(k,K+1))/s_div if k>1 else 0.0
        a=lk[k-1]/all_div
        if k==K: a=a/top_div
        out.append(ready+a)
    return out
CASES=[('00',5,[0,13]),('01',5,[4]),('02',5,[8]),('b00',5,[24]),('b01',5,[28]),('b02',5,[32]),('00',10,[2,15]),('01',10,[6]),('02',10,[10]),('b00',10,[26]),('b01',10,[30]),('b02',10,[34])]
print('| case | probe | window meas | model | |err| | ends K1..K | binding | 2nd | slack | A(K) | static(K) | ready(2)=Σstatic | A(1) | A(K)÷2 | A(K)=0 | all A÷2 | all A÷4 | all A÷4 + phase0÷2 |')
print('|'+'---|'*19)
for f,K,ids in CASES:
    for i in ids:
        d=load('r22',i); tp=d['tower_phases_ms']; sk=tp['static_by_k']; lk=tp['lots_by_k']
        e=ends(sk,lk); w=tp['static']+tp['lots']; m=max(e)
        order=sorted(range(K),key=lambda j:-e[j])
        b=order[0]+1; s2=order[1]+1
        print(f'| {f} K{K} | {i} | {w:.1f} | {m:.1f} | {abs(w-m):.2f} | {", ".join(f"{x:.0f}" for x in e)} | K{b} | K{s2} | {e[order[0]]-e[order[1]]:.1f} | {lk[-1]:.1f} | {sk[-1]:.1f} | {sum(sk):.1f} | {lk[0]:.1f} | {max(ends(sk,lk,top_div=2)):.1f} | {max(ends(sk,lk,top_div=1e9)):.1f} | {max(ends(sk,lk,all_div=2)):.1f} | {max(ends(sk,lk,all_div=4)):.1f} | {max(ends(sk,lk,all_div=4,s_div=2)):.1f} |')
