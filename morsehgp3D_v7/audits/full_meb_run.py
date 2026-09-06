"""Bounded independent FULL/proposal replay; all writes remain under audits."""
from __future__ import annotations

import argparse
from collections import Counter
from copy import deepcopy
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
from datetime import datetime, timezone

import full_lazy_audit as lazy

A = Path(__file__).resolve().parent
ROOT = A.parents[1]
R = A / "receipts_full_meb_20260906"
W = A / ".work_full_meb_20260906"
L = A / "receipts_full_successor_20260905"
BRIDGE = A / "full_meb_bridge.cpp"
HEADERS = {"src/forest/full_gabriel.hpp": "a946e31dde8fbd8ec528d6f5e94f9c727998acc172b4dd29c084dd522c730d1d",
           "src/forest/meb_proposal.hpp": "f922544b5cfdc214de96ecd49520e318ea8632d14a8142ef21fd248f9cc38fb3"}
POLICIES = [0, 1, 1000000]
ACCOUNTING = "reference_ordinal_plus_native_z_q3_q4_proposal_v2"


def require(ok, reason):
    if not ok:
        raise ValueError(reason)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write(path, data):
    path.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n")


def stamp():
    return datetime.now(timezone.utc).isoformat()


def execute(argv, name, data=None, env=None):
    require(not (R / (name + ".json")).exists(), "immutable command name: " + name)
    argv = ["timeout", "--signal=TERM", "--kill-after=5", "180", "taskset", "-c", "1", *map(str, argv)]
    intent = {"argv": argv, "started_utc": stamp(), "cwd": str(ROOT), "environment": env or {},
              "input_sha256": hashlib.sha256(data or b"").hexdigest()}
    write(R / (name + ".intent.json"), intent)
    p = subprocess.run(argv, cwd=ROOT, input=data, capture_output=True,
                       env={**os.environ, **(env or {})})
    # Keep every stdout losslessly, without duplicate expanded JSON streams.
    outpath = R / (name + ".stdout.gz")
    outpath.write_bytes(gzip.compress(p.stdout, mtime=0))
    errpath = R / (name + ".stderr")
    errpath.write_bytes(p.stderr)
    result = dict(intent, ended_utc=stamp(), exit_code=p.returncode,
                  stdout_sha256=hashlib.sha256(p.stdout).hexdigest(),
                  stdout_gzip_sha256=sha(outpath), stderr_sha256=sha(errpath))
    write(R / (name + ".json"), result)
    print(json.dumps({"command": name, "exit_code": p.returncode}), flush=True)
    require(p.returncode == 0, "command failed; all streams retained: " + name)
    return result


def fixture_sets():
    old = lazy.load_fixtures()
    lazy.verify_fixtures(old)
    mixed = json.loads((A / "receipts_full_singleton_20260905/target_fixtures.json").read_text())
    data = {"legacy": old, "mixed": mixed}
    high = R / "higher_order_fixtures.json"
    if high.exists():
        data["higher"] = json.loads(high.read_text())
    return data


def prepare():
    require(not (R / "source_binding.json").exists() and not W.exists(), "create-only source/build")
    R.mkdir(exist_ok=True)
    W.mkdir()
    pins = {}
    historical = json.loads((L / "source_pins.json").read_text())["pins"]
    for path, digest in historical.items():
        q = L / "source" / path
        require(sha(q) == digest, "sealed L dependency changed: " + path)
        pins[str(q.relative_to(ROOT))] = digest
    for path, digest in HEADERS.items():
        live = ROOT / "morsehgp3D_v7" / path
        require(sha(live) == digest, "live capture premise: " + path)
        dest = R / "source/morsehgp3D_v7" / path
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_bytes(live.read_bytes())
        require(sha(dest) == digest and sha(live) == digest, "capture race: " + path)
        pins[str(dest.relative_to(ROOT))] = digest
    # Old frozen dependencies are explicitly reused as bytes, not as results.
    for path, digest in historical.items():
        if path != "morsehgp3D_v7/src/forest/full_gabriel.hpp":
            require(sha(ROOT / path) == digest, "unexpected other live product delta: " + path)
    pin_files = [BRIDGE, Path(__file__), A / "full_lazy_audit.py", A / "full_producer_audit.py",
                 A / "receipts_full_producer_20260905/fixtures.json",
                 A / "receipts_full_lazy_20260905/fixtures.json",
                 A / "receipts_full_singleton_20260905/target_fixtures.json",
                 L / "O2_output.json", L / "O2_mixed_output.json"]
    for path in pin_files:
        pins[str(path.relative_to(ROOT))] = sha(path)
    write(R / "source_binding.json", {"created_utc": stamp(), "pins": pins,
          "source_head": subprocess.check_output(["git", "rev-parse", "HEAD"], text=True).strip(),
          "worktree": subprocess.check_output(["git", "status", "--porcelain"], text=True),
          "compiler": subprocess.check_output(["g++", "--version"], text=True),
          "scope": "Two captured product headers, unchanged frozen L dependencies; no inherited test result."})
    for name, fixture in fixture_sets().items():
        (R / (name + ".stdin")).write_text(lazy.fixture_text(fixture))


