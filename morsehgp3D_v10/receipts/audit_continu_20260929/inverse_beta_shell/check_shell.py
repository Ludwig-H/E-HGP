"""Small exact audit: fixed inverse-beta atoms change at cospherical contact.

No engine invocation. Geometry and Gamma use the frozen Fraction reference.
The majority reconstruction below is independent of native event IDs.
"""
from fractions import Fraction as Q
from itertools import combinations
from pathlib import Path
import importlib.util
import hashlib
import json
import sys
import time

if len(sys.argv) != 2:
    raise SystemExit('usage: check_shell.py FROZEN_FRACTION_REFERENCE')
REFERENCE = Path(sys.argv[1]).resolve()
INPUTS = (Path(__file__).resolve(), REFERENCE)
BEFORE = {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in INPUTS}
spec = importlib.util.spec_from_file_location('frozen_ref', REFERENCE)
ref = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ref)


def check(condition, message):
    if not condition:
        raise RuntimeError(message)


def part_and_witnesses(P, beta):
    k = 2
    balls = [b for b in ref.critical_balls(P)
             if b.p + b.m >= k and b.p + b.qmin <= k]
    # Full Gamma, not a graph inferred only from our atoms.
    pairs = list(combinations(range(len(P)), k))
    pair_beta = {F: ref.meb(P, F)[0] for F in pairs}
    triples = list(combinations(range(len(P)), k + 1))
    triple_beta = {F: ref.meb(P, F)[0] for F in triples}
    dsu = ref.DSU()
    for F in pairs:
        if pair_beta[F] <= beta:
            dsu.find(F)
    for G in triples:
        if triple_beta[G] <= beta:
            Fs = list(combinations(G, k))
            for F in Fs[1:]:
                dsu.union(Fs[0], F)
    W = [Q(0) for _ in P]
    masses = [{} for _ in P]
    trace = []
    for b in balls:
        check(b.level > 0 and b.p == 0 and b.qmin == 2, 'not strong K2 atom')
        w = 1 / b.level
        ids = tuple(sorted(set(b.I) | set(b.U)))
        for x in ids:
            W[x] += w
        component = None
        if b.level <= beta:
            # All k-parties inside one closed ball connect through k+1 unions
            # at this beta; choosing any first k ids anchors the same component.
            anchors = list(combinations(ids, k))
            check(all(pair_beta[F] <= b.level for F in anchors), 'invalid anchor')
            roots = {dsu.find(F) for F in anchors}
            check(len(roots) == 1, 'ball straddles Gamma components')
            component = next(iter(roots))
            for x in ids:
                masses[x][component] = masses[x].get(component, Q(0)) + w
        trace.append({'beta': str(b.level), 'I': list(b.I), 'U': list(b.U),
                      'qmin': b.qmin, 'weight': str(w)})
    owners = []
    for x in range(len(P)):
        found = [c for c, m in masses[x].items() if 2 * m > W[x]]
        check(len(found) <= 1, 'majority lost exclusivity')
        owners.append(('component', found[0]) if found else ('singleton', x))
    groups = {}
    for x, owner in enumerate(owners):
        groups.setdefault(owner, []).append(x)
    covers = {}
    for F in pairs:
        if pair_beta[F] <= beta:
            covers.setdefault(dsu.find(F), set()).update(F)
    return {'blocks': sorted(groups.values()),
            'gamma_covers': sorted(sorted(s) for s in covers.values()),
            'atoms': trace, 'total_weights': [str(w) for w in W],
            'component_weights': [sorted(str(m) for m in row.values()) for row in masses]}, owners


