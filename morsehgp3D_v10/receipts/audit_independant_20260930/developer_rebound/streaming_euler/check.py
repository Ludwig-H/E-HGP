"""Exact independent oracle for the isolated native two-extreme reducer."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parent
NONE = 2**32-1


def need(ok,text):
    if not ok:
        raise RuntimeError(text)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def euler(parents):
    children = [[] for _ in parents]
    for v,p in enumerate(parents):
        if p != -1:
            children[p].append(v)
    tin,tout,clock = [None]*len(parents),[None]*len(parents),0
    def visit(v):
        nonlocal clock
        tin[v] = clock
        clock += 1
        for c in children[v]:
            visit(c)
        tout[v] = clock
    for v,p in enumerate(parents):
        if p == -1:
            visit(v)
    need(clock == len(parents), 'invalid oracle forest')
    return {v:(tin[v],tout[v]) for v in range(len(parents))}


def ancestor(parents,a,v):
    while v != -1:
        if a == v:
            return True
        v = parents[v]
    return False


def lca(parents,a,b):
    while a != -1:
        if ancestor(parents,a,b):
            return a
        a = parents[a]
    return None


def expected(table,selected,parents=None):
    if not selected:
        return [0],None
    distinct = set(selected)
    def above(a,v):
        if parents is not None:
            return ancestor(parents,a,v)
        # Independent virtual oracle over certified laminar Euler intervals.
        return table[a][0] <= table[v][0] and table[v][1] <= table[a][1]
    minima = [v for v in distinct if not any(w != v and above(v,w) for w in distinct)]
    minima.sort(key=lambda v:table[v][0])  # pairwise antichain oracle only
    left,right = minima[0],minima[-1]
    joint = None
    if parents is not None:
        joint = minima[0]
        for v in minima[1:]:
            if joint is None:
                break
            joint = lca(parents,joint,v)
        need(lca(parents,left,right) == joint, 'extrema LCA theorem disagrees with all minima')
    return [1,left,right],joint


def make_cases():
    cases = []
    shapes = ([1,2,3,-1], [4,4,4,4,-1], [4,4,5,5,6,6,-1], [2,2,-1,-1])
    for shape_id,parents in enumerate(shapes):
        table = euler(parents)
        for mask in range(1,1<<len(parents)):
            selected = [v for v in range(len(parents)) if mask>>v&1]
            for order in (selected,list(reversed(selected)),selected+list(reversed(selected))):
                want,joint = expected(table,order,parents)
                cases.append({'name':'shape%d_mask%d_order%d'%(shape_id,mask,len(cases)),
                              'n':len(parents),'table':table,'selected':order,
                              'expected':want,'joint':joint,'parents':parents})
    # These virtual cases do not allocate N nodes. All coordinates are exact
    # mathematical integers; N and tout can reach UINT32_MAX, node cannot.
    virtual = [
        ('empty_zero',0,{},[]),
        ('empty_max_count',NONE,{},[]),
        ('cross_i32_boundary',2**31+4,{0:(2**31-1,2**31+3),1:(2**31,2**31+3)},[0,1]),
        ('last_node_last_end',NONE,{NONE-1:(NONE-1,NONE)},[NONE-1]),
        ('same_end_unsigned_max',NONE,{NONE-2:(0,NONE),NONE-1:(NONE-1,NONE)},[NONE-2,NONE-1]),
        ('branches_across_i32',NONE,{3:(2**31-2,NONE),4:(2**31-1,2**31),
                                   5:(NONE-1,NONE)},[3,4,5,3,5]),
        ('disjoint_high_intervals',NONE,{8:(2**31-1,2**31),9:(NONE-1,NONE)},[8,9]),
        ('zero_id_is_real',3,{0:(0,3),1:(1,2),2:(2,3)},[0,2,1,0]),
    ]
    for name,n,table,selected in virtual:
        for order in (selected,list(reversed(selected)),selected+list(reversed(selected))):
            want,joint = expected(table,order)
            cases.append({'name':name,'n':n,'table':table,'selected':order,
                          'expected':want,'joint':joint,'parents':None})
    return cases


def payload(cases):
    lines = [str(len(cases))]
    for ci,case in enumerate(cases):
        lines.append('%d %d 5'%(case['n'],len(case['selected'])))
        for i,v in enumerate(case['selected']):
            tin,tout = case['table'][v]
            lines.append('%d %d %d %d'%(v,tin,tout,(2*i+ci)%5))
    return '\n'.join(lines)+'\n'


def run(binary,data,out,name):
    result = subprocess.run([str(binary)],input=data,text=True,capture_output=True,timeout=10)
    (out/(name+'.stdout')).write_text(result.stdout)
    (out/(name+'.stderr')).write_text(result.stderr)
    return result


def main():
    p = argparse.ArgumentParser()
    for name in ('release','ubsan','mutant','out','build-receipt'):
        p.add_argument('--'+name,type=Path,required=True)
    args = p.parse_args()
    args.out.mkdir(parents=True,exist_ok=False)
    pins = [Path(__file__).resolve(),ROOT/'extrema.hpp',ROOT/'probe.cpp',
            args.release,args.ubsan,args.mutant,args.build_receipt,Path(sys.executable).resolve()]
    before = {str(x.resolve()):sha(x) for x in pins}
    cases = make_cases()
    data = payload(cases)
    (args.out/'cases.txt').write_text(data)
    counts = {'cases':len(cases),'summary_comparisons':0,'different_root_cases':0,
              'virtual_cases':sum(c['parents'] is None for c in cases)}
    for name,binary in (('release',args.release),('ubsan',args.ubsan)):
        result = run(binary,data,args.out,name)
        need(result.returncode == 0 and not result.stderr,'native '+name+' failure')
        values = [json.loads(line) for line in result.stdout.splitlines()]
        need(len(values) == len(cases),'wrong native batch size')
        for i,(case,value) in enumerate(zip(cases,values)):
            need(value['case'] == i and len(value['states']) == 6,'wrong transport row')
            need(all(state == case['expected'] for state in value['states']),
                 'native extremes/worker merges disagree: '+case['name'])
            counts['summary_comparisons'] += 6
    counts['different_root_cases'] = sum(c['parents'] is not None and c['joint'] is None for c in cases)
    # Explicit empty state is independent of valid node zero, and node NONE is
    # refused even though an Euler end equal to NONE is valid with N==NONE.
    negatives = {
        'node_none':f'1\n{NONE} 1 1\n{NONE} 0 1 0\n',
        'node_equals_count':'1\n3 1 1\n3 0 1 0\n',
        'empty_interval':'1\n3 1 1\n0 1 1 0\n',
        'end_above_count':'1\n3 1 1\n0 1 4 0\n',
        'negative_numeric':'1\n3 1 1\n0 -1 2 0\n',
        'numeric_overflow':f'1\n{NONE+1} 0 1\n',
        'worker_out_of_range':'1\n3 1 1\n0 0 1 1\n',
        'nonempty_zero_count':'1\n0 1 1\n0 0 1 0\n',
    }
    for label,data_bad in negatives.items():
        result = run(args.ubsan,data_bad,args.out,'refusal_'+label)
        need(result.returncode == 2 and result.stderr.startswith('reject:'),'wrong explicit refusal '+label)
    # Native mutant returns 0 but chooses the wrong ancestor, causally changing
    # the implied LCA. Thus the failure is numeric, not a compiler/crash artefact.
    parents = [1,2,-1]
    case = {'n':3,'table':euler(parents),'selected':[2,1,0]}
    mutant_input = payload([case])
    result = run(args.mutant,mutant_input,args.out,'mutant_drop_tin')
    need(result.returncode == 0 and not result.stderr,'mutant did not execute normally')
    state = json.loads(result.stdout)['states'][0]
    need(state == [1,2,0] and expected(case['table'],case['selected'],parents)[0] == [1,0,0],
         'tie mutant did not causally choose wrong ancestor')
    need(lca(parents,state[1],state[2]) == 2 and lca(parents,0,0) == 0,'mutant LCA consequence absent')
    after = {str(x.resolve()):sha(x) for x in pins}
    need(before == after,'source or binary changed during consumer checks')
    report = {'status':'PASS','counts':counts,'refusals':len(negatives),'mutant_returncode':0,
              'mutant_native_global':state,'correct_global':[1,0,0],
              'mutant_implied_lca':2,'correct_implied_lca':0,'sources_before':before,
              'sources_after':after,'optimize_flag':sys.flags.optimize,'GCP_used':False,
              'scope':'isolated native associative u32 reducer only; no LCA-index, FULL, statistics or performance qualification'}
    (args.out/'receipt.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps({'status':'PASS','counts':counts,'refusals':len(negatives),
                      'mutant_killed_by_wrong_native_value':True},sort_keys=True))


if __name__ == '__main__':
    main()
