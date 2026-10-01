#!/usr/bin/env python3
"""Strict READ-ONLY reader. External manifests before JSON; independent analytic signatures. No probe replay."""
import argparse
import datetime
import hashlib
import json
import math
import os
import pathlib
import re
import stat
import sys
from fractions import Fraction

sys.dont_write_bytecode = True
BASE = pathlib.Path(__file__).absolute().parent
FILES = {"README.txt", "origin.json", "protocol.json", "probe.py", "read.py", "record.py",
         "original/fullk.py", "original/frontier_core.py"}
CASES = [(8, 5, True), (16, 5, True), (32, 5, False), (16, 10, True), (32, 10, False)]
PINS = {"original/fullk.py": "a09a6c85687971f80e0df030effaf4cc112b9641176badf8460296898625a44e",
        "original/frontier_core.py": "86ba984ff7986bdd44a5d53e9b2c7d3f2c0eb847764938cd28259d18439850b7"}
PREDECESSOR = "f59cd502d0205c6379fa2f033d79e3377f29dc5d10d12f1e22e92ea910376330"
CAPFILES = {"normal.json", "optimized.json", "run_receipt.json"}


def require(cond, message):
    if not cond:
        raise RuntimeError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def keys(value, expected, message):
    require(type(value) is dict and set(value) == set(expected), message)


def integer(value, expected=None):
    require(type(value) is int, "true integer required")
    if expected is not None:
        require(value == expected, "integer value")


def absolute(value):
    require(type(value) is str and value, "absolute path string")
    p = pathlib.Path(value)
    require(p.is_absolute() and str(p) == value and os.path.normpath(value) == value, "absolute path canonical")
    return p


def utc(value):
    require(type(value) is str and re.fullmatch(r"\d{4}-\d\d-\d\dT\d\d:\d\d:\d\d(?:\.\d{1,6})?Z", value),
            "real UTC string with Z")
    parsed = datetime.datetime.fromisoformat(value[:-1] + "+00:00")
    require(parsed.utcoffset() == datetime.timedelta(0), "UTC offset")
    return parsed


def strict_json(data):
    def object_hook(pairs):
        out = {}
        for name, value in pairs:
            require(name not in out, "JSON duplicate key")
            out[name] = value
        return out
    def no_constant(value):
        raise RuntimeError("JSON nonfinite constant: " + value)
    result = json.loads(data, object_pairs_hook=object_hook, parse_constant=no_constant)
    def walk(value):
        if type(value) is float:
            require(math.isfinite(value), "JSON nonfinite float")
        elif type(value) is dict:
            for v in value.values():
                walk(v)
        elif type(value) is list:
            for v in value:
                walk(v)
    walk(result)
    return result

def safe_absolute(path, must_exist=True, directory=False):
    require(path.is_absolute() and str(path) == str(pathlib.Path(os.path.normpath(str(path)))),
            "canonical absolute path")
    for part in [path] + list(path.parents):
        try:
            mode = part.lstat().st_mode
        except FileNotFoundError:
            require(part == path and not must_exist, "missing parent/path")
            continue
        require(not stat.S_ISLNK(mode), "symlink root/parent/payload: " + str(part))
        if part != path or directory:
            require(stat.S_ISDIR(mode), "directory required: " + str(part))
        elif must_exist:
            require(stat.S_ISREG(mode), "regular file required: " + str(part))
    return path

def verify_tree(base, external_pin, expected):
    safe_absolute(base, must_exist=True, directory=True)
    require(type(external_pin) is str and re.fullmatch("[0-9a-f]{64}", external_pin), "external manifest SHA")
    manifest = base / "SHA256SUMS"
    safe_absolute(manifest, must_exist=True, directory=False)
    raw = manifest.read_bytes()
    require(sha(raw) == external_pin, "external manifest SHA BEFORE parsing/import")
    listed = {}
    for line in raw.decode("ascii").splitlines():
        digest, rel = line.split("  ", 1)
        p = pathlib.PurePosixPath(rel)
        require(re.fullmatch("[0-9a-f]{64}", digest) and rel and not p.is_absolute()
                and ".." not in p.parts and str(p) == rel and rel not in listed, "manifest syntax")
        listed[rel] = digest
    require(set(listed) == expected, "manifest inventory")
    actual = set()
    for path in base.rglob("*"):
        mode = path.lstat().st_mode
        require(not stat.S_ISLNK(mode), "payload symlink")
        require(stat.S_ISDIR(mode) or stat.S_ISREG(mode), "nonregular payload")
        if stat.S_ISREG(mode):
            actual.add(path.relative_to(base).as_posix())
    require(actual == expected | {"SHA256SUMS"}, "actual inventory missing/extra")
    for rel, digest in listed.items():
        payload = base / rel
        safe_absolute(payload, must_exist=True, directory=False)
        require(sha(payload.read_bytes()) == digest, "payload SHA: " + rel)
    return listed

