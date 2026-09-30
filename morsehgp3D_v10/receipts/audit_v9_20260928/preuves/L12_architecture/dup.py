import os, difflib, sys
root='/workspaces/E-HGP/build/v9-open-worktree'
def files(d):
    out={}
    for dp,dn,fn in os.walk(os.path.join(root,d)):
        for f in fn:
            if f.endswith(('.hpp','.cpp','.cu')):
                out[os.path.join(dp,f)]=f
    return out
v9=files('morsehgp3D_v9/src')
for other in ['morsehgp3D_v8/src','morsehgp3D_v7/src']:
    o=files(other)
    byname={}
    for p,f in o.items(): byname.setdefault(f,[]).append(p)
    tot_v9=0; tot_same=0
    for p,f in sorted(v9.items()):
        if f in byname:
            a=open(p,errors='ignore').read().splitlines()
            best=None
            for q in byname[f]:
                b=open(q,errors='ignore').read().splitlines()
                sm=difflib.SequenceMatcher(None,a,b,autojunk=False)
                same=sum(bl.size for bl in sm.get_matching_blocks())
                if best is None or same>best[0]: best=(same,q,len(a),len(b))
            print(f"{other[:11]} {os.path.relpath(p,root)} {best[2]} lines, {best[0]} identical with {os.path.relpath(best[1],root)} ({best[3]})")
