"""Bounded vertical audit from sealed E deltas; never runs the product.

Gamma is an exhaustive audit judge on at most seven points. The conjugated
maps below are audit objects, not a v7 vertical export or a product resolver.
"""

from __future__ import annotations

from datetime import datetime, timezone
import hashlib
import itertools
import json
from pathlib import Path
import sys
from typing import Any

from horizontal_rational_oracle import (
    Component, Facet, Gamma, apply_batch, corpus, correspondence, coverage,
    facets, level, require,
)

AUDIT = Path(__file__).resolve().parent
INPUT = AUDIT / "receipts_horizontal_20260905/pipeline"
OUTPUT = AUDIT / "receipts_vertical_20260905/vertical"


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def unique_container(labels: Component, partition: list[Component]) -> Component:
    matches = [group for group in partition if labels <= group]
    require(len(matches) == 1, "vertical.gamma_target_not_unique")
    return matches[0]


def project(component: Component, size: int) -> Component:
    return frozenset(face for label in component
                     for face in itertools.combinations(label, size))


def lookup(face: Facet, roots: dict[Facet, Component]) -> Facet:
    matches = [output for output, group in roots.items() if face in group]
    require(len(matches) == 1, "vertical.lower_label_unmaterialized")
    return matches[0]


def run_case(case: dict[str, Any], row: dict[str, Any], gamma: Gamma) -> dict[str, Any]:
    require(row["status"] == "complete_regular", "input.not_terminal_complete")
    require(row["kmax"] == len(case["ids"]) - 1, "input.order_window")
    require([f["K"] for f in row["forests"]] == list(range(1, row["kmax"] + 1)),
            "input.order_sequence")
    forests = {f["K"]: f for f in row["forests"]}
    require(all(f["normalized"] == 1 for f in forests.values()), "input.payload_type")
    levels = {k: sorted(set(level(d) for d in f["deltas"])) for k, f in forests.items()}
    roots = {k: ({(p,): frozenset({(p,)}) for p in case["ids"]} if k == 1 else {})
             for k in forests}
    core = {k: {f for c in gamma.direct if len(c) == k + 1 for f in facets(c)}
            for k in forests}
    stats = dict(cuts=0, adjacent_order_cuts=0, component_maps=0,
                 label_face_queries=0, unmaterialized_face_occurrences=0,
                 labels_with_no_retained_face=0, naturality_squares=0,
                 two_step_squares=0, strict_point_inclusions=0,
                 lower_target_changes=0)
    missing_example = None
    stale_example = None
    previous = None
    for cut in sorted(set(gamma.levels.values())):
        for closed in (False, True):
            if closed:
                for k, forest in forests.items():
                    if cut in levels[k]:
                        roots[k] = apply_batch(roots[k],
                                              [d for d in forest["deltas"] if level(d) == cut],
                                              k, case["ids"])
            reference = {k: gamma.partition(k, cut, closed) for k in forests}
            phi = {k: correspondence(roots[k], reference[k], core[k]) for k in forests}
            maps = {}
            for k in range(1, row["kmax"]):
                stats["adjacent_order_cuts"] += 1
                maps[k] = {}
                for source, group in roots[k + 1].items():
                    # All faces of ALL full-Gamma labels define the reference.
                    image = unique_container(project(phi[k + 1][source], k), reference[k])
                    target = next(output for output, component in phi[k].items()
                                  if component == image)
                    maps[k][source] = target
                    require(coverage(group) <= coverage(roots[k][target]),
                            "vertical.points_not_contained")
                    stats["strict_point_inclusions"] += coverage(group) < coverage(roots[k][target])
                    for label in group:
                        retained = 0
                        for face in facets(label):
                            stats["label_face_queries"] += 1
                            require(face in image, "vertical.representative_disagreement")
                            try:
                                found = lookup(face, roots[k])
                            except ValueError as error:
                                require(str(error) == "vertical.lower_label_unmaterialized",
                                        "negative.wrong_rejection")
                                stats["unmaterialized_face_occurrences"] += 1
                                if missing_example is None:
                                    missing_example = dict(
                                        cloud=case["cloud"], lower_order=k,
                                        cut=str(cut), side="closed" if closed else "open",
                                        source_output=source, source_label=label,
                                        missing_face=face, correct_target=target,
                                        rejection=str(error),
                                        kind="audit_lookup_shortcut_not_product_mutant")
                            else:
                                require(found == target, "vertical.retained_anchor_disagreement")
                                retained += 1
                        stats["labels_with_no_retained_face"] += retained == 0
                    stats["component_maps"] += 1
            for k in range(1, row["kmax"] - 1):
                for source in roots[k + 2]:
                    via = maps[k][maps[k + 1][source]]
                    direct = unique_container(project(phi[k + 2][source], k), reference[k])
                    require(phi[k][via] == direct, "vertical.two_step_composition")
                    stats["two_step_squares"] += 1
            if previous is not None:
                old_roots, old_phi, old_maps, old_cut, old_closed = previous
                for k in maps:
                    for source, old_target in old_maps[k].items():
                        high = unique_container(old_roots[k + 1][source], list(roots[k + 1].values()))
                        successor = next(output for output, group in roots[k + 1].items() if group == high)
                        low = unique_container(old_roots[k][old_target], list(roots[k].values()))
                        require(roots[k][maps[k][successor]] == low, "vertical.naturality")
                        stats["naturality_squares"] += 1
                        current = phi[k][maps[k][successor]]
                        if old_phi[k][old_target] != current:
                            stats["lower_target_changes"] += 1
                            if stale_example is None:
                                stale_example = dict(
                                    lower_order=k, source_output=source,
                                    previous_cut=str(old_cut), previous_closed=old_closed,
                                    cut=str(cut), closed=closed,
                                    old_target_facets=[list(f) for f in sorted(old_phi[k][old_target])],
                                    current_target_facets=[list(f) for f in sorted(current)],
                                    kind="stale_component_requires_horizontal_successor")
            previous = ({k: dict(r) for k, r in roots.items()}, phi, maps, cut, closed)
            stats["cuts"] += 1
    return dict(stats=stats, missing_face_example=missing_example,
                stale_target_example=stale_example)


