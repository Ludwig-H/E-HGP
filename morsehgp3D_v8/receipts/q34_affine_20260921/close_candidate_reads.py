#!/usr/bin/env python3
"""Read the four final tranche33 qualifications; run no native command."""
import json
import os
from pathlib import Path
import signal
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(ROOT / "morsehgp3D_v8/bench"))
import run_q34_affine_checks as checks
from run_q4_family_checks import digest, pins, read_json
from run_p0_matrix import invoke, on_signal, parse_result, require, utc_stamp, write_json

CAPTURES = (("candidate/smoke_jeqq5w9w", "smoke"), ("candidate/smoke_nm_g9o7b", "smoke"),
            ("candidate/mutations/compiled_1cp9ywhr", "mutation"), ("candidate/regression_gxkxxjxq", "regression"))
PREFLIGHTS = ("preflight/smoke_8dhs98j4", "preflight/smoke_284ctuzk", "preflight/smoke_2j1ieuq1",
             "preflight/mutations/compiled_ztqh7g1n")


def main():
    output = HERE / "CANDIDATE_READBACK.json"
    require(not output.exists(), "readback already exists")
    inputs = {str(Path(__file__).resolve().relative_to(ROOT))}
    sources = pins(checks.SOURCES)
    artifacts, captured, commands, rows = {}, [], [], []
    def add_artifacts(values):
        for name, value in values.items():
            require(name not in artifacts or artifacts[name] == value, "conflicting captured artifact")
            artifacts[name] = value
    for relative, kind in CAPTURES:
        path = HERE / relative
        manifest, completion = read_json(path / "MANIFEST.json"), read_json(path / "COMPLETION.json")
        require(completion["status"] == "passed" and completion["error"] is None and completion["closing_errors"] == [] and
                completion["manifest_sha256"] == digest(path / "MANIFEST.json"), "qualification not closed")
        require(manifest["source_sha256"] == sources == completion["source_sha256_after"] and
                manifest["artifact_sha256"] == completion["artifact_sha256_after"], "qualification source/artifact closure")
        inputs.update(str(p.relative_to(ROOT)) for p in path.rglob("*") if p.is_file())
        inputs.update(manifest.get("input_sha256", {}))
        add_artifacts(manifest["artifact_sha256"])
        if kind == "mutation":
            inputs.add(manifest["compiler"])
            require(digest(Path(manifest["compiler"])) == manifest["compiler_sha256"] == completion["compiler_sha256_after"],
                    "mutation compiler changed")
            for evidence in completion["evidence"]:
                add_artifacts({evidence[field]: evidence[field + "_sha256"] for field in ("binary", "object")})
        if manifest.get("ctest"):
            inputs.add(manifest["ctest"])
        if kind == "smoke":
            require(manifest["config"]["qualification"] == "candidate", "preflight presented as final smoke")
            for file in sorted(path.glob("record_*.json")):
                item = read_json(file)
                if item["kind"] == "probe":
                    rows.append(item["row"])
        reader = ROOT / ("morsehgp3D_v8/tests/q34_affine_mutations.py" if kind == "mutation"
                         else "morsehgp3D_v8/bench/run_q34_affine_checks.py")
        commands.append([str(reader), "read", str(path), "--check-live", *([] if kind == "mutation" else ["--compact"])])
        if kind == "smoke":
            commands.append([str(reader), "selftest", str(path)])
        captured.append(dict(path=relative, kind=kind, manifest_sha256=digest(path / "MANIFEST.json"),
                             completion_sha256=digest(path / "COMPLETION.json")))
    sanitizer = read_json(HERE / CAPTURES[1][0] / "MANIFEST.json")
    require(sanitizer["sanitizer_environment"]["ASAN_OPTIONS"] == "detect_leaks=1:halt_on_error=1" and
            sanitizer["sanitizer_environment"]["UBSAN_OPTIONS"] == "halt_on_error=1:print_stacktrace=1" and
            "MHGP8_SANITIZE:BOOL=ON" in sanitizer["compiler_cache"], "sanitizer leak instrumentation not active")
    preflights = []
    for relative in PREFLIGHTS:
        path = HERE / relative
        manifest, completion = read_json(path / "MANIFEST.json"), read_json(path / "COMPLETION.json")
        expected = "failed" if relative.endswith("smoke_2j1ieuq1") else "passed"
        require(completion["status"] == expected and completion["closing_errors"] == [] and
                completion["manifest_sha256"] == digest(path / "MANIFEST.json") and
                manifest["source_sha256"] == sources == completion["source_sha256_after"] and
                manifest["artifact_sha256"] == completion["artifact_sha256_after"], "preflight history changed")
        for item in completion["records"]:
            require(digest(path / item["path"]) == item["sha256"], "preflight record changed")
        inputs.update(str(p.relative_to(ROOT)) for p in path.rglob("*") if p.is_file())
        add_artifacts(manifest["artifact_sha256"])
        preflights.append(dict(path=relative, status=expected, manifest_sha256=digest(path / "MANIFEST.json"),
                              completion_sha256=digest(path / "COMPLETION.json")))
    require(len(rows) == 48 and pins(artifacts) == artifacts, "native inputs/count changed")
    checks.paired(rows)
    files = pins(inputs)
    previous = {sig: signal.signal(sig, on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}
    records, summaries, error, status, started = [], [], None, "failed", utc_stamp()
    try:
        for optimized in (False, True):
            values = []
            for command in commands:
                argv = [sys.executable, "-B", *(["-O"] if optimized else []), *command]
                record = dict(command=argv, cwd=str(ROOT), started_utc=utc_stamp(), status="failed",
                    exit_code=None, stdout="", stderr="", stdout_base64="", stderr_base64="")
                try:
                    invoke(argv, dict(os.environ), ROOT, record, new_session=True)
                    require(record["exit_code"] == 0 and not record["stderr"], "candidate reader failed")
                    result = parse_result(record["stdout"].encode())
                    require(result["status"] == "passed", "reader result failed")
                    values.append(result)
                    record["status"] = "passed"
                finally:
                    record["finished_utc"] = utc_stamp()
                    records.append(record)
            summaries.append(values)
        require(summaries[0] == summaries[1], "normal/-O results differ")
        status = "passed"
    except BaseException as cause:
        error = f"{type(cause).__name__}: {cause}"
        raise
    finally:
        for sig in previous:
            signal.signal(sig, signal.SIG_IGN)
        errors = []
        def close(label, action):
            try:
                return action()
            except Exception as cause:
                errors.append(f"{label}: {type(cause).__name__}: {cause}")
                return None
        result = dict(schema="mhgp8_q34_affine_candidate_readback_v1", status=status, error=error,
            started_utc=started, finished_utc=utc_stamp(), commands=len(records), records=records,
            captures=captured, preserved_preflights=preflights, preflights_promoted=False,
            native_reexecutions=0, small_measurements=48, paired_full_records=status == "passed",
            release_ctests=95, sanitizer_native_gates=4, sanitizer_probe_measurements=24, compiled_product_mutants=3,
            normal_optimized_identical=len(summaries) == 2 and summaries[0] == summaries[1],
            results=summaries[0] if summaries else [], source_sha256=sources,
            source_sha256_after=close("sources", lambda: pins(checks.SOURCES)), input_sha256=files,
            input_sha256_after=close("inputs", lambda: pins(inputs)), artifact_sha256=artifacts,
            artifact_sha256_after=close("artifacts", lambda: pins(artifacts)), closing_errors=errors,
            full_contract_qualified=False, universal_subquadratic_claim=False, gcp_used=False)
        if errors or any(result[k] != result[k + "_after"] for k in ("source_sha256", "input_sha256", "artifact_sha256")):
            result.update(status="failed", error=error or "readback closure changed")
        write_json(output, result)
        for sig, handler in previous.items():
            signal.signal(sig, handler)
        print(json.dumps(dict(path=str(output), status=result["status"], error=result["error"], commands=len(records),
            sources=len(sources), inputs=len(files), artifacts=len(artifacts))))
    require(result["status"] == "passed", "candidate closure failed")


if __name__ == "__main__":
    main()
