"""Six-label simplicial counterexample; no geometry or product execution.

Horizontal component bijection and point coverage alone do not imply that a
lower retained component contains a face of any retained upper label.
"""

from __future__ import annotations

import hashlib
import itertools
import json
from pathlib import Path
import sys

Label = tuple[int, ...]
Component = frozenset[Label]


def require(condition: bool, reason: str) -> None:
    if not condition:
        raise ValueError(reason)


def faces(label: Label, size: int) -> frozenset[Label]:
    return frozenset(itertools.combinations(label, size))


def closure(maximal: list[Label]) -> frozenset[Label]:
    return frozenset(face for label in maximal
                     for size in range(1, len(label) + 1)
                     for face in itertools.combinations(label, size))


def gamma(k: int, complex_: frozenset[Label]) -> list[Component]:
    vertices = {label for label in complex_ if len(label) == k}
    adjacency = {label: set() for label in vertices}
    for first, second in itertools.combinations(sorted(vertices), 2):
        joined = tuple(sorted(set(first) | set(second)))
        if len(joined) == k + 1 and joined in complex_:
            adjacency[first].add(second)
            adjacency[second].add(first)
    result = []
    while vertices:
        todo = [min(vertices)]
        reached = set()
        while todo:
            label = todo.pop()
            if label in reached:
                continue
            reached.add(label)
            todo.extend(adjacency[label] - reached)
        vertices -= reached
        if k == 1 or len(reached) > 1:
            result.append(frozenset(reached))
    return result


def points(component: Component) -> frozenset[int]:
    return frozenset(point for label in component for point in label)


def encoded(component: Component) -> list[list[int]]:
    return [list(label) for label in sorted(component)]


def main() -> int:
    # A,B,C,D,X,Y. Every displayed maximal simplex includes all its faces.
    star = [(point, 4, 5) for point in range(4)]
    tetrahedron = (0, 1, 2, 3)
    bridge = (0, 1, 4)
    first = closure(star)
    second = closure(star + [tetrahedron, bridge])
    require(first <= second, "fixture.not_a_filtration")

    # Literal combinatorial expectations, independent of graph traversal.
    star_edges = frozenset({(4, 5)} | {(p, q) for p in range(4) for q in (4, 5)})
    tetra_edges = frozenset({(0, 1), (0, 2), (0, 3), (1, 2), (1, 3), (2, 3)})
    tetra_faces = frozenset({(0, 1, 2), (0, 1, 3), (0, 2, 3), (1, 2, 3)})
    full_lower = gamma(2, second)
    retained_lower = gamma(2, first)
    full_upper = gamma(3, second)
    require(gamma(2, first) == [star_edges], "fixture.initial_lower")
    require(gamma(3, first) == [], "fixture.initial_upper")
    require(full_lower == [star_edges | tetra_edges], "fixture.connected_lower")
    require(retained_lower == [star_edges], "fixture.retained_lower")
    require(full_upper == [tetra_faces], "fixture.upper_component")
    require(star_edges < full_lower[0], "horizontal.strict_facet_reduction")
    require(points(star_edges) == points(full_lower[0]), "horizontal.point_cover")
    require(len(retained_lower) == len(full_lower) == 1, "horizontal.bijection")
    require(points(tetra_faces) < points(star_edges), "vertical.strict_point_inclusion")

    queries = [face for label in tetra_faces for face in sorted(faces(label, 2))]
    require(len(queries) == 12 and len(set(queries)) == 6, "nonvacuity.faces")
    require(all(face in full_lower[0] for face in queries), "vertical.full_target")
    require(all(face not in star_edges for face in queries), "counterexample.sparse_absence")
    try:
        require(any(face in star_edges for face in queries), "shortcut.no_retained_anchor")
    except ValueError as error:
        rejection = str(error)
        require(rejection == "shortcut.no_retained_anchor", "negative.wrong_rejection")
    else:
        raise ValueError("negative.anchor_shortcut_survived")

    # Removing the bridge is a DIFFERENT valid complex. Now the two lower
    # components both cover the source points, but only the K4 is its target.
    no_bridge = closure(star + [tetrahedron])
    lower_without_bridge = gamma(2, no_bridge)
    require(set(lower_without_bridge) == {star_edges, tetra_edges}, "fixture.no_bridge_components")
    point_containers = [c for c in lower_without_bridge if points(tetra_faces) <= points(c)]
    facet_containers = [c for c in lower_without_bridge if tetra_edges <= c]
    require(len(point_containers) == 2 and facet_containers == [tetra_edges],
            "counterexample.point_containment")
    require(gamma(1, no_bridge) == [frozenset((p,) for p in range(6))],
            "vertical.noninjective_two_to_one")

    script = Path(__file__).resolve()
    receipt = dict(
        schema="mhgp7-vertical-combinatorial-math-counterexample-v1",
        status="passed", scope="combinatorial_only", labels=["A", "B", "C", "D", "X", "Y"],
        first_level_maximal=[list(label) for label in star],
        second_level_added=[list(tetrahedron), list(bridge)],
        retained_lower=encoded(star_edges), full_lower=encoded(full_lower[0]),
        source_component=encoded(tetra_faces), missing_distinct_faces=encoded(tetra_edges),
        source_components=1, full_lower_components=1, retained_lower_components=1,
        source_face_occurrences=12, missing_face_occurrences=12,
        nominal_full_face_lookup_successes=12,
        rejected_shortcut=rejection,
        no_bridge_point_containers=2, no_bridge_facet_containers=1,
        geometric_realizability_claimed=False, product_E_counterexample_claimed=False,
        product_executed=False, public_status="not_claimed", gcp_used=False,
        python_optimized=bool(sys.flags.optimize), script_sha256=hashlib.sha256(script.read_bytes()).hexdigest(),
    )
    destination = script.parent / ("combinatorial_optimized.json" if sys.flags.optimize else "combinatorial_normal.json")
    destination.write_text(json.dumps(receipt, indent=2, ensure_ascii=False) + "\n")
    print(json.dumps(dict(status="passed", source_face_occurrences=12,
                          missing_face_occurrences=12, rejected_shortcut=rejection)))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except ValueError as error:
        print(str(error), file=sys.stderr)
        raise SystemExit(1)
