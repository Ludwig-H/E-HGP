"""Six-site exact audit, independent of developer geometry/oracle implementations.

Only Fraction arithmetic drives decisions. Decimal display is informational.
There is no native engine invocation or implied ideal-equilateral qualification.
"""
from decimal import Decimal, localcontext
from fractions import Fraction as F
from itertools import combinations, permutations
from pathlib import Path
import importlib.util
import json
import struct

ROOT = Path(__file__).resolve().parent
CHECKS = 0

def need(ok, message):
    global CHECKS
    CHECKS += 1
    if not ok:
        raise RuntimeError(message)

def d2(a, b):
    return sum((x-y)**2 for x, y in zip(a, b))

def circ(points):
    """Exact affine circumcenter, also returns barycentric coefficients."""
    p = points[0]
    V = [tuple(x-y for x, y in zip(q, p)) for q in points[1:]]
    m = len(V)
    if not m:
        return tuple(map(F, p)), F(0), (F(1),)
    dot = lambda a, b: sum(x*y for x, y in zip(a, b))
    A = [[F(dot(a, b)) for b in V] + [F(dot(a, a), 2)] for a in V]
    for k in range(m):
        pivot = next((j for j in range(k, m) if A[j][k]), None)
        if pivot is None:
            return None
        A[k], A[pivot] = A[pivot], A[k]
        divisor = A[k][k]
        A[k] = [v/divisor for v in A[k]]
        for j in range(m):
            if j != k:
                f = A[j][k]
                A[j] = [x-f*y for x, y in zip(A[j], A[k])]
    coeff = tuple(row[-1] for row in A)
    center = tuple(F(p[k]) + sum(v[k]*t for v, t in zip(V, coeff)) for k in range(3))
    return center, d2(center, p), (1-sum(coeff),) + coeff

def meb(points):
    solutions = []
    for q in range(1, min(4, len(points))+1):
        for support in combinations(points, q):
            c = circ(support)
            if c and all(d2(c[0], p) <= c[1] for p in points):
                solutions.append(c[1])
    need(bool(solutions), 'MEB has no solution')
    return min(solutions)

def catalogue(P):
    balls = {}
    for q in range(2, 5):
        for S in combinations(range(len(P)), q):
            r = circ([P[i] for i in S])
            if r is None or min(r[2]) <= 0:
                continue
            c, beta, _ = r
            I = tuple(i for i, p in enumerate(P) if d2(c, p) < beta)
            U = tuple(i for i, p in enumerate(P) if d2(c, p) == beta)
            key = (c, beta)
            if key not in balls or (q, S) < (balls[key]['q'], balls[key]['S']):
                balls[key] = dict(q=q, S=S, I=I, U=U)
    return {key: b for key, b in balls.items() if len(b['I']) + b['q'] <= 3}

def gamma(P):
    V = {s: meb([P[i] for i in s]) for s in combinations(range(len(P)), 2)}
    E = {s: meb([P[i] for i in s]) for s in combinations(range(len(P)), 3)}
    return V, E

def components(V, E, beta, closed=True):
    admitted = (lambda x: x <= beta) if closed else (lambda x: x < beta)
    parent = {s:s for s, birth in V.items() if admitted(birth)}
    def find(s):
        while parent[s] != s:
            s = parent[s]
        return s
    for s, birth in E.items():
        if admitted(birth):
            faces = tuple(combinations(s, 2))
            need(all(f in parent for f in faces), 'Gamma face absent')
            for f in faces[1:]:
                parent[find(f)] = find(faces[0])
    out = {}
    for s in parent:
        out.setdefault(find(s), set()).update(s)
    return parent, find, frozenset(frozenset(v) for v in out.values())

def point_partition(P, V, E, beta, core=False):
    parent, find, _ = components(V, E, beta)
    groups = {}
    for x in range(len(P)):
        Fs = [s for s in V if x in s]
        alpha = min(V[s] for s in Fs)
        date = min(d2(P[x], P[y]) for y in range(len(P)) if y != x) if core else alpha
        if date > beta:
            groups[('singleton', x)] = {x}
        else:
            first = min(s for s in Fs if V[s] == alpha)
            groups.setdefault(('component', find(first)), set()).add(x)
    return frozenset(frozenset(v) for v in groups.values())

