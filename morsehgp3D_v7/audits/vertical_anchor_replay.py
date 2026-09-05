"""Construct vertical maps from birth tokens and inherited anchors alone.

This bounded audit implementation reads sealed E outputs. Gamma is used only
by the separate judge, never by the token consumer or its anchor lookup.
"""

from __future__ import annotations

from copy import deepcopy
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import sys
from typing import Any

from horizontal_rational_oracle import (
    Component, Facet, Gamma, apply_batch, corpus, correspondence, facets, level,
    require,
)
from vertical_replay import project, unique_container

AUDIT = Path(__file__).resolve().parent
INPUT = AUDIT / "receipts_horizontal_20260905/pipeline"
OUTPUT = AUDIT / "receipts_resolver_20260905/anchors"


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def owner(face: Facet, roots: dict[Facet, Component]) -> Facet | None:
    matches = [key for key, component in roots.items() if face in component]
    require(len(matches) <= 1, "lookup.ambiguous")
    return matches[0] if matches else None


class TokenVertical:
    """Audit set reader; no coordinates, MEB, Gamma or coface events."""

    def __init__(self, ids: list[int], forests: list[dict[str, Any]],
                 mutant: str = "", lookup_cap: int = 1000000) -> None:
        self.ids = ids
        self.forests = {forest["K"]: forest for forest in forests}
        self.roots = {k: ({(p,): frozenset({(p,)}) for p in ids} if k == 1 else {})
                      for k in self.forests}
        self.anchors: dict[int, dict[Facet, Facet]] = {k: {} for k in self.forests if k > 1}
        self.cached_targets: dict[int, dict[Facet, Facet]] = {k: {} for k in self.anchors}
        self.mutant = mutant
        self.lookup_cap = lookup_cap
        self.stats = dict(births=0, birth_lookups=0, birth_misses=0,
                          max_birth_lookups=0, parent_lookups=0,
                          continuations=0, multifusions=0)
        self.first_miss: dict[str, Any] | None = None

    def birth_lookup(self, face: Facet, k: int) -> Facet | None:
        require(self.stats["birth_lookups"] < self.lookup_cap, "budget.birth_lookup")
        self.stats["birth_lookups"] += 1
        return owner(face, self.roots[k - 1])

    def advance(self, cut: Any) -> None:
        # Exact common level, lower order first: lower closed state is ready
        # before every upper birth/merge, including simultaneous levels.
        for k, forest in sorted(self.forests.items()):
            batch = [d for d in forest["deltas"] if level(d) == cut]
            if k > 1:
                consumed = set()
                pending = {}
                cached = {}
                for delta in batch:
                    parents = [tuple(p) for p in delta["parents"]]
                    output = tuple(delta["output"])
                    if parents:
                        require(all(p in self.anchors[k] for p in parents), "anchor.parent_missing")
                        candidates = [self.anchors[k][p] for p in parents]
                        targets = [owner(face, self.roots[k - 1]) for face in candidates]
                        self.stats["parent_lookups"] += len(targets)
                        require(None not in targets and len(set(targets)) == 1,
                                "anchor.parents_disagree")
                        anchor, target = min(candidates), targets[0]
                        self.stats["continuations"] += len(parents) == 1
                        self.stats["multifusions"] += len(parents) > 1
                    else:
                        born = [tuple(f) for f in delta["born"]]
                        candidates = born[:1] if self.mutant == "first-born-only" else born
                        anchor = target = None
                        tries = 0
                        for label in candidates:
                            candidate = label[:-1]  # One deterministic face per source label.
                            target = self.birth_lookup(candidate, k)
                            tries += 1
                            if target is not None:
                                anchor = candidate
                                break
                            self.stats["birth_misses"] += 1
                            if self.first_miss is None:
                                self.first_miss = dict(source_K=k, cut=str(cut),
                                                       source_output=output, source_label=label,
                                                       missing_lower_face=candidate)
                            if self.mutant == "miss-means-no-image":
                                break
                        if self.mutant == "miss-means-no-image" and anchor is None:
                            # Deliberately publish no map entry for this source;
                            # the independent totality check must reject it.
                            continue
                        require(anchor is not None and target is not None,
                                "anchor.birth_not_resolved")
                        self.stats["births"] += 1
                        self.stats["max_birth_lookups"] = max(self.stats["max_birth_lookups"], tries)
                    pending[output] = anchor
                    cached[output] = target
                    consumed.update(parents)
                self.anchors[k] = {key: face for key, face in self.anchors[k].items() if key not in consumed}
                self.anchors[k].update(pending)
                self.cached_targets[k] = {key: target for key, target in self.cached_targets[k].items()
                                          if key not in consumed}
                self.cached_targets[k].update(cached)
            self.roots[k] = apply_batch(self.roots[k], batch, k, self.ids)

    def images(self, k: int) -> dict[Facet, Facet]:
        if self.mutant != "miss-means-no-image":
            require(self.anchors[k].keys() == self.roots[k].keys(), "anchor.source_totality")
        result = {}
        for source, face in self.anchors[k].items():
            target = (self.cached_targets[k][source] if self.mutant == "stale-target"
                      else owner(face, self.roots[k - 1]))
            require(target in self.roots[k - 1], "anchor.target_not_current")
            result[source] = target
        return result


