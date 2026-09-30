"""Strict hash-first archived alias proof plus isolated AST replay, no LIVE imports/native."""
import argparse
import ast
import contextlib
import csv
import hashlib
import io
import json
import os
from pathlib import Path
import re
import tempfile
from types import SimpleNamespace

ORIGINAL = "/tmp/mhgp10-banc-alias-audit-20260930.6wo4SVV0"
MANIFEST = "3f23f6d3980296230a225e1425b5f44ad25bd3e66c6b8a917a4afa7bdb225cc0"
SOURCE = "14f3915daef73088360cf5d90be1a76aab81666ddb22764dc2ea003822714dea"
LIVE = "/workspaces/E-HGP/build/v10-integration-r2/src/morsehgp3D_v10/bench/scaling/scale_run.py"
FILES = {
    "MANIFEST.json", "README.md", "probe.py", "receipt.json", "scale_run_snapshot.py",
    "normal.stdout", "normal.stderr", "optimized.stdout", "optimized.stderr",
    "distinct.csv", "distinct.jsonl", "exact_alias.csv",
}


def require(ok, message):
    if not ok:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inventory(root):
    require(sha(root / "SHA256SUMS") == MANIFEST, "fixed original manifest identity")
    pins = {}
    for line in (root / "SHA256SUMS").read_text().splitlines():
        match = re.fullmatch(r"([0-9a-f]{64})  ([A-Za-z0-9_.-]+)", line)
        require(match is not None, "manifest line")
        value, name = match.groups()
        require(name in FILES and name not in pins, "manifest inventory")
        pins[name] = value
    require(set(pins) == FILES, "manifest set")
    actual = {str(p.relative_to(root)) for p in root.rglob("*") if p.is_file()}
    require(actual == FILES | {"SHA256SUMS"}, "original actual inventory")
    for name, value in pins.items():
        require(not (root / name).is_symlink() and sha(root / name) == value, "SHA " + name)
    require(pins["scale_run_snapshot.py"] == SOURCE, "source identity")
    return pins


def judge_files(csv_bytes, journal_bytes):
    try:
        records = [json.loads(s) for s in journal_bytes.decode().splitlines() if s.strip()]
        journal_valid = len(records) == 2 and [c["call"] for c in records] == ["catalogue", "tower"]
    except (ValueError, TypeError, KeyError):
        journal_valid = False
    rows = list(csv.DictReader(io.StringIO(csv_bytes.decode())))
    csv_valid = len(rows) == 1 and rows[0].get("status") == "ok"
    return csv_valid, journal_valid


