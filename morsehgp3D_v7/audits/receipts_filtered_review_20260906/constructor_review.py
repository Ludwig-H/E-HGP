"""Read-only independent review of the constructor's filtered MEB R2 captures.

No constructor controller is imported. The earlier audit's frozen rational
geometry primitive supplies expected balls; parsing, charges, ordinal inventory,
capture closure and checks below are this review's own code. No C++ is launched.
"""
from __future__ import annotations

from collections import Counter
from fractions import Fraction
import hashlib
import importlib.util
import itertools
import json
from pathlib import Path
import posixpath
import shlex
import sys

HERE = Path(__file__).resolve().parent
AUDITS = HERE.parent
PACKET = AUDITS.parent / "receipts/meb_filtered_20260906"
RUN = PACKET / "runs/v7_meb_filter_qualification_20260906_r2"
CHECKS = PACKET / "checks/checks_r2"
MAX = (1 << 64) - 1
EXPECTED_SEAL = "c8268e851e19c9a7b0e2433fec7cf0f715cbccda75d9c63c8e8645e57b492e2f"
EXPECTED_RUN = "981f3b3e67f3f8e731aceca964c9faaae32b16b372f58704b205585374f83e87"
EXPECTED_SOURCES = "7e881f998a2f6bcac8e709a3ee3fa0c176973a7670e2a22960a1d46641627172"
EXPECTED_ORACLE = "ad6c0d6c041ff788180a400f6ba2ad2b1546f8607e8f2c91fefca9133a8e7f2b"


def require(ok: bool, message: str) -> None:
    if not ok:
        raise ValueError(message)


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read_json(path: Path):
    return json.loads(path.read_text())