def relabel(case: dict[str, Any], row: dict[str, Any]) -> tuple[dict[str, Any], dict[str, Any]]:
    """Relabel a sealed certificate, not a fresh run on the relabeled cloud."""
    old_ids = case["ids"]
    new_ids = [3, 211, 65537, 4000000000, 4000000007, 4294967294, 19]
    require(len(old_ids) == len(new_ids) == 7, "relabel.domain")
    mapping = dict(zip(old_ids, new_ids))

    def mapped(face: Facet) -> Facet:
        return tuple(sorted(mapping[p] for p in face))

    out = deepcopy(row)
    out["digest"] = "audit_relabeling_no_engine_digest"
    for forest in out["forests"]:
        k = forest["K"]
        old = {(p,): frozenset({(p,)}) for p in old_ids} if k == 1 else {}
        deltas = forest["deltas"]
        transformed = []
        for cut in sorted(set(level(d) for d in deltas)):
            batch = [d for d in deltas if level(d) == cut]
            target_batch = []
            for delta in batch:
                item = deepcopy(delta)
                parents = [tuple(p) for p in delta["parents"]]
                parent_groups = [frozenset(mapped(f) for f in old[p]) for p in parents]
                born = sorted(mapped(tuple(f)) for f in delta["born"])
                group = frozenset(born).union(*parent_groups)
                item["parents"] = [list(f) for f in sorted(min(g) for g in parent_groups)]
                item["born"] = [list(f) for f in born]
                item["output"] = list(min(group))
                target_batch.append(item)
            transformed.extend(sorted(target_batch, key=lambda d: d["output"]))
            old = apply_batch(old, batch, k, old_ids)
        forest["deltas"] = transformed
    new_case = dict(case, ids=new_ids, cloud=4)
    return new_case, out


