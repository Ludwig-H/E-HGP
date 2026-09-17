#!/usr/bin/env python3
"""Close normal/-O reads and analysis for explicit final/historical/failed captures.

Port of tranche18's recorder. Historical PASS rows remain tied to their own
source and artifact pins, are labelled in analysis, and are excluded from
the final speed aggregate. FAILED attempts are input evidence only.
"""
import argparse
import json
import os
from pathlib import Path
import signal
import sys
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(ROOT / "morsehgp3D_v8/bench"))
from run_wspd_q2_batched_checks import digest, require, source_pins, write_json, strict_json
from run_p0_matrix import invoke, on_signal, utc_stamp
import analyze as analysis


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("captures", type=Path, nargs="+")
    parser.add_argument("--failed", type=Path, action="append", default=[])
    parser.add_argument("--historical", type=Path, action="append", default=[])
    parser.add_argument("--archive", type=Path, action="append", default=[])
    args = parser.parse_args()
    paths = [p.resolve() for p in args.captures]
    failures = [p.resolve() for p in args.failed]
    historical = [p.resolve() for p in args.historical]
    archives = [p.resolve() for p in args.archive]
    all_captures = [*paths, *historical, *failures]
    require(len(all_captures) == len(set(all_captures)) and
            all(p.parent == HERE for p in all_captures) and
            all(p.is_relative_to(HERE) and p != HERE and p.exists() for p in archives),
            "explicit distinct capture paths and scoped archives required")
    inputs = [p for capture in all_captures for p in capture.rglob("*") if p.is_file()]
    for archive in archives:
        inputs.extend([archive] if archive.is_file() else [p for p in archive.rglob("*") if p.is_file()])
    inputs.extend([HERE / "analyze.py", HERE / "record_quantums.py", Path(__file__).resolve()])
    pins = {str(p.relative_to(ROOT)): digest(p) for p in inputs}
    sources = source_pins()
    archive_files = [p for archive in archives for p in
                     ([archive] if archive.is_file() else archive.rglob("*")) if p.is_file()]
    archive_pins = {str(p.relative_to(ROOT)): digest(p) for p in archive_files}
    artifacts = {}
    provenance = []
    for capture in (*paths, *historical):
        manifest = strict_json((capture / "MANIFEST.json").read_text())
        is_historical = capture in historical
        if not is_historical:
            require(manifest["source_sha256"] == sources, "final capture does not pin the final sources")
            if "helper_path" in manifest:
                require(digest(ROOT / manifest["helper_path"]) == manifest["helper_sha256"],
                        "final helper no longer matches its capture")
        source_archives = {}
        if is_historical:
            for name, pin in manifest["source_sha256"].items():
                matches = [path for path, archived_pin in archive_pins.items()
                           if archived_pin == pin and Path(path).name == Path(name).name]
                if matches:
                    source_archives[name] = sorted(matches)
                require(sources.get(name) == pin or matches,
                        "historical changed source is not preserved by an authenticated archive: " + name)
        provenance.append(dict(path=str(capture.relative_to(ROOT)), historical=is_historical,
                               source_sha256=manifest["source_sha256"],
                               authenticated_source_archives=source_archives,
                               artifact_sha256=manifest["artifact_sha256"]))
        for name, pin in manifest["artifact_sha256"].items():
            require(name not in artifacts or artifacts[name] == pin,
                    "same artifact path changed between passed captures: distinct pinned builds required")
            artifacts[name] = pin
    for capture in failures:
        analysis.failed_summary(capture)
    require(all(digest(ROOT / name) == pin for name, pin in artifacts.items()), "captured binary/cache changed")
    output = Path(tempfile.mkdtemp(prefix="analysis_", dir=HERE))
    previous = {sig: signal.signal(sig, on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}
    records, summaries = [], []
    status, error = "failed", None
    try:
        scripts = []
        for capture in (*paths, *historical):
            manifest = strict_json((capture / "MANIFEST.json").read_text())
            script = (HERE / "record_quantums.py" if manifest["schema"] == analysis.quantums.SCHEMA
                      else ROOT / "morsehgp3D_v8/bench/run_wspd_q2_batched_checks.py")
            scripts.append([str(script), "read", str(capture)])
        analyze_command = [str(HERE / "analyze.py"), *map(str, paths)]
        for flag, selected in (("--failed", failures), ("--historical", historical)):
            for path in selected:
                analyze_command.extend([flag, str(path)])
        scripts.append(analyze_command)
        for optimized in (False, True):
            parsed = []
            for number, script in enumerate(scripts):
                command = [sys.executable, "-B", *(["-O"] if optimized else []), *script]
                record = dict(command=command, cwd=str(ROOT), started_utc=utc_stamp(), status="failed",
                              exit_code=None, stdout="", stderr="", stdout_base64="", stderr_base64="")
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
                    records.append(dict(path=target.name, sha256=digest(target)))
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
            input_sha256=pins, source_sha256=sources, artifact_sha256=artifacts,
            capture_provenance=provenance, records=records,
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
