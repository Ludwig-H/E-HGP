"""Small exact Gamma K / component CDF counterproof, two collinear clouds."""
from fractions import Fraction as F
from itertools import combinations
from math import comb
import json


def require(ok, why):
    if not ok:
        raise ValueError(why)


def proof(K, coords, radii, expected):
    # In one dimension the MEB radius is half the difference of endpoints.
    facets = list(combinations(range(len(coords)), K))
    birth = {S: (max(coords[i] for i in S)-min(coords[i] for i in S))/2 for S in facets}
    cofaces = list(combinations(range(len(coords)), K+1))
    events = {S: (max(coords[i] for i in S)-min(coords[i] for i in S))/2 for S in cofaces}
    alpha = min(v for S, v in birth.items() if 0 in S)
    band = F(9, 8)*alpha
    ballots = [S for S in facets if 0 in S and birth[S] <= band]
    require(len(ballots) == comb(len(coords)-1, K-1), 'all ballots in band')
    snapshots = []
    for r, want in zip(radii, expected):
        active = [S for S in facets if birth[S] <= r]
        parent = {S: S for S in active}

        def find(S):
            while parent[S] != S:
                S = parent[S]
            return S

        executed = []
        for E in cofaces:
            if events[E] <= r:
                vs = list(combinations(E, K))
                require(all(S in parent for S in vs), 'unborn endpoint')
                for S in vs[1:]:
                    parent[find(S)] = find(vs[0])
                executed.append(E)
        groups = {}
        for S in active:
            groups.setdefault(find(S), []).append(S)
        require(len(groups) == 1, 'not one Gamma component')
        vertices = next(iter(groups.values()))
        covered = sorted({i for S in vertices for i in S})
        mass = sum(S in vertices for S in ballots)
        cardinal_shortcut = comb(len(covered)-1, K-1)
        require(covered == list(range(len(coords))) and (len(vertices), mass) == want, 'CDF/cover result')
        require(all(birth[S] <= band for S in ballots), 'fixed reference universe')
        snapshots.append(dict(radius=str(r), covered=covered, active_vertices=[list(S) for S in vertices],
            active_cofaces=[list(E) for E in executed], actual_point0_mass=mass,
            cardinal_shortcut=cardinal_shortcut, not_yet_admitted_ballots=[list(S) for S in ballots if birth[S] > r]))
    require(snapshots[0]['covered'] == snapshots[1]['covered'] and
            snapshots[0]['actual_point0_mass'] < snapshots[1]['actual_point0_mass'], 'silent admission witness')
    # There is one old component before/after. New facets have immediately
    # joined it at the second plateau: no merger of two old components.
    return dict(K=K, coordinates=[str(c) for c in coords], alpha_point0=str(alpha),
        eta='1/8', fixed_band_upper_radius=str(band), fixed_ballots=[list(S) for S in ballots],
        full_facets=len(facets), full_cofaces=len(cofaces), snapshots=snapshots,
        scope='Gamma exact cuts and ballot counts, not a native FULL export or changed hard hierarchy')


def main():
    cases = [
        proof(2, [F(0), F(19, 10), F(39, 20), F(2)], [F(39, 40), F(1)], [(5, 2), (6, 3)]),
        proof(5, [F(0)]+[F(i, 50) for i in range(95, 101)], [F(99, 100), F(1)], [(11, 5), (21, 15)]),
    ]
    print(json.dumps(dict(status='COMPONENT_COVER_IS_NOT_BALLOT_CDF_PROVED', cases=cases,
        native_calls=0, GCP_used=False, scope='two tiny Fraction Gamma cases; no large K-subset enumeration proposed'),
        sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
