import os, difflib, subprocess
root='/workspaces/E-HGP/build/v9-open-worktree'
def srcs(d):
    out=[]
    for dp,dn,fn in os.walk(d):
        for f in fn:
            if f.endswith(('.hpp','.cpp','.cu')):
                out.append(os.path.join(dp,f))
    return sorted(out)
def cmp(sub, other):
    base=os.path.join(root,'morsehgp3D_v9/src',sub)
    tot=0; same=0; same_nontriv=0; nontriv=0; missing=[]
    for p in srcs(base):
        rel=os.path.relpath(p,base)
        a=open(p,errors='ignore').read().splitlines()
        tot+=len(a)
        nontriv+=sum(1 for l in a if l.strip() not in ('','{','}','};'))
        q=os.path.join(root,other,rel)
        if not os.path.exists(q):
            missing.append((rel,len(a))); continue
        b=open(q,errors='ignore').read().splitlines()
        sm=difflib.SequenceMatcher(None,a,b,autojunk=False)
        s=0; snt=0
        for bl in sm.get_matching_blocks():
            s+=bl.size
            snt+=sum(1 for l in a[bl.a:bl.a+bl.size] if l.strip() not in ('','{','}','};'))
        same+=s; same_nontriv+=snt
    print(sub,'vs',other,'total',tot,'identical',same,f'{same/tot:.3f}','nontrivial',nontriv,'identical_nontriv',same_nontriv,f'{same_nontriv/nontriv:.3f}')
    print(' no counterpart:',missing)
cmp('gen','morsehgp3D_v8/src')
cmp('tower','morsehgp3D_v7/src')
