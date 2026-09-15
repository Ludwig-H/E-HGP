#!/usr/bin/env python3
"""Close normal/-O readers and analysis over the explicitly named captures."""
import json
import os
from pathlib import Path
import signal
import sys
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(ROOT / "morsehgp3D_v8/bench"))
from run_wspd_q2_ranges_checks import digest, require, source_pins, write_json, strict_json
from run_p0_matrix import invoke, on_signal, utc_stamp


def main():
    paths = [Path(arg).resolve() for arg in sys.argv[1:]]
    require(paths and len(paths) == len(set(paths)) and all(p.parent == HERE for p in paths),
            "explicit distinct capture paths from this receipt directory required")
    inputs = [p for capture in paths for p in capture.rglob("*") if p.is_file()]
    inputs.extend([HERE / "analyze.py", Path(__file__).resolve()])
    pins = {str(p.relative_to(ROOT)): digest(p) for p in inputs}
    sources = source_pins()
    artifacts = {}
    for capture in paths:
        manifest = strict_json((capture / "MANIFEST.json").read_text())
        require(manifest["source_sha256"] == sources, "capture does not pin the final sources")
        for name, pin in manifest["artifact_sha256"].items():
            require(name not in artifacts or artifacts[name] == pin, "same artifact changed between captures")
            artifacts[name] = pin
    require(all(digest(ROOT / name) == pin for name, pin in artifacts.items()), "captured binary/cache changed")
    output = Path(tempfile.mkdtemp(prefix="analysis_", dir=HERE))
    previous = {sig: signal.signal(sig, on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}
    records, summaries = [], []
    status, error = "failed", None
    try:
        scripts = [[str(ROOT / "morsehgp3D_v8/bench/run_wspd_q2_ranges_checks.py"), "read", str(path)]
                   for path in paths]
        scripts.append([str(HERE / "analyze.py"), *map(str, paths)])
        for optimized in (False, True):
            parsed = []
            for number, script in enumerate(scripts):
                command = [sys.executable, "-B", *(["-O"] if optimized else []), *script]
                record = dict(command=command, cwd=str(ROOT), started_utc=utc_stamp(), status="failed", exit_code=None,
                              stdout="", stderr="", stdout_base64="", stderr_base64="")
                try:
                    invoke(command, dict(os.environ), ROOT, record, new_session=True)
                    require(record["exit_code"] == 0, "reader/analysis failed")
                    summary = strict_json(record["stdout"])
                    require(summary["status"] == "passed", "reader/analysis did not pass")
                    parsed.append(summary)
                    record["status"] = "passed"
                finally:
                    record["finished_utc"] = utc_stamp()
                    target = output / f"{'optimized' if optimized else 'normal'}_{number}.json"
                    write_json(target, record)
                    records.append({"path": target.name, "sha256": digest(target)})
            summaries.append(parsed)
        require(summaries[0] == summaries[1], "normal/-O analyses differ")
        require(sources == source_pins() and all(digest(ROOT / name) == pin for name, pin in pins.items()) and
                all(digest(ROOT / name) == pin for name, pin in artifacts.items()), "analysis inputs changed")
        write_json(output / "SUMMARY.json", summaries[0][-1])
        status = "passed"
    except BaseException as cause:
        error = f"{type(cause).__name__}: {cause}"
        raise
    finally:
        write_json(output / "COMPLETION.json", dict(status=status, error=error, finished_utc=utc_stamp(),
            input_sha256=pins, source_sha256=sources, artifact_sha256=artifacts, records=records,
            summary_sha256=digest(output / "SUMMARY.json") if (output / "SUMMARY.json").is_file() else None,
            input_sha256_after={name: digest(ROOT / name) for name in pins},
            artifact_sha256_after={name: digest(ROOT / name) for name in artifacts},
            source_sha256_after=source_pins()))
        for sig, handler in previous.items():
            signal.signal(sig, handler)
        print(json.dumps(dict(path=str(output), status=status, error=error)))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
