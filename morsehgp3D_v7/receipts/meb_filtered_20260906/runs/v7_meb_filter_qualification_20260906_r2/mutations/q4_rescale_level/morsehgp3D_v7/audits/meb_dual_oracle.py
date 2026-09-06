"""Independent rational judge of sealed private dual-budget MEB proposals.

--run builds isolated audit copies. Default replays preserved stdout only.
No product integration, no inherited qualification of a private proposal.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import difflib
from fractions import Fraction as Q
import gzip
import hashlib
import itertools
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import time

from meb_rational_oracle_20260905 import corpus, expected_meb, dot, subtract

HERE = Path(__file__).resolve().parent
LINEAGE = HERE.parent
RECEIPTS = HERE / "receipts_meb_dual_20260905"
INPUTS = RECEIPTS / "inputs"
GEOMETRY = RECEIPTS / "geometry"
WORK = HERE / ".work_meb_dual"


def require(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def write_json(path: Path, obj: dict) -> None:
    path.write_text(json.dumps(obj, indent=2, ensure_ascii=False) + "\n")


def make_cases() -> tuple[list, list, str]:
    # Same 89 point sets as the independently qualified rational MEB oracle.
    # Reverse local positions explicitly; do not borrow F to choose caps.
    scenes = [points for original in corpus() for points in (original, original[::-1])]
    oracles = [expected_meb(points) for points in scenes]
    rows, commands = [], []
    for i, points in enumerate(scenes):
        needed = oracles[i]["charged"]
        for legacy in sorted({0, needed - 1, needed, needed + 1}):
            for proposal in (0, 1, 4, 5, 401):
                rows.append((i, legacy, proposal))
                commands.append(f"M {len(points)} {legacy} {proposal} " +
                                " ".join(str(x) for p in points for x in p))
    return oracles, rows, "\n".join(commands) + "\n"


def judge(raw: str, oracles: list, rows: list) -> dict:
    observed = [json.loads(line) for line in raw.splitlines()]
    require(len(observed) == len(rows), "output.row_count")
    counts = dict(calls=len(rows), fast_q2=0, fast_q3=0, fast_q4=0,
                  budget_refusals=0, shell_refusals=0, fallbacks=0,
                  certified_but_legacy_refused=0, proposal_forms=0,
                  early_legacy_no_proposal=0, proposal_disabled=0)
    scenes = [points for original in corpus() for points in (original, original[::-1])]
    for index, (record, (scene, legacy, proposal)) in enumerate(zip(observed, rows)):
        r = record["proposed"]
        e = oracles[scene]
        needed = e["charged"]
        require(r["stats"] == [2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 38, min(legacy, needed)],
                f"oracle.stats.{index}")
        work = record["work"]
        forms, searches, violations = record["observer"]
        require(forms == work[0] <= min(proposal, 401) and violations == 0,
                f"proposal.prospective.{index}")
        require(work[1] <= 16 and work[2] <= 1 and work[3] <= 1, "proposal.local_bounds")
        require(r["events_size"] == 0, "events.unchanged")
        if legacy == 0:
            require(work == [0, 0, 0, 0] and searches == 0, "legacy.early")
            counts["early_legacy_no_proposal"] += 1
        if proposal == 0:
            require(forms == searches == work[2] == 0, "proposal.disabled")
            counts["proposal_disabled"] += 1
        counts["fallbacks"] += work[3]
        counts["proposal_forms"] += forms
        if legacy < needed:
            counts["budget_refusals"] += 1
            counts["certified_but_legacy_refused"] += work[2]
            require(not r["ok"] and r["status"] == 3 and
                    r["reason"] == "silent_meb_support_budget", f"oracle.budget.{index}")
            require(r["q"] == 9 and r["key"] == [7, 11, 13, 17, 19] and
                    r["num"] == [23, 29, 31] and r["den"] == 37 and
                    r["support_slots"] == [-1, -2, -3, -4], f"oracle.sentinel.{index}")
        else:
            degenerate = e["degenerate"]
            require(r["ok"] != degenerate and r["status"] == (2 if degenerate else 4) and
                    r["reason"] == ("silent_local_nonessential_shell" if degenerate else
                                    "audit_initial_status"), f"oracle.terminal.{index}")
            require(r["q"] == e["q"] and r["support_slots"] ==
                    e["support"] + [0] * (4 - e["q"]), f"oracle.support.{index}")
            require(r["key"] == e["key"], f"oracle.key.{index}")
            numerator = sum(v << (64 * j) for j, v in enumerate(r["num"]))
            require(Q(numerator, r["den"]) == e["radius"], f"oracle.radius.{index}")
            if e["q"] < 4:
                require(numerator == e["radius"].numerator and r["den"] == e["radius"].denominator,
                        f"oracle.reduced_level.{index}")
            else:
                base = scenes[scene][e["support"][0]]
                raw_center = [x * e["scale"] for x in subtract(e["center"], base)]
                require(numerator == dot(raw_center, raw_center) and r["den"] == e["scale"] ** 2,
                        f"oracle.q4_raw_level.{index}")
            if degenerate:
                require(work[2] == 0 and work[3] == 1, f"oracle.shell_fallback.{index}")
                counts["shell_refusals"] += 1
            elif work[2]:
                counts["fast_q" + str(e["q"])] += 1
        # Separate differential check; expected geometry above never calls F.
        require(record["same_as_F"] and record["reference"] == r, f"differential.F.{index}")
    for name in ("fast_q2", "fast_q3", "fast_q4", "shell_refusals", "fallbacks",
                 "certified_but_legacy_refused", "early_legacy_no_proposal", "proposal_disabled"):
        require(counts[name] > 0, "nonvacuity." + name)
    return counts


def ordinal_commands() -> tuple[list[int], str]:
    commands, expected = [], []
    for n in range(2, 12):
        rank = 0
        for q in range(2, min(4, n) + 1):
            for slots in itertools.combinations(range(n), q):
                rank += 1
                expected.append(rank)
                commands.append(f"O {n} {q} " + " ".join(map(str, slots)))
    require(len(expected) == 1507, "ordinal.nonvacuity")
    return expected, "\n".join(commands) + "\n"


def execute(command: list[str], label: str, deadline: float, stdin: str = "") -> dict:
    remaining = min(40.0, deadline - time.monotonic())
    require(remaining > 0, "campaign.deadline")
    before = time.monotonic()
    proc = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, start_new_session=True,
                            env=dict(os.environ, TMPDIR=str(WORK), PYTHONDONTWRITEBYTECODE="1",
                                     UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"))
    expired = False
    try:
        stdout, stderr = proc.communicate(stdin.encode(), timeout=remaining)
    except subprocess.TimeoutExpired:
        expired = True
        os.killpg(proc.pid, signal.SIGKILL)
        stdout, stderr = proc.communicate()
    for suffix, body in (("stdout", stdout), ("stderr", stderr)):
        (GEOMETRY / f"{label}.{suffix}.gz").write_bytes(gzip.compress(body, mtime=0))
    result = dict(command=command, exit_code=proc.returncode, timeout=expired,
                  wall_seconds=time.monotonic() - before, stdout_sha256=sha(stdout),
                  stderr_sha256=sha(stderr), stdin_sha256=sha(stdin.encode()))
    write_json(GEOMETRY / f"{label}.json", result)
    require(not expired and proc.returncode == 0, "command.failed." + label)
    require(not stderr, "command.stderr." + label)
    return result


def run_campaign(commands: str, ordinal_input: str) -> None:
    require(not (GEOMETRY / "run.json").exists(), "run.already_exists_use_replay")
    WORK.mkdir(exist_ok=True)
    deadline = time.monotonic() + 120
    source_pins = json.loads((INPUTS / "source_F.json").read_text())["pins"]
    froot = WORK / "F"
    for name, pin in source_pins.items():
        body = (LINEAGE / name).read_bytes()
        require(sha(body) == pin, "source.F_changed." + name)
        path = froot / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(body)
    input_pins = {str(p.relative_to(HERE)): sha(p.read_bytes()) for p in
                  [Path(__file__), HERE / "meb_dual_bridge.cpp", HERE / "meb_rational_oracle_20260905.py",
                   *sorted(INPUTS.iterdir())] if p.is_file()}
    report = dict(status="running", source_pins=source_pins, input_pins=input_pins,
                  utc=datetime.now(timezone.utc).isoformat(), deadline_seconds=120,
                  source_head=subprocess.check_output(["git", "rev-parse", "HEAD"], text=True).strip(),
                  worktree=subprocess.check_output(["git", "status", "--porcelain"], text=True),
                  commands={}, binaries={})
    write_json(GEOMETRY / "run.json", report)
    original_dual = (INPUTS / "dual_pivot.hpp").read_text()
    original_legacy = (INPUTS / "legacy_pivot.hpp").read_text()
    mutations = {
        "skip_shell": ("dual", "if (shell != candidate.q) return false;", "if (false && shell != candidate.q) return false;"),
        "ordinal_plus_one": ("dual", "const u64 count = ordinal(n, candidate);", "const u64 count = ordinal(n, candidate) + 1;"),
        "q4_rescale_level": ("legacy", "ball.level = q4_level_raw(candidate.four);",
            "ball.level = q4_level_raw(candidate.four);\n"
            "    for (size_t j = 2; j > 0; --j) ball.level.num[j] = (ball.level.num[j] << 1) | (ball.level.num[j - 1] >> 63);\n"
            "    ball.level.num[0] <<= 1; ball.level.den *= 2;")}
    try:
        report["commands"]["compiler"] = execute(["g++", "--version"], "compiler", deadline)
        for variant in ("o2", "ubsan", *mutations):
            base = WORK / variant
            dual, legacy = original_dual, original_legacy
            if variant in mutations:
                target, old, new = mutations[variant]
                source = dual if target == "dual" else legacy
                require(source.count(old) == 1, "mutation.unique_site." + variant)
                changed = source.replace(old, new)
                (GEOMETRY / f"{variant}.patch").write_text("".join(difflib.unified_diff(
                    source.splitlines(True), changed.splitlines(True), fromfile=target, tofile=target)))
                if target == "dual": dual = changed
                else: legacy = changed
            for folder, source in (("v7_meb_dual_budget_prototype", dual), ("v7_meb_pivot_prototype", legacy)):
                path = base / folder / "pivot.hpp"
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(source)
            binary = base / "probe"
            compile_command = ["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                               "-O1" if variant == "ubsan" else "-O2"]
            if variant == "ubsan": compile_command += ["-fsanitize=undefined", "-fno-sanitize-recover=all"]
            compile_command += ["-I", str(base), "-I", str(froot), str(HERE / "meb_dual_bridge.cpp"),
                                "-MMD", "-MF", str(GEOMETRY / f"{variant}.d"), "-o", str(binary)]
            report["commands"][variant + "_compile"] = execute(compile_command, variant + "_compile", deadline)
            pin = sha(binary.read_bytes())
            report["commands"][variant] = execute([str(binary)], variant, deadline, commands)
            if variant in ("o2", "ubsan"):
                label = variant + "_ordinals"
                report["commands"][label] = execute([str(binary)], label, deadline, ordinal_input)
            require(sha(binary.read_bytes()) == pin, "binary.changed")
            report["binaries"][variant] = pin
        for name, pin in source_pins.items():
            require(sha((LINEAGE / name).read_bytes()) == pin, "F.changed_during_campaign." + name)
        for name, pin in input_pins.items():
            require(sha((HERE / name).read_bytes()) == pin, "input.changed." + name)
        report["status"] = "completed"
    except Exception as error:
        report["status"] = "failed"
        report["error"] = str(error)
        raise
    finally:
        write_json(GEOMETRY / "run.json", report)


def main() -> int:
    global GEOMETRY
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--run", action="store_true")
    parser.add_argument("--destination", type=Path, help="receipt directory relative to audits; stays inside audits")
    args = parser.parse_args()
    if args.destination is not None:
        GEOMETRY = (HERE / args.destination).resolve()
        require(GEOMETRY.is_relative_to(HERE), "destination.outside_audits")
    GEOMETRY.mkdir(parents=True, exist_ok=True)
    oracles, rows, commands = make_cases()
    ordinals, ordinal_input = ordinal_commands()
    if args.run: run_campaign(commands, ordinal_input)
    receipt = dict(status="running", optimization=sys.flags.optimize,
                   public_status="not_claimed", gcp="not_used", source_sets=89,
                   local_orders=178, geometry_judge="independent Fraction Gram elimination",
                   private_prototype_only=True, product_integration=False, counts={}, mutants={})
    try:
        run = json.loads((GEOMETRY / "run.json").read_text())
        require(run["status"] == "completed", "campaign.not_completed")
        for label in ("o2", "ubsan", "skip_shell", "ordinal_plus_one", "q4_rescale_level"):
            require(run["commands"][label]["stdin_sha256"] == sha(commands.encode()), "replay.geometry_input." + label)
        for label in ("o2_ordinals", "ubsan_ordinals"):
            require(run["commands"][label]["stdin_sha256"] == sha(ordinal_input.encode()), "replay.ordinal_input." + label)
        for name, pin in run["input_pins"].items():
            # The initial driver expected an ordinal counter mismatch first;
            # the cap-at-R fixture correctly rejects the earlier terminal instead.
            # Preserve the original driver as execution provenance; only replay
            # the same compiled bytes with the corrected judge classification.
            path = HERE / name
            if name == "meb_dual_oracle.py" and sha(path.read_bytes()) != pin:
                path = GEOMETRY / "initial_driver.py.txt"
            require(sha(path.read_bytes()) == pin, "replay.input_changed." + name)
        for label, metadata in run["commands"].items():
            for suffix in ("stdout", "stderr"):
                data = gzip.decompress((GEOMETRY / f"{label}.{suffix}.gz").read_bytes())
                require(sha(data) == metadata[suffix + "_sha256"], "replay.bytes." + label)
        for variant in ("o2", "ubsan"):
            raw = gzip.decompress((GEOMETRY / f"{variant}.stdout.gz").read_bytes()).decode()
            receipt["counts"][variant] = judge(raw, oracles, rows)
            raw_ord = gzip.decompress((GEOMETRY / f"{variant}_ordinals.stdout.gz").read_bytes()).decode()
            require([json.loads(row)["ordinal"] for row in raw_ord.splitlines()] == ordinals, "ordinal.enumeration")
        for mutant, wanted in (("skip_shell", "oracle.terminal."),
                               ("ordinal_plus_one", "oracle.terminal."),
                               ("q4_rescale_level", "oracle.q4_raw_level.")):
            raw = gzip.decompress((GEOMETRY / f"{mutant}.stdout.gz").read_bytes()).decode()
            try: judge(raw, oracles, rows)
            except ValueError as error:
                require(str(error).startswith(wanted), "mutant.wrong_rejection." + str(error))
                receipt["mutants"][mutant] = str(error)
            else: raise ValueError("mutant.survived." + mutant)
        receipt["ordinal_checks_per_build"] = len(ordinals)
        receipt["replay_driver_sha256"] = sha(Path(__file__).read_bytes())
        driver_changed = run["input_pins"]["meb_dual_oracle.py"] != receipt["replay_driver_sha256"]
        receipt["initial_driver_preserved"] = "initial_driver.py.txt" if driver_changed else None
        receipt["execution_not_repeated_after_judge_classification_fix"] = driver_changed
        receipt["status"] = "passed"
    except Exception as error:
        receipt["status"] = "failed"
        receipt["error"] = str(error)
        raise
    finally:
        write_json(GEOMETRY / ("optimized.json" if sys.flags.optimize else "normal.json"), receipt)
    print(json.dumps(receipt, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