def closure() -> dict:
    require(sha(PACKET / "SHA256SUMS") == EXPECTED_SEAL, "publication.seal")
    entries = {}
    for line in (PACKET / "SHA256SUMS").read_text().splitlines():
        digest, name = line.split("  ", 1)
        require(name not in entries and ".." not in Path(name).parts,
                "publication.safe_unique_path")
        entries[name] = digest
        require(sha(PACKET / name) == digest, "publication.hash:" + name)
    actual = {str(p.relative_to(PACKET)) for p in PACKET.rglob("*") if p.is_file()}
    require(actual == set(entries) | {"SHA256SUMS"}, "publication.closed_inventory")
    publication = read_json(PACKET / "publication.json")
    require(set(publication["copied_files"]) == actual - {"publication.json", "SHA256SUMS"},
            "publication.copy_inventory")
    for name, pin in publication["copied_files"].items():
        require(pin == {"bytes": (PACKET / name).stat().st_size, "sha256": sha(PACKET / name)},
                "publication.copy_pin:" + name)
    require(sha(RUN / "run.json") == EXPECTED_RUN, "run.pin")
    require(sha(RUN / "source_pins.json") == EXPECTED_SOURCES, "sources.pin")
    sources = read_json(RUN / "source_pins.json")
    for name, digest in sources.items():
        require(sha(RUN / "snapshot" / name) == digest, "snapshot.pin:" + name)
    mutation_changes = {}
    expected_mutations = {
        "skip_shell": ("build/v7_meb_filtered_preparation_20260905/pivot.hpp",
                       "if (shell != candidate.q) return false;",
                       "if (false && shell != candidate.q) return false;"),
        "ordinal_plus_one": ("build/v7_meb_filtered_preparation_20260905/pivot.hpp",
                             "const u64 count = ordinal(n, candidate);",
                             "const u64 count = ordinal(n, candidate) + 1;"),
        "q4_rescale_level": ("build/v7_meb_pivot_prototype/pivot.hpp",
                             "    ball.level = q4_level_raw(candidate.four);",
                             "    ball.level = q4_level_raw(candidate.four);\n"
                             "    for (size_t j = 2; j > 0; --j) ball.level.num[j] = (ball.level.num[j] << 1) | (ball.level.num[j - 1] >> 63);\n"
                             "    ball.level.num[0] <<= 1; ball.level.den *= 2;")}
    for name, (changed, before, after) in expected_mutations.items():
        changes = []
        for path in (RUN / "mutations" / name).rglob("*"):
            if not path.is_file():
                continue
            relative = str(path.relative_to(RUN / "mutations" / name))
            original = RUN / "snapshot" / relative
            if path.read_bytes() != original.read_bytes():
                changes.append(relative)
                require(relative == changed and original.read_text().count(before) == 1 and
                        original.read_text().replace(before, after) == path.read_text(),
                        "mutation.single_causal_change:" + name)
        require(changes == [changed], "mutation.changed_inventory:" + name)
        mutation_changes[name] = changed
    run = read_json(RUN / "run.json")
    require(run["status"] == "completed" and run["source_map_sha256"] == EXPECTED_SOURCES,
            "run.closed")
    for name, terminal in run["commands"].items():
        prefix = RUN / "commands" / name
        require(read_json(prefix.with_suffix(".json")) == terminal, "command.terminal:" + name)
        intent = read_json(prefix.with_suffix(".intent.json"))
        for key in ("argv", "cwd", "started_utc", "expected_exit_code", "stdin_sha256"):
            require(intent[key] == terminal[key], "command.intent:" + name + ":" + key)
        require(intent["timeout_seconds"] == 120 and terminal["elapsed_seconds"] < 120 and
                terminal["exit_code"] == terminal["expected_exit_code"] and
                terminal["interrupted"] is None, "command.closed:" + name)
        for stream in ("stdin", "stdout", "stderr"):
            require(sha(prefix.with_suffix("." + stream)) == terminal[stream + "_sha256"],
                    "command.stream:" + name + ":" + stream)
    builds = [k for k in run["commands"] if k.endswith("_compile")]
    dependency_count = 0
    for name in builds:
        terminal = run["commands"][name]
        argv = terminal["argv"]
        for flag in ("-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-MMD"):
            require(flag in argv, "build.flag:" + name + ":" + flag)
        require(not any("MHGP7_TESTING" in arg for arg in argv), "build.no_testing:" + name)
        if name.startswith("san_"):
            require("-fsanitize=address,undefined" in argv and "-fno-sanitize-recover=all" in argv,
                    "build.sanitizers:" + name)
        else:
            require("-O2" in argv, "build.O2:" + name)
        require(terminal["exit_code"] == 0, "build.success:" + name)
        dep_name = name.removesuffix("_compile")
        pins = read_json(RUN / "dependencies" / (dep_name + ".json"))
        source_root = RUN / "snapshot"
        for mutant in ("skip_shell", "ordinal_plus_one", "q4_rescale_level"):
            if name.startswith(mutant):
                source_root = RUN / "mutations" / mutant
        for path, digest in pins.items():
            require(sha(source_root / path) == digest, "dependency.pin:" + name + ":" + path)
        raw = (RUN / "dependencies" / (dep_name + ".d")).read_text().replace("\\\n", " ")
        dependencies = shlex.split(raw.split(":", 1)[1])
        historic_root = str(source_root).replace(str(RUN), "/workspaces/E-HGP/build/" + RUN.name)
        resolved = {str(Path(posixpath.normpath(p)).relative_to(historic_root)) for p in dependencies}
        require(resolved == set(pins), "dependency.complete:" + name)
        dependency_count += len(pins)
        binary = dep_name
        require(binary in run["binaries"], "binary.recorded:" + name)
        require(run["binaries"][binary] == publication["runs"][0]["source_inventory"]["bin/" + binary]["sha256"],
                "binary.publication_binding:" + name)
    replay = read_json(CHECKS / "receipt.json")
    require(replay["status"] == "completed" and replay["engine_runs"] == replay["compiler_runs"] == 0,
            "constructor_replays.closed")
    require(replay["run_sha256"] == EXPECTED_RUN and replay["source_sha256"] == EXPECTED_SOURCES,
            "constructor_replays.binding")
    require((CHECKS / "normal.stdout").read_bytes() == (CHECKS / "optimized.stdout").read_bytes(),
            "constructor_replays.equal")
    for name, pin in replay["artifacts"].items():
        require(sha(CHECKS / name) == pin, "constructor_replays.artifact:" + name)
    return {"files": len(actual), "manifest_entries": len(entries), "copied_files": len(publication["copied_files"]),
            "sources": len(sources), "commands": len(run["commands"]), "builds": len(builds),
            "binary_identities_only": len(run["binaries"]), "dependency_entries": dependency_count,
            "mutation_single_file_changes": mutation_changes,
            "ended_utc": run["ended_utc"], "hermetic_build_claim": False}