def replay(root, source):
    tree = ast.parse(source)
    functions = [n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name == "cmd_run"]
    require(len(functions) == 1, "cmd_run inventory")
    assignments = {}
    for node in tree.body:
        if isinstance(node, ast.Assign) and len(node.targets) == 1 and isinstance(node.targets[0], ast.Name):
            name = node.targets[0].id
            if name in ("COLS", "CALL_KEYS"):
                assignments[name] = ast.literal_eval(node.value)
    require(set(assignments) == {"COLS", "CALL_KEYS"}, "constant inventory")
    require(all(isinstance(t, tuple) and all(type(x) is str for x in t) for t in assignments.values()),
            "literal string tuples")
    observed = {}
    with tempfile.TemporaryDirectory(prefix="mhgp10-alias-ast-") as scratch:
        tmp = Path(scratch)
        (tmp / "MANIFEST.json").write_bytes((root / "MANIFEST.json").read_bytes())
        measure_calls = []

        def denied(*_a, **_kw):
            raise ValueError("forbidden subprocess/native/fallback/signals")

        def safe_open(path, *args, **kwargs):
            p = Path(path).resolve()
            require(p.parent == tmp, "AST IO outside owned temporary directory")
            return open(p, *args, **kwargs)

        def measure(_build, _path, k, threads):
            measure_calls.append((k, threads))
            calls = []
            for name in ("catalogue", "tower"):
                native = json.dumps(dict(status="ok", balls=7, fixture=name)) + "\n"
                calls.append(dict(call=name, argv=["NOT_EXECUTED"], code=0, timed_out=False,
                                  wall_s=0.01, cpu_s=0.01, max_rss_kb=1, stdout=native, stderr=""))
            return dict(k=k, threads=threads, status="ok", balls=7, tower_s=0.01), calls

        env = {
            "__builtins__": {"open": safe_open, "sorted": sorted, "int": int, "any": any,
                             "dict": dict, "print": print},
            "os": SimpleNamespace(path=SimpleNamespace(join=os.path.join, exists=os.path.exists)),
            "json": SimpleNamespace(load=json.load, dumps=json.dumps),
            "csv": SimpleNamespace(DictWriter=csv.DictWriter),
            "signal": SimpleNamespace(SIGTERM=15, SIGHUP=1, SIGINT=2, SIG_DFL=0,
                                     default_int_handler="default", getsignal=lambda _s: "ignored",
                                     signal=denied),
            "time": SimpleNamespace(time=lambda: 0.0), "measure": measure, "infer_manifest": denied,
            "terminate": denied, "GroupNotClosed": type("GroupNotClosed", (RuntimeError,), {}),
            "subprocess": SimpleNamespace(run=denied, Popen=denied), **assignments,
        }
        exec(compile(ast.Module(body=functions, type_ignores=[]), "archived_cmd_run", "exec"), env)
        for name in ("distinct", "exact_alias"):
            out = tmp / (name + ".csv")
            calls = out if name == "exact_alias" else tmp / (name + ".jsonl")
            args = SimpleNamespace(data=str(tmp), build="NOT_USED", k="5", threads=1,
                                   timeout=None, budget=None, only="", out=str(out), calls=str(calls))
            console = io.StringIO()
            with contextlib.redirect_stdout(console):
                code = env["cmd_run"](args)
            require(type(code) is int and code == 0, "AST cmd_run code")
            csv_valid, journal_valid = judge_files(out.read_bytes(), calls.read_bytes())
            require(csv_valid is (name == "distinct") and journal_valid is (name == "distinct"),
                    "AST control/alias outcome")
            require(out.read_bytes() == (root / (name + ".csv")).read_bytes(), "exact CSV/alias bytes")
            if name == "distinct":
                require(calls.read_bytes() == (root / "distinct.jsonl").read_bytes(), "exact journal bytes")
            observed[name] = dict(code=code, csv_valid=csv_valid, journal_valid=journal_valid,
                                  runner_stdout=console.getvalue(), file_sha256=sha(out))
        require(measure_calls == [(5, 1), (5, 1)], "stub measure calls/non-vacuity")
    return observed


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--archive", default=ORIGINAL)
    root = Path(ap.parse_args().archive).resolve()
    before = inventory(root)  # Entire original inventory before ANY receipt/source parse.
    receipt = json.loads((root / "receipt.json").read_text())
    require(receipt["status"] == "OUTPUT_ALIAS_COUNTEREXAMPLE" and receipt["source"] == LIVE
            and receipt["source_before"] == SOURCE == receipt["source_after"], "original source pins")
    require(receipt["cmd_run_invocations"] == 4 and receipt["native_executions"] == 0
            and receipt["real_measurements"] == 0 and receipt["GCP_used"] is False, "original scope")
    require(receipt["control"] == {"returncode": 0, "csv_valid": True, "journal_valid": True}
            and receipt["exact_alias"] == {"returncode": 0, "csv_valid": False, "journal_valid": False},
            "original outcome")
    require(len(receipt["commands"]) == 2, "original commands inventory")
    outputs = []
    for i, command in enumerate(receipt["commands"]):
        mode = "optimized" if i else "normal"
        require(command["argv"] == ["python3", "-B"] + (["-O"] if i else []) + ["probe.py"]
                and type(command["returncode"]) is int and command["returncode"] == 0
                and command["stdout"] == mode + ".stdout" and command["stderr"] == mode + ".stderr"
                and (root / command["stderr"]).read_bytes() == b"", "original command/log linkage")
        data = json.loads((root / command["stdout"]).read_text())
        require(type(data["optimize"]) is int and data["optimize"] == i
                and data["status"] == receipt["status"] and data["source"] == LIVE
                and data["source_before"] == SOURCE == data["source_after"]
                and data["native_executions"] == 0 and data["real_measurements"] == 0, "original capture scope")
        outputs.append(data["results"])
    require(outputs[0] == outputs[1], "original normal/O semantic equivalence")
    for name in ("distinct", "exact_alias"):
        csv_bytes = (root / (name + ".csv")).read_bytes()
        journal_bytes = csv_bytes if name == "exact_alias" else (root / "distinct.jsonl").read_bytes()
        csv_valid, journal_valid = judge_files(csv_bytes, journal_bytes)
        result = outputs[0][name]
        require(type(result["code"]) is int and result["code"] == 0
                and result["csv_valid"] is csv_valid and result["journal_valid"] is journal_valid
                and result["file_sha256"] == hashlib.sha256(csv_bytes).hexdigest(), "captured exports")
    require(replay(root, (root / "scale_run_snapshot.py").read_text()) == outputs[0], "AST exact replay")
    require(inventory(root) == before, "original archive changed during read")
    print(json.dumps({
        "status": "ARCHIVE_ALIAS_AST_PASS", "original_files": len(FILES), "original_manifest_sha256": MANIFEST,
        "source_sha256": SOURCE, "AST_cmd_run_calls": 2, "simulated_measure_calls": 2,
        "native_calls": 0, "subprocess_calls": 0, "GCP_used": False,
        "scope": "archived cmd_run exact-name output collision; no performance or general inode proof",
        "original_acquisition_utc": "not recorded; not inferred",
    }, sort_keys=True))


if __name__ == "__main__":
    main()