def synthetic_multifusion() -> tuple[dict[str, Any], dict[str, Any], Gamma]:
    """Full mathematical Gamma stream, separately attributed from E output."""
    case = dict(cloud=5, ids=[3, 19, 211, 65537, 4000000000, 4294967294],
                points=[(t, t * t, t**3) for t in (0, 1, 2, 10, 11, 12)])
    gamma = Gamma(case)
    forests = []
    for k in range(1, len(case["ids"])):
        old = gamma.partition(k, 0, True)
        deltas = []
        cuts = sorted({v for c, v in gamma.levels.items() if len(c) == k + 1})
        for batch, cut in enumerate(cuts):
            current = gamma.partition(k, cut, True)
            seen = frozenset().union(*old)
            pending = []
            for component in current:
                parents = [group for group in old if group <= component]
                born = component - seen
                if len(parents) == 1 and not born:
                    continue
                numerator = cut.numerator
                pending.append(dict(output=list(min(component)),
                                    parents=[list(f) for f in sorted(min(g) for g in parents)],
                                    born=[list(f) for f in sorted(born)], batch=batch,
                                    num=[(numerator >> (64 * i)) & ((1 << 64) - 1) for i in range(3)],
                                    den=cut.denominator))
            deltas.extend(sorted(pending, key=lambda d: d["output"]))
            old = current
        forests.append(dict(K=k, normalized=1, deltas=deltas))
    row = dict(status="complete_regular", kmax=len(case["ids"]) - 1,
               forests=forests, digest="synthetic_mathematical_stream_no_engine_digest")
    return case, row, gamma


def judge(case: dict[str, Any], row: dict[str, Any], gamma: Gamma,
          mutant: str = "", cap: int = 1000000) -> dict[str, Any]:
    require(row["status"] == "complete_regular", "input.not_terminal")
    require([f["K"] for f in row["forests"]] == list(range(1, len(case["ids"]))),
            "input.order_window")
    require(all(f["normalized"] == 1 for f in row["forests"]), "input.payload_type")
    reader = TokenVertical(case["ids"], row["forests"], mutant, cap)
    core = {k: {f for c in gamma.direct if len(c) == k + 1 for f in facets(c)}
            for k in reader.forests}
    stats = dict(cuts=0, component_maps=0, naturality_squares=0, two_step_squares=0)
    previous = None
    for cut in sorted(set(gamma.levels.values())):
        for closed in (False, True):
            if closed:
                reader.advance(cut)
            reference = {k: gamma.partition(k, cut, closed) for k in reader.forests}
            phi = {k: correspondence(reader.roots[k], reference[k], core[k]) for k in reader.forests}
            images = {k: reader.images(k) for k in reader.anchors}
            for k, table in images.items():
                require(table.keys() == reader.roots[k].keys(), "judge.vertical_totality")
                for source, target in table.items():
                    expected = unique_container(project(phi[k][source], k - 1), reference[k - 1])
                    require(phi[k - 1][target] == expected, "judge.vertical_target")
                    stats["component_maps"] += 1
            for k in range(3, row["kmax"] + 1):
                for source in reader.roots[k]:
                    target = images[k - 1][images[k][source]]
                    expected = unique_container(project(phi[k][source], k - 2), reference[k - 2])
                    require(phi[k - 2][target] == expected, "judge.two_step")
                    stats["two_step_squares"] += 1
            if previous is not None:
                old_roots, old_images = previous
                for k, table in old_images.items():
                    for source, target in table.items():
                        successor = unique_container(old_roots[k][source], list(reader.roots[k].values()))
                        source_now = next(key for key, group in reader.roots[k].items() if group == successor)
                        lower_now = unique_container(old_roots[k - 1][target], list(reader.roots[k - 1].values()))
                        require(reader.roots[k - 1][images[k][source_now]] == lower_now,
                                "judge.naturality")
                        stats["naturality_squares"] += 1
            previous = ({k: dict(r) for k, r in reader.roots.items()}, images)
            stats["cuts"] += 1
    return dict(stats={**stats, **reader.stats}, first_birth_miss=reader.first_miss)


