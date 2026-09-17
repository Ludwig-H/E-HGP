#!/usr/bin/env python3
"""Light model of mhgp8 Front::filter with the two proposed levers (read-only audit aid)."""
import random, sys, itertools
from collections import Counter

class Node:
    __slots__=("first","last","lo","hi","left","right","parent","depth")

def build(points):
    order=list(range(len(points))); nodes=[]
    def rec(first,last,parent,depth):
        lo=[min(points[order[i]][ax] for i in range(first,last)) for ax in range(3)]
        hi=[max(points[order[i]][ax] for i in range(first,last)) for ax in range(3)]
        nd=Node(); nd.first,nd.last,nd.lo,nd.hi,nd.left,nd.right,nd.parent,nd.depth=first,last,lo,hi,None,None,parent,depth
        nid=len(nodes); nodes.append(nd)
        if last-first==1: return nid
        axis=0
        for o in (1,2):
            if hi[o]-lo[o]>hi[axis]-lo[axis]: axis=o
        mid=(lo[axis]+hi[axis])//2
        seg=order[first:last]
        l=[i for i in seg if points[i][axis]<=mid]; r=[i for i in seg if points[i][axis]>mid]
        order[first:last]=l+r
        split=first+len(l)
        assert first<split<last
        nd.left=rec(first,split,nid,depth+1); nd.right=rec(split,last,nid,depth+1)
        return nid
    sys.setrecursionlimit(10000)
    rec(0,len(points),None,0)
    return order,nodes

def hmin(a,b,z):
    res=0
    for ax in range(3):
        v=z[ax]
        res+=min(x*y for x in (v-a.hi[ax],v-a.lo[ax]) for y in (b.lo[ax]-v,b.hi[ax]-v))
    return res