def band_majority(P, V, E, beta, inverse_beta=False, closed=True):
    """Diagnostic only: all incident pair witnesses inside one fixed radius band.

    The universe/denominator is fixed before the cut; unborn pairs contribute to
    the denominator but not the numerator. This is not a production rule port.
    """
    parent, find, _ = components(V, E, beta, closed)
    groups = {}
    for x in range(len(P)):
        alpha = min(v for s,v in V.items() if x in s)
        band = [s for s,v in V.items() if x in s and v <= F(1001,1000)**2*alpha]
        weight = lambda s: 1/V[s] if inverse_beta else F(1)
        denominator = sum(map(weight, band))
        score = {}
        for s in band:
            if s in parent:
                score[find(s)] = score.get(find(s), F(0)) + weight(s)
        winners = [c for c,w in score.items() if 2*w > denominator]
        need(len(winners) <= 1, 'unique strict majority with fixed denominator')
        key = ('component', winners[0]) if winners else ('singleton',x)
        groups.setdefault(key, set()).add(x)
    return frozenset(frozenset(v) for v in groups.values())

def labelset(group):
    return ''.join('ABCDEF'[x] for x in sorted(group))

def labelled(groups):
    return sorted(labelset(g) for g in groups)

def number(x):
    return [str(x.numerator), str(x.denominator)]

def sqrt_text(x):
    with localcontext() as ctx:
        ctx.prec = 40
        return str((Decimal(x.numerator)/Decimal(x.denominator)).sqrt())

def native_cover(export, beta, closed=True):
    levels = [F(*map(int, v)) for v in export['levels']]
    nodes = export['orders'][0]['nodes']
    raw_ids = [site[0] for site in export['sites']]
    covers = {}
    def cover(i):
        if i not in covers:
            _, _, ball, children = nodes[i]
            if children:
                covers[i] = frozenset().union(*(cover(c) for c in children))
            else:
                b = export['balls'][ball]
                covers[i] = frozenset(raw_ids[x] for x in b['I']+b['U'])
        return covers[i]
    admitted = (lambda x: x <= beta) if closed else (lambda x: x < beta)
    return frozenset(cover(i) for i, (lv, p, _, _) in enumerate(nodes)
                     if admitted(levels[lv]) and (p < 0 or not admitted(levels[nodes[p][0]])))