def main() -> int:
    cases = corpus()
    oracles = {case["cloud"]: Gamma(case) for case in cases[::4]}
    pins = {name: sha(AUDIT / name) for name in (
        Path(__file__).name, "horizontal_rational_oracle.py", "vertical_replay.py",
        "meb_rational_oracle_20260905.py")}
    results = []
    for build in ("o2", "ubsan"):
        raw = INPUT / build / "nominal.stdout"
        pins[str(raw.relative_to(AUDIT))] = sha(raw)
        rows = [json.loads(line) for line in raw.read_text().splitlines()]
        require(len(rows) == len(cases), "input.case_count")
        derived_case, derived_row = relabel(cases[8], rows[8])
        oracles[4] = Gamma(derived_case)
        synthetic_case, synthetic_row, oracles[5] = synthetic_multifusion()
        all_cases = cases + [derived_case, synthetic_case]
        all_rows = rows + [derived_row, synthetic_row]
        outputs = [judge(case, row, oracles[case["cloud"]]) for case, row in zip(all_cases, all_rows)]
        totals = {key: (max(out["stats"][key] for out in outputs) if key == "max_birth_lookups"
                        else sum(out["stats"][key] for out in outputs)) for key in outputs[0]["stats"]}
        for key in ("birth_misses", "births", "continuations", "multifusions", "naturality_squares"):
            require(totals[key] > 0, "nonvacuity." + key)
        negatives = []
        for mutant, case, row, cap, expected in (
            ("first-born-only", derived_case, derived_row, 1000000, "anchor.birth_not_resolved"),
            ("miss-means-no-image", derived_case, derived_row, 1000000, "judge.vertical_totality"),
            ("stale-target", cases[0], rows[0], 1000000, "anchor.target_not_current"),
            ("", cases[0], rows[0], 0, "budget.birth_lookup"),
        ):
            try:
                judge(case, row, oracles[case["cloud"]], mutant, cap)
            except ValueError as error:
                require(str(error) == expected, "negative.wrong_rejection:" + str(error))
                negatives.append(dict(fault=mutant or "zero_lookup_budget", rejection=str(error),
                                      kind="audit_consumer_fault_or_budget_not_product_mutant"))
            else:
                raise ValueError("negative.survived:" + mutant)
        empty = judge(cases[12], rows[12], oracles[3], cap=0)
        require(empty["stats"]["birth_lookups"] == empty["stats"]["component_maps"] == 0,
                "budget.empty_vertical_family")
        original_totals = {key: (max(out["stats"][key] for out in outputs[:16]) if key == "max_birth_lookups"
                                 else sum(out["stats"][key] for out in outputs[:16]))
                           for key in outputs[0]["stats"]}
        require(outputs[-1]["stats"]["multifusions"] > 0, "synthetic.no_multifusion")
        results.append(dict(sealed_source_build=build, original_cases=16, relabeled_certificates=1,
                            synthetic_mathematical_streams=1, totals=totals,
                            original_only=original_totals, relabel_only=outputs[-2]["stats"],
                            synthetic_only=outputs[-1]["stats"],
                            first_birth_miss=outputs[-2]["first_birth_miss"],
                            negatives=negatives, empty_family_zero_budget_accepted=True))
    require(results[0]["totals"] == results[1]["totals"], "sealed_builds.disagree")
    OUTPUT.mkdir(parents=True, exist_ok=True)
    receipt = dict(schema="mhgp7-token-vertical-anchor-audit-v1", status="passed",
                   utc=datetime.now(timezone.utc).isoformat(), python_optimized=bool(sys.flags.optimize),
                   results=results, pins=pins, geometry_in_consumer=False,
                   product_executed=False, product_vertical_export_qualified=False,
                   source_authority="sealed E; one explicit audit relabeling, not a new product run",
                   synthetic_source="full Gamma on six moment-curve points, not an E run; fusion branch only",
                   synthetic_points=synthetic_case["points"],
                   relabeled_ids=derived_case["ids"], public_status="not_claimed", gcp_used=False)
    (OUTPUT / ("optimized.json" if sys.flags.optimize else "normal.json")).write_text(
        json.dumps(receipt, indent=2, ensure_ascii=False) + "\n")
    print(json.dumps(dict(status="passed", results=results)))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (ValueError, OSError) as error:
        print(str(error), file=sys.stderr)
        raise SystemExit(1)
