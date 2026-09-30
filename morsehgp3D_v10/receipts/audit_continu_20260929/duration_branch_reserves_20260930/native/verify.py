"""Audit only: two saved native exports against exact small geometry.

Never invokes a native binary. Fraction replay and independent MEB/nerve
judge are both copied here. IDs are reconciled by geometric components.
"""
from fractions import Fraction as Q
from pathlib import Path
from itertools import combinations
import hashlib
import importlib.util
import json
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parent
NONE = 2**32-1
EXPECTED_FRACTION = 'dfdf70c06712b43050c371e3349a643181eb72dea51cef6cdec5373c67f43b74'

def require(condition, message):
    if not condition:
        raise RuntimeError(message)

def module(name, filename):
    spec = importlib.util.spec_from_file_location(name, ROOT / filename)
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m

def hashes():
    return {name: hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in
            ('verify.py', 'check_reserves.py', 'frozen_reference.py', 'check_native_geometry.py',
             'k3.txt', 'k5.txt', 'k3.stdout.json', 'k5.stdout.json')}

start = time.monotonic()
before = hashes()
argv = [sys.executable, '-B'] + (['-O'] if sys.flags.optimize else [])
argv += [str(ROOT/'check_reserves.py'), str(ROOT/'frozen_reference.py')]
replay = subprocess.run(argv, capture_output=True, text=True, timeout=15)
require(replay.returncode == 0 and replay.stderr == '', 'Fraction replay failed')
fraction = json.loads(replay.stdout)
semantic = {k:fraction[k] for k in ('missing_leaf', 'nearest_K2_controls', 'ghosts')}
digest = hashlib.sha256(json.dumps(semantic, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
require(digest == EXPECTED_FRACTION, 'Fraction semantic replay differs from original captures')
ref = module('frozen_ref', 'frozen_reference.py')
native = module('native_geometry', 'check_native_geometry.py')
counts = dict(scenes=0, k1_site_table_orders=0, cuts_checked=0, point_component_lists=0,
              multi_component_lists=0, ball_resolution_checks=0, resolution_after_own_radius=0,
              deep_balls_p_ge_k=0, interior_q3_q4_checks=0, first_point_checks=0)
details = []
for name, k, x_coordinate, expected_beta in (
        ('k3', 3, (15,4,0), Q(25)), ('k5', 5, (325,325,650), Q(105625))):
    data = json.loads((ROOT/(name+'.stdout.json')).read_text())
    points = [tuple(p) for p in data['points']]
    supplied = [tuple(map(int,line.split())) for line in (ROOT/(name+'.txt')).read_text().splitlines()]
    require(len(points) == len(set(points)) and set(points) == set(supplied), 'input mapping not bijective')
    require(all(0 <= v < 2**18 for point in points for v in point), 'native input outside u18')
    require(ref.rank([ref.sub(p, points[0]) for p in points[1:]]) == 3, 'not affine dimension3')
    x = points.index(x_coordinate)  # Morton order is not input-file order.
    native.check_geometry(name, data, counts)  # complete nerve cuts, births, raw I/U, resolutions; not raw IDs
    order = next(o for o in data['orders'] if o['k'] == k)
    levels = [Q(int(a), int(b)) for a,b in data['levels']]
    node_levels = [Q(0) if r == 0 else levels[r-1] for r in order['rank']]
    native_strong = [(i,b) for i,b in enumerate(data['balls']) if b['p']+b['u'] >= k and b['p']+b['q'] <= k]
    exact_strong = [b for b in ref.critical_balls(points) if b.p+b.m >= k and b.p+b.qmin <= k]
    native_keys = set()
    for _,b in native_strong:
        S = [s for s in b['S'] if s != NONE]
        center,_ = native.center([points[s] for s in S])
        native_keys.add((center,levels[b['rank']],tuple(b['I']),tuple(b['U']),b['q']))
    exact_keys = {(b.center,b.level,b.I,b.U,b.qmin) for b in exact_strong}
    require(native_keys == exact_keys and len(native_keys) == len(native_strong), 'strong catalogue incomplete/duplicate')
    covering = [(i,b) for i,b in native_strong if x in b['I']+b['U']]
    require(len(covering) == 1, 'expected one strong ball covering x')
    i,b = covering[0]
    v = order['ball_node'][i]
    require(levels[b['rank']] == expected_beta and order['birth'][v] == NONE, 'cover not on expected internal branch')
    require(order['point_node'][x] == v and levels[order['point_cat_rank'][x]-1] == expected_beta, 'first-cover attachment differs')
    leaf_ids = [v for v,birth in enumerate(order['birth']) if birth != NONE]
    leaf_intervals = {v:(node_levels[v], None if order['parent'][v] == NONE else node_levels[order['parent'][v]])
                      for v in leaf_ids}
    leaf_covers = {v:set() for v in leaf_ids}
    ancestor_tests = 0
    for i,b in native_strong:
        beta = levels[b['rank']]
        u = order['ball_node'][i]
        seen = set()
        while u != NONE:
            require(u not in seen, 'parent cycle')
            seen.add(u)
            ancestor_tests += 1
            born = node_levels[u]
            death = None if order['parent'][u] == NONE else node_levels[order['parent'][u]]
            if order['birth'][u] != NONE and (death is None or max(beta,born) < death):
                leaf_covers[u].update(b['I']+b['U'])
            u = order['parent'][u]
    require(all(x not in cover for cover in leaf_covers.values()), 'x covered during a native leaf lifetime')
    first = min(levels[b['rank']] for _,b in covering)
    require(all(dead is not None and dead < first for _,dead in leaf_intervals.values()), 'leaf outlives x coverage')
    details.append({'case':name, 'k':k, 'sites':len(points), 'x_coordinate':list(x_coordinate),
                    'x_native_morton_index':x, 'strong_balls':len(native_strong),
                    'strong_catalogue_complete_against_Fraction':True,
                    'strong_ball_ancestor_tests':ancestor_tests,
                    'strong_covering_ball':{'index':i if len(covering)==0 else covering[0][0],
                        'beta':str(first), 'q_min':covering[0][1]['q'], 'p':covering[0][1]['p'],
                        'shell_points':[list(points[t]) for t in covering[0][1]['U']],
                        'internal_ball_node':v,'internal_node_birth':str(node_levels[v])},
                    'leaf_lifetimes':[{'node':w,'birth':str(born),'death':None if dead is None else str(dead),
                                       'ever_covers_x':x in leaf_covers[w]} for w,(born,dead) in leaf_intervals.items()],
                    'all_strong_balls_times_ancestors_leaf_x_count':0})
after = hashes()
require(before == after, 'audit input changed')
print(json.dumps({'status':'DURATION_NATIVE_COUNTER_REVIEW_PASS', 'optimized':sys.flags.optimize,
                  'native_calls_in_this_verifier':0,'native_exports_consumed':2,'GCP_used':False,'engine_modified':False,
                  'counts':counts,'details':details,'source_hashes_before':before,'source_hashes_after':after,
                  'fraction_replay':{'argv':argv,'returncode':replay.returncode,'stderr':replay.stderr,
                    'status':fraction['status'],'semantic_sha256':digest,'semantic_matches_original':True,
                    'nearest_K2_point_controls':sum(len(x) for x in fraction['nearest_K2_controls'].values()),
                    'ghost_scales':[g['N'] for g in fraction['ghosts']],
                    'hashes_before':fraction['hashes_before'],'hashes_after':fraction['hashes_after']},
                  'wall_seconds':time.monotonic()-start},indent=2))