def load_geometry():
    path = AUDITS / "meb_rational_oracle_20260905.py"
    require(sha(path) == EXPECTED_ORACLE, "independent_geometry.pin")
    spec = importlib.util.spec_from_file_location("independent_rational_geometry", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def rational_review(geometry, name: str, expected_cache: dict, mutant: bool = False) -> dict:
    inputs = (RUN / "commands" / (name + ".stdin")).read_text().splitlines()
    outputs = (RUN / "commands" / (name + ".stdout")).read_text().splitlines()
    require(len(inputs) == len(outputs) == 3430, "rational.row_count:" + name)
    counts = Counter()
    first = None
    for i, (text, raw) in enumerate(zip(inputs, outputs)):
        fields = text.split()
        require(fields[0] == "M", "rational.mode")
        n, legacy, proposal = map(int, fields[1:4])
        coords = list(map(int, fields[4:]))
        require(len(coords) == 3 * n, "rational.coordinates")
        points = tuple(tuple(coords[j:j + 3]) for j in range(0, len(coords), 3))
        if points not in expected_cache:
            expected_cache[points] = geometry.expected_meb(points)
        expected = expected_cache[points]
        rank = expected["charged"]
        record = json.loads(raw)
        out = record["proposed"]
        problems = []
        if out != record["reference"] or not record["same_as_F"]:
            problems.append("literal_F_difference")
        if out["stats"] != [2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 38, min(legacy, rank)]:
            problems.append("legacy_charge_or_other_stat")
        forms, pivots, certified, fallback = record["work"]
        observed_forms, searches, violations = record["observer"]
        if not (forms == observed_forms <= min(proposal, 146) and pivots <= 16 and
                certified <= 1 and fallback <= 1 and violations == 0):
            problems.append("prospective_or_bounded_work")
        if legacy == 0 and (record["work"] != [0, 0, 0, 0] or searches != 0):
            problems.append("early_legacy_refusal")
        if proposal == 0 and (forms or searches or certified):
            problems.append("proposal_disabled")
        if out["events_size"] != 0:
            problems.append("events_changed")
        if legacy < rank:
            counts["budget_refusals"] += 1
            if out["ok"] or out["status"] != 3 or out["reason"] != "silent_meb_support_budget":
                problems.append("budget_terminal")
            sentinel = {"q": 9, "key": [7, 11, 13, 17, 19], "num": [23, 29, 31],
                        "den": 37, "support_slots": [-1, -2, -3, -4]}
            if any(out[k] != value for k, value in sentinel.items()):
                problems.append("budget_sentinel")
            counts["certified_then_legacy_refused"] += certified
        else:
            degenerate = expected["degenerate"]
            reason = "silent_local_nonessential_shell" if degenerate else "audit_initial_status"
            if out["ok"] == degenerate or out["status"] != (2 if degenerate else 4) or out["reason"] != reason:
                problems.append("geometric_terminal")
            if out["q"] != expected["q"] or out["support_slots"] != expected["support"] + [0] * (4 - expected["q"]):
                problems.append("support")
            if out["key"] != expected["key"]:
                problems.append("key")
            numerator = sum(v << (64 * j) for j, v in enumerate(out["num"]))
            if Fraction(numerator, out["den"]) != expected["radius"]:
                problems.append("radius")
            if expected["q"] == 4:
                base = points[expected["support"][0]]
                delta = [expected["scale"] * (a - b) for a, b in zip(expected["center"], base)]
                if numerator != sum(a * a for a in delta) or out["den"] != expected["scale"] ** 2:
                    problems.append("q4_raw_level")
            elif numerator != expected["radius"].numerator or out["den"] != expected["radius"].denominator:
                problems.append("reduced_level")
            if degenerate:
                counts["shell_refusals"] += 1
                if certified or fallback != 1:
                    problems.append("shell_fallback")
            elif certified:
                counts["fast_q" + str(expected["q"])] += 1
        counts["proposal_forms"] += forms
        if problems:
            counts["differing_rows"] += 1
            for problem in set(problems):
                counts[problem] += 1
            if first is None:
                first = {"row_zero_based": i, "input": text, "causes": problems}
            require(mutant, "rational:" + name + ":" + str(first))
    require(mutant == bool(first), "rational.expected_verdict:" + name)
    return {"calls": len(inputs), **dict(counts), "first_difference": first}


def ordinal_review() -> dict:
    wanted = []
    for n in range(2, 12):
        rank = 0
        for q in range(2, min(n, 4) + 1):
            for subset in itertools.combinations(range(n), q):
                rank += 1
                wanted.append((f"O {n} {q} " + " ".join(map(str, subset)), rank))
    for build in ("o2", "san"):
        inputs = (RUN / "commands" / (build + "_ordinals.stdin")).read_text().splitlines()
        outputs = (RUN / "commands" / (build + "_ordinals.stdout")).read_text().splitlines()
        require(inputs == [x[0] for x in wanted] and
                [json.loads(x) for x in outputs] == [{"ordinal": x[1]} for x in wanted],
                "ordinals.complete:" + build)
    return {"per_build": len(wanted), "last_rank": wanted[-1][1], "domain": "2<=n<=11, 2<=q<=min(n,4)"}


def budget_review(build: str) -> dict:
    lines = (RUN / "commands" / (build + "_budget_gate.stdout")).read_text().splitlines()
    rows = [dict(item.split("=") for item in line.split()) for line in lines[:-1]]
    summary = json.loads(lines[-1])
    initial = {
        "MAX_last_charge_fallback": (MAX - 4, MAX - 1), "MAX_exact_fast": (MAX - 4, MAX - 2),
        "MAX_fast_legacy_refusal": (MAX - 1, MAX - 2), "MAX_legacy_already_full": (MAX, MAX - 1),
        "MAX_P_already_full": (MAX - 4, MAX), "MAX_submax_P": (MAX - 4, MAX - 2),
        "MAX_legacy_above_L": (MAX, 0), "MAX_P_slack": (MAX - 4, MAX - 3)}
    cumulative_c = cumulative_p = 0
    totals = Counter()
    for i, row in enumerate(rows):
        name = row["case"]
        data = {key: int(value) for key, value in row.items() if key != "case"}
        c, p = initial.get(name, (0, 0))
        if name == "cumulative_P3_L12":
            c, p = cumulative_c, cumulative_p
        square = name == "square_final_shell_fallback"
        rank = 2 if square else (1 if data["n"] == 2 else 4)
        needed_forms = 1 if square or data["n"] == 2 or name == "pivot_cap_zero" else 2
        room = max(0, data["L"] - c)
        forms = min(max(0, data["P"] - p), needed_forms) if room else 0
        fallback = bool(room) and (forms < needed_forms or square or name == "pivot_cap_zero")
        success = room >= rank and not square
        expected = {"forms_delta": forms, "legacy_delta": min(room, rank),
                    "fallback_delta": int(fallback), "success": int(success),
                    "sentinel": int(room < rank), "terminal_equal": 1,
                    "NoObserver_equal": 1, "prospective_violations": 0}
        require(all(data[key] == value for key, value in expected.items()), "budget.row:" + str(i))
        if name == "cumulative_P3_L12":
            cumulative_c += min(room, rank)
            cumulative_p += forms
        totals["proposal_forms"] += forms
        totals["successes"] += success
        totals["budget_refusals"] += room < rank
        totals["shell_refusals"] += square and room >= rank
    require(len(rows) == 59 and all(summary[k] == v for k, v in totals.items()), "budget.summary")
    require(summary["accounting"] == "reference_ordinal_plus_native_z_q3_q4_proposal_v2", "budget.accounting")
    return {"rows": len(rows), **dict(totals), "near_MAX_rows": len(initial), "cumulative_calls": 4}


def gates_review() -> dict:
    result = {}
    for build in ("o2", "san"):
        geometry = read_json(RUN / "commands" / (build + "_geometry_gate.stdout"))
        trajectory = read_json(RUN / "commands" / (build + "_trajectory_gate.stdout"))
        require(geometry["status"] == trajectory["status"] == "passed", "gates.nominal")
        require(geometry["main_comparisons"] + geometry["boundary_comparisons"] == 9344,
                "geometry.floor")
        require(geometry["complete"] + geometry["degenerate"] + geometry["capped"] == 9344,
                "geometry.status_partition")
        require(geometry["q4_high_limb"] == 522 and geometry["direct_form_checks"] == 6,
                "geometry.q4_nonvacuum")
        for key, value in {"local_permutations": 62, "prefix_calls": 654, "native_calls": 180,
                           "admissible_order_local_calls": 8, "admissible_order_native_calls": 6,
                           "admissible_order_global_replays": 1, "admissible_order_budget_differences": 3,
                           "admissible_order_same_support": 3}.items():
            require(trajectory[key] == value, "trajectory.floor:" + key)
        require(not trajectory["native_ambiguity_claim"] and not trajectory["engine_integration"] and
                not trajectory["independent_geometry_oracle"], "trajectory.scope")
        for gate, expected in (("budget", 50), ("geometry", 22661)):
            name = build + "_" + gate + "_gate_charge_after"
            raw = (RUN / "commands" / (name + ".stdout")).read_text().splitlines()
            data = json.loads(raw[-1])
            require(data["status"] == "causal_violation" and data["cause"] == "charge_not_prospective" and
                    data["prospective_violations"] == expected,
                    "mutant.prospective:" + name)
        for mutant, cause in (("order", "order_mutant.first_support_changed"),
                              ("admissible_order", "order_budget.calendar_changed")):
            raw = (RUN / "commands" / (build + "_trajectory_" + mutant + "_mutant.stderr")).read_text()
            require(raw.strip() == "trajectory rejected: " + cause, "mutant.trajectory:" + mutant)
        result[build] = {"geometry_captured_comparisons": 9344, "trajectory_captured": trajectory,
                         "budget_recomputed": budget_review(build)}
    for suffix in ("budget_gate", "geometry_gate", "trajectory_gate", "rational", "ordinals"):
        require((RUN / "commands" / ("o2_" + suffix + ".stdout")).read_bytes() ==
                (RUN / "commands" / ("san_" + suffix + ".stdout")).read_bytes(), "builds.same:" + suffix)
    return result


def main() -> None:
    require(len(sys.argv) == 1, "arguments.none")
    geometry = load_geometry()
    binding = closure()
    cache = {}
    rational = {build: rational_review(geometry, build + "_rational", cache) for build in ("o2", "san")}
    mutants = {name: rational_review(geometry, name, cache, True)
               for name in ("skip_shell", "ordinal_plus_one", "q4_rescale_level")}
    require(mutants["q4_rescale_level"].get("q4_raw_level", 0) > 0 and
            mutants["q4_rescale_level"].get("radius", 0) == 0, "mutant.q4_same_radius")
    output = {"schema": "mhgp7-independent-filtered-capture-review-v1", "status": "passed",
              "public_status": "not_claimed", "gcp": "not_used", "engines_invoked": 0,
              "constructor_judges_imported": 0, "capture_binding": binding,
              "seal_sha256": EXPECTED_SEAL, "run_sha256": EXPECTED_RUN,
              "source_map_sha256": EXPECTED_SOURCES, "audit_geometry_sha256": EXPECTED_ORACLE,
              "independent_geometry_ordered_clouds": len(cache), "rational": rational,
              "ordinal_inventory": ordinal_review(), "constructor_gates_captured": gates_review(),
              "mutants_rejudged": mutants,
              "limits": ["new Python review of preserved C++ outputs, no new C++ execution",
                         "gate summaries bound to captured code and exit, not per-case geometry telemetry",
                         "source-level sanitizer environment setting; no independent environment capture",
                         "no FULL integration, persistent Builder, observer exception or latency qualification"]}
    print(json.dumps(output, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
