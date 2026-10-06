#!/usr/bin/env python3
"""Bounded portable oracle follow-up; no native CLI invocation."""
import argparse
import ast
import copy
from fractions import Fraction as F
import hashlib
import itertools
import json
from pathlib import Path
import subprocess
import sys
import types
import tempfile
from projection import spanning

CHECKS=0
def require(ok,message):
    global CHECKS
    if not ok:raise RuntimeError(message)
    CHECKS+=1
def morton(p):return sum(((x>>b)&1)<<(3*b+a) for a,x in enumerate(p) for b in range(24))
def dist(a,b):return sum((x-y)**2 for x,y in zip(a,b))
class DSU:
    def __init__(self,items):self.parent={x:x for x in items}
    def find(self,x):
        while self.parent[x]!=x:x=self.parent[x]
        return x
    def union(self,a,b):
        a,b=self.find(a),self.find(b)
        if a!=b:self.parent[b]=a

def functions(source):
    """Run only pure captured projection functions, never imports or CLI/main."""
    tree=ast.parse(source)
    chosen=[n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name in ('star','spanning')]
    env={'formats':types.SimpleNamespace(morton=morton),'Fraction':F}
    for n in tree.body:
        if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id in ('SPANNING_BALL','SPANNING_NODE') for t in n.targets):
            env[n.targets[0].id]=ast.literal_eval(n.value)
    exec(compile(ast.Module(body=chosen,type_ignores=[]),'<captured_projection>','exec'),env)
    return env['spanning']

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--repo',default='/workspaces/E-HGP');args=parser.parse_args()
    base=Path(__file__).parent;manifest=json.loads((base/'sources.json').read_text())
    source=(base/'source/morsehgp3D_v11/tests/cli/cli_supports_oracle.py').read_bytes()
    require(hashlib.sha256(source).hexdigest()==manifest['files']['morsehgp3D_v11/tests/cli/cli_supports_oracle.py']['sha256'],'WIP changed')
    observed=functions(source.decode());proposed=functions((base/'proposed_cli_supports_oracle.py').read_text())
    # Load only the independent definition/supports stage from immutable Git; no package __init__/constructive.
    package=types.ModuleType('audit_mst_definition');package.__path__=[];sys.modules[package.__name__]=package
    modules={}
    for name in ('model','definition','supports'):
        path='morsehgp3D_v11/reference/hgp11_ref/'+name+'.py'
        data=subprocess.check_output(['git','-C',args.repo,'show',manifest['pin']+':'+path])
        require(hashlib.sha256(data).hexdigest()==manifest['git'][path]['sha256'],'Git reference changed')
        module=types.ModuleType(package.__name__+'.'+name);module.__package__=package.__name__
        sys.modules[module.__name__]=module;exec(compile(data,path,'exec'),module.__dict__);modules[name]=module
    cases=[('equal_triangle_K1',[(0,0,0),(1,1,0),(1,0,1)],1),
           ('center_order_not_BallIdx',[(1,0,0),(0,1,0),(0,0,1)],1),
           ('triangle_K2_hyperedge',[(0,0,0),(2,2,0),(2,0,2)],2),
           ('tetra_K3_hyperedge',[(0,0,0),(2,2,0),(2,0,2),(0,2,2)],3)]
    results=[]
    for name,points,k in cases:
        oracle=modules['supports'].Supports(points);doc=json.loads(json.dumps(oracle.canonical(k,[100+7*i for i in range(len(points))])))
        expected=spanning(doc,morton)
        require(proposed(doc)==expected,'inline patch differs from helper')
        require(sum(b['role']=='naissance' for b in expected['balls'])==sum(n['kind']==1 for n in doc['nodes']),'births preserved')
        require(all(len(b['supports'])==1 and len(b['supports'][0])==b['qmin'] for b in expected['balls']),'one canonical minimal-arity support')
        old=observed(doc)
        if k==1:
            require(len(old['balls'])==3 and len(expected['balls'])==2,'triangle role filter still mirrors cycle')
            require(old!=expected,'exact native comparison must reject cycle')
            require(spanning(old,morton)==expected,'reselecting native hides cycle')
        else:
            merges=[b for b in expected['balls'] if b['role']=='fusion']
            require(len(merges)==1 and merges[0]['components']==k+1,'hyperedge can carry multiple Kruskal unions')
        if name=='center_order_not_BallIdx':
            kept={tuple(sorted(morton(tuple(p)) for p in b['supports'][0])) for b in expected['balls']}
            require(kept=={(1,2),(1,4)},'MST tie order must be Morton S*, not oracle center order')
        # Guard tests are modifications of oracle documents, not geometric/native mutants.
        malformed=copy.deepcopy(doc);malformed['balls'][0]['qmin']=1
        try:spanning(malformed,morton)
        except ValueError:pass
        else:raise RuntimeError('qmin guard absent')
        require(True,'qmin guard')
        results.append(dict(name=name,k=k,oracle_balls=len(doc['balls']),observed_selected=len(old['balls']),
                            corrected_selected=len(expected['balls']),selected_stars=[b['supports'][0] for b in expected['balls']]))
    proposed_tree=ast.parse((base/'proposed_cli_supports_oracle.py').read_text())
    compare=next(n for n in proposed_tree.body if isinstance(n,ast.FunctionDef) and n.name=='compare')
    calls=[n for n in ast.walk(compare) if isinstance(n,ast.Call) and isinstance(n.func,ast.Attribute) and n.func.attr=='first_difference']
    require(len(calls)==1 and isinstance(calls[0].args[0],ast.Name) and calls[0].args[0].id=='mine','native side never projected')
    def shell_dump(text):
        node=next(n for n in ast.parse(text).body if isinstance(n,ast.FunctionDef) and n.name=='shell_cases')
        return ast.dump(node,include_attributes=False)
    require(shell_dump(source.decode())==shell_dump((base/'proposed_cli_supports_oracle.py').read_text()),'developer shell correction preserved exactly')
    require("max(report['decoded'].m) == 25" not in source.decode(),'false shell25 max removed from source')
    require("counts['roles']['merge'] >= 1" in source.decode(),'developer admission/merge guard preserved')
    with tempfile.TemporaryDirectory(prefix='mhgp11_oracle_remaining_patch_') as directory:
        target=Path(directory)/'morsehgp3D_v11/tests/cli/cli_supports_oracle.py'
        target.parent.mkdir(parents=True);target.write_bytes(source)
        result=subprocess.run(['git','apply','--check',str((base/'proposed.patch').resolve())],cwd=directory,capture_output=True,text=True)
        require(result.returncode==0,'updated patch applicability: '+result.stderr)
    print(json.dumps(dict(status='PASS_portable_remaining_projection',checks=CHECKS,native_executed=False,
        wip_head=manifest['head'],cases=results,shell_correction_source='preserved, not natively qualified',
        patch_apply_check='PASS',limits='Four existing small exact oracle cases, AST and patch applicability only. No extended shell replay or native invocation.'),sort_keys=True,indent=2))

if __name__=='__main__':main()
