#!/usr/bin/env python3
"""OPEN recorder; launch only after root review. Does not execute the static reader."""
import argparse
import hashlib
import json
import os
import shlex
import signal
import subprocess
import tempfile
import time
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).absolute().parent
SNAP = ("src/head/head.cpp", "src/head/head.hpp", "src/points/dendrogram.cpp",
        "src/points/dendrogram.hpp", "src/core/status.hpp", "src/core/types.hpp", "src/core/reasons.def")
CODE = tuple("snapshot/" + p for p in SNAP) + ("mutant/head.cpp", "probe.cpp")
BASE = CODE + ("source_pins.json", "README.txt", "protocol.json", "record.py", "read.py")
GENERATED = ("capture.json", "source_close.json", "record_state.json")
PREP = "preparation.json"
CLOSED = "manifest.json"


def need(c, m):
    if not c:
        raise RuntimeError(m)


def sha(b):
    return hashlib.sha256(b).hexdigest()


def stamp():
    return datetime.now(timezone.utc).isoformat()


def inventory():
    need(ROOT.resolve() == ROOT, "archive path uses a symlink")
    out = set()
    for p in ROOT.rglob("*"):
        need(not p.is_symlink(), "symlink in preparation: " + str(p))
        if p.is_file():
            out.add(p.relative_to(ROOT).as_posix())
        else:
            need(p.is_dir(), "special entry in preparation")
    return out


def pins(names):
    return {n: sha((ROOT / n).read_bytes()) for n in names}


def save(name, value):
    p = ROOT / name
    tmp = ROOT / (name + ".tmp")
    with tmp.open("x", encoding="utf-8") as f:
        json.dump(value, f, sort_keys=True, indent=2, allow_nan=False)
        f.write("\n")
    os.replace(tmp, p)


def dep_names(stdout):
    out = set()
    for line in stdout.replace("\\\n", " ").splitlines():
        need(":" in line, "bad GNU dependency rule")
        for p in shlex.split(line.split(":", 1)[1]):
            q = Path(p)
            need(q.is_absolute() and q.resolve().is_relative_to(ROOT), "dependency outside snapshots")
            out.add(q.relative_to(ROOT).as_posix())
    return out


def component(path):
    p = Path(path).resolve(strict=True)
    return {"path": str(p), "sha256": sha(p.read_bytes()), "bytes": p.stat().st_size}


def text(x):
    return x.decode("utf-8", "replace") if isinstance(x, bytes) else (x or "")


