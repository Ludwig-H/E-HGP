"""Contrelecture bornee S0/S1 : sept nuages, K2, tous leurs plateaux exacts.

On charge exclusivement model/definition/supports de copies Git 5ad, dans
un paquet prive vide : ni __init__, constructive, judge ou dumps. La voie
opposee est le solveur strict Gram/Fraction et Gamma2 du recu D2 deja clos,
copie sans modification ; son algorithme n'importe aucun module produit.
Les 13 mutants publies sont appliques en memoire aux seuls modules copies,
sur leurs petites fixtures designees. On exige une exception portant la
cause nommee ; aucun crash ou simple changement de digest ne suffit.
Ce n'est pas une nouvelle qualification native ni la suite S1 complete.
"""
from fractions import Fraction as F
from itertools import combinations
from pathlib import Path
import hashlib
import importlib.util
import json
import sys
import types


HERE = Path(__file__).resolve().parent
CHECKS = 0


def require(ok, message):
    global CHECKS
    CHECKS += 1
    if not ok:
        raise ValueError(message)


def module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    obj = importlib.util.module_from_spec(spec)
    sys.modules[name] = obj
    spec.loader.exec_module(obj)
    return obj


def load():
    path = HERE / 'published/morsehgp3D_v11/reference/hgp11_ref'
    package = types.ModuleType('audit_supports_private')
    package.__path__ = [str(path)]
    sys.modules[package.__name__] = package
    for name in ('model', 'definition', 'supports'):
        obj = module(package.__name__ + '.' + name, path / (name + '.py'))
    require(not any(x.endswith(('.constructive', '.judge', '.dumps')) for x in sys.modules),
            'independent stage loader')
    return obj, module('audit_independent_gram', HERE / 'independent/gram_gamma.py')


FIXTURES = {
    'd2': [(2, 10, 0), (18, 10, 0), (10, 20, 0), (9, 3, 0), (11, 3, 0)],
    'line_weak': [(0, 0, 0), (1, 0, 0), (2, 0, 0)],
    'passenger': [(0, 0, 0), (2, 2, 0), (4, 0, 0), (8, 0, 0)],
    'growth': [(1, 8, 0), (5, 10, 0), (9, 8, 0), (5, 0, 0)],
    'equilateral': [(0, 0, 0), (2, 2, 0), (2, 0, 2)],
    'cube': [(x, y, z) for x in (0, 2) for y in (0, 2) for z in (0, 2)],
    'e5': [(0, 0, 7), (0, 9, 6), (1, 4, 0), (0, 0, 1), (4, 1, 2)],
}


def window_events(pts, pairs, triples, levels, G, all_vertices):
    """Reconstruction independante par toutes les partitions, sans memo de
    l'oracle S1 ni son sweep : supprimer les liaisons hors fenetre, garder
    ou retirer les sommets dont la boule est hors fenetre."""
    all_balls = G.catalogue(pts, len(pts))
    info = {(b['center'], b['level']): b for b in all_balls}
    def meb_key(part):
        candidates = []
        for q in range(1, min(4, len(part)) + 1):
            for support in combinations(part, q):
                sphere = G.sphere(support, pts)
                if sphere is None or not all(w > 0 for w in sphere[2]):
                    continue
                center, radius, _weights = sphere
                if all(G.dot(G.sub(pts[i], center), G.sub(pts[i], center)) <= radius for i in part):
                    candidates.append((radius, center))
        radius, center = min(candidates)
        return center, radius
    keep = {p: len(info[meb_key(p)]['I']) + info[meb_key(p)]['qmin'] <= 3 for p in pairs + triples}
    previous, events = (), []
    for cut in sorted(set(levels.values())):
        retained = [g for g in triples if keep[g] and levels[g] <= cut]
        vertices = {v for v in pairs if levels[v] <= cut and (all_vertices or keep[v])}
        vertices.update(v for g in retained for v in combinations(g, 2))
        now = G.gamma(tuple(sorted(vertices)), tuple(retained), levels, cut)
        for group in now:
            kids = [old for old in previous if set(old) & set(group)]
            if len(kids) > 1:
                events.append((str(cut), len(kids)))
        previous = now
    return events


