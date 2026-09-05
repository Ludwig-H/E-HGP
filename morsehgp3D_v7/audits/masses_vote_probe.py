"""Small rational incidence/weight counterexamples; no product code is executed."""

from __future__ import annotations

import argparse
from collections import defaultdict
from datetime import datetime, timezone
from fractions import Fraction as Q
import hashlib
from itertools import combinations
import json
from pathlib import Path
import subprocess
import sys


HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
OUTPUT = HERE / "receipts_vertical_20260905/masses"
POINTS = {0: (2, 4, 4), 1: (2, 2, 2), 2: (4, 2, 4), 3: (4, 4, 2),
          4: (2, 0, 0), 5: (0, 2, 0), 6: (0, 0, 2)}
FIRST_LEVEL = Q(8, 3)
OMIT_A = (1, 2, 3)
OMIT_B = (1, 5, 6)
FACETS_A = set(combinations((0, 1, 2, 3), 2))
FACETS_B = set(combinations((1, 4, 5, 6), 2))


def require(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def dot(a: tuple, b: tuple) -> Q:
    return sum((Q(x) * Q(y) for x, y in zip(a, b)), Q(0))


def sub(a: tuple, b: tuple) -> tuple:
    return tuple(Q(x) - Q(y) for x, y in zip(a, b))


def distance2(a: tuple, b: tuple) -> Q:
    d = sub(a, b)
    return dot(d, d)


def meb_triangle(sigma: tuple) -> tuple[tuple, Q]:
    """All pair balls and the affine circumcircle; tiny exact reference only."""
    points = [POINTS[x] for x in sigma]
    candidates = []
    for p, q in combinations(points, 2):
        center = tuple(Q(x + y, 2) for x, y in zip(p, q))
        beta = distance2(p, q) / 4
        if all(distance2(center, z) <= beta for z in points):
            candidates.append((beta, center))
    origin, second, third = points
    u, v = sub(second, origin), sub(third, origin)
    aa, ab, bb = dot(u, u), dot(u, v), dot(v, v)
    determinant = aa * bb - ab * ab
    if determinant:
        a = bb * (aa - ab) / (2 * determinant)
        b = aa * (bb - ab) / (2 * determinant)
        center = tuple(Q(origin[j]) + a * u[j] + b * v[j] for j in range(3))
        candidates.append((distance2(center, origin), center))
    require(bool(candidates), "meb.no_candidate")
    beta, center = min(candidates)
    require(beta > 0 and all(distance2(center, z) <= beta for z in points), "meb.containment")
    return center, beta


def boundary(sigma: tuple) -> set:
    return set(combinations(sigma, len(sigma) - 1))


def reduced_trace(cofaces: dict) -> tuple[dict, dict]:
    """Set partitions by coface levels, followed by semantic token reduction."""
    batches = defaultdict(list)
    for sigma, level in cofaces.items():
        batches[level].append(boundary(sigma))
    state, seen, deltas, cuts = set(), set(), [], {}
    for level, edges in sorted(batches.items()):
        previous = state.copy()
        touched = set().union(*edges)
        born = touched - seen
        state.update(frozenset((facet,)) for facet in born)
        for edge in edges:
            joined = frozenset(edge)
            keep = set()
            for component in state:
                if component & edge:
                    joined |= component
                else:
                    keep.add(component)
            state = keep | {joined}
        group = []
        for component in state:
            if not component & touched:
                continue
            parents = sorted(min(old) for old in previous if old & component)
            new = sorted(component & born)
            if len(parents) == 1 and not new:
                continue
            group.append({"output": min(component), "parents": parents, "born": new})
        deltas.append({"level": str(level), "deltas": sorted(group, key=lambda item: item["output"])})
        cuts[str(level)] = {
            "strict": sorted(tuple(sorted(component)) for component in previous),
            "closed": sorted(tuple(sorted(component)) for component in state),
        }
        seen |= touched
    return {"catalog": sorted(seen), "batches": deltas}, cuts


def direct_scores(cofaces: dict, facets: set) -> dict:
    # Fixed explicit profile: p=2, hence psi(rho)=1/rho^2=1/beta.
    return {facet: sum((1 / beta for sigma, beta in cofaces.items() if set(facet) < set(sigma)), Q(0))
            for facet in facets}


def histogram_scores(cofaces: dict, collapse: bool = False) -> dict:
    histogram = defaultdict(lambda: defaultdict(int))
    for sigma, beta in cofaces.items():
        for facet in boundary(sigma):
            histogram[facet][beta] += 1
    scores = {}
    for facet, row in histogram.items():
        scores[facet] = sum((Q(1 if collapse else count) / beta for beta, count in row.items()), Q(0))
    return scores


def weights(scores: dict, universe: set) -> tuple[dict, dict, dict]:
    totals = {x: sum((score for facet, score in scores.items() if x in facet), Q(0)) for x in universe}
    incidence = {(x, facet): score / totals[x] for facet, score in scores.items()
                 for x in facet if totals[x] > 0}
    masses = {facet: sum((incidence.get((x, facet), Q(0)) for x in facet), Q(0)) for facet in scores}
    for x in universe:
        require(sum((w for (point, _), w in incidence.items() if point == x), Q(0)) == int(totals[x] > 0),
                "weights.partition_of_unity")
    require(sum(masses.values(), Q(0)) == sum(total > 0 for total in totals.values()), "weights.total_mass")
    return totals, incidence, masses


def label(facet: tuple) -> int:
    return 1 if facet in FACETS_A else 0 if facet in FACETS_B else -1


def vote(point: int, incidence: dict, totals: dict) -> tuple[int, dict]:
    votes = {c: sum((w for (x, facet), w in incidence.items() if x == point and label(facet) == c), Q(0))
             for c in (-1, 0, 1)}
    if totals.get(point, Q(0)) == 0:
        return -1, votes
    # Noise competes, exact ties use the smallest declared label.
    return min(votes, key=lambda c: (-votes[c], c)), votes


def formatted(mapping: dict) -> dict:
    return {str(key): str(value) for key, value in sorted(mapping.items())}


def run_cases() -> dict:
    geometry = {sigma: meb_triangle(sigma) for sigma in combinations(POINTS, 3)}
    complete = {sigma: result[1] for sigma, result in geometry.items()}
    window = {sigma: beta for sigma, beta in complete.items() if beta <= FIRST_LEVEL}
    require(len(complete) == 35 and len(window) == 8, "geometry.nonvacuity")
    for sigma, beta in window.items():
        center = geometry[sigma][0]
        centroid = tuple(sum(Q(POINTS[x][j]) for x in sigma) / 3 for j in range(3))
        require(beta == FIRST_LEVEL and center == centroid, "geometry.positive_equal_triangle")
        require(all(distance2(center, POINTS[x]) > beta for x in POINTS if x not in sigma),
                "geometry.regular_gabriel")
    families = {"complete_cech": complete,
                "omit_A": {sigma: beta for sigma, beta in complete.items() if sigma != OMIT_A},
                "omit_B": {sigma: beta for sigma, beta in complete.items() if sigma != OMIT_B}}
    traces = {name: reduced_trace(cofaces) for name, cofaces in families.items()}
    require(traces["complete_cech"] == traces["omit_A"] == traces["omit_B"], "topology.same_payload_and_cuts")
    catalog = set().union(*(boundary(sigma) for sigma in complete))
    require(len(catalog) == 21, "geometry.facets")
    scores, totals, incidences, masses, votes, winners = {}, {}, {}, {}, {}, {}
    for name, cofaces in families.items():
        scores[name] = direct_scores(cofaces, catalog)
        require(scores[name] == histogram_scores(cofaces), "histogram.sufficient_statistic")
        totals[name], incidences[name], masses[name] = weights(scores[name], set(POINTS) | {99})
        for point in POINTS:
            coface_total = 2 * sum((1 / beta for sigma, beta in cofaces.items() if point in sigma), Q(0))
            require(totals[name][point] == coface_total, "totals.complete_boundary_identity")
        winners[name], votes[name] = vote(1, incidences[name], totals[name])
        require(vote(99, incidences[name], totals[name])[0] == -1, "zero_total.noise")
    require(winners == {"complete_cech": 0, "omit_A": 0, "omit_B": 1}, "vote.same_payload_opposite_unique_winners")
    require(votes["omit_A"][0] > votes["omit_A"][1] and votes["omit_B"][1] > votes["omit_B"][0],
            "vote.nonzero_margins")
    require(votes["complete_cech"][0] == votes["complete_cech"][1] == Q(1, 2), "vote.exact_tie")
    # At the first nontrivial cut both components cover all seven points,
    # but their facets omit future global leaves; only a complete antichain conserves all mass.
    cut_facets = FACETS_A | FACETS_B
    cut_mass = sum((masses["complete_cech"][facet] for facet in cut_facets), Q(0))
    reserve = sum((masses["complete_cech"][facet] for facet in catalog - cut_facets), Q(0))
    require(0 < cut_mass < 7 and cut_mass + reserve == 7, "antichain.residual_required")
    _, local_incidence, local_masses = weights({f: scores["complete_cech"][f] for f in cut_facets}, set(POINTS))
    require(sum(local_masses.values(), Q(0)) == 7, "cut_normalization.unit_mass")
    require(local_incidence[(0, (0, 1))] != incidences["complete_cech"][(0, (0, 1))],
            "cut_normalization.changes_existing_atom")
    # These are explicitly audit-side corruptions, never product MHGP7 mutants.
    faults = {}
    def reject(name: str, reason: str, operation) -> None:
        try:
            operation()
        except ValueError as error:
            require(str(error) == reason, "fault.wrong_gate")
            faults[name] = reason
        else:
            raise ValueError("fault.survived:" + name)
    reject("audit_collapse_level_multiplicity", "histogram.multiplicity_lost",
           lambda: require(histogram_scores(complete, collapse=True) == scores["complete_cech"], "histogram.multiplicity_lost"))
    reject("audit_sum_raw_component_points", "mass.overlapping_points_counted_twice",
           lambda: require(4 + 4 == 7, "mass.overlapping_points_counted_twice"))
    reject("audit_claim_every_antichain_total_n", "mass.partial_antichain",
           lambda: require(cut_mass == 7, "mass.partial_antichain"))
    reject("audit_square_radius_as_radius_p3", "weight.exponent_doubled",
           lambda: require(Q(1, 4**3) == Q(1, 2**3), "weight.exponent_doubled"))
    require(len(faults) == 4, "fault.nonvacuity")
    # Algorithm 1 takes every facet of a Gabriel triangle, including its
    # non-Gabriel attachment edge: the operational F=boundary(C) is explicit.
    require(distance2((2, 0, 0), (1, 1, 0)) == 2 < 4, "obtuse.attachment_not_gabriel")
    return {
        "points_u16": POINTS, "order_K": 2, "numeric_profile": "p2_exact_rational",
        "cofaces_complete_cech": [{"sigma": sigma, "beta": str(beta)} for sigma, beta in complete.items()],
        "omitted_A": OMIT_A, "omitted_B": OMIT_B,
        "complete_cofaces": 35, "sparse_cofaces_each": 34, "facets_each": 21,
        "regular_Gabriel_cofaces_at_first_cut": 8, "first_cut_beta": str(FIRST_LEVEL),
        "same_normalized_tokens_and_all_cuts": True,
        "level_count": len(traces["complete_cech"][1]),
        "normalized_trace": traces["complete_cech"][0],
        "cuts": traces["complete_cech"][1],
        "scores": {name: formatted(row) for name, row in scores.items()},
        "point_1_totals": {name: str(row[1]) for name, row in totals.items()},
        "point_1_votes": {name: formatted(row) for name, row in votes.items()},
        "point_1_winners": winners,
        "coface_formula_for_point_totals_checks": 21,
        "obtuse_source_clarification": {
            "triangle": ((0, 0, 0), (4, 0, 0), (1, 1, 0)),
            "center": (2, 0, 0), "beta": 4,
            "third_point_power": -2,
            "triangle_Gabriel_on_its_three_points": True,
            "longest_edge_Gabriel": False,
            "algorithm_1_keeps_longest_edge_in_F": True,
        },
        "all_leaf_mass": "7", "auxiliary_zero_score_point_not_in_geometric_cloud": 99,
        "point_99_total": "0", "point_99_label": -1,
        "first_cut_mass_with_global_atoms": str(cut_mass), "future_leaf_reserve": str(reserve),
        "first_cut_sum_raw_point_cardinals": 8,
        "edge_01_atom_at_point_0_global": str(incidences["complete_cech"][(0, (0, 1))]),
        "edge_01_atom_at_point_0_cut_renormalized": str(local_incidence[(0, (0, 1))]),
        "audit_faults_rejected": faults,
        "scope": "pure_rational_math_fixture_no_product_execution_no_route_E_admission_claim",
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--receipt", choices=("normal", "optimized"), required=True)
    args = parser.parse_args()
    OUTPUT.mkdir(parents=True, exist_ok=True)
    paths = [Path(__file__).resolve(), ROOT / "docs/references/MANUSCRIT_THESE_HAUSEUX.pdf",
             ROOT / "morsehgp3D_v7/src/forest/render.hpp", ROOT / "morsehgp3D_v7/src/forest/fold.hpp",
             ROOT / "morsehgp3D_v7/src/pipeline/run.hpp"]
    def source_hashes() -> dict:
        return {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest() for path in paths}
    before = source_hashes()
    report = {
        "status": "running", "phase": "exploration_v7_hors_registre", "backend": "cpu_reference",
        "profile": "quantized_u16_input_only", "mode": "audit_independant_math_and_architecture",
        "public_status": "not_claimed", "gcp": "not_used", "python": sys.version,
        "optimization": sys.flags.optimize, "argv": sys.argv,
        "head": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
        "utc": datetime.now(timezone.utc).isoformat(), "sources_before": before,
    }
    try:
        report["result"] = run_cases()
        report["sources_after"] = source_hashes()
        require(before == report["sources_after"], "sources.changed")
        report["status"] = "completed"
    except Exception as error:
        report["status"] = "failed"
        report["error"] = str(error)
    (OUTPUT / f"{args.receipt}.json").write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n")
    print(f"masses vote status={report['status']} optimization={sys.flags.optimize}")
    if "error" in report:
        print(report["error"])
    return int(report["status"] != "completed")


if __name__ == "__main__":
    raise SystemExit(main())