def window(pivot,count,n):
    first=min(pivot-count//2 if pivot>count//2 else 0,n-count)
    return first,first+count

def run(points,K,s,factor=1,limit=None,resume=False,inherit=False,root_tested=True,order_nodes=None):
    order,nodes=order_nodes
    n=len(points); W=Counter(); per=[]; rects=[]; visited=set()
    def dist4(c4,nd):
        W["box"]+=1
        r=0
        for ax in range(3):
            d=max(0,4*nd.lo[ax]-c4[ax],c4[ax]-4*nd.hi[ax]); r+=d*d
        return r
    def contains4(c4,nd): return all(4*nd.lo[ax]<=c4[ax]<=4*nd.hi[ax] for ax in range(3))
    def search(a,b,leaf,lst,use_inherit=True,count=True,from_leaf=True):
        # returns (rejected, leaf, final list, record)
        c4=[a.lo[ax]+a.hi[ax]+b.lo[ax]+b.hi[ax] for ax in range(3)]
        rec=dict(I=0,N=0,D=0,resumed=False,up=0,vdepth=0,steps=0)
        node=0
        if resume and leaf is not None and from_leaf:
            rec["resumed"]=True; W["resumed"]+=1
            node=leaf
            while True:
                if node==0 and not root_tested: break
                W["climb"]+=1
                if contains4(c4,nodes[node]): break
                rec["up"]+=1; node=nodes[node].parent
            assert contains4(c4,nodes[node])
            if node==0: W["fallback"]+=1
            rec["vdepth"]=nodes[node].depth; W["resume_depth_sum"]+=nodes[node].depth
        while nodes[node].left is not None:
            W["steps"]+=1; rec["steps"]+=1
            l,r=nodes[node].left,nodes[node].right
            node=l if dist4(c4,nodes[l])<=dist4(c4,nodes[r]) else r
        pivot=nodes[node].first
        cnt=min(K,n); w=window(pivot,cnt,n)
        credited=list(lst) if (inherit and use_inherit) else []
        rec["I"]=len(credited); W["inh_credits"]+=len(credited)
        inherited=set(credited)
        mask=1; ext=False; wider=None
        for phase in range(3):
            if mask==0: break
            rank,last=w
            if phase==1:
                if factor==1 or (limit is not None and max(a.last-a.first,b.last-b.first)>limit): break
                wider=window(pivot,min(K*factor,n),n)
                assert wider[0]<=w[0] and wider[1]>=w[1] and wider!=w
                W["ext_products"]+=1; ext=True
                rank,last=wider[0],w[0]
            elif phase==2:
                rank,last=w[1],wider[1]
            while rank<last and mask!=0:
                W["proposed"]+=1
                if ext: W["ext_proposals"]+=1
                inf=(a.first<=rank<a.last) or (b.first<=rank<b.last)
                if inf:
                    assert rank not in inherited, "Theorem H violated: inherited rank inside a factor"
                    W["in_factors"]+=1
                    if ext: W["ext_in_factors"]+=1
                    rank+=1; continue
                if rank in inherited:
                    W["dups"]+=1; rec["D"]+=1
                    if ext: W["ext_dups"]+=1
                    rank+=1; continue
                W["h"]+=1
                h=hmin(a,b,points[order[rank]])
                if h>0:
                    W["credits"]+=1; rec["N"]+=1
                    if ext: W["ext_credits"]+=1
                    credited.append(rank)
                    if len(credited)==K: mask=0
                rank+=1
        if ext and mask==0: W["ext_rejections"]+=1
        return mask==0,node,credited,rec
    def ref_rejects(a,b):
        # window-only reference filter, uncounted (judge of the counterfactual)
        nonlocal W
        saved=W; W=Counter()
        rej,_,_,_=search(a,b,None,[],use_inherit=False,from_leaf=False)
        W=saved
        return rej
    stack=[(0,0,None,())]
    while stack:
        ia,ib,leaf,lst=stack.pop()
        W["visits"]+=1; visited.add((ia,ib))
        a,b=nodes[ia],nodes[ib]
        if ia==ib:
            if a.left is None: W["diag_leaves"]+=1; continue
            W["diag_splits"]+=1
            stack.append((a.right,a.right,None,())); stack.append((a.left,a.right,None,())); stack.append((a.left,a.left,None,()))
            continue
        na,nb=a.last-a.first,b.last-b.first
        searched=False; rejected=False; rec=None
        if n-na-nb>=K:
            searched=True; W["searches"]+=1
            rejected,leaf2,lst2,rec=search(a,b,leaf,lst)
            rec["fate"]=None
            per.append(rec)
            if rejected:
                if inherit:
                    if rec["N"]+rec["D"]<K: W["inh_rej_defB"]+=1
                    if not ref_rejects(a,b): W["inh_rej_true"]+=1
            else:
                assert ref_rejects(a,b)==False, "monotonicity violated"
            leaf,lst=leaf2,tuple(lst2)
        else:
            assert leaf is None and len(lst)==0, "skipped search received a leaf or a list (non-vacuous!)"
            W["skipped"]+=1
        if rejected:
            W["rejected"]+=1; W["rej_mass"]+=na*nb; rec["fate"]="rejected"; continue
        W["sep"]+=1
        da=sum((a.hi[ax]-a.lo[ax])**2 for ax in range(3)); db=sum((b.hi[ax]-b.lo[ax])**2 for ax in range(3))
        gap=sum(max(0,a.lo[ax]-b.hi[ax],b.lo[ax]-a.hi[ax])**2 for ax in range(3))
        if gap>=s*s*max(da,db):
            W["emitted"]+=1; W["res_mass"]+=na*nb; rects.append((ia,ib))
            if rec is not None: rec["fate"]="emitted"; W["emitted_credits"]+=rec["I"]+rec["N"]
            continue
        W["splits"]+=1
        if rec is not None: rec["fate"]="split"; W["split_credits"]+=rec["I"]+rec["N"]
        split_a=a.left is not None and (b.left is None or da>=db)
        sp=a if split_a else b
        assert sp.left is not None
        if not inherit: lst=()
        stack.append((sp.right if split_a else ia, ib if split_a else sp.right, leaf, lst))
        stack.append((sp.left if split_a else ia, ib if split_a else sp.left, leaf, lst))
    return W,per,sorted(rects),visited

def clouds(rng):
    out={}
    out["grid4"]=[(x*10,y*10,z*10) for x in range(4) for y in range(4) for z in range(4)]
    def uniq(gen,n):
        s=set()
        while len(s)<n: s.add(gen())
        return sorted(s)
    out["uniform_small"]=uniq(lambda:(rng.randrange(40),rng.randrange(40),rng.randrange(40)),150)
    out["uniform_big"]=uniq(lambda:(rng.randrange(65536),rng.randrange(65536),rng.randrange(65536)),200)
    out["rows"]=[(i*7,0,0) for i in range(50)]+[(i*7,50,0) for i in range(50)]
    out["line_even"]=[(2*i,0,0) for i in range(80)]
    cs=[(rng.randrange(60000),rng.randrange(60000),rng.randrange(60000)) for _ in range(6)]
    out["clusters"]=uniq(lambda:tuple(min(65535,max(0,c+rng.randrange(-300,300))) for c in rng.choice(cs)),150)
    return out

def main():
    rng=random.Random(3)
    HIST=("visits","diag_leaves","diag_splits","searches","proposed","in_factors","h","credits","rejected","rej_mass","sep","emitted","res_mass","splits",
          "ext_products","ext_proposals","ext_in_factors","ext_credits","ext_rejections","skipped")
    for name,pts in clouds(rng).items():
        idx=build(pts)
        depth=max(nd.depth for nd in idx[1])
        for K,(factor,limit) in itertools.product((1,2,5,10),((1,None),(2,None),(2,16))):
            ref,_,rref,vref=run(pts,K,8,factor,limit,order_nodes=idx)
            res,pres,rres,vres=run(pts,K,8,factor,limit,resume=True,order_nodes=idx)
            inh,pinh,rinh,vinh=run(pts,K,8,factor,limit,inherit=True,order_nodes=idx)
            both,pboth,rboth,vboth=run(pts,K,8,factor,limit,resume=True,inherit=True,order_nodes=idx)
            nor,_,_,_=run(pts,K,8,factor,limit,resume=True,root_tested=False,order_nodes=idx)
            tag=f"{name} n={len(pts)} depth={depth} K={K} f={factor} lim={limit}"
            issues=[]
            # (e)
            for k in HIST:
                if ref[k]!=res[k]: issues.append(f"(e) historical {k} differs {ref[k]} vs {res[k]}")
            if not(res["steps"]<=ref["steps"] and res["box"]<=ref["box"]): issues.append("(e) steps/box not <=")
            if res["box"]!=2*res["steps"]: issues.append("box != 2 steps")
            if ref["steps"]!=res["steps"]+res["resume_depth_sum"]: issues.append("EXACT steps_ref = steps_res + resume_depth_sum FAILS")
            if rref!=rres: issues.append("rectangles differ under resume")
            # box counter if containment were routed through midpoint_distance4
            alt=res["box"]+res["climb"]
            if alt>ref["box"]: issues.append(f"[cond e] box+climb={alt} > ref box={ref['box']} (if containment uses midpoint_distance4)")
            # existing theorem L318
            if not(res["searches"]<=res["steps"]): issues.append(f"[L318] searches {res['searches']} > steps {res['steps']} under resume")
            zero=sum(1 for r in pres if r["resumed"] and r["steps"]==0)
            # (d)
            R,C,Wn=res["resumed"],res["climb"],res["searches"]
            if not(R<=Wn<=C+(Wn-R)): issues.append("(d) fails with root tested")
            R2,C2=nor["resumed"],nor["climb"]
            if not(R2<=C2): issues.append("(d) fails with root untested")
            if C!=R+sum(r["up"] for r in pres if r["resumed"]): issues.append("climb identity fails")
            if C2!=C-nor["fallback"]: issues.append("climb(no root)=climb-fallback fails")
            # (a)
            for nm,w in (("inh",inh),("both",both)):
                if w["h"]!=w["proposed"]-w["in_factors"]-w["dups"]: issues.append(f"(a) fails {nm}")
                if not(w["dups"]<=w["inh_credits"]<=(K-1)*w["searches"]): issues.append(f"(b) fails {nm}")
                if w["inh_credits"]!=2*w["split_credits"]: issues.append(f"EXACT inh=2*split_credits fails {nm}")
                if w["credits"]*2+w["inh_credits"]!=2*(K*w["rejected"]+w["emitted_credits"]): issues.append(f"EXACT credit conservation fails {nm}")
                if not(w["inh_rej_true"]<=w["inh_rej_defB"]<=w["rejected"]): issues.append(f"(c) ordering fails {nm}")
            # existing theorems under inherit
            flags=[]
            if inh["h"]+inh["in_factors"]!=inh["proposed"]: flags.append("L321 h+in_factors==proposed BROKEN")
            if inh["credits"]<K*inh["rejected"]: flags.append("credits>=K*rejected BROKEN")
            # runner projection
            hproj=inh["h"]-(inh["ext_proposals"]-inh["ext_in_factors"])
            pproj=inh["proposed"]-inh["ext_proposals"]; iproj=inh["in_factors"]-inh["ext_in_factors"]
            if hproj+iproj!=pproj: flags.append(f"runner projection BROKEN (ext_dups={inh['ext_dups']}, hist_dups={inh['dups']-inh['ext_dups']})")
            if hproj<0: flags.append("projected h_bound_tests NEGATIVE")
            if inh["ext_proposals"]-inh["ext_in_factors"]>inh["h"]: flags.append("runner L123 extra-skipped<=h BROKEN")
            if inh["credits"]>hproj and factor!=1: pass
            hist=min(K,len(pts)); un=inh["searches"]-inh["ext_products"]
            if not(pproj<=hist*inh["searches"] and pproj>=hist*inh["ext_products"]+un and un>=inh["rejected"]-inh["ext_rejections"]): flags.append("probe L166-168 BROKEN")
            if factor!=1 and limit is None and un!=inh["rejected"]-inh["ext_rejections"]: flags.append("probe L171 BROKEN")
            if not(inh["ext_credits"]<=inh["ext_proposals"]-inh["ext_in_factors"]-inh["ext_dups"]): flags.append("tight ext credit bound broken")
            # monotonicity
            if not(vinh<=vref and set(rinh)<=set(rref) and inh["res_mass"]<=ref["res_mass"]): issues.append("monotonicity fails")
            for k in HIST+("dups","inh_credits","inh_rej_defB","inh_rej_true"):
                if inh[k]!=both[k]: issues.append(f"both vs inherit differ on {k}")
            print(tag, "| searches",ref["searches"],"steps ref/res",ref["steps"],res["steps"],"resumed",R,"climb",C,"fallback",res["fallback"],"zero-step",zero,
                  "| inh: dups",inh["dups"],"inh_credits",inh["inh_credits"],"rej ref/inh",ref["rejected"],inh["rejected"],"defB",inh["inh_rej_defB"],"true",inh["inh_rej_true"],
                  "visits ref/inh",ref["visits"],inh["visits"],"|",";".join(flags),"| ISSUES:",issues,flush=True)
main()