def check_fixture(name, points, S, G):
    pts = tuple(tuple(F(v) for v in p) for p in points)
    oracle = S.Supports(points)
    result = oracle.order(2)
    pairs = tuple(combinations(range(len(pts)), 2))
    triples = tuple(combinations(range(len(pts)), 3))
    levels = {p: G.meb(p, pts) for p in pairs + triples}
    for part, beta in levels.items():
        require(oracle.definition.beta(part) == beta, name + ' independent MEB')
    cuts = sorted(set(levels.values()))
    all_groups = {}
    for cut in cuts:
        groups = G.gamma(pairs, triples, levels, cut)
        by_node = {}
        for part in pairs:
            if levels[part] <= cut:
                node = oracle.definition.node_at(2, part, cut)
                by_node.setdefault(node, []).append(part)
        got = tuple(sorted(tuple(sorted(x)) for x in by_node.values()))
        require(got == groups, name + ' exact closed Gamma partition')
        all_groups[cut] = groups
    catalogue = G.catalogue(pts, 2)
    cat = {(b['center'], b['level']): b for b in catalogue if len(b['I']) + len(b['U']) >= 2}
    balls = {(b.center, b.level): b for b in result.balls}
    require(set(cat) == set(balls), name + ' complete event window')
    for key, ball in balls.items():
        independent = cat[key]
        require((ball.inner, ball.shell) == (independent['I'], independent['U']), name + ' complete I/U')
        require(set(ball.supports) == set(independent['supports']), name + ' all positive supports')
        parts = tuple(combinations(ball.pop, 2))
        owners = {oracle.definition.node_at(2, p, ball.level) for p in parts}
        require(owners == {ball.node}, name + ' closed attachment')
        strict = [p for p in parts if levels[p] < ball.level]
        prev = max((c for c in cuts if c < ball.level), default=None)
        require(not strict or prev is not None, name + ' strict cut exists')
        ant = {oracle.definition.node_at(2, p, prev) for p in strict}
        require(ant == set(ball.ant), name + ' all strict branches')
        t = 2 - ball.p
        compressed = [tuple(sorted(ball.inner + a)) for a in combinations(ball.shell, t)]
        strict_count = sum(levels[p] < ball.level for p in compressed)
        cofaces = [g for g in combinations(ball.pop, 3) if levels[g] == ball.level]
        per_q = [sum(set(q) <= set(g) for g in cofaces) for q in ball.supports]
        require(ball.counts['strict_traces'] == strict_count, name + ' strict trace counts')
        require(ball.counts['cofaces'] == len(cofaces), name + ' coface counts')
        require(list(ball.counts['cofaces_support']) == per_q, name + ' support incidences')
        if prev is not None:
            for u in ant:
                parent = result.parent[u]
                owner = parent if parent >= 0 and result.tree.nodes[parent].level == ball.level else u
                require(owner == ball.node, name + ' whole-plateau parent rule')
    extra = {}
    if name == 'd2':
        lam = F(1681, 25)
        previous_catalogue = max(b['level'] for b in catalogue if b['level'] < lam)
        previous_gamma = max(c for c in cuts if c < lam)
        require(previous_catalogue == 41 and levels[(0, 1)] == 64, 'D2 published levels')
        require(previous_gamma >= 64, 'direct Gamma cut contains strict trace')
        require(oracle.definition.node_at(2, (3, 4), previous_catalogue)
                == oracle.definition.node_at(2, (0, 1), previous_gamma), 'D2 transported seed class')
        rejected = False
        try:
            oracle.definition.node_at(2, (0, 1), previous_catalogue)
        except S.InvariantError:
            rejected = True
        require(rejected, 'D2 false trace query must fail')
        extra = {'previous_catalogue': str(previous_catalogue), 'initial': '64', 'target': str(lam),
                 'direct_gamma_previous': str(previous_gamma), 'invalid_trace_query_rejected': rejected}
    if name == 'line_weak':
        ball = next(b for b in result.balls if b.level == 1)
        require(ball.p == 1 and ball.q == 2 and not ball.strong, 'weak line event')
        require(levels[(0, 2)] == ball.level, 'E2 arbitrary part non-strict')
    if name == 'passenger':
        # The documented passenger is K1, not K2. Both orders use the
        # same complete cloud but have different open branch counts.
        k1 = oracle.order(1)
        passenger = next(b for b in k1.balls if b.level == 4 and b.m == 3)
        require(passenger.role == S.ROLE_MERGE and len(passenger.ant) == 1, 'passenger at whole-plateau merger')
    if name == 'cube':
        ball = next(b for b in result.balls if b.level == 3)
        require(sorted(map(len, ball.supports)) == [2, 2, 2, 2, 4, 4], 'cube all supports preserved')
    if name in ('d2', 'e5'):
        windows = {}
        for retained in (True, False):
            independent = window_events(pts, pairs, triples, levels, G, retained)
            actual = [(str(level), kids) for level, kids in oracle.window_tree(2, all_vertices=retained)]
            require(actual == independent, name + ' independent window restriction')
            windows[str(retained)] = independent
        common = [('162/25', 3), ('189/17', 3)] if name == 'e5' else [('65/2', 3)]
        top = '83886/3563' if name == 'e5' else '1681/25'
        late = '24' if name == 'e5' else '145/2'
        require(windows['True'] == common + [(top, 3), (late, 2)], name + ' keep vertices reading')
        require(windows['False'] == common + [(top, 2), (late, 2)], name + ' omit vertices reading')
        extra['window_readings'] = windows
    return dict(n=len(points), cuts=len(cuts), balls=len(balls), extra=extra)