def analyze(P):
    check(ref.rank([ref.sub(p, P[0]) for p in P[1:]]) == 3, 'not full dimensional')
    levels, closed, _opened = ref.gamma_cuts(P, 2)
    critical = {b.level for b in ref.critical_balls(P)
                if b.p + b.m >= 2 and b.p + b.qmin <= 2}
    cuts = sorted(set(levels) | critical)
    # Intermediates matter: membership follows Gamma between all events.
    cuts = sorted(set(cuts) | {(a+b)/2 for a,b in zip(cuts, cuts[1:])})
    first_CA = None
    previous = None
    checks = 0
    trace = []
    for beta in cuts:
        record, owners = part_and_witnesses(P, beta)
        if previous is not None:
            for i, j in combinations(range(len(P)), 2):
                check(previous[i] != previous[j] or owners[i] == owners[j], 'nonlaminar majority')
                checks += 1
        if owners[0] == owners[1] and first_CA is None:
            first_CA = beta
        # Cross-check coverage against the separate complete cut oracle.
        t = max(i for i, value in enumerate(levels) if value <= beta)
        check(record['gamma_covers'] == sorted(sorted(s) for s in closed[t]), 'Gamma oracle mismatch')
        trace.append({'beta': str(beta), 'blocks': record['blocks'],
                      'gamma_covers': record['gamma_covers']})
        previous = owners
    local_beta = ref.d2(P[0], P[1]) / 4
    local, _ = part_and_witnesses(P, local_beta)
    check(first_CA is not None, 'no final pair union')
    return {'points': P, 'local_beta': str(local_beta), 'CA_first_beta': str(first_CA),
            'at_local_beta': local, 'trace': trace, 'nesting_pair_checks': checks}


def main():
    started = time.monotonic()
    rows = []
    for M in (10, 100, 1024, 2048):
        original = [(0,0,0), (4*M,0,0), (0,5*M,0), (0,0,100*M)]
        moved = [(1,1,1)] + original[1:]
        a = analyze(original)
        b = analyze(moved)
        check(len(a['at_local_beta']['atoms']) == 6, 'original catalogue wrong')
        check(len(b['at_local_beta']['atoms']) == 3, 'perturbed catalogue wrong')
        check(a['CA_first_beta'] == str(Q(41*M*M,4)), 'original pair date wrong')
        check(b['CA_first_beta'] == b['local_beta'], 'perturbed entry not early')
        check([0,1] not in a['at_local_beta']['blocks'] and [0,1] in b['at_local_beta']['blocks'], 'no early change')
        check(all(max(p) <= 262143 and min(p) >= 0 for P in (original,moved) for p in P), 'outside u18')
        # The fusion AB is NOT discarded from FULL: it has p+qmin=3
        # after perturbation and is still admitted at K2 (fusion allowance K+1).
        diameter_AB_beta = Q(41*M*M, 4)
        fusion_before = [ball for ball in ref.catalogue(original, 2)
                         if ball.level == diameter_AB_beta]
        fusion_after = [ball for ball in ref.catalogue(moved, 2)
                        if ball.level == diameter_AB_beta]
        check(len(fusion_before) == len(fusion_after) == 1, 'fusion disappeared from FULL catalogue')
        check(fusion_before[0].p == 0 and fusion_before[0].U == (0,1,2), 'original fusion census')
        check(fusion_after[0].p == 1 and fusion_after[0].I == (0,) and fusion_after[0].U == (1,2), 'perturbed fusion census')
        covers_before = part_and_witnesses(original, diameter_AB_beta)[0]['gamma_covers']
        covers_after = part_and_witnesses(moved, diameter_AB_beta)[0]['gamma_covers']
        check([0,1,2] in covers_before and [0,1,2] in covers_after, 'Gamma ABC fusion missing')
        rows.append({'M': M, 'matched_displacement_squared': 3,
                     'normalized_displacement_squared': str(Q(3,M*M)),
                     'original': a, 'perturbed': b,
                     'FULL_ABC_fusion_beta_before_and_after': str(diameter_AB_beta),
                     'FULL_fusion_p_q_before': [fusion_before[0].p, fusion_before[0].qmin],
                     'FULL_fusion_p_q_after': [fusion_after[0].p, fusion_after[0].qmin],
                     'normalized_pair_jump': str((Q(a['CA_first_beta'])-Q(b['CA_first_beta']))/(M*M))})
    after = {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in INPUTS}
    check(BEFORE == after, 'input changed during audit')
    print(json.dumps({'status': 'INVERSE_BETA_SHELL_DISCONTINUITY_REPRODUCED',
                      'scope': 'K2_four_site_exact_Fraction_Gamma_no_native_call',
                      'GCP_used': False, 'engine_modified': False,
                      'argv': sys.argv, 'optimized': sys.flags.optimize,
                      'hashes_before': BEFORE, 'hashes_after': after,
                      'wall_seconds': time.monotonic() - started,
                      'rows': rows}, indent=2))


if __name__ == '__main__':
    main()