def python_identity():
    exe = pathlib.Path(sys.executable).resolve(strict=True)
    safe_absolute(exe, must_exist=True, directory=False)
    return dict(path=str(exe), sha256=sha(exe.read_bytes()), version=sys.version,
                version_info=list(sys.version_info), implementation=sys.implementation.name)


def validate_execution(exe):
    keys(exe, {"path", "sha256", "version", "version_info", "implementation"}, "Python identity fields")
    absolute(exe["path"])
    require(type(exe["sha256"]) is str and re.fullmatch("[0-9a-f]{64}", exe["sha256"]), "Python binary SHA")
    require(type(exe["version"]) is str and exe["version"] and type(exe["implementation"]) is str
            and exe["implementation"], "Python version text")
    vi = exe["version_info"]
    require(type(vi) is list and len(vi) == 5 and all(type(vi[i]) is int for i in (0, 1, 2, 4))
            and type(vi[3]) is str and vi[3] in {"alpha", "beta", "candidate", "final"}, "Python version info types")
    return exe


def verify_package(pin):
    listed = verify_tree(BASE, pin, FILES)
    require(all(listed[name] == value for name, value in PINS.items()), "snapshot source pins")
    origin = strict_json((BASE / "origin.json").read_bytes())
    keys(origin, {"schema", "prepared_at_utc", "predecessor", "sources"}, "preparation keys")
    require(origin["schema"] == "ancestor_closure_1d_preparation_v2", "preparation schema")
    utc(origin["prepared_at_utc"])
    keys(origin["predecessor"], {"root", "manifest_sha256", "status"}, "predecessor keys")
    absolute(origin["predecessor"]["root"])
    require(origin["predecessor"]["manifest_sha256"] == PREDECESSOR, "predecessor pin")
    require(type(origin["predecessor"]["status"]) is str and origin["predecessor"]["status"], "predecessor status")
    keys(origin["sources"], PINS, "preparation source inventory")
    for name, pin_source in PINS.items():
        row = origin["sources"][name]
        keys(row, {"original_path", "copied_from", "sha256"}, "preparation source keys")
        absolute(row["original_path"])
        absolute(row["copied_from"])
        require(row["sha256"] == pin_source, "preparation source pin")
    protocol_bytes = (BASE / "protocol.json").read_bytes()
    protocol = strict_json(protocol_bytes)
    keys(protocol, {"schema", "scope", "sites", "cases", "source_pins", "limits", "timeout_seconds"}, "protocol keys")
    require(protocol["schema"] == "ancestor_closure_1d_protocol_v2" and protocol["source_pins"] == PINS, "protocol pins/schema")
    require(type(protocol["scope"]) is str and protocol["scope"], "scope")
    require(protocol["sites"] == "x_j=(j*(j+1)/2,0,0), j=0..n-1", "site formula")
    require(type(protocol["limits"]) is list and all(type(v) is str and v for v in protocol["limits"]), "limits")
    integer(protocol["timeout_seconds"], 30)
    require(type(protocol["cases"]) is list and len(protocol["cases"]) == len(CASES), "protocol case count")
    for row, (n, K, g) in zip(protocol["cases"], CASES):
        keys(row, {"n", "K", "gamma"}, "protocol case keys")
        integer(row["n"], n)
        integer(row["K"], K)
        require(type(row["gamma"]) is bool and row["gamma"] is g, "protocol Gamma bool")
    return listed, sha(protocol_bytes)


def encoded(value):
    return (json.dumps(value, sort_keys=True, separators=(",", ":")) + "\n").encode()


def q(value):
    value = Fraction(value)
    return str(value.numerator) + "/" + str(value.denominator)


