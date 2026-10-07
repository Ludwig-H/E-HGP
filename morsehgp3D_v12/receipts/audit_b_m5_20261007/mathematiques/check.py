#!/usr/bin/env python3
"""Small independent exact checks of MES-M5; native host simulation, never CUDA.

Uses the developer's dump encoder only as a codec. The independent DFS below
uses global affine corner extrema instead of its shifted square/scaled terms.
The geometry oracle is bounded to <= 8 sites; no geometry oracle on the shell48.
"""
from fractions import Fraction
from itertools import permutations, product
from pathlib import Path
import argparse
import hashlib
import importlib.util
import json
import random
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
M5 = ROOT/'morsehgp3D_v12/microbancs/mes_m5_parcours'


def require(ok, reason):
    if not ok:
        raise RuntimeError(reason)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def closure():
    manifest = json.loads((HERE/'sources.json').read_text())
    head = subprocess.check_output(['git', '-C', str(ROOT), 'rev-parse', 'HEAD'], text=True).strip()
    for item in manifest['files']:
        require(sha(ROOT/item['path']) == item['sha256'], 'source changed: '+item['path'])
        blob = subprocess.check_output(['git', '-C', str(ROOT), 'rev-parse', manifest['pin']+':'+item['path']], text=True).strip()
        require(blob == item['blob'], 'blob mismatch: '+item['path'])
    return dict(pin=manifest['pin'], head=head, sources=len(manifest['files']),
                manifest_sha256=sha(HERE/'sources.json'), script_sha256=sha(Path(__file__)),
                probe_source_sha256=sha(HERE/'predicates.cpp'))


def load_module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


codec = load_module('m5_codec', M5/'oracle/oracle_parcours.py')
sys.path.insert(0, str(ROOT/'morsehgp3D_v12/reference'))
from hgp12_ref.constructive import Reference


def morton(p):
    return sum(((p[a] >> b) & 1) << (3*b+a) for b in range(32) for a in range(3))