def main() -> int:
    start = datetime.now(timezone.utc).isoformat()
    cases = corpus()
    oracles = {case["cloud"]: Gamma(case) for case in cases[::4]}
    pins = {str(Path(__file__).relative_to(AUDIT)): sha(Path(__file__))}
    results = []
    for build in ("o2", "ubsan"):
        build_path = INPUT / build / "build.json"
        record = json.loads(build_path.read_text())
        require(record["exit_code"] == 0, "input.failed_build")
        for name in ("horizontal_rational_oracle.py", "meb_rational_oracle_20260905.py"):
            require(record["source_pins"]["morsehgp3D_v7/audits/" + name] == sha(AUDIT / name),
                    "input.oracle_source_changed")
            pins[name] = sha(AUDIT / name)
        raw_path = INPUT / build / "nominal.stdout"
        run_path = INPUT / build / "nominal.run.json"
        require(json.loads(run_path.read_text())["exit_code"] == 0, "input.failed_run")
        for path in (build_path, raw_path, run_path):
            pins[str(path.relative_to(AUDIT))] = sha(path)
        rows = [json.loads(line) for line in raw_path.read_text().splitlines()]
        require(len(rows) == len(cases), "input.case_count")
        outputs = [run_case(case, row, oracles[case["cloud"]]) for case, row in zip(cases, rows)]
        totals = {name: sum(out["stats"][name] for out in outputs) for name in outputs[0]["stats"]}
        for name in ("component_maps", "naturality_squares", "two_step_squares",
                     "unmaterialized_face_occurrences", "strict_point_inclusions", "lower_target_changes"):
            require(totals[name] > 0, "nonvacuity." + name)
        results.append(dict(build_of_sealed_input=build, cases=len(cases), totals=totals,
                            missing_face_example=next(out["missing_face_example"] for out in outputs
                                                      if out["missing_face_example"]),
                            stale_target_example=next(out["stale_target_example"] for out in outputs
                                                      if out["stale_target_example"])))
    require(results[0]["totals"] == results[1]["totals"], "sealed_builds.disagree")
    OUTPUT.mkdir(parents=True, exist_ok=True)
    receipt = dict(schema="mhgp7-vertical-replay-audit-v1", status="passed",
                   started_utc=start, ended_utc=datetime.now(timezone.utc).isoformat(),
                   python_optimized=bool(sys.flags.optimize), results=results,
                   pins=pins, source_authority="E sealed horizontal pipeline receipts",
                   product_executed=False, vertical_product_qualification=False,
                   resolver_totality_proved_by_test=False, public_status="not_claimed", gcp_used=False)
    name = "optimized.json" if sys.flags.optimize else "normal.json"
    (OUTPUT / name).write_text(json.dumps(receipt, indent=2, ensure_ascii=False) + "\n")
    print(json.dumps(dict(status="passed", results=results)))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (ValueError, OSError) as error:
        print(str(error), file=sys.stderr)
        raise SystemExit(1)
