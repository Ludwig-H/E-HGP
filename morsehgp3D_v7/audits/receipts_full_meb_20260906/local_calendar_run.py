"""Compile and judge the exact product calendar on captured audit sources only."""
from __future__ import annotations

import argparse
import gzip
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
import full_meb_run as replay  # noqa: E402


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("build", "run", "judge"))
    parser.add_argument("--name", choices=("O2", "sanitized", "q4_first"), default="O2")
    args = parser.parse_args()
    name = "calendar_" + args.name
    binary = replay.W / (name + ".bin")
    if args.action == "build":
        source = HERE / "source"
        if args.name == "q4_first":
            mutation = json.loads((HERE / "local_calendar_q4_first.json").read_text())
            original = HERE / "source" / mutation["file"]
            replay.require(replay.sha(original) == mutation["source_sha256"], "mutation source pin")
            data = original.read_text()
            replay.require(data.count(mutation["before"]) == 1, "unique calendar mutation")
            source = HERE / "q4_first/source"
            target = source / mutation["file"]
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text(data.replace(mutation["before"], mutation["after"]))
            replay.require(replay.sha(target) == mutation["mutated_sha256"], "exact calendar mutant")
        flags = ["-O1", "-g0", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer"] if args.name == "sanitized" else ["-O2"]
        dep = HERE / (name + ".d")
        replay.execute(["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", *flags,
                        "-MMD", "-MF", dep, *replay.include_flags(source), HERE / "local_calendar.cpp",
                        "-o", binary], name + "_build")
        dependencies = {str(Path(p).resolve().relative_to(replay.ROOT)): replay.sha(Path(p))
                        for p in dep.read_text().replace("\\\n", " ").split(":", 1)[1].split()}
        replay.require(all(p.startswith("morsehgp3D_v7/audits/") for p in dependencies), "live product dependency")
        replay.write(HERE / (name + "_binary.json"), {"sha256": replay.sha(binary), "dependencies": dependencies,
                     "scope": "Local current-product dispatcher; no FULL wrapper execution in this bridge."})
    elif args.action == "run":
        replay.require(replay.sha(binary) == json.loads((HERE / (name + "_binary.json")).read_text())["sha256"], "binary binding")
        env = {"ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1", "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"} if args.name == "sanitized" else {}
        replay.execute([binary], name + "_run", env=env)
    else:
        receipt = json.loads((HERE / (name + "_run.json")).read_text())
        stream = HERE / (name + "_run.stdout.gz")
        replay.require(receipt["exit_code"] == 0 and replay.sha(stream) == receipt["stdout_gzip_sha256"] and
                       not (HERE / (name + "_run.stderr")).read_bytes(), "calendar transport")
        raw = gzip.decompress(stream.read_bytes())
        import hashlib
        replay.require(hashlib.sha256(raw).hexdigest() == receipt["stdout_sha256"], "calendar raw hash")
        # Temporary expanded bytes stay under the disposable audit build directory.
        expanded = replay.W / (name + ".jsonl")
        expanded.write_bytes(raw)
        mode = "optimized" if sys.flags.optimize else "normal"
        options = ["-O"] if sys.flags.optimize else []
        argv = [sys.executable, "-B", *options, HERE / "local_calendar_judge.py", expanded]
        if args.name == "q4_first":
            argv.append("--expect-q4-first")
        replay.execute(argv, name + "_" + mode)
        expanded.unlink()


if __name__ == "__main__":
    main()