def dfs(points, k, leaf, maximum, bits):
    """Exact affine G1; success records match the documented dump schema."""
    nodes, leaves, ids = [], [], []
    ledger = dict(nodes=0, leaves=0, filter_tests=0, max_depth=0, max_leaf=0)

    class Refusal(Exception):
        pass

    def visit(parent, lo, hi, depth, path):
        if depth > 3*bits:
            raise Refusal(2)
        ledger['nodes'] += 1
        ledger['max_depth'] = max(ledger['max_depth'], depth)
        witnesses = sorted(range(len(parent)), key=lambda j: (
            sum((2*points[parent[j]][a]-lo[a]-hi[a])**2 for a in range(3)), j))[:3*k]
        kept, tests = [], 0
        for i in parent:
            x = points[i]
            found = 0
            for j in witnesses:
                y = points[parent[j]]
                # Exact minimum over the CLOSED box of |x-c|²-|y-c|².
                margin = sum(x[a]*x[a]-y[a]*y[a]
                             - 2*(hi[a] if x[a] >= y[a] else lo[a])*(x[a]-y[a]) for a in range(3))
                tests += 1
                found += margin > 0
                if found == k:
                    break
            if found < k:
                kept.append(i)
        ledger['filter_tests'] += tests
        node = dict(lo=list(lo), hi=list(hi), path=list(path), tests=tests, candidates=len(parent),
                    count=0, depth=depth, kind=0, fnv=0)
        nodes.append(node)
        if not kept:
            return
        low = [max(lo[a], min(points[i][a] for i in kept)) for a in range(3)]
        high = [min(hi[a], max(points[i][a] for i in kept)+1) for a in range(3)]
        if any(low[a] >= high[a] for a in range(3)):
            return
        node.update(count=len(kept), fnv=codec.list_fnv(kept))
        axis = max(range(3), key=lambda a: high[a]-low[a])
        width = high[axis]-low[axis]
        if len(kept) <= leaf or width <= 1:
            node['kind'] = 1
            ledger['leaves'] += 1
            ledger['max_leaf'] = max(ledger['max_leaf'], len(kept))
            if len(kept) > maximum:
                raise Refusal(1)
            leaves.append(dict(begin=len(ids), m=len(kept), depth=depth, lo=low, hi=high, path=list(path)))
            ids.extend(kept)
            return
        node['kind'] = 2
        middle = low[axis]+width//2
        left, right = high.copy(), low.copy()
        left[axis] = middle
        right[axis] = middle
        visit(kept, low, left, depth+1, path)
        rpath = list(path)
        rpath[depth//64] |= 1 << (63-depth%64)
        visit(kept, right, high, depth+1, rpath)

    try:
        visit(list(range(len(points))), [min(p[a] for p in points) for a in range(3)],
              [max(p[a] for p in points)+1 for a in range(3)], 0, [0, 0])
    except Refusal as e:
        return e.args[0], [], [], [], dict.fromkeys(ledger, 0)
    return 0, nodes, leaves, ids, ledger


def primitive_checks(binary):
    rng = random.Random(20261008)
    queries, expected, spans = [], [], {}
    for bits in (1, 2, 16, 28, 29, 30, 31, 32, 33):
        top = min((1 << bits)-1, (1 << 32)-1)
        for trial in range(12):
            offset = 0 if trial % 2 == 0 else (1 << 32)-1-top
            mn, mx = [offset]*3, [offset+top]*3
            lo = [offset+rng.randrange(top) for _ in range(3)]
            hi = [rng.randrange(lo[a]+1, mx[a]+2) for a in range(3)]
            if bits == 33:
                hi[0] = 1 << 32
            x = [rng.randrange(mn[a], mx[a]+1) for a in range(3)]
            y = [rng.randrange(mn[a], mx[a]+1) for a in range(3)]
            queries.append('P '+' '.join(map(str, lo+hi+mn+mx+x+y)))
            span = max(max(mx[a], hi[a])-mn[a] for a in range(3)).bit_length()
            key = sum((2*x[a]-lo[a]-hi[a])**2 for a in range(3))
            margin = min(sum((x[a]-c[a])**2-(y[a]-c[a])**2 for a in range(3))
                         for c in product(*zip(lo, hi)))
            expected.append([span, key, int(margin > 0)])
            spans[span] = spans.get(span, 0)+1
    # Strict contact preserved; far candidate forces wide reservoir although child box is tiny.
    for lo, hi, mn, mx, x, y in (
        ([1, 0, 0], [2, 1, 1], [0, 0, 0], [2, 0, 0], [0, 0, 0], [2, 0, 0]),
        ([0]*3, [1]*3, [0]*3, [(1 << 30)-1]*3, [(1 << 30)-1]*3, [0]*3)):
        queries.append('P '+' '.join(map(str, lo+hi+mn+mx+x+y)))
        span = max(max(mx[a], hi[a])-mn[a] for a in range(3)).bit_length()
        key = sum((2*x[a]-lo[a]-hi[a])**2 for a in range(3))
        margin = min(sum((x[a]-c[a])**2-(y[a]-c[a])**2 for a in range(3)) for c in product(*zip(lo, hi)))
        expected.append([span, key, int(margin > 0)])
    require(expected[-2][2] == 0 and expected[-1][1] > (1 << 63)-1, 'targeted cases')
    top_count = 0
    for n in (31, 32, 33, 36, 37, 64, 255, 256, 257, 513):
        for k in (1, 5, 10, 11, 12):
            # Distinct sites, many equidistant keys, arbitrary parent order; two numerical routes.
            points = [(i*4194304, (i % 2)*4194304, 0) for i in range(n)]
            if k % 2:
                points = [(i, i % 2, 0) for i in range(n)]
            rng.shuffle(points)
            lo = [min(p[a] for p in points) for a in range(3)]
            hi = [max(p[a] for p in points)+1 for a in range(3)]
            ranks = sorted(range(n), key=lambda i: (sum((2*points[i][a]-lo[a]-hi[a])**2 for a in range(3)), i))[:3*k]
            queries.append('T '+str(n)+' '+str(k)+' '+' '.join(str(c) for p in points for c in p))
            expected.append([len(ranks)]+ranks)
            top_count += 1
    proc = subprocess.run([str(binary)], input='\n'.join(queries)+'\n', text=True, capture_output=True, timeout=20)
    require(proc.returncode == 0, 'probe failed: '+proc.stderr)
    actual = [list(map(int, line.split())) for line in proc.stdout.splitlines()]
    require(actual == expected, 'primitive/top mismatch')
    return dict(predicates=110, top_reservoirs=top_count, spans=spans, all_equal=True,
                strict_contact_preserved=True, s30_key_beyond_i64=str(expected[109][1]),
                stdin_sha256=hashlib.sha256(('\n'.join(queries)+'\n').encode()).hexdigest(),
                stdout_sha256=hashlib.sha256(proc.stdout.encode()).hexdigest())


def cases():
    points = [(2,5,2),(4,7,4),(8,10,9),(9,12,11),(10,8,6),(12,10,4)]
    low = [min(p[a] for p in points) for a in range(3)]
    out = [dict(name='morton_absolute', points=sorted(points,key=morton), bits=21, k=1, leaf=4, maximum=256),
           dict(name='morton_normalized', points=sorted(points,key=lambda p:morton(tuple(p[a]-low[a] for a in range(3)))),
                bits=21, k=1, leaf=4, maximum=256)]
    cube = list(product((0,16), repeat=3))
    out += [dict(name='cube_k'+str(k),points=sorted(cube,key=morton),bits=21,k=k,leaf=max(5,k+3),maximum=256)
            for k in (1,2,5)]
    for bits, scale, offset in ((21,1 << 18,3 << 18),(32,1 << 29,1 << 31)):
        shell = sorted({tuple(offset+sign*p[a]*scale for a,sign in enumerate(signs))
                        for p in set(permutations((1,2,3))) for signs in product((-1,1),repeat=3)},key=morton)
        out.append(dict(name='depth_'+str(3*bits),points=shell,bits=bits,k=5,leaf=24,maximum=256,expected_depth=3*bits))
        if bits == 21:
            out.append(dict(name='wide_leaf',points=shell,bits=bits,k=5,leaf=8,maximum=8,expected_status=1))
    out.append(dict(name='closed_box33', points=[(0,0,0),((1 << 32)-1,0,0)],bits=32,k=1,leaf=4,maximum=256))
    return out


def geometric_coverage(points, k, leaves, ids):
    ref = Reference(points, k)
    total = 0
    for ball in ref.balls:
        if ball.qmin < 2:
            continue
        owners = [leaf for leaf in leaves if all(leaf['lo'][a] <= ball.center[a] < leaf['hi'][a] for a in range(3))]
        require(len(owners) == 1, 'critical center without unique owner')
        leaf = owners[0]
        population = {ref.sites[i] for i in ball.inner_sites+ball.shell_sites}
        contained = {points[i] for i in ids[leaf['begin']:leaf['begin']+leaf['m']]}
        require(population <= contained, 'critical population lost')
        total += 1
    return total


def run_cases(binary):
    reports = []
    with tempfile.TemporaryDirectory(prefix='m5-math-') as folder:
        for case in cases():
            points, bits, k = case['points'], case['bits'], case['k']
            status, nodes, leaves, ids, ledger = dfs(points,k,case['leaf'],case['maximum'],bits)
            require(status == case.get('expected_status',0), 'unexpected independent status')
            if 'expected_depth' in case:
                require(ledger['max_depth'] == case['expected_depth'], 'deep witness depth')
            columns = list(zip(*points))
            header = dict(producer=2,coord_bits=bits,kmax=k,leaf_size=case['leaf'],max_leaf=case['maximum'],status=status,ledger=ledger)
            raw = codec.encode(header,*columns,nodes,leaves,ids)
            path = Path(folder)/(case['name']+'.bin');path.write_bytes(raw)
            proc = subprocess.run([str(binary),'--nodes','--unit','--mutants','none',str(path)],text=True,capture_output=True,timeout=30)
            rows = [json.loads(line) for line in proc.stdout.splitlines()]
            require(proc.returncode == 0, case['name']+': '+proc.stderr+' '+proc.stdout)
            unit, row = rows
            require(unit['ok'] and row['identity'] and (status != 0 or row['nodes_equal']), 'native identity proof missing')
            count = geometric_coverage(points,k,leaves,ids) if status == 0 and len(points) <= 8 and bits <= 21 else None
            geometric_leaves = sorted((tuple(leaf['lo']), tuple(leaf['hi']),
                                      tuple(sorted(points[i] for i in ids[leaf['begin']:leaf['begin']+leaf['m']])))
                                     for leaf in leaves)
            geometric_hash = hashlib.sha256(json.dumps(geometric_leaves,separators=(',',':')).encode()).hexdigest()
            row.pop('host_ns',None);row['dump']=path.name
            reports.append(dict(name=case['name'],points=len(points),dump_sha256=sha(path),native=row,
                                independent_ledger=ledger,critical_balls_with_unique_owner_and_complete_population=count))
            reports[-1]['geometric_leaf_sha256'] = geometric_hash
    require(reports[0]['native']['digest'] != reports[1]['native']['digest'], 'Morton witness no longer changes leaves')
    require(reports[0]['geometric_leaf_sha256'] != reports[1]['geometric_leaf_sha256'],
            'Morton witness changes only SiteIdx, not geometric leaves')
    return reports


def main():
    ap=argparse.ArgumentParser();ap.add_argument('--identity',required=True,type=Path);ap.add_argument('--probe',required=True,type=Path)
    args=ap.parse_args();before=closure();binaries={p.name:sha(p) for p in (args.identity,args.probe)}
    result=dict(schema='ehgp.v12.m5.math_audit.v1',native_host_simulation=True,gcp_used=False,
                primitives=primitive_checks(args.probe),cases=run_cases(args.identity))
    require(closure()==before and binaries=={p.name:sha(p) for p in (args.identity,args.probe)},'sources or binaries changed')
    result.update(source_closure=before,binary_sha256=binaries,before_after_unchanged=True,
                  verdicts=dict(g1_and_reservoir_exact=True,all_native_traversals_equal_independent_dfs=True,
                                closed_box33_and_depth96=True,wide_leaf_transactional_refusal=True,
                                morton_order_convention_observed_natively=True))
    print(json.dumps(result,indent=2,sort_keys=True))


if __name__ == '__main__':
    main()
