#!/usr/bin/env python3
"""Separate TSan attempt; runtime startup failures are retained, not passes."""
import argparse
import importlib.util
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import time

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("parallel_capture", HERE / "run.py")
base = importlib.util.module_from_spec(spec)
spec.loader.exec_module(base)
need, sha = base.need, base.sha
OUT = base.V9 / "receipts/full_parallel_parent_20260927/tsan_r1"
BUILD = Path("/workspaces/E-HGP/build/v9-audit-full-parallel-parent-tsan-20260927-r1")
ENV = {"TSAN_OPTIONS": "halt_on_error=1:exitcode=66"}


def pins():
    return base.pins() | {str(HERE / "tsan.py"): sha(HERE / "tsan.py")}


def recipe():
    cc = shutil.which("g++")
    need(cc is not None, "g++ required")
    common = ["-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-pthread", "-I", str(base.V9 / "src")]
    source = str(HERE / "gate.cpp")
    return [("version", [cc, "--version"], {}),
            ("dependencies", [cc, *common, "-MM", "-MT", "gate", source], {}),
            ("compile", [cc, *common, "-O1", "-g1", "-fsanitize=thread", "-fno-omit-frame-pointer", source, "-latomic", "-o", str(BUILD / "gate")], {}),
            ("gate", [str(BUILD / "gate")], ENV)]


def dump(path, obj):
    base.base.dump(path, obj)


def capture():
    base.check(base.RECEIPT)
    need(not OUT.exists() and not BUILD.exists(), "fresh TSan capture/build only")
    OUT.mkdir(parents=True)
    BUILD.mkdir(parents=True)
    state = dict(schema="mhgp9_full_parallel_parent_tsan_v1", status="running", GCP_used=False,
                 build=str(BUILD), source_before=pins(), commands=[])
    dump(OUT / "capture.json", state)
    try:
        for name, argv, env in recipe():
            row = dict(name=name, argv=argv, cwd=str(base.V9.parent), extra_env=env, status="running")
            state["commands"].append(row)
            dump(OUT / "capture.json", state)
            start = time.monotonic()
            with (OUT / (name + ".stdout")).open("wb") as stdout, (OUT / (name + ".stderr")).open("wb") as stderr:
                proc = subprocess.run(argv, cwd=base.V9.parent, env=os.environ | env, stdout=stdout, stderr=stderr, check=False)
            row.update(returncode=proc.returncode, elapsed_s=time.monotonic() - start, status="completed",
                       stdout_sha256=sha(OUT / (name + ".stdout")), stderr_sha256=sha(OUT / (name + ".stderr")))
            dump(OUT / "capture.json", state)
            if name != "gate":
                need(proc.returncode == 0 and not (OUT / (name + ".stderr")).read_bytes(), "TSan build command")
        state["source_after"] = pins()
        need(state["source_before"] == state["source_after"], "source drift")
        state["binary"] = {str(BUILD / "gate"): sha(BUILD / "gate")}
        last = state["commands"][-1]
        err = (OUT / "gate.stderr").read_text()
        out = (OUT / "gate.stdout").read_text()
        if last["returncode"] == 0 and not err:
            need(json.loads(out) == json.loads((base.RECEIPT / "release_gate.stdout").read_text()), "TSan semantic output")
            state["status"] = "passed"
        elif last["returncode"] == 66 and not out and err.startswith("FATAL: ThreadSanitizer: unexpected memory mapping "):
            state["status"] = "runtime_unavailable"
        else:
            raise RuntimeError("TSan gate failed; not classified as runtime mapping failure")
    except BaseException as error:
        state["status"] = "failed"
        state["error"] = type(error).__name__ + ": " + str(error)
        raise
    finally:
        dump(OUT / "capture.json", state)
    return check()


def check():
    base.check(base.RECEIPT)
    d = json.loads((OUT / "capture.json").read_text())
    need(d["schema"] == "mhgp9_full_parallel_parent_tsan_v1" and d["status"] in ("passed", "runtime_unavailable") and d["GCP_used"] is False, "closed TSan attempt")
    need(d["source_before"] == d["source_after"] == pins() and d["build"] == str(BUILD), "LIVE sources/build")
    need(d["binary"] == {str(BUILD / "gate"): sha(BUILD / "gate")}, "LIVE binary")
    need(len(d["commands"]) == 4, "recipe inventory")
    for row, (name, argv, env) in zip(d["commands"], recipe()):
        need(row["name"] == name and row["argv"] == argv and row["extra_env"] == env and row["cwd"] == str(base.V9.parent) and row["status"] == "completed", "command binding")
        for stream in ("stdout", "stderr"):
            need(sha(OUT / (name + "." + stream)) == row[stream + "_sha256"], "output hash")
        if name != "gate":
            need(row["returncode"] == 0 and not (OUT / (name + ".stderr")).read_bytes(), "TSan build command")
    words = shlex.split((OUT / "dependencies.stdout").read_text().replace("\\\n", " "))
    need(words[0] == "gate:" and all(str(Path(w).resolve()) in pins() for w in words[1:]), "compiled dependencies")
    out, err = (OUT / "gate.stdout").read_text(), (OUT / "gate.stderr").read_text()
    rc = d["commands"][-1]["returncode"]
    if d["status"] == "passed":
        need(rc == 0 and not err and json.loads(out) == json.loads((base.RECEIPT / "release_gate.stdout").read_text()), "TSan PASS")
    else:
        need(rc == 66 and not out and err.startswith("FATAL: ThreadSanitizer: unexpected memory mapping "), "runtime unavailable, never PASS")
    return dict(status=d["status"], commands=4, TSan_qualified=d["status"] == "passed", GCP_used=False)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("mode", choices=("capture", "check"))
    args = parser.parse_args()
    print(json.dumps(capture() if args.mode == "capture" else check(), sort_keys=True))