def analytic_signatures(n, K):
    """Independent closed-form nodes/covers: NO AST source, Tree/export adapter or Gamma invocation."""
    xs = [j * (j + 1) // 2 for j in range(n)]
    m = n - K + 1
    leaves = [Fraction((xs[i + K - 1] - xs[i]) ** 2, 4) for i in range(m)]
    joins = [Fraction((xs[i + K] - xs[i]) ** 2, 4) for i in range(m - 1)]
    lh = [sha(encoded([q(b), []])) for b in leaves]
    jh = []
    for i, b in enumerate(joins):
        previous = lh[0] if i == 0 else jh[i - 1]
        jh.append(sha(encoded([q(b), sorted([previous, lh[i + 1]])])))
    nodes = []
    for i, b in enumerate(leaves):
        death = None if m == 1 else joins[max(0, i - 1)]
        nodes.append([lh[i], q(b), None if death is None else q(death), []])
    for i, b in enumerate(joins):
        death = joins[i + 1] if i + 1 < len(joins) else None
        child = [lh[0] if i == 0 else jh[i - 1], lh[i + 1]]
        nodes.append([jh[i], q(b), None if death is None else q(death), sorted(child)])
    cov = []
    for j, x in enumerate(xs):
        relation = [[lh[i], q(leaves[i])] for i in range(m) if i <= j <= i + K - 1]
        relation.extend([jh[i], q(joins[i])] for i in range(m - 1) if j <= i + K)
        cov.append([[x, 0, 0], sorted(relation)])
    return sha(encoded(sorted(nodes))), sha(encoded(cov))


def judge(result, protocol_sha, execution):
    keys(result, {"schema", "protocol_sha", "sources", "cases", "execution"}, "result keys")
    require(result["schema"] == "ancestor_closure_1d_result_v2" and result["protocol_sha"] == protocol_sha
            and result["sources"] == PINS, "result provenance/schema")
    validate_execution(result["execution"])
    require(result["execution"] == execution, "child execution identity")
    require(type(result["cases"]) is list and len(result["cases"]) == len(CASES), "result case count")
    for row, (n, K, exhaustive) in zip(result["cases"], CASES):
        keys(row, {"n", "K", "H", "seeds", "D", "catalogue_balls", "weak_inc",
                   "forest_sha", "cover_sha", "gamma"}, "result row keys")
        expected = {"n": n, "K": K, "H": 2 * (n - K + 1) - 1, "seeds": K * (n - K + 1),
                    "D": K * (n - K + 1) + (n - K) * (n + K + 1) // 2,
                    "catalogue_balls": sum(n - p + 1 for p in range(2, K + 2)),
                    "weak_inc": K * (n - K + 1) + (K + 1) * (n - K)}
        for name, value in expected.items():
            integer(row[name], value)
        forest, coverage = analytic_signatures(n, K)
        require(row["forest_sha"] == forest and row["cover_sha"] == coverage, "independent analytic signatures")
        g = row["gamma"]
        if exhaustive:
            keys(g, {"vertices", "edges", "H", "D", "forest_sha", "cover_sha"}, "Gamma keys")
            for name, value in {"vertices": math.comb(n, K), "edges": math.comb(n, K + 1),
                                "H": expected["H"], "D": expected["D"]}.items():
                integer(g[name], value)
            require(g["forest_sha"] == forest and g["cover_sha"] == coverage, "Gamma signatures")
        else:
            require(g is None, "Gamma32 prohibited")
    return True


def verify_capture(cap, cap_pin, source_pin, listed, protocol_sha):
    verify_tree(cap, cap_pin, CAPFILES)
    raw = [(cap / name).read_bytes() for name in ("normal.json", "optimized.json")]
    receipt = strict_json((cap / "run_receipt.json").read_bytes())
    keys(receipt, {"schema", "source_manifest_sha", "origin", "protocol", "run_limit_seconds", "runs"}, "receipt keys")
    require(receipt["schema"] == "ancestor_closure_1d_run_receipt_v2" and receipt["source_manifest_sha"] == source_pin,
            "receipt schema/source manifest")
    integer(receipt["run_limit_seconds"], 30)
    origin = receipt["origin"]
    keys(origin, {"package_root", "source_files", "manifest", "executable"}, "origin keys")
    root = absolute(origin["package_root"])
    keys(origin["source_files"], FILES, "origin source inventory")
    for name in FILES:
        row = origin["source_files"][name]
        keys(row, {"path", "sha256"}, "origin source fields")
        require(row["path"] == str(root / name) and row["sha256"] == listed[name], "exact source path/pin")
    keys(origin["manifest"], {"path", "sha256"}, "manifest origin")
    require(origin["manifest"] == dict(path=str(root / "SHA256SUMS"), sha256=source_pin), "exact manifest origin")
    exe = origin["executable"]
    keys(exe, {"path", "sha256", "version", "version_info", "implementation"}, "Python identity fields")
    absolute(exe["path"])
    require(type(exe["sha256"]) is str and re.fullmatch("[0-9a-f]{64}", exe["sha256"]), "Python binary SHA")
    require(type(exe["version"]) is str and exe["version"] and type(exe["implementation"]) is str
            and exe["implementation"], "Python version text")
    vi = exe["version_info"]
    require(type(vi) is list and len(vi) == 5 and all(type(vi[i]) is int for i in (0, 1, 2, 4))
            and type(vi[3]) is str and vi[3] in {"alpha", "beta", "candidate", "final"}, "Python version info types")
    require(exe == python_identity(), "LIVE reader Python SHA/version mismatch")
    keys(receipt["protocol"], {"path", "sha256"}, "protocol origin fields")
    require(receipt["protocol"] == dict(path=str(root / "protocol.json"), sha256=protocol_sha), "exact protocol origin")
    runs = receipt["runs"]
    require(type(runs) is list and len(runs) == 2, "two runs required")
    for i, (run, payload) in enumerate(zip(runs, raw)):
        keys(run, {"mode", "argv", "cwd", "timeout_seconds", "timed_out", "code", "start_utc", "end_utc",
                   "duration_seconds", "stdout_file", "stdout_sha256", "stderr"}, "run fields")
        mode = "normal" if i == 0 else "optimized"
        flags = ["-B"] if i == 0 else ["-B", "-O"]
        expected_argv = [exe["path"]] + flags + [str(root / "probe.py"), "--protocol", str(root / "protocol.json")]
        require(run["mode"] == mode and run["argv"] == expected_argv and run["cwd"] == str(root), "EXACT argv/cwd")
        integer(run["timeout_seconds"], 30)
        require(type(run["timed_out"]) is bool and run["timed_out"] is False, "timeout must be false")
        integer(run["code"], 0)
        start, end = utc(run["start_utc"]), utc(run["end_utc"])
        elapsed = run["duration_seconds"]
        require(type(elapsed) in (int, float) and math.isfinite(elapsed) and 0 < elapsed <= 30.25,
                "finite duration within timeout")
        require(end >= start and abs((end - start).total_seconds() - elapsed) <= 0.25, "UTC duration agreement")
        require(run["stdout_file"] == mode + ".json" and run["stdout_sha256"] == sha(payload), "stdout identity")
        require(type(run["stderr"]) is str and run["stderr"] == "", "stderr must be empty")
        judge(strict_json(payload), protocol_sha, exe)
    require(raw[0] == raw[1], "normal/-O bytes differ")
    # Final integrity checks include ALL source files and ALL capture payloads, not just parsed results.
    verify_package(source_pin)
    verify_tree(cap, cap_pin, CAPFILES)
    require(exe == python_identity(), "Python changed during reading")
    return True


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--source-manifest-sha", required=True)
    ap.add_argument("--capture")
    ap.add_argument("--capture-manifest-sha")
    a = ap.parse_args()
    listed, protocol_sha = verify_package(a.source_manifest_sha)
    if a.capture is None:
        require(a.capture_manifest_sha is None, "capture SHA without capture")
        verify_package(a.source_manifest_sha)
        print("source_package_verified; NO probe or recorder executed")
    else:
        require(a.capture_manifest_sha is not None, "capture external manifest required")
        verify_capture(absolute(a.capture), a.capture_manifest_sha, a.source_manifest_sha, listed, protocol_sha)
        print("capture_verified; independent analytic signatures; five cases/three Gamma; normal/-O identical; no replay")
    return 0


if __name__ == "__main__":
    sys.exit(main())
