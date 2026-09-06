"""Exact input for a future CPU prefilter/admission seam test, not a FULL run.

For a valid q-support on n distinct sites, depth <= n-q. Consequently
smax=n gives depth < smax+1-q: the prefilter cannot reject any candidate.
The smallest n allowing a rejection in the accepted Kmax=5 profile is 7.
This model supplies five exact q2 balls; it does not claim the production
generator emits that list. No product import, allocation instrumentation,
subprocess, compilation or cloud access.
"""

from fractions import Fraction
import json


def require(value: bool, reason: str) -> None:
    if not value:
        raise ValueError(reason)


def main() -> None:
    points = (0, 1, 3, 7, 15, 31, 63)
    pairs = ((0, 1), (1, 3), (3, 7), (7, 15), (0, 63))
    smax, threshold = 6, 5
    records = []
    for a, b in pairs:
        # Primitive power on the x-axis: (x-a)(x-b); in ambient 3D add y²+z².
        powers = [(x - a) * (x - b) for x in points]
        interior = [x for x, power in zip(points, powers) if power < 0]
        shell = [x for x, power in zip(points, powers) if power == 0]
        radius_squared = Fraction((b - a) ** 2, 4)
        require(shell == [a, b] and a < b, "valid_minimal_q2_support")
        records.append(dict(support=[a, b], key=[1, -(a + b), 0, 0, a * b],
                            radius_squared=[radius_squared.numerator, radius_squared.denominator],
                            interior=interior, shell=shell,
                            prefilter_survives=len(interior) < threshold))
    require(threshold == smax + 1 - 2, "strict_depth_threshold")
    require([len(r['interior']) for r in records] == [0, 0, 0, 0, 5], "literal_depths")
    require(len({tuple(r['key']) for r in records}) == 5, "distinct_primitive_keys")
    require(len({tuple(r['radius_squared']) for r in records}) == 5, "distinct_levels")
    require(all(0 <= x <= 65535 for x in points), "u16_domain")
    for n in range(2, 7):
        effective = min(n, 6)
        for q in range(2, min(4, n) + 1):
            require(n - q < effective + 1 - q, "no_smaller_n_at_Kmax5")
    for q in (2, 3, 4):
        require(8 - q < 8 + 1 - q, "n8_Kmax10_prefilter_cannot_reject")
    unique = len(records)
    surviving = sum(r['prefilter_survives'] for r in records)
    candidate, survivor, ball, budget = 144, 16, 224, 1600
    sort = 2 * unique * candidate
    prefilter = unique * (candidate + 2 * survivor)
    census = unique * candidate + surviving * (survivor + ball)
    wrong = surviving * (candidate + survivor + ball)
    require((unique, surviving) == (5, 4), "nonvacuous_U_S_difference")
    require(sort <= budget and prefilter <= budget and wrong <= budget < census,
            "reachable_admission_counterexample")
    require(census == 1680 and census - 1 == 1679, "exact_census_boundary")
    print(json.dumps(dict(status="passed", public_status="not_claimed", points=points,
                          kmax=5, smax=smax, candidates=records, unique=unique, survivors=surviving,
                          abi_bytes=dict(candidate=candidate, survivor=survivor, ball=ball),
                          admission=dict(budget=budget, sort=sort, prefilter=prefilter,
                                         census=census, wrong_U_replaced_by_S=wrong),
                          authority="exact_geometry_and_admission_model_only",
                          generator_executed=False, census_executed=False,
                          allocation_instrumented=False, GCP_used=False),
                     sort_keys=True, separators=(",", ":")))


if __name__ == '__main__':
    main()