def include_flags(source=None):
    source = source or R / "source"
    return ["-I", str(source / "morsehgp3D_v7"),
            "-I", str(L / "source/morsehgp3D_v7/src/forest"),
            "-I", str(L / "source/morsehgp3D_v7")]


def build(name, mutant=False):
    require(name in ("O2", "sanitized", "reset_work"), "build name")
    source = R / "source"
    if mutant:
        source = R / "reset_work/source"
        original = (R / "source/morsehgp3D_v7/src/forest/full_gabriel.hpp").read_text()
        target = "    meb_proposal_detail::NoObserver observer;"
        require(original.count(target) == 1, "unique reset-work mutation site")
        for path in HEADERS:
            dest = source / "morsehgp3D_v7" / path
            dest.parent.mkdir(parents=True, exist_ok=True)
            data = (R / "source/morsehgp3D_v7" / path).read_text()
            if path.endswith("full_gabriel.hpp"):
                data = data.replace(target, "    meb_work = {};  // Private audit mutant.\n" + target)
            dest.write_text(data)
    adapter = R / (name + "_adapter.cpp")
    require(not adapter.exists(), "adapter snapshot exists")
    adapter.write_bytes(BRIDGE.read_bytes())
    flags = ["-O1", "-g0", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer"] if name == "sanitized" else ["-O2"]
    dep = R / (name + ".d")
    exe = W / (name + ".bin")
    execute(["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", *flags,
             "-MMD", "-MF", dep, *include_flags(source), adapter, "-o", exe], name + "_build")
    items = dep.read_text().replace("\\\n", " ").split(":", 1)[1].split()
    dependencies = {str(Path(p).resolve().relative_to(ROOT)): sha(Path(p)) for p in items}
    require(all(p.startswith("morsehgp3D_v7/audits/") for p in dependencies), "build consumed live product")
    write(R / (name + "_binary.json"), {"sha256": sha(exe), "dependencies": dependencies,
          "source_kind": "private reset-work mutant" if mutant else "unchanged captured product",
          "system_headers": "excluded by -MMD; not a hermetic build claim"})


def run(name, group):
    fixture = fixture_sets()[group]
    stdin = lazy.fixture_text(fixture).encode()
    path = R / (group + ".stdin")
    if path.exists():
        require(path.read_bytes() == stdin, "fixture transport changed")
    else:
        path.write_bytes(stdin)
    env = {"ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1", "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"} if name == "sanitized" else {}
    exe = W / (name + ".bin")
    require(sha(exe) == json.loads((R / (name + "_binary.json")).read_text())["sha256"], "binary binding")
    for cap in POLICIES if name != "reset_work" else [1]:
        execute([exe, str(cap)], name + "_" + group + "_P" + str(cap), stdin, env)


def read_output(name):
    receipt = json.loads((R / (name + ".json")).read_text())
    path = R / (name + ".stdout.gz")
    require(receipt["exit_code"] == 0 and sha(path) == receipt["stdout_gzip_sha256"], "stream binding")
    data = gzip.decompress(path.read_bytes())
    require(hashlib.sha256(data).hexdigest() == receipt["stdout_sha256"] and
            not (R / (name + ".stderr")).read_bytes(), "transport or sanitizer failure")
    return json.loads(data)["records"]


def historical(row):
    row = deepcopy(row)
    row.pop("proposal_limit", None)
    row.pop("meb_accounting", None)
    row["stats"].pop("proposal", None)
    for t in row["budget_trials"]:
        t["stats"].pop("proposal", None)
    return row


def work_checks(row, cap, successful=True):
    s, w = row["stats"], row["stats"]["proposal"]
    c, calls = s["geometry"]["meb_supports"], s["meb_calls"]
    require(set(w) == {"p", "pivots", "certified", "fallback", "A"}, "proposal schema")
    require(all(type(x) is int and 0 <= x < (1 << 64) for x in w.values()), "proposal u64")
    require(w["p"] <= cap and w["A"] <= c and w["pivots"] <= w["p"], "proposal bounds")
    if successful:
        require(w["certified"] + w["fallback"] == calls, "persistent terminal counter partition")
        require(w["A"] + w["certified"] <= c, "certified ordinal excluded from A")
        if cap == 0:
            require(w == {"p": 0, "pivots": 0, "certified": 0, "fallback": calls, "A": c}, "P0 physical work")
    if cap == 1:
        require(w["p"] == int(calls > 0) if successful else w["p"] <= 1, "P1 spent once per order")
        require(w["certified"] <= 1, "P1 at most one certificate")


def judge():
    counts = Counter()
    sets = fixture_sets()
    for group, fixture in sets.items():
        prior_name = "O2_output.json" if group == "legacy" else "O2_mixed_output.json"
        prior = json.loads((L / prior_name).read_text())["records"] if group != "higher" else None
        nominal = {}
        for name in ("O2", "sanitized"):
            for cap in POLICIES:
                rows = read_output(name + "_" + group + "_P" + str(cap))
                require(len(rows) == len(fixture["records"]), "record inventory")
                if cap == 0:
                    nominal[name] = rows
                for i, (row, f) in enumerate(zip(rows, fixture["records"])):
                    require(row["proposal_limit"] == cap and row["meb_accounting"] == ACCOUNTING, "declared P/calendar")
                    require(row["id"] == f["id"] and row["status"] == 0 and row["order"] == f["order"] and
                            row["reason"] == lazy.reference.AUTHORITY, "identity/status")
                    e = f["expected"]
                    require(len(row["nodes"]) == len(e["nodes"]), "Gamma node inventory")
                    for node, want in zip(row["nodes"], e["nodes"]):
                        require(lazy.reference.rational(node) == lazy.reference.rational(want) and
                                (node["first"], node["parent_count"]) == (want["first"], want["parent_count"]), "Gamma ordered nodes")
                    for field in ("minima", "parents", "coverage"):
                        require(row[field] == e[field], "Gamma " + field)
                    require(len(row["cuts"]) == len(e["roots"]), "Gamma cuts length")
                    for cut, want in zip(row["cuts"], e["roots"]):
                        require(cut["status"] == 0 and cut["reason"] == "structural_only" and cut["roots"] == want, "Gamma strict/closed cut")
                    legacy_stats = deepcopy(row["stats"])
                    legacy_stats.pop("proposal")
                    lazy.check_stats(f, legacy_stats)
                    if prior:
                        require(historical(row) == prior[i], "literal L reference including failed prefixes")
                    require(historical(row) == historical(nominal[name][i]), "proposal noninterference including caps")
                    work_checks(row, cap)
                    for t in row["budget_trials"]:
                        work_checks(t, cap, t["status"] == 0)
                    counts["records"] += 1
                    counts["cuts"] += len(row["cuts"])
                    counts["budget_trials"] += len(row["budget_trials"])
                    w = row["stats"]["proposal"]
                    counts["certified_outputs_P" + str(cap)] += w["certified"] > 0
                    counts["mixed_outputs_P" + str(cap)] += w["certified"] > 0 and w["fallback"] > 0
                    if group == "higher":
                        counts["higher_K" + str(f["order"]) + "_chain_outputs"] += row["stats"]["chain_steps"] > 0
                        if f["lazy"] and f["capacity"] == 0:
                            witness = next(o["witness"] for o in fixture["orders"] if o["order"] == f["order"])
                            for field in ("portal_requests", "chain_steps", "singleton_intruder_resolutions"):
                                require(row["stats"][field] == witness[field], "higher exact C0 path count: " + field)
                            counts["higher_C0_model_count_matches"] += 1
            for cap in POLICIES:
                require(read_output("O2_" + group + "_P" + str(cap)) == read_output("sanitized_" + group + "_P" + str(cap)), "build equality")
        rows = read_output("reset_work_" + group + "_P1") if group == "legacy" else []
        for i, row in enumerate(rows):
            require(historical(row) == historical(nominal["O2"][i]), "reset mutant changed legacy forest or prefix")
            try:
                work_checks(row, 1)
            except ValueError as error:
                require(str(error) == "persistent terminal counter partition", "causal reset mutant: " + str(error))
                counts["reset_work_mutant_witnesses"] += 1
    require(counts["reset_work_mutant_witnesses"] > 0 and counts["mixed_outputs_P1"] > 0 and
            counts["higher_K10_chain_outputs"] > 0, "nonvacuity of state and high orders")
    result = {"schema": "mhgp7-independent-full-meb-replay-v1", "status": "passed", "counts": dict(counts),
              "source_headers": HEADERS, "scripts": {str(p.relative_to(ROOT)): sha(p) for p in (Path(__file__), BRIDGE)},
              "builds": ["O2", "sanitized"], "policies": POLICIES,
              "scope": "Fresh bounded FULL engines on captured headers, independent rational Gamma fixtures and literal L differential; no performance or catalogue producer qualification.",
              "public_status": "not_claimed", "gcp": "not_used"}
    write(R / ("optimized.json" if sys.flags.optimize else "normal.json"), result)
    print(json.dumps(result))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("prepare", "build", "run", "judge"))
    parser.add_argument("--name", choices=("O2", "sanitized", "reset_work"), default="O2")
    parser.add_argument("--group", choices=("legacy", "mixed", "higher"), default="legacy")
    args = parser.parse_args()
    if args.action == "prepare":
        prepare()
    elif args.action == "build":
        build(args.name, args.name == "reset_work")
    elif args.action == "run":
        run(args.name, args.group)
    else:
        judge()


if __name__ == "__main__":
    main()
