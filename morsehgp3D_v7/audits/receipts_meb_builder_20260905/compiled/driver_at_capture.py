"""Bounded qualification of the private Builder; default replays sealed bytes."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import difflib
from functools import lru_cache
import gzip
import hashlib
import itertools
import json
import os
from pathlib import Path
import shlex
import signal
import subprocess
import sys
import time

from meb_rational_oracle_20260905 import corpus, expected_meb, power, dot, subtract

HERE = Path(__file__).resolve().parent
RECEIPTS = HERE / "receipts_meb_builder_20260905"
RUN = RECEIPTS / "compiled"
WORK = HERE / ".work_meb_builder"
MAX = (1 << 64) - 1
TRIANGLE = [(0, 0, 0), (2, 2, 0), (2, 0, 2)]
CHAIN = [(0, 4, 0), (8, 4, 0), (4, 10, 0), (3, 1, 0), (5, 1, 0)]
PRIMES = [2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31]


def require(value, cause):
    if not value:
        raise ValueError(cause)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def save(path, value):
    path.write_text(json.dumps(value, indent=2, ensure_ascii=False) + "\n")


def literal_ball(points):
    return cached_ball(tuple(tuple(p) for p in points))


@lru_cache(maxsize=None)
def cached_ball(points):
    e = expected_meb(points)
    numerator, denominator = e["radius"].numerator, e["radius"].denominator
    if e["q"] == 4:
        relative = [v * e["scale"] for v in subtract(e["center"], points[e["support"][0]])]
        numerator, denominator = int(dot(relative, relative)), int(e["scale"] ** 2)
    return e, dict(q=e["q"], key=e["key"], num=[(numerator >> (64 * i)) & MAX for i in range(3)],
                   den=denominator, support_slots=e["support"] + [0] * (4 - e["q"]))


def local(points, caps, *, name="rational", c=0, p=0, a=0, fault=0, at=0, extra=None):
    e, ball = literal_ball(points)
    return dict(mode="M", name=name, points=points, caps=caps, c=c, p=p, a=a,
                fault=fault, at=at, rank=e["charged"], degenerate=e["degenerate"], ball=ball,
                expected_extra=extra)


def catalogue(points, order):
    result = []
    for ids in itertools.combinations(range(len(points)), order + 1):
        selected = [points[i] for i in ids]
        e, b = literal_ball(selected)
        if e["degenerate"] or any(power(e, p) == 0 for i, p in enumerate(points) if i not in ids):
            continue
        if any(power(e, p) < 0 for i, p in enumerate(points) if i not in ids):
            continue
        support = [ids[i] for i in e["support"]]
        interior = [i for i in ids if i not in support]
        result.append([e["q"], len(interior), (1 << e["q"]) - 1, b["num"], b["den"], support, interior])
    return result


def cases():
    result = []
    for original in corpus():
        for points in (original, original[::-1]):
            e, _ = literal_ball(points)
            for limit in sorted({0, e["charged"] - 1, e["charged"], e["charged"] + 1}):
                for proposal in (0, 1, 4, 5, 401):
                    result.append(local(points, [(limit, proposal)]))
    require(len(result) == 3430, "cases.rational_floor")
    result += [
        local(TRIANGLE, [(12, 7)] * 4, name="persistent_P7",
              extra=[[5, 0, 1, 1, 0], [7, 4, 2, 1, 1], [7, 8, 2, 1, 2], [7, 8, 2, 1, 2]]),
        local(TRIANGLE, [(20, 5), (20, 0), (20, 1)], name="lowered_P",
              extra=[[5, 0, 1, 1, 0], [5, 4, 1, 1, 1], [5, 8, 1, 1, 2]]),
        local(TRIANGLE, [(MAX, 0)] * 2, c=MAX - 4, a=MAX - 4, name="MAX_reference",
              extra=[[0, MAX, 0, 0, 1], [0, MAX, 0, 0, 1]]),
        local(TRIANGLE, [(MAX, 5)] * 2, c=MAX - 4, a=MAX - 4, name="MAX_certificate",
              extra=[[5, MAX - 4, 1, 1, 0], [5, MAX - 4, 1, 1, 0]]),
        local(TRIANGLE, [(551, MAX)], p=MAX - 1, name="MAX_proposal",
              extra=[[MAX, 4, 1, 0, 1]]),
        local(TRIANGLE, [(3, 0), (3, 401)], c=7, a=5, name="legacy_above_cap",
              extra=[[0, 5, 0, 0, 0], [0, 5, 0, 0, 0]]),
    ]
    for points, name in ((TRIANGLE, "triangle"), (CHAIN, "chain")):
        direct = catalogue(points, 2)
        require(len(direct) == (1 if name == "triangle" else 5), "catalogue.independent_floor")
        for limit in (0, 2, 3, 8, 551):
            for proposal in (0, 1, 2, 3, 7, 401):
                result.append(dict(mode="R", name=name, points=points, direct=direct,
                                   limit=limit, proposal=proposal, fault=0, at=0))
    return result


def fault_cases():
    result = []
    for kind in (1, 2, 3, 4):
        proposal = 401 if kind < 3 else 1
        result.append(local(TRIANGLE, [(551, proposal)], name="local_exception", fault=kind, at=1))
        result.append(dict(mode="R", name="chain_exception", points=CHAIN, direct=catalogue(CHAIN, 2),
                           limit=551, proposal=401 if kind < 3 else 2, fault=kind,
                           at=3 if kind < 3 else 1))
    return result


def commands(corpus_rows):
    lines = []
    for c in corpus_rows:
        coords = [x for p in c["points"] for x in p]
        if c["mode"] == "M":
            values = ["M", len(c["points"]), len(c["caps"]), c["c"], c["p"], c["a"], 0, 0, 0,
                      c["fault"], c["at"], *coords, *(x for pair in c["caps"] for x in pair)]
        else:
            values = ["R", len(c["points"]), len(c["direct"]), c["limit"], c["proposal"], c["fault"], c["at"], *coords]
            for q, d, mask, num, den, support, interior in c["direct"]:
                values.extend([q, d, mask, *num, den, *support, *interior])
        lines.append(" ".join(map(str, values)))
    return "\n".join(lines) + "\n"


def read_output(label):
    return gzip.decompress((RUN / (label + ".stdout.gz")).read_bytes()).decode()


def judge(raw, corpus_rows, instrumented=False):
    rows = [json.loads(line) for line in raw.splitlines()]
    require(len(rows) == sum(len(c["caps"]) if c["mode"] == "M" else 1 for c in corpus_rows), "rows.nonvacuity")
    counts = dict(local_steps=0, public_runs=0, certificates_q2=0, certificates_q3=0,
                  certificates_q4=0, legacy_refusals=0, shell_refusals=0, public_added_events=0)
    cursor = 0
    for case in corpus_rows:
        c, p, a, cert, fall = case.get("c", 0), case.get("p", 0), case.get("a", 0), 0, 0
        for step in range(len(case["caps"]) if case["mode"] == "M" else 1):
            r = rows[cursor]; cursor += 1
            require(r["mode"] == case["mode"] and r["exception"] == "none", "terminal.mode_or_exception")
            require(r["same_as_F"] and r["reference"] == r["actual"], "differential.full_F")
            actual, extra = r["actual"], r["extra"]
            require(len(extra) == 5 and all(type(v) is int and 0 <= v <= MAX for v in extra), "extra.shape")
            if instrumented:
                require(r["hooks"][3] == 0 and extra[0] - case.get("p", 0) == r["hooks"][0]
                        and extra[1] - case.get("a", 0) == r["hooks"][2], "instrumented.causal_or_physical")
            else:
                require(r["hooks"] == [0] * 5, "native.NoObserver")
            if case["mode"] == "R":
                counts["public_runs"] += 1
                require(extra[0] <= case["proposal"] and extra[1] <= actual["stats"][12], "public.budgets")
                if actual["status"] == 0:
                    if case["name"] == "triangle":
                        require(actual["stats"][11:13] == [3, 3] and len(actual["events"]) == 0, "public.triangle")
                    else:
                        require(actual["stats"][:8] == [11, 6, 1, 1, 1, 1, 0, 1]
                                and actual["stats"][11:13] == [8, 8], "public.chain_counts")
                        event = [2, 1, 3, [16, 0, 0], 1, [0, 1] + [0] * 9, [3] + [0] * 8]
                        require(actual["events"] == [event], "public.chain_event")
                        counts["public_added_events"] += 1
                    calls = 3 if case["name"] == "triangle" else 8
                    paid = min(calls, case["proposal"])
                    require(extra == [paid, calls - paid, 0, paid, calls - paid], "public.persistent_Work")
                else:
                    require(actual["status"] == 3 and actual["reason"] == "silent_meb_support_budget"
                            and actual["events"] == [], "public.closed_refusal")
                continue
            counts["local_steps"] += 1
            limit, proposal = case["caps"][step]
            delta = min(case["rank"], max(0, limit - c))
            require(actual["stats"] == PRIMES + [38 + step, c + delta], "local.legacy_stats")
            require(len(actual["events"]) == 2 and actual["events"][0] == actual["events"][1]
                    and actual["events"][0][5] == list(range(101, 112))
                    and actual["events"][0][6] == list(range(211, 220)), "local.sentinels")
            if c >= limit or delta < case["rank"]:
                counts["legacy_refusals"] += 1
                require(not r["ok"] and actual["status"] == 3 and actual["reason"] == "silent_meb_support_budget", "local.refusal")
                expected = dict(q=9, key=[7, 11, 13, 17, 19], num=[23, 29, 31], den=37, support_slots=[-1, -2, -3, -4])
            else:
                degenerate = case["degenerate"]
                counts["shell_refusals"] += int(degenerate)
                require(r["ok"] != degenerate and actual["status"] == (2 if degenerate else 4)
                        and actual["reason"] == ("silent_local_nonessential_shell" if degenerate else "audit_initial_status"), "local.terminal")
                expected = case["ball"]
                if extra[3] > cert:
                    counts["certificates_q" + str(expected["q"])] += 1
            require(all(actual[k] == v for k, v in expected.items()), "local.rational_geometry_or_literal_level")
            require(p <= extra[0] <= p + min(401, max(0, proposal - p)) and extra[3] >= cert
                    and extra[4] >= fall and extra[3] - cert + extra[4] - fall == int(c < limit), "local.persistent_budget")
            require(extra[1] == a + (delta if extra[4] > fall else 0), "local.physical_reference")
            if case["expected_extra"] is not None:
                require(extra == case["expected_extra"][step], "local.independent_sequence")
            c, p, a, cert, fall = c + delta, extra[0], extra[1], extra[3], extra[4]
    require(all(counts[k] > 0 for k in counts), "campaign.nonvacuity")
    return counts


def judge_faults(raw):
    rows = [json.loads(line) for line in raw.splitlines()]
    require(len(rows) == 8, "faults.row_count")
    details = []
    for case, r in zip(fault_cases(), rows):
        proposal_fault = case["fault"] < 3
        bad_alloc = case["fault"] % 2 == 1
        if case["mode"] == "M":
            require(r["exception"] == ("bad_alloc" if bad_alloc else "runtime_error"), "fault.local_propagation")
            require(r["extra"] == ([1, 0, 0, 0, 0] if proposal_fault else [1, 1, 1, 0, 1]), "fault.local_mirrors")
            require(r["actual"]["stats"] == PRIMES + ([37, 0] if proposal_fault else [38, 1]), "fault.local_paid_work")
            require(r["actual"]["q"] == 9 and len(r["actual"]["events"]) == 2, "fault.local_sentinels")
        else:
            require(r["hooks"][4] == 1, "fault.public_nonempty_before_throw")
            if bad_alloc:
                require(r["exception"] == "none" and r["actual"]["status"] == 3
                        and r["actual"]["reason"] == "silent_allocation_failure"
                        and r["actual"]["events"] == [], "fault.public_purge")
                require(r["extra"] == ([3, 0, 0, 2, 0] if proposal_fault else [2, 1, 0, 2, 1]), "fault.public_mirrors")
                require(r["actual"]["stats"][11:13] == ([2, 2] if proposal_fault else [3, 3]), "fault.public_paid_work")
            else:
                require(r["exception"] == "runtime_error", "fault.public_propagation")
                # The failed return assignment exposes no internal Result.
        require(r["hooks"][3] == 0, "fault.prospective")
        details.append(dict(mode=case["mode"], kind=case["fault"], before_throw_events=r["hooks"][4],
                            internal_result_observable=case["mode"] == "M" or bad_alloc))
    return details


def patch_once(text, before, after):
    require(text.count(before) == 1, "patch.nonunique")
    return text.replace(before, after)


def prepare(label, instrumented, mutant):
    mapping = json.loads((RECEIPTS / "inputs/include_map.json").read_text())["mapping"]
    target = WORK / label
    target.mkdir(parents=True, exist_ok=False)
    pins = {}
    for virtual, row in mapping.items():
        original = (HERE / row["audit_path"]).read_bytes()
        require(sha(original) == row["sha256"], "input.pin")
        text = original.decode()
        if instrumented and virtual == "overlay/meb_proposal.hpp":
            text = patch_once(text, "void before_pair_selection(const Work&, const Limits&) const noexcept {}",
                              "void before_pair_selection(const Work& w, const Limits& l) const noexcept { audit_hooks::pair(w.meb_proposal_supports,l.max_meb_proposal_supports); }")
            text = patch_once(text, "void before_form(const Work&, const Limits&, u8) const noexcept {}",
                              "void before_form(const Work& w, const Limits& l, u8) const { audit_hooks::form(w.meb_proposal_supports,l.max_meb_proposal_supports); }")
        if virtual == "overlay/silent_incidence.hpp":
            if instrumented:
                text = patch_once(text, "++counter;", '++counter;\n    if (std::strcmp(reason, "silent_meb_support_budget") == 0) audit_hooks::reference(counter);')
                text = patch_once(text, "LocalBall* ball) {\n    struct WorkMirror", "LocalBall* ball) {\n    audit_hooks::events_seen = out.events.size();\n    struct WorkMirror")
            if mutant == "charge_after":
                text = patch_once(text, "meb_proposal_detail::propose<false>", "meb_proposal_detail::propose<true>")
            if mutant == "reset_work":
                text = patch_once(text, "LocalBall* ball) {\n    audit_hooks::events_seen", "LocalBall* ball) {\n    meb_work = {};\n    audit_hooks::events_seen")
            if mutant == "drop_P_mirror":
                text = patch_once(text, "        stats.meb_proposal_supports = work.meb_proposal_supports;\n", "")
            if mutant == "drop_A_mirror":
                text = patch_once(text, "stats.meb_fallback_supports += stats.meb_supports - prior;", "(void)stats; (void)prior;")
        data = text.encode()
        path = target / virtual
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
        pins[virtual] = sha(data)
        if data != original:
            diff = "".join(difflib.unified_diff(original.decode().splitlines(True), text.splitlines(True),
                                              fromfile=virtual + "@captured", tofile=virtual + "@audit_" + label))
            (RUN / (label + "_" + Path(virtual).name + ".patch")).write_text(diff)
    return target, pins


def execute(argv, label, deadline, stdin=""):
    before = time.monotonic()
    remaining = min(45, deadline - before)
    require(remaining > 0, "run.deadline")
    proc = subprocess.Popen(argv, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                            start_new_session=True, env=dict(os.environ, TMPDIR=str(WORK),
                            PYTHONDONTWRITEBYTECODE="1", UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"))
    expired = False
    try:
        stdout, stderr = proc.communicate(stdin.encode(), timeout=remaining)
    except subprocess.TimeoutExpired:
        expired = True
        os.killpg(proc.pid, signal.SIGKILL)
        stdout, stderr = proc.communicate()
    for stream, data in (("stdout", stdout), ("stderr", stderr)):
        (RUN / (label + "." + stream + ".gz")).write_bytes(gzip.compress(data, mtime=0))
    return dict(label=label, argv=argv, exit_code=proc.returncode, timeout=expired,
                wall_seconds=time.monotonic() - before, stdin_sha256=sha(stdin.encode()),
                stdout_sha256=sha(stdout), stderr_sha256=sha(stderr))


def run():
    require(not (RUN / "run.json").exists(), "run.create_only")
    RUN.mkdir(parents=True, exist_ok=True)
    WORK.mkdir(exist_ok=True)
    all_cases, faults = cases(), fault_cases()
    save(RUN / "cases.json", all_cases)
    save(RUN / "fault_cases.json", faults)
    report = dict(status="running", started_utc=datetime.now(timezone.utc).isoformat(),
                  deadline_seconds=180, public_status="not_claimed", gcp="not_used",
                  inputs={str(p.relative_to(HERE)): sha(p.read_bytes()) for p in
                          (HERE / "meb_builder_bridge.cpp", Path(__file__), HERE / "meb_rational_oracle_20260905.py",
                           RECEIPTS / "inputs/include_map.json", RUN / "cases.json", RUN / "fault_cases.json")},
                  builds={}, commands=[])
    deadline = time.monotonic() + 180
    try:
        compiler = str(Path(subprocess.check_output(["which", "g++"], text=True).strip()).resolve())
        report["compiler"] = dict(path=compiler, sha256=sha(Path(compiler).read_bytes()))
        report["commands"].append(execute([compiler, "--version"], "compiler", deadline))
        for label, flags, instrumented, mutant in (
            ("O2", ["-O2"], False, None),
            ("ubsan", ["-O1", "-g", "-fsanitize=undefined", "-fno-sanitize-recover=all"], False, None),
            ("instrumented", ["-O2"], True, None),
            ("reset_work", ["-O2"], True, "reset_work"),
            ("drop_P_mirror", ["-O2"], True, "drop_P_mirror"),
            ("drop_A_mirror", ["-O2"], True, "drop_A_mirror"),
            ("charge_after", ["-O2"], True, "charge_after"),
        ):
            target, pins = prepare(label, instrumented, mutant)
            binary = target / "bridge"
            dep = RUN / (label + ".d")
            argv = [compiler, "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", *flags,
                    "-I", str(target), "-I", str(target / "f_include_root"), "-MMD", "-MF", str(dep),
                    str(HERE / "meb_builder_bridge.cpp"), "-o", str(binary)]
            compiled = execute(argv, label + "_compile", deadline)
            report["commands"].append(compiled)
            require(compiled["exit_code"] == 0 and not compiled["timeout"], "compile." + label)
            binary_sha = sha(binary.read_bytes())
            dependencies = shlex.split(dep.read_text().replace("\\\n", " ").split(":", 1)[1])
            dependency_pins = {p: sha(Path(p).read_bytes()) for p in dependencies}
            require(len(dependency_pins) == 20, "compile.dependency_floor")
            report["builds"][label] = dict(instrumented=instrumented, mutant=mutant, sources=pins,
                                           dependency_pins=dependency_pins, binary_sha256=binary_sha)
            command = execute([str(binary)], label, deadline, commands(all_cases))
            report["commands"].append(command)
            require(command["exit_code"] == 0 and not command["timeout"], "execution." + label)
            if label in ("instrumented", "drop_P_mirror", "drop_A_mirror"):
                report["commands"].append(execute([str(binary)], label + "_faults", deadline, commands(faults)))
            require(sha(binary.read_bytes()) == binary_sha and all(sha(Path(p).read_bytes()) == h
                    for p, h in dependency_pins.items()), "compile.terminal_drift")
        report["status"] = "completed"
    except Exception as error:
        report["status"] = "failed"
        report["error"] = str(error)
        raise
    finally:
        report["finished_utc"] = datetime.now(timezone.utc).isoformat()
        save(RUN / "run.json", report)


def replay():
    report = json.loads((RUN / "run.json").read_text())
    require(report["status"] == "completed", "receipt.not_completed")
    for p, h in report["inputs"].items():
        require(sha((HERE / p).read_bytes()) == h, "receipt.input_changed")
    nominal, faults = cases(), fault_cases()
    require(json.loads((RUN / "cases.json").read_text()) == json.loads(json.dumps(nominal)) and
            json.loads((RUN / "fault_cases.json").read_text()) == json.loads(json.dumps(faults)), "receipt.cases_changed")
    for r in report["commands"]:
        require(r["exit_code"] == 0 and not r["timeout"], "receipt.command_terminal")
        for stream in ("stdout", "stderr"):
            data = gzip.decompress((RUN / (r["label"] + "." + stream + ".gz")).read_bytes())
            require(sha(data) == r[stream + "_sha256"], "receipt.output_changed")
        if r["label"] in report["builds"] or r["label"].endswith("_faults"):
            expected = commands(faults if r["label"].endswith("_faults") else nominal)
            require(sha(expected.encode()) == r["stdin_sha256"], "receipt.stdin_changed")
    result = dict(status="passed", public_status="not_claimed", optimization=sys.flags.optimize,
                  engine_runs=0, gcp="not_used", normal_builds={})
    for label in ("O2", "ubsan", "instrumented"):
        result["normal_builds"][label] = judge(read_output(label), nominal, label == "instrumented")
    native = [json.loads(x) for x in read_output("O2").splitlines()]
    observed = [json.loads(x) for x in read_output("instrumented").splitlines()]
    require(all({k: v for k, v in a.items() if k != "hooks"} == {k: v for k, v in b.items() if k != "hooks"}
                for a, b in zip(native, observed)), "instrumentation.nominal_equivalence")
    result["exceptions"] = judge_faults(read_output("instrumented_faults"))
    result["mutants"] = []
    for label in ("reset_work", "drop_P_mirror", "drop_A_mirror", "charge_after"):
        try:
            judge(read_output(label), nominal, True)
        except ValueError as error:
            result["mutants"].append(dict(name=label, first_rejection=str(error)))
        else:
            raise ValueError("mutant.survived." + label)
    for label in ("drop_P_mirror", "drop_A_mirror"):
        try:
            judge_faults(read_output(label + "_faults"))
        except ValueError as error:
            result["mutants"].append(dict(name=label + "_exception", first_rejection=str(error)))
        else:
            raise ValueError("exception_mutant.survived")
    save(RUN / ("optimized.json" if sys.flags.optimize else "normal.json"), result)
    print(json.dumps(result))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--run", action="store_true")
    args = parser.parse_args()
    if args.run:
        run()
    replay()