def main():
    a = argparse.ArgumentParser()
    a.add_argument("--preparation-sha", required=True)
    args = a.parse_args()
    raw = (ROOT / PREP).read_bytes()
    need(sha(raw) == args.preparation_sha, "external preparation SHA mismatch")
    prep = json.loads(raw)
    need(prep["status"] == "source_only_review_pending", "preparation status")
    need(set(prep["files"]) == set(BASE), "preparation whitelist")
    need(inventory() == set(BASE) | {PREP}, "fresh output / exact preparation inventory")
    for n, v in prep["files"].items():
        b = (ROOT / n).read_bytes()
        need(sha(b) == v["sha256"] and len(b) == v["bytes"], "preparation pin: " + n)
    protocol = json.loads((ROOT / "protocol.json").read_text())
    runtime = Path(tempfile.mkdtemp(prefix="mhgp10-head-direct-exit-native-"))
    need(not list(runtime.iterdir()), "runtime not fresh")
    # Exclusive claim: neither an old capture nor a running capture is overwritten.
    with (ROOT / "record_state.json").open("x", encoding="utf-8") as f:
        json.dump({"status": "recording", "started_utc": stamp()}, f)
    capture = {"schema": "head_direct_exit_capture_v1", "source_root": str(ROOT),
               "runtime": str(runtime), "runtime_before": [], "first_failure": None,
               "steps": [], "checkpoints": []}
    close = {"schema": "head_direct_exit_source_close_v1", "project_before": pins(CODE)}
    env = dict(os.environ)
    env["LC_ALL"] = "C"
    stage = "initial"
    start = time.monotonic()

    def checkpoint(label):
        capture["checkpoints"].append({"label": label, "steps": len(capture["steps"]), "utc": stamp()})
        save("capture.json", capture)

    def run(label, argv, timeout, expected=0):
        nonlocal stage
        stage = label
        checkpoint("before_" + label)
        t0 = time.monotonic()
        proc = subprocess.Popen(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env, start_new_session=True)
        timed_out = False
        interruption = None
        try:
            stdout, stderr = proc.communicate(timeout=timeout)
        except subprocess.TimeoutExpired:
            timed_out = True
            try:
                os.killpg(proc.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            stdout, stderr = proc.communicate()
        except BaseException as exc:
            try:
                os.killpg(proc.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            stdout, stderr = proc.communicate()
            interruption = type(exc).__name__
        row = {"stage": label, "argv": argv, "timeout_s": timeout, "exit": proc.returncode,
               "signal": -proc.returncode if proc.returncode < 0 else None,
               "seconds": time.monotonic() - t0, "stdout": text(stdout), "stderr": text(stderr),
               "timed_out": timed_out, "interruption": interruption}
        capture["steps"].append(row)
        checkpoint("after_" + label)
        need(not timed_out and interruption is None and proc.returncode == expected,
             label + " unexpected exit/timeout/interruption")
        return row

    def interrupt(_sig, _frame):
        raise InterruptedError("recorder interrupted")

    signal.signal(signal.SIGTERM, interrupt)
    try:
        checkpoint("initial")
        compiler = protocol["compiler"]
        close["compiler_driver_before"] = component(compiler)
        run("compiler_version", [compiler, "--version"], 10)
        backend = run("compiler_backend", [compiler, "-print-prog-name=cc1plus"], 10)["stdout"].strip()
        close["compiler_backend_before"] = component(backend)
        flags = protocol["flags"]
        inc = "-I" + str(ROOT / "snapshot/src")
        close["dependencies"] = {}
        close["binaries"] = {}
        for case, head, expected in (("baseline", "snapshot/src/head/head.cpp", 0),
                                      ("mutant", "mutant/head.cpp", 1)):
            units = ["probe.cpp", head, "snapshot/src/points/dendrogram.cpp"]
            sources = [str(ROOT / n) for n in units]
            deps = dep_names(run("dependencies_" + case, [compiler] + flags + [inc, "-MM"] + sources, 10)["stdout"])
            wanted = set(CODE) - {"mutant/head.cpp" if case == "baseline" else "snapshot/src/head/head.cpp"}
            need(deps == wanted, "complete project dependency inventory " + case)
            before = pins(sorted(deps))
            binary = runtime / case
            need(not binary.exists(), "stale binary " + case)
            run("compile_" + case, [compiler] + flags + [inc] + sources + ["-o", str(binary)], 10)
            close["binaries"][case] = dict(component(binary), absent_before_compile=True)
            run("run_" + case, [str(binary)], 5, expected)
            stage = "dependencies_close_" + case
            after = pins(sorted(deps))
            need(before == after, "dependencies changed " + case)
            close["dependencies"][case] = {"before": before, "after": after}
        stage = "source_close"
        close["project_after"] = pins(CODE)
        close["compiler_driver_after"] = component(compiler)
        close["compiler_backend_after"] = component(backend)
        need(close["project_before"] == close["project_after"], "project changed")
        need(close["compiler_driver_before"] == close["compiler_driver_after"], "compiler driver changed")
        need(close["compiler_backend_before"] == close["compiler_backend_after"], "compiler backend changed")
        capture["runtime_after"] = sorted(p.name for p in runtime.iterdir())
        need(capture["runtime_after"] == ["baseline", "mutant"], "unexpected runtime output")
        save("source_close.json", close)
        checkpoint("complete")
        save("record_state.json", {"status": "recorded", "ended_utc": stamp(),
                                  "seconds": time.monotonic() - start, "native_compiles": 2, "native_runs": 2})
        names = sorted(set(BASE) | {PREP} | set(GENERATED))
        need(inventory() == set(names), "exact final payload inventory")
        stage = "manifest"
        manifest = {"schema": "head_direct_exit_closed_v1", "status": "recorded_static_reader_not_yet_run",
                    "files": {n: {"sha256": sha((ROOT / n).read_bytes()), "bytes": (ROOT / n).stat().st_size}
                              for n in names},
                    "scope": "TEXT-ONLY code/capture; no native reexecution by reader; no engine/FULL/G4 claim"}
        with (ROOT / CLOSED).open("x", encoding="utf-8") as f:
            json.dump(manifest, f, sort_keys=True, indent=2, allow_nan=False)
            f.write("\n")
        print(json.dumps({"manifest_sha256": sha((ROOT / CLOSED).read_bytes()),
                          "payload_count": len(names), "static_reader_executed": False}))
        return 0
    except BaseException as exc:
        if capture["first_failure"] is None:
            capture["first_failure"] = {"stage": stage, "type": type(exc).__name__, "message": str(exc), "utc": stamp()}
        save("capture.json", capture)
        close["project_after_failure"] = pins(CODE)
        save("source_close.json", close)
        save("record_state.json", {"status": "failed_open", "ended_utc": stamp(),
                                  "seconds": time.monotonic() - start})
        print(json.dumps({"status": "failed_open", "first_failure": capture["first_failure"]}))
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
