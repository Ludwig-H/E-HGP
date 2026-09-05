"""Small Gamma oracle and delta-only reader, independent of product geometry.

Exhaustive subsets are bounded to seven audit points. No product architecture
or Delaunay catalogue is proposed. Replays do not compile or run the product.
"""

from __future__ import annotations

import argparse
from datetime import datetime, timezone
from fractions import Fraction as Q
import hashlib
import itertools
import json
import os
from pathlib import Path
import subprocess
import sys
from typing import Any

from meb_rational_oracle_20260905 import circumball, power

AUDIT = Path(__file__).resolve().parent
ROOT = AUDIT.parent.parent
WORK = AUDIT / ".work_horizontal_rational_20260905"
OUT = AUDIT / "receipts_horizontal_20260905/pipeline"
PINNED_E = "61f72a6805e27f1bc216b5d7444164b31fc970b6"
Facet = tuple[int, ...]
Component = frozenset[Facet]


def require(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def save(path: Path, value: Any) -> None:
    path.write_text(json.dumps(value, indent=2, ensure_ascii=False) + "\n")


def corpus() -> list[dict[str, Any]]:
    e5 = [(0, 0, 7), (0, 9, 6), (1, 4, 0), (0, 0, 1), (4, 1, 2)]
    clouds = [e5, [tuple(1000 + 6000 * x for x in p) for p in e5],
              [(t, t * t, t**3) for t in range(7)],
              [(0, 0, 0), (65535, 65535, 65535)]]
    ids = [3, 19, 211, 65537, 4000000000, 4000000007, 4294967294]
    cases = []
    for index, points in enumerate(clouds):
        for threads, layout, reverse, separation in ((1, 0, 0, 8), (1, 1, 0, 8),
                                                      (2, 0, 1, 12), (2, 1, 1, 12)):
            cases.append(dict(cloud=index, points=points, ids=ids[:len(points)],
                              threads=threads, layout=layout, reverse=reverse,
                              separation=separation, complete=1))
    return cases


def command(case: dict[str, Any]) -> str:
    return (f"{len(case['points'])} {case['threads']} {case['layout']} "
            f"{case['reverse']} {case['complete']} {case['separation']} "
            + " ".join(str(v) for ident, point in zip(case["ids"], case["points"])
                       for v in (ident, *point)))


def facets(coface: Facet) -> list[Facet]:
    return [coface[:i] + coface[i + 1:] for i in range(len(coface))]


def coverage(component: Component) -> frozenset[int]:
    return frozenset(p for f in component for p in f)


class Gamma:
    def __init__(self, case: dict[str, Any]) -> None:
        points, self.ids = case["points"], case["ids"]
        supports = []
        for size in range(2, min(4, len(points)) + 1):
            for support in itertools.combinations(range(len(points)), size):
                ball = circumball([points[i] for i in support])
                if ball is None or not ball["positive"]:
                    continue
                values = [power(ball, point) for point in points]
                supports.append((set(support), ball,
                                 {i for i, p in enumerate(values) if p <= 0},
                                 {i for i, p in enumerate(values) if p < 0},
                                 {i for i, p in enumerate(values) if p == 0}))
        self.levels: dict[Facet, Q] = {}
        self.direct: set[Facet] = set()
        self.regular = True
        for size in range(2, len(points) + 1):
            for subset in itertools.combinations(range(len(points)), size):
                indices = set(subset)
                found = next((row for row in supports
                              if row[0] <= indices <= row[2]), None)
                require(found is not None, "oracle.miniball_missing")
                support, ball, closed, interior, shell = found
                del closed
                key = tuple(sorted(self.ids[i] for i in subset))
                self.levels[key] = ball["radius"]
                if interior <= indices:
                    self.direct.add(key)
                self.regular &= shell == support
        require(self.regular, "fixture.global_regularity")

    def partition(self, k: int, cut: Q, closed: bool) -> list[Component]:
        components = [frozenset({(ident,)}) for ident in self.ids] if k == 1 else []
        for coface, level in self.levels.items():
            if len(coface) != k + 1 or not (level <= cut if closed else level < cut):
                continue
            group = frozenset(facets(coface))
            connected = [c for c in components if c & group]
            components = [c for c in components if not c & group]
            components.append(group.union(*connected))
        return components


def level(delta: dict[str, Any]) -> Q:
    require(len(delta["num"]) == 3 and delta["den"] > 0, "delta.level_encoding")
    require(all(isinstance(w, int) and 0 <= w < 1 << 64 for w in delta["num"]),
            "delta.level_words")
    value = Q(sum(word << (64 * i) for i, word in enumerate(delta["num"])), delta["den"])
    require(value > 0, "delta.level_positive")
    return value


def key(raw: Any, k: int, ids: list[int]) -> Facet:
    require(isinstance(raw, list) and len(raw) == k and raw == sorted(set(raw))
            and set(raw) <= set(ids), "delta.facet_domain")
    return tuple(raw)


def apply_batch(roots: dict[Facet, Component], deltas: list[dict[str, Any]],
                k: int, ids: list[int]) -> dict[Facet, Component]:
    consumed: set[Facet] = set()
    born_used: set[Facet] = set()
    seen = frozenset().union(*roots.values())
    pending = {}
    for delta in deltas:
        parents = [key(f, k, ids) for f in delta["parents"]]
        born = [key(f, k, ids) for f in delta["born"]]
        output = key(delta["output"], k, ids)
        require(parents == sorted(set(parents)) and born == sorted(set(born)), "delta.token_order")
        require(all(p in roots for p in parents), "delta.parent_not_live")
        require(not consumed.intersection(parents), "delta.parent_consumed_twice")
        require(not seen.intersection(born) and not born_used.intersection(born), "delta.born_not_first")
        group = frozenset(born).union(*(roots[p] for p in parents))
        require(bool(group) and output == min(group), "delta.output_not_canonical")
        require(len(parents) != 1 or bool(born), "delta.empty_continuation")
        require(output not in pending, "delta.duplicate_output")
        consumed.update(parents)
        born_used.update(born)
        pending[output] = group
    result = {p: c for p, c in roots.items() if p not in consumed}
    require(not result.keys() & pending.keys(), "delta.output_clobber")
    result.update(pending)
    return result


def correspondence(roots: dict[Facet, Component], reference: list[Component],
                   core: set[Facet]) -> dict[Facet, Component]:
    mapping = {}
    for output, component in roots.items():
        images = [group for group in reference if component <= group]
        require(len(images) == 1, "horizontal.inclusion_not_defined")
        image = images[0]
        require(image not in mapping.values(), "horizontal.inclusion_not_injective")
        require(coverage(component) == coverage(image), "horizontal.point_cover")
        mapping[output] = image
    require(set(mapping.values()) == set(reference), "horizontal.inclusion_not_surjective")
    retained_active = frozenset().union(*roots.values())
    gamma_active = frozenset().union(*reference)
    require(retained_active & core == gamma_active & core, "horizontal.core_activation")
    return mapping


def judge(case: dict[str, Any], run: dict[str, Any], oracle: Gamma) -> dict[str, int]:
    require(run["status"] == "complete_regular" and bool(run["digest"]), "run.not_terminal_complete")
    require(run["kmax"] == len(case["ids"]) - 1, "run.kmax")
    require([f["K"] for f in run["forests"]] == list(range(1, run["kmax"] + 1)), "run.callback_orders")
    stats = dict(orders=0, cuts=0, deltas=0, naturality_squares=0,
                 births=0, multifusions=0, growths=0, silent_added=sum(run["silent_added"]),
                 omitted_gamma_facets=0)
    for forest in run["forests"]:
        k = forest["K"]
        # This flag records the request. K1 retains normative root behavior.
        require(forest["normalized"] == 1, "forest.normalized_contract")
        cofaces = {f: t for f, t in oracle.levels.items() if len(f) == k + 1}
        cuts = sorted(set(cofaces.values()))
        core = {f for c in oracle.direct if len(c) == k + 1 for f in facets(c)}
        levels = [level(d) for d in forest["deltas"]]
        require(levels == sorted(levels) and set(levels) <= set(cuts), "delta.level_order")
        roots = {(p,): frozenset({(p,)}) for p in case["ids"]} if k == 1 else {}
        previous_roots: dict[Facet, Component] = {}
        previous_mapping: dict[Facet, Component] = {}
        for cut in cuts:
            before = dict(roots)
            pre_gamma = oracle.partition(k, cut, False)
            pre_map = correspondence(before, pre_gamma, core)
            batch = [d for d, t in zip(forest["deltas"], levels) if t == cut]
            roots = apply_batch(before, batch, k, case["ids"])
            post_gamma = oracle.partition(k, cut, True)
            post_map = correspondence(roots, post_gamma, core)
            for current_roots, mapping in ((before, pre_map), (roots, post_map)):
                for output, component in previous_roots.items():
                    targets = [p for p, c in current_roots.items() if component <= c]
                    require(len(targets) == 1 and previous_mapping[output] <= mapping[targets[0]],
                            "horizontal.naturality")
                    stats["naturality_squares"] += 1
                previous_roots, previous_mapping = current_roots, mapping
            published = set()
            for delta in batch:
                image = post_map[tuple(delta["output"])]
                expected_parents = {c for c in pre_gamma if c <= image}
                actual_parents = {pre_map[tuple(p)] for p in delta["parents"]}
                require(actual_parents == expected_parents and len(actual_parents) == len(delta["parents"]),
                        "horizontal.abstract_parents")
                require(image not in published, "horizontal.duplicate_transition")
                published.add(image)
            for image in post_gamma:
                parents = [c for c in pre_gamma if c <= image]
                old_points = frozenset().union(*(coverage(c) for c in parents))
                changed_points = old_points != coverage(image)
                if len(parents) != 1 or changed_points:
                    require(image in published, "horizontal.missing_transition")
                    stats["births"] += not parents
                    stats["multifusions"] += len(parents) >= 2
                    stats["growths"] += len(parents) == 1 and changed_points
            stats["cuts"] += 2
            stats["deltas"] += len(batch)
            stats["omitted_gamma_facets"] += (sum(len(c) for c in post_gamma)
                                               - sum(len(c) for c in roots.values()))
        stats["orders"] += 1
    return stats


def validate_outputs(rows: list[dict[str, Any]], cases: list[dict[str, Any]],
                     oracles: dict[int, Gamma]) -> dict[str, int]:
    require(len(rows) == len(cases), "transport.cardinality")
    totals: dict[str, int] = {}
    for case, row in zip(cases, rows):
        for name, value in judge(case, row, oracles[case["cloud"]]).items():
            totals[name] = totals.get(name, 0) + value
    for key_name in ("cuts", "deltas", "naturality_squares", "births", "multifusions",
                     "growths", "silent_added", "omitted_gamma_facets"):
        require(totals[key_name] > 0, "nonvacuity." + key_name)
    for start in range(0, len(rows), 4):
        baseline = rows[start]
        require(all(row["digest"] == baseline["digest"] for row in rows[start:start + 4]),
                "paired.layout_permutation_separation")
    return totals


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--replay", action="store_true")
    args = parser.parse_args()
    WORK.mkdir(exist_ok=True)
    OUT.mkdir(parents=True, exist_ok=True)
    cases = corpus()
    oracles = {c["cloud"]: Gamma(c) for c in cases[::4]}
    data = "\n".join(command(c) for c in cases) + "\n"
    sources = [Path(__file__), AUDIT / "horizontal_pipeline_bridge.cpp",
               AUDIT / "meb_rational_oracle_20260905.py"]
    source_pins = {str(p.relative_to(ROOT)): digest(p) for p in sources}
    frozen = WORK / "source"
    source_names = subprocess.check_output(
        ["git", "ls-tree", "-r", "--name-only", PINNED_E, "morsehgp3D_v7/src"],
        cwd=ROOT, text=True).splitlines()
    require(bool(source_names), "snapshot.empty")
    for name in source_names:
        data_bytes = subprocess.check_output(["git", "show", PINNED_E + ":" + name], cwd=ROOT)
        source_pins[name] = hashlib.sha256(data_bytes).hexdigest()
        if not args.replay:
            path = frozen / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data_bytes)
    frozen_bridge = frozen / "morsehgp3D_v7/audits/horizontal_pipeline_bridge.cpp"
    if not args.replay:
        frozen_bridge.parent.mkdir(parents=True, exist_ok=True)
        frozen_bridge.write_bytes((AUDIT / "horizontal_pipeline_bridge.cpp").read_bytes())
    results = []
    start = datetime.now(timezone.utc).isoformat()
    if not args.replay:
        (OUT / "input.txt").write_text(data)
    else:
        require((OUT / "input.txt").read_text() == data, "replay.input")
    for label, flags in (("o2", ["-O2"]), ("ubsan", ["-O1", "-g", "-fsanitize=undefined", "-fno-sanitize-recover=all"])):
        directory = OUT / label
        directory.mkdir(exist_ok=True)
        binary = WORK / label
        if not args.replay:
            argv = ["g++", "-std=c++20", *flags, "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                    "-DMHGP7_TESTING", "-pthread", str(frozen_bridge), "-o", str(binary)]
            compiled = subprocess.run(argv, text=True, capture_output=True,
                                      env=dict(os.environ, TMPDIR=str(WORK)), timeout=240, check=False)
            (directory / "compile.stdout").write_text(compiled.stdout)
            (directory / "compile.stderr").write_text(compiled.stderr)
            save(directory / "build.json", dict(command=argv, exit_code=compiled.returncode, source_pins=source_pins))
            require(compiled.returncode == 0 and not compiled.stderr, "compile." + label)
            for name, command_data, mutant in (("nominal", data, ""),
                                               ("silent-drop-coface", command(cases[0]) + "\n", "silent-drop-coface"),
                                               ("direct-route", command(dict(cases[0], complete=0)) + "\n", "")):
                run = subprocess.run([str(binary)] + ([mutant] if mutant else []), input=command_data,
                                     text=True, capture_output=True, timeout=90, check=False)
                (directory / (name + ".stdout")).write_text(run.stdout)
                (directory / (name + ".stderr")).write_text(run.stderr)
                save(directory / (name + ".run.json"), dict(exit_code=run.returncode, binary_sha256=digest(binary),
                                                            input=command_data, mutant=mutant))
                require(run.returncode == 0 and not run.stderr, "execute." + name)
        else:
            build = json.loads((directory / "build.json").read_text())
            require(build["exit_code"] == 0 and not (directory / "compile.stderr").read_text(), "replay.build")
            for name in (Path(__file__).name, "horizontal_pipeline_bridge.cpp", "meb_rational_oracle_20260905.py"):
                require(build["source_pins"]["morsehgp3D_v7/audits/" + name] == digest(AUDIT / name), "replay.judge_changed")
        nominal_record = json.loads((directory / "nominal.run.json").read_text())
        require(nominal_record["exit_code"] == 0 and not (directory / "nominal.stderr").read_text(), "nominal.exit")
        nominal = [json.loads(line) for line in (directory / "nominal.stdout").read_text().splitlines()]
        totals = validate_outputs(nominal, cases, oracles)
        failures = []
        for name in ("silent-drop-coface", "direct-route"):
            record = json.loads((directory / (name + ".run.json")).read_text())
            require(record["exit_code"] == 0 and not (directory / (name + ".stderr")).read_text(), "replay.exit")
            row = json.loads((directory / (name + ".stdout")).read_text())
            require(row["status"] == "complete_regular", "negative.must_succeed_before_judgment")
            if name == "direct-route":
                # Explicit reader fault, not product mutation: ignore the actual
                # compatible type and try to consume its tokens as normalized.
                require(all(f["normalized"] == 0 for f in row["forests"]), "negative.actual_compatible_type")
                for forest in row["forests"]:
                    forest["normalized"] = 1
            expected = ("horizontal.core_activation" if name == "silent-drop-coface"
                        else "delta.parent_not_live")
            try:
                judge(cases[0], row, oracles[0])
            except ValueError as error:
                require(str(error) == expected, "negative.wrong_rejection." + str(error))
                failures.append(dict(case=name, rejection=str(error), bridge_exit=0,
                                     kind="product_mutant" if name == "silent-drop-coface" else "audit_reader_type_fault"))
            else:
                raise ValueError("negative.survived." + name)
        results.append(dict(build=label, totals=totals, causal_rejections=failures))
    if not args.replay:
        for path in sources:
            require(source_pins[str(path.relative_to(ROOT))] == digest(path), "judge.changed_during_run")
        for name in source_names:
            require(source_pins[name] == digest(frozen / name), "frozen_source.changed_during_run")
    require(results[0]["totals"] == results[1]["totals"], "builds.differ")
    receipt = dict(schema="mhgp7-horizontal-rational-audit-v1", start_utc=start,
                   end_utc=datetime.now(timezone.utc).isoformat(), results=results,
                   cases=len(cases), clouds=len(oracles), python_optimized=bool(sys.flags.optimize),
                   replay_only=args.replay, source_pins=source_pins, source_authority=PINNED_E,
                   source_isolation="private immutable E snapshot, not concurrent live F",
                   public_status="not_claimed", gcp_used=False)
    name = ("replay_optimized.json" if sys.flags.optimize else "replay_normal.json") if args.replay else "summary.json"
    save(OUT / name, receipt)
    print(json.dumps(dict(status="passed", cases=len(cases), results=results)))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (ValueError, subprocess.TimeoutExpired) as error:
        print(str(error), file=sys.stderr)
        raise SystemExit(1)