def analyze(name):
    b = (ROOT/'inputs'/f'{name}.u32le').read_bytes()
    ints = struct.unpack('<'+'I'*(len(b)//4), b)
    P = list(zip(ints[::3], ints[1::3], ints[2::3]))
    ex = json.loads((ROOT/'inputs'/f'{name}.json').read_text())
    need(len(P) == 6 and all(0 <= c < 2**18 for p in P for c in p), 'six u18 points')
    need(ex['n_points'] == ex['n_sites'] == 6, 'native cardinality')
    need(ex['K'] == ex['kmax_catalogue'] == 2, 'native order')
    need(sorted(ex['sites']) == [[i, *p] for i, p in enumerate(P)], 'PID coordinate mapping')
    ranked = [tuple(s[1:]) for s in ex['sites']]
    expected = catalogue(ranked)
    actual = {}
    for b in ex['balls']:
        beta = F(*map(int, ex['levels'][b['lv']]))
        c = tuple(F(int(v), int(b['c'][3])) for v in b['c'][:3])
        key = (c, beta)
        need(key not in actual, 'duplicate native ball')
        actual[key] = dict(q=b['q'], S=tuple(b['S']), I=tuple(b['I']), U=tuple(b['U']))
        need(b['p'] == len(b['I']) and b['u'] == len(b['U']), 'native incidence counts')
        need(len(set(b['I'])) == len(b['I']) and len(set(b['U'])) == len(b['U']), 'duplicate I/U')
    need(actual == expected and len(actual) == 13, 'complete native I/U/S* catalogue')
    V, E = gamma(P)
    events = sorted(set(V.values()) | set(E.values()) |
                    {F(*map(int, v)) for v in ex['levels']} |
                    {F(1300**2), F(1700**2)} |
                    {min(d2(P[x], P[y]) for y in range(6) if x != y) for x in range(6)})
    for beta in events:
        for closed in (True, False):
            need(native_cover(ex, beta, closed) == components(V, E, beta, closed)[2],
                 f'native Gamma plateau cover {name} beta={beta} closed={closed}')
    cohort = {}
    for x in range(6):
        inc = [s for s in V if x in s]
        alpha = min(V[s] for s in inc)
        first = sorted(s for s in inc if V[s] == alpha)
        cohort['ABCDEF'[x]] = {'alpha2': number(alpha), 'first': [labelset(s) for s in first]}
    relevant = [(0,1),(0,2),(1,2),(2,3),(3,4),(3,5),(4,5)]
    lengths = {labelset(s): {'squared': int(d2(P[s[0]], P[s[1]])),
                            'length_display_only': sqrt_text(F(d2(P[s[0]], P[s[1]])))} for s in relevant}
    need(len({lengths[k]['squared'] for k in ('AB','AC','BC')}) == 2, 'near-equilateral left')
    need(len({lengths[k]['squared'] for k in ('DE','DF','EF')}) == 2, 'near-equilateral right')
    delta = max(abs(d2(P[i], P[j])-4_000_000) for i, j in relevant)
    # Radius-band inclusion is judged without square roots: beta <= (1+eta)^2 alpha2.
    eta = F(1,1000)
    bands = {}
    for x in (2,3):
        alpha = min(v for s, v in V.items() if x in s)
        band = [s for s, v in V.items() if x in s and v <= (1+eta)**2*alpha]
        wanted = {(0,2),(1,2),(2,3)} if x == 2 else {(2,3),(3,4),(3,5)}
        need(set(band) == wanted, 'fixed eta=.001 has three local witnesses')
        bands['ABCDEF'[x]] = [labelset(s) for s in band]
    middle = {}
    for radius in (1300,1700):
        beta = F(radius**2)
        middle[str(radius)] = {'full_cover': labelled(components(V,E,beta)[2]),
                               'first_cover_projection': labelled(point_partition(P,V,E,beta)),
                               'core_projection': labelled(point_partition(P,V,E,beta,True)),
                               'fixed_band_uniform_majority': labelled(band_majority(P,V,E,beta)),
                               'fixed_band_inverse_beta_majority': labelled(band_majority(P,V,E,beta,True))}
        need(middle[str(radius)]['full_cover'] == ['ABC','CD','DEF'], 'three overlapping covering components')
        need(middle[str(radius)]['core_projection'] == list('ABCDEF'), 'core has no entered point')
        need(middle[str(radius)]['fixed_band_uniform_majority'] == ['ABC','DEF'], 'target with fixed band uniform')
        need(middle[str(radius)]['fixed_band_inverse_beta_majority'] == ['ABC','DEF'], 'target with fixed band inverse beta')
    for inverse in (False,True):
        previous = frozenset(frozenset({x}) for x in range(6))
        for beta in events:
            for closed in (False,True):
                groups = band_majority(P,V,E,beta,inverse,closed)
                need(all(any(old <= new for new in groups) for old in previous), 'nested fixed-band diagnostic')
                previous = groups
    # Every PID permutation changes representation only, not the exact first-cover/core partition.
    base_cover = point_partition(P,V,E,F(1500**2))
    base_core = point_partition(P,V,E,F(1500**2),True)
    for order in permutations(range(6)):
        inverse = {old:new for new, old in enumerate(order)}
        PP = [P[i] for i in order]
        VV = {tuple(sorted(inverse[i] for i in s)): v for s,v in V.items()}
        EE = {tuple(sorted(inverse[i] for i in s)): v for s,v in E.items()}
        remap = lambda groups: frozenset(frozenset(order[i] for i in g) for g in groups)
        need(remap(point_partition(PP,VV,EE,F(1500**2))) == base_cover, 'PID permutation cover projection')
        need(remap(point_partition(PP,VV,EE,F(1500**2),True)) == base_core, 'PID permutation core projection')
        need(labelled(remap(band_majority(PP,VV,EE,F(1500**2)))) == ['ABC','DEF'], 'PID permutation fixed band vote')
    # These similarities preserve the integer grid; arbitrary rotation then requantization does not.
    for scale in (1,3):
        PP = [tuple(scale*p[k]+(17,29,41)[k] for k in (2,0,1)) for p in P]
        CC = catalogue(PP)
        need(len(CC) == len(catalogue(P)), 'similarity catalogue cardinality')
        VP, EP = gamma(PP)
        need(VP == {s:v*scale**2 for s,v in V.items()}, 'similarity pair levels')
        need(EP == {s:v*scale**2 for s,v in E.items()}, 'similarity triple levels')
        need(point_partition(PP,VP,EP,F((1500*scale)**2)) == base_cover, 'similarity point projection')
    return {'points': P, 'engine_commit_claim_in_capture': ex['engine_commit'],
            'catalogue_balls': len(actual), 'native_tree_nodes': len(ex['orders'][0]['nodes']),
            'exact_pair_vertices': len(V), 'exact_triple_edges': len(E),
            'open_closed_cuts': 2*len(events), 'squared_length_error_bound': delta,
            'edge_geometry': lengths, 'exact_first_cohorts': cohort,
            'middle_cuts': middle, 'band_eta_1_over_1000': bands,
            'triangle_radius2': number(E[(0,1,2)]),
            'first_full_global_radius2': number(next(beta for beta in sorted(set(V.values())|set(E.values()))
                if components(V,E,beta)[2] == frozenset({frozenset(range(6))}))),
            'core_radius2_by_pid': [min(d2(P[x],P[y]) for y in range(6) if x != y) for x in range(6)],
            'pid_permutations_checked': 720, 'integer_similarities_checked': 2}

results = {n: analyze(n) for n in ('aretes_plus_courtes','pont_plus_court','pont_plus_long')}
need((ROOT/'inputs/aretes_plus_courtes.u32le').read_bytes() == (ROOT/'inputs/pont_plus_long.u32le').read_bytes(), 'two fixture names same binary')
need((ROOT/'inputs/aretes_plus_courtes.json').read_bytes() == (ROOT/'inputs/pont_plus_long.json').read_bytes(), 'two fixture names same export')
need(results['aretes_plus_courtes']['exact_first_cohorts']['C']['first'] == ['AC','BC'], 'base C first cohort')
need(results['pont_plus_court']['exact_first_cohorts']['C']['first'] == ['CD'], 'short bridge C first cohort')
need(results['pont_plus_court']['middle_cuts']['1300']['first_cover_projection'] == ['AB','CD','EF'], 'first-cover short bridge behavior')
need(results['aretes_plus_courtes']['middle_cuts']['1300']['first_cover_projection'] == ['ABC','DEF'], 'first-cover longer bridge behavior')
for a in (1,1000):
    P = [(0,0,0),(a,a,0),(a,0,a)]
    need({d2(P[i],P[j]) for i,j in combinations(range(3),2)} == {2*a*a}, 'integer exact 3D equilateral control')
    cr = circ(P)
    need(cr[1] == F(2*a*a,3) and cr[2] == (F(1,3),)*3, '3D equilateral center')

spec = importlib.util.spec_from_file_location('frozen_vx', ROOT/'sources/vx.py')
vx = importlib.util.module_from_spec(spec)
spec.loader.exec_module(vx)
t = vx.Tower([(F(3,2),0,0),(3,0,0)], 2)
need(t.P == [(1,0,0),(3,0,0)], 'frozen integer verifier silently int-coerces Fraction coordinates')
out = {'status': 'PASS', 'checks': CHECKS, 'arithmetic': 'Fraction; no floats used for decisions',
       'scope': 'Archived u18 six-site catalogue and FULL_K2 cover, not ideal equilateral coordinates or a fresh native run',
       'fraction_coordinate_int_coercion_demonstrated': {'input': '3/2', 'stored': 1,
         'scope': 'Outside intended integer input domain; future ideal oracle must not pass through this constructor'},
       'fixtures': results}
print(json.dumps(out, indent=2))
