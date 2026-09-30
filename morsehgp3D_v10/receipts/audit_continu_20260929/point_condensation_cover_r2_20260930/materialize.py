"""Translate a HISTORICAL native cover export and independently recoup Gamma3."""
from fractions import Fraction as F
from hashlib import sha256
import importlib.util
from itertools import combinations
import json
from pathlib import Path
import struct
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parent


def require(ok, message):
    if not ok:
        raise ValueError(message)


def load(name,path):
    spec=importlib.util.spec_from_file_location(name,path)
    module=importlib.util.module_from_spec(spec)
    sys.modules[name]=module
    spec.loader.exec_module(module)
    return module


def prove():
    fc=load('frozen_cover_framework',ROOT/'reference/frontier_core.py')
    R=load('independent_meb_reference',ROOT/'reference/hgp10_ref.py')
    export=fc.load_export(ROOT/'historical/internal_k3.json')
    raw=json.loads((ROOT/'historical/internal_k3.json').read_text())
    history=json.loads((ROOT/'historical/receipt.json').read_text())
    row=[r for r in history['clouds'] if r['case']=='internal_k3']
    require(len(row)==1 and row[0]['returncode']==0 and
            row[0]['export_sha256']==sha256((ROOT/'historical/internal_k3.json').read_bytes()).hexdigest(),
            'historical source pin')
    points=list(struct.iter_unpack('<III',(ROOT/'historical/internal_k3.u32le').read_bytes()))
    require(len(points)==6 and export.K==3 and len(export.sites)==6,'fixture scope')
    for point_id,site in zip(export.point_id,export.sites):
        require(tuple(points[point_id])==tuple(site),'historical ID/coordinate mapping')
    P=export.sites
    vertices={v:R.meb(P,v)[0] for v in combinations(range(6),3)}
    edges={e:R.meb(P,e)[0] for e in combinations(range(6),4)}
    o,f=export.order(3),export.forest
    representatives={}
    for v in f.births:
        members=tuple(sorted(export.balls[f.birth[v]].members()))
        require(len(members)==3 and vertices[members]==f.level(v),'birth support representative')
        representatives[v]=members
    def leaf(v):
        if v in representatives:
            return representatives[v]
        return leaf(f.children[v][0])
    def gamma(beta):
        parent={v:v for v,t in vertices.items() if t<=beta}
        def find(v):
            while parent[v]!=v:
                v=parent[v]
            return v
        for e,t in edges.items():
            if t>beta:
                continue
            faces=[tuple(x for x in e if x!=omit) for omit in e]
            for v in faces:
                require(v in parent,'non-monotonic MEB')
            anchor=find(faces[0])
            for v in faces[1:]:
                parent[find(v)]=anchor
        labels={}
        groups={}
        for v in parent:
            groups.setdefault(find(v),[]).append(v)
        for group in groups.values():
            key=min(group)
            labels.update({v:key for v in group})
        return labels
    events=sorted(set(vertices.values())|set(edges.values())|set(f.levels)|{F(0)})
    cuts=sorted(set(events+[(a+b)/2 for a,b in zip(events,events[1:])]+[max(events)+1]))
    for beta in cuts:
        labels=gamma(beta)
        owners={}
        for v in f.components(beta):
            keys={labels[representatives[w]] for w in f.births_under(v)}
            require(len(keys)==1,'forest component not a Gamma component')
            key=next(iter(keys))
            require(key not in owners,'two forest components share Gamma component')
            owners[key]=v
        require(set(labels.values())==set(owners),'forest/Gamma component coverage differs')
    alpha=[]
    for x in range(6):
        a=min(t for v,t in vertices.items() if x in v)
        alpha.append(a)
        require(a==o.cover_level(x),'first cover differs from independent MEB')
        owner=f.ancestor(o.cover_node[x],a,True)
        require(owner==o.cover_node[x],'cover owner not alive')
        labels=gamma(a)
        key=labels[leaf(owner)]
        require(any(x in v and t==a and labels[v]==key for v,t in vertices.items()),
                'chosen cover owner has no first-cover Gamma vertex')
    x=export.point_id.index(0)
    require(alpha[x]==25 and o.cover_node[x]==f.root and f.level(f.root)==F(169,9),
            'late root cover witness missing')
    require(all(a<F(169,9) for i,a in enumerate(alpha) if i!=x),'five others not earlier than root birth')
    off=[0]
    child=[]
    for children in f.children:
        child.extend(children);off.append(len(child))
    cpp=lambda values:'{'+', '.join(str(v) for v in values)+'}'
    levels='{'+', '.join(str(t.numerator)+'.0/'+str(t.denominator)+'.0' for t in f.levels)+'}'
    header='#pragma once\n#include "points/dendrogram.hpp"\ninline mhgp10::PointDendrogram historical_cover() {\n'
    header+='  mhgp10::PointDendrogram d;\n'
    header+='  d.level = '+levels+';\n'
    for name,values in (('node_rank',f.lv),('child_off',off),('child_val',child),
                        ('parent',[4294967295 if p==-1 else p for p in f.parent]),
                        ('point_node',o.cover_node),('point_rank',o.cover_lv),('point_weight',[1]*6)):
        header+='  d.'+name+' = '+cpp(values)+';\n'
    header+='  return d;\n}\n'
    return dict(status='EXACT_HISTORICAL_COVER_MATERIALIZATION',K=3,n=6,
                vertices=20,unions=15,cuts=len(cuts),first_cover_checks=6,
                point_ids=export.point_id,point_alpha=list(map(str,alpha)),
                late_point_id=0,late_root_beta='169/9',late_point_entry='25',
                header=header,native_generator_invocations=0,
                historical_export_sha256=sha256((ROOT/'historical/internal_k3.json').read_bytes()).hexdigest(),
                scope='historical exact native export + newly recouped tiny Gamma; not new generator execution')


if __name__=='__main__':
    print(json.dumps(prove(),sort_keys=True,indent=2))

