#!/usr/bin/env python3
"""Audit borne de T2 ; aucun produit modifie, aucune suite complete ni donnee reelle."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
from itertools import combinations
from fractions import Fraction

ROOT = Path(__file__).resolve().parents[4]
PIN = '274592a30f6961cb7702125dcd2f031ff22b7df2'
REF = ROOT / 'morsehgp3D_v12/reference'
sys.path.insert(0, str(REF))
from hgp12_ref import Definition, Reference
from hgp12_ref.judge import coherence, compare_orders
import test_resolution_v12 as gate


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def digest(data):
    return hashlib.sha256(data).hexdigest()


sources = sorted(str(p.relative_to(ROOT)) for p in (REF / 'hgp12_ref').glob('*.py')) + [
    'morsehgp3D_v12/reference/test_resolution_v12.py',
    'morsehgp3D_v12/reference/test_supports.py',
    'morsehgp3D_v12/docs/CONTRAT_TOUR.md',
    'morsehgp3D_v12/docs/CONTRAT_CATALOGUE.md',
    'morsehgp3D_v12/docs/CONTRAT_NUMERIQUE.md',
    'morsehgp3D_v12/docs/OBJET_ET_CONTRAT_MATHEMATIQUE.md',
    'morsehgp3D_v11/receipts/conception_v11_20261002/conception/CONCEPTION_TOUR.md',
    'morsehgp3D_v11/docs/MATHEMATIQUES.md',
    'docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md',
]
before = {p: digest((ROOT / p).read_bytes()) for p in sources}
for path, sha in before.items():
    blob = subprocess.check_output(['git', 'show', PIN + ':' + path], cwd=ROOT)
    require(digest(blob) == sha, 'source differente du pin : ' + path)
own_before = digest(Path(__file__).read_bytes())


class Traced(Reference):
    def __init__(self, *args, **kwargs):
        self.trace_active = False
        self.traces = []
        super().__init__(*args, **kwargs)

    def _meb(self, part):
        out = super()._meb(part)
        if self.trace_active:
            self.current.append(out[2][1])
        return out

    def resolve_v12(self, part, k):
        self.current = []
        self.trace_active = True
        try:
            out = super().resolve_v12(part, k)
        finally:
            self.trace_active = False
        require(all(a > b for a, b in zip(self.current, self.current[1:])), 'niveau non decroissant')
        require(self.balls[out[1]].level <= self.current[0], 'cible future')
        self.traces.append(tuple(self.current))
        return out


FIXTURES = [
    ('WIT-D2', [(2,10,0),(18,10,0),(10,20,0),(9,3,0),(11,3,0)], 2),
    ('WIT-MEMO', [(x,0,0) for x in (0,2,4,6)], 2),
    ('carre', [(0,0,0),(2,0,0),(2,2,0),(0,2,0)], 4),
    ('cube', [(x,y,z) for x in (0,2) for y in (0,2) for z in (0,2)], 4),
    ('tetra_centre', [(0,0,0),(2,2,0),(2,0,2),(0,2,2),(1,1,1)], 5),
    ('WIT-E5', [(0,0,7),(0,9,6),(1,4,0),(0,0,1),(4,1,2)], 4),
    ('triangle_equilateral', [(1,0,0),(0,1,0),(0,0,1)], 3),
    ('voisins_repli', [(x,0,0) for x in (8,9,10,50,70,110,111,112)], 2),
    ('grid3_11_3_inerte', [(1,2,1),(0,2,2),(2,1,0),(0,0,2),(0,0,1),(2,0,0),(2,0,1)], 4),
    ('carre_sature_du_catalogue', [(0,0,0),(4,0,0),(4,4,0),(0,4,0),(2,2,0),(2,1,0)], 5),
]

rows, total_traces, total_steps, inert_targets, lemma_cases = [], 0, 0, 0, 0
for name, pts, kmax in FIXTURES:
    truth = Definition(pts)
    expected = [truth.order(k) for k in range(1, kmax + 1)]
    stats = {}
    for policy in gate.POLICIES:
        ref = Traced(pts, kmax, resolution=policy)
        previous = None
        for k, want in enumerate(expected, 1):
            got = ref.order(k)
            require(coherence(got, len(pts), previous) is None, name + ' coherence')
            require(not compare_orders(want, got), name + ' ecart definition ' + policy)
            previous = got
        # Tous les representants de cette petite entree : cible strictement avant la jonction appelante.
        for ball in ref.balls:
            for k in range(ball.lo, ball.hi + 1):
                kind, reps = ref.cell(ball, k)
                if kind == 'birth':
                    continue
                for rep in reps:
                    target = ref.resolve_v12(tuple(sorted(ball.inner + rep)), k)
                    require(ref.balls[target[1]].level < ball.level, name + ' cible au meme plateau')
        stats[policy] = dict(ref.v12_stats)
        total_traces += len(ref.traces)
        total_steps += sum(len(t) for t in ref.traces)
        inert_targets += ref.v12_stats['inert_targets']
    # Lemme hors catalogue juge pour toutes les parties, a partir de boules minimales exactes.
    base = Reference(pts, kmax)
    for k in range(1, kmax + 1):
        for part in combinations(range(len(pts)), k):
            a, c, key = base._meb(part)
            if key in base._by_key:
                continue
            inner, shell = base._census(a, c)
            ball = base._ball(a, c, key, inner, shell)
            lemma_cases += 1
            require(ball.qmin <= min(4, k), 'borne Caratheodory')
            require(ball.p >= kmax - 2, 'hors catalogue p')
            require(ball.p >= k or k >= kmax - 1, 'hors catalogue ordre')
            require(ball.p != kmax - 2 or ball.qmin == 4, 'hors catalogue egalite')
    rows.append({'name': name, 'points': pts, 'kmax': kmax, 'stats': stats})
require(inert_targets > 0, 'cibles inertes non exercees')
require(lemma_cases > 0, 'hors catalogue non exerce')
require(not gate.fact_neighbour_fallback(sys.modules['hgp12_ref']), 'fait repli')
require(not gate.fact_complete_census_in_catalogue(sys.modules['hgp12_ref']), 'fait complet catalogue')

# T1 exclut les boules de rayon nul, contrairement au catalogue INTERNE de Reference.
singleton_ref = Reference([(0,0,0),(2,0,0),(4,0,0)], 3)
sa, sc, skey = singleton_ref._meb((singleton_ref.internal[0],))
sball = singleton_ref._by_key[skey]
positive_catalogue = {key for key in singleton_ref._by_key if key[1] > 0}
require(skey not in positive_catalogue and sball.level == 0 and sball.p == 0 and sball.qmin == 1,
        'singleton hors catalogue positif')
require(not sball.p >= singleton_ref.kmax - 2, 'contre-exemple k1')
zero_radius_counterexample = {'points': singleton_ref.input, 'K': 3, 'k': 1, 'F': [0],
    'level': '0', 'p': 0, 'qmin': 1, 'reference_internal_catalogue_contains_ball': True,
    'positive_Cat_K_contains_ball': False, 'claimed_p_at_least_K_minus_2': False,
    'resolve1_k1_is_early_site_return': True}

# Echec de la sonde S* ne signifie pas hors catalogue, meme pour un census sature.
pts = FIXTURES[-1][1]
ref = Reference(pts, 5)
part = tuple(sorted(ref.internal[i] for i in (1, 3)))
a, c, key = ref._meb(part)
ball = ref._by_key[key]
local_support = tuple(sorted(ref.site_of[x] for x in part))
require(ball.support != local_support and ball.p == 2 and ball.qmin == 2, 'carre sature')
require(all(b.support != local_support for b in ref.balls), 'support local dans la table')
saturated_in_catalogue = {'k': 2, 'K': 5, 'p': ball.p, 'qmin': ball.qmin,
    'level': str(ball.level), 'support_positions': [ref.sites[s] for s in ball.support],
    'part_positions': [ref.sites[s] for s in local_support], 'catalogue_hit_by_key': True,
    'support_table_miss': True, 'census_saturated': True}

# Dates explicites : le rang precedent ne majore pas beta(F0), et le terminal ne date pas la cible.
ref = Traced(FIXTURES[0][1], 2, resolution='v12_indices')
part = tuple(sorted(ref.internal[i] for i in (0, 1)))
kind, target = ref.resolve_v12(part, 2)
lam = Fraction(1681, 25)
previous = max(b.level for b in ref.balls if b.level < lam)
require(ref.traces[-1] == (Fraction(64), Fraction(1)) and previous == 41, 'WIT-D2 dates')
d2_dates = {'initial': '64', 'previous_catalogue_level': '41', 'junction': str(lam),
            'terminal': str(ref.balls[target].level), 'kind': kind}
ref = Traced(FIXTURES[1][1], 2, resolution='v12_indices')
part = tuple(sorted(ref.internal[i] for i in (0, 3)))
kind, target = ref.resolve_v12(part, 2)
require(ref.traces[-1] == (Fraction(9), Fraction(1)), 'WIT-MEMO dates')
memo_dates = {'initial': '9', 'terminal': str(ref.balls[target].level), 'kind': kind,
              'early_use_at_terminal_level_authorized': False}

# Meme plateau (cycle a quatre sommets), meme union par taille et departage par indice.
def attachment(edges):
    parent, size = list(range(4)), [1] * 4
    for a, b in edges:
        while parent[a] != a:
            a = parent[a]
        while parent[b] != b:
            b = parent[b]
        if a != b:
            if (size[a], -a) < (size[b], -b):
                a, b = b, a
            parent[b] = a
            size[a] += size[b]
    depths = []
    for x in range(4):
        depth = 0
        while parent[x] != x:
            x = parent[x]
            depth += 1
        depths.append(depth)
    return {'edges': edges, 'parent': parent, 'depths': depths, 'maximum': max(depths)}

order_a = attachment([(0,1),(1,2),(2,3),(3,0)])
order_b = attachment([(0,1),(2,3),(1,2),(3,0)])
require(order_a['maximum'] == 1 and order_b['maximum'] == 2, 'profondeurs DSU')

# Census sature simple : meme certificat p>=2, ordre de visite different, travail different.
def visits(xs):
    inside = 0
    for seen, x in enumerate(xs, 1):
        inside += (x - 5) ** 2 < 25
        if inside >= 2:
            return seen
    raise RuntimeError('pas de saturation')
require(visits([4,6,20]) == 2 and visits([20,4,6]) == 3, 'ordre census')

after = {p: digest((ROOT / p).read_bytes()) for p in sources}
require(before == after and own_before == digest(Path(__file__).read_bytes()), 'sources modifiees')
out = {'pin': PIN, 'source_sha256': before, 'check_sha256': own_before,
       'source_pin_and_before_after_verified': True, 'clouds': rows,
       'cloud_count': len(rows), 'orders_per_policy': sum(c[2] for c in FIXTURES),
       'policies': list(gate.POLICIES), 'trace_count': total_traces,
       'trace_meb_steps': total_steps, 'inert_targets': inert_targets,
       'outside_catalogue_parts_checked': lemma_cases, 'all_definition_comparisons_pass': True,
       'developer_facts_pass': 2, 'saturated_in_catalogue': saturated_in_catalogue,
       'HORS_CAT_zero_radius_counterexample': zero_radius_counterexample,
       'WIT_D2_dates': d2_dates, 'WIT_MEMO_dates': memo_dates,
       'same_plateau_attachment_orders': [order_a, order_b],
       'same_saturated_certificate_visits': [2, 3],
       'gcp_used': False, 'native_tests_run': False, 'full_quick_suite_run': False}
print(json.dumps(out, sort_keys=True, separators=(',', ':')))