def check_mutants(S):
    M = module('audit_support_mutants', HERE / 'published/morsehgp3D_v11/reference/ref_mutants.py')
    source_path = HERE / 'published/morsehgp3D_v11/reference/hgp11_ref/supports.py'
    source = source_path.read_text()
    require(len(M.SUPPORT_MUTANTS) == 13, 'published mutant count13')
    fixtures = {
        'triangle_aigu': [(0, 0, 0), (2, 0, 0), (1, 2, 0)],
        'passagere': FIXTURES['passenger'], 'triangle_equilateral': FIXTURES['equilateral'],
        'carre': [(0, 0, 0), (2, 0, 0), (2, 2, 0), (0, 2, 0)],
        'cube': FIXTURES['cube'], 'growth_abcz': FIXTURES['growth'],
        'triangle_droit': [(0, 0, 0), (4, 0, 0), (0, 3, 0)], 'ligne3': FIXTURES['line_weak'],
    }
    results = {}
    for name, recipe in sorted(M.SUPPORT_MUTANTS.items()):
        require(source.count(recipe['old']) == 1, 'exact published mutant patch ' + name)
        cases = [(f.split('@')[0], int(f.split('@')[1])) for f in recipe['fixtures']]
        for label, k in cases:
            S.Supports(fixtures[label]).order(k)
            require(True, 'intact mutant witness ' + name)
        altered = types.ModuleType('audit_supports_private.mutant_' + name)
        altered.__package__ = 'audit_supports_private'
        altered.__file__ = str(source_path)
        exec(compile(source.replace(recipe['old'], recipe['new']), str(source_path), 'exec'), altered.__dict__)
        failures = []
        for label, k in cases:
            try:
                altered.Supports(fixtures[label]).order(k)
            except S.InvariantError as exc:
                failures.append(str(exc))
        require(any(recipe['cause'] in f for f in failures), 'causal oracle mutant kill ' + name)
        results[name] = dict(cause=recipe['cause'], fixtures=recipe['fixtures'], errors=failures)
    return results


def main():
    S, G = load()
    data = {name: check_fixture(name, pts, S, G) for name, pts in FIXTURES.items()}
    mutants = check_mutants(S)
    sources = {str(p.relative_to(HERE)): hashlib.sha256(p.read_bytes()).hexdigest()
               for p in [HERE / 'published/morsehgp3D_v11/reference/hgp11_ref' / (x + '.py')
                         for x in ('model', 'definition', 'supports')] + [HERE / 'independent/gram_gamma.py']}
    print(json.dumps(dict(status='PASS', checks=CHECKS, independent_guards=G.CHECKS,
                          independent_mebs=G.MEB_CALLS, fixtures=data, mutants=mutants, sources=sources,
                          script_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                          scope='seven bounded exact fixtures + 13 oracle mutants; no native or full S1 suite'),
                     indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
