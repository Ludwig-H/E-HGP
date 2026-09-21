#!/usr/bin/env python3
"""Close constructor34 r2 readers, preserving r1 failures and source authority."""
import hashlib
import json
import os
from pathlib import Path
import sys
import tarfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(ROOT / "morsehgp3D_v8/bench"))
import run_q4_seed_cells_checks as checks
from run_q4_family_checks import digest, pins, read_json
from run_p0_matrix import invoke, parse_result, require, utc_stamp, write_json

CAPTURES = (
    ("qualification_r2/smoke_ewedfs4y", "smoke", "qualification/smoke_o0m_lvjj"),
    ("qualification_sanitize_r2/smoke_lqs4dx7b", "smoke", "qualification_sanitize/smoke_i7ezt4gx"),
    ("disabled_scalar_r2/lidar_is050o4p", "lidar", "disabled_scalar/lidar_qp4ptdxk"),
    ("regression_r2/regression_hcudjmag", "regression", "regression/regression_s74hd4yw"),
)


def main():
    output = HERE / "CANDIDATE_READBACK.json"
    require(not output.exists(), "candidate readback already exists")
    inputs = {str(Path(__file__).resolve().relative_to(ROOT))}
    sources, artifacts, commands, rows, captures, comparisons = pins(checks.SOURCES), {}, [], [], [], []
    previous_sources = None
    expected_changes = {"morsehgp3D_v8/bench/run_q4_seed_cells_checks.py", "morsehgp3D_v8/tests/q4_seed_cells_gate.cpp"}
    def add_artifacts(values):
        for name, value in values.items():
            require(name not in artifacts or artifacts[name] == value, "conflicting artifact snapshots")
            artifacts[name] = value
    for name, kind, old_name in CAPTURES:
        path, old_path = HERE / name, HERE / old_name
        manifest, completion = read_json(path / "MANIFEST.json"), read_json(path / "COMPLETION.json")
        old_manifest, old_completion = read_json(old_path / "MANIFEST.json"), read_json(old_path / "COMPLETION.json")
        for directory, m, c in ((path, manifest, completion), (old_path, old_manifest, old_completion)):
            require(c["status"] == "passed" and c["error"] is None and c["closing_errors"] == [] and
                    c["manifest_sha256"] == digest(directory / "MANIFEST.json") and
                    m["source_sha256"] == c["source_sha256_after"] and
                    m["artifact_sha256"] == c["artifact_sha256_after"], "native capture is not closed")
            inputs.update(str(p.relative_to(ROOT)) for p in directory.rglob("*") if p.is_file())
            inputs.update(m["input_sha256"])
            if m.get("ctest"): inputs.add(m["ctest"])
            add_artifacts(m["artifact_sha256"])
        require(manifest["source_sha256"] == sources, "r2 sources differ from final reader")
        require(set(old_manifest["source_sha256"]) == set(sources), "r1 source inventory differs")
        changed = {p for p, h in sources.items() if old_manifest["source_sha256"][p] != h}
        require(changed == expected_changes, "more than the announced gate and reader changed")
        require(previous_sources is None or previous_sources == old_manifest["source_sha256"], "r1 source snapshots disagree")
        previous_sources = old_manifest["source_sha256"]
        old_probes = {Path(p).name: dict(path=p, sha256=h) for p, h in old_manifest["artifact_sha256"].items()
                      if Path(p).name in ("mhgp8_wspd_q34_probe", "libmhgp8_p0.a")}
        new_probes = {Path(p).name: dict(path=p, sha256=h) for p, h in manifest["artifact_sha256"].items()
                      if Path(p).name in ("mhgp8_wspd_q34_probe", "libmhgp8_p0.a")}
        require(old_probes.keys() == new_probes.keys(), "r1/r2 probe inventory differs")
        for executable in old_probes:
            comparisons.append(dict(capture=name, artifact=executable, old=old_probes[executable], new=new_probes[executable],
                byte_identical=old_probes[executable]["sha256"] == new_probes[executable]["sha256"]))
        if kind != "regression":
            current = [read_json(p)["row"] for p in sorted(path.glob("record_*.json")) if read_json(p)["kind"] == "probe"]
            previous = [read_json(p)["row"] for p in sorted(old_path.glob("record_*.json")) if read_json(p)["kind"] == "probe"]
            require(len(current) == len(previous), "r1/r2 native measurement counts differ")
            # Timing and assignment of work to workers remain actual separate
            # observations. Compare geometry and complete payloads, not clocks.
            checks.paired([*previous, *current])
            rows.extend(current)
        reader = str(ROOT / "morsehgp3D_v8/bench/run_q4_seed_cells_checks.py")
        commands.append([reader, "read", str(path), "--check-live", "--compact"])
        if kind == "smoke": commands.append([reader, "selftest", str(path)])
        captures.append(dict(path=name, kind=kind, previous=old_name,
            manifest_sha256=digest(path / "MANIFEST.json"), completion_sha256=digest(path / "COMPLETION.json")))
    require(len(rows) == 51, "r2 small measurement count")
    checks.paired(rows)
    san = read_json(HERE / CAPTURES[1][0] / "MANIFEST.json")
    require(san["sanitizer_environment"]["ASAN_OPTIONS"] == "detect_leaks=1:halt_on_error=1" and
            san["sanitizer_environment"]["UBSAN_OPTIONS"] == "halt_on_error=1:print_stacktrace=1" and
            "MHGP8_SANITIZE:BOOL=ON" in san["compiler_cache"], "r2 leak instrumentation missing")
    archive = HERE / "preflight/SOURCES_R1.tar.gz"
    with tarfile.open(archive, "r:gz") as source_archive:
        members = source_archive.getmembers()
        require(len(members) == len(previous_sources) and {m.name for m in members} == set(previous_sources), "r1 archive inventory")
        for member in members:
            require(member.isfile(), "r1 archive contains non-file")
            stream = source_archive.extractfile(member)
            require(stream is not None and hashlib.sha256(stream.read()).hexdigest() == previous_sources[member.name],
                    "r1 archived source differs from captured bytes")
    history = [archive, HERE / "preflight/SELFTEST_R1_FAILURE.json", HERE / "preflight/READER_AFTER_MODULO_FIX.json",
               HERE / "MUTANTS_R2_READBACK.json"]
    for file in history: inputs.add(str(file.relative_to(ROOT)))
    failed_mutation = HERE / "preflight/mutations/compiled_9pi54doq"
    inputs.update(str(p.relative_to(ROOT)) for p in failed_mutation.rglob("*") if p.is_file())
    require(read_json(failed_mutation / "COMPLETION.json")["status"] == "failed" and
            read_json(HERE / "preflight/SELFTEST_R1_FAILURE.json")["status"] == "failed" and
            read_json(HERE / "MUTANTS_R2_READBACK.json")["status"] == "passed", "r1/r2 failure history was overwritten")
    require(pins(artifacts) == artifacts, "r1/r2 pinned artifacts changed")
    before, records, results, error, started = pins(inputs), [], [], None, utc_stamp()
    try:
        for optimized in (False, True):
            values = []
            for tail in commands:
                command = [sys.executable, "-B", *(["-O"] if optimized else []), *tail]
                record = dict(command=command, cwd=str(ROOT), started_utc=utc_stamp(), status="failed", exit_code=None,
                    stdout="", stderr="", stdout_base64="", stderr_base64="")
                try:
                    invoke(command, dict(os.environ), ROOT, record, new_session=True)
                    require(record["exit_code"] == 0 and not record["stderr"], "r2 candidate reader failed")
                    value = parse_result(record["stdout"].encode())
                    require(value["status"] == "passed", "candidate reader verdict")
                    values.append(value);record["status"] = "passed"
                finally:
                    record["finished_utc"] = utc_stamp();records.append(record)
            results.append(values)
        require(results[0] == results[1], "normal/-O readers differ")
    except BaseException as cause:
        error = f"{type(cause).__name__}: {cause}"
    after_sources, after_inputs, after_artifacts = pins(checks.SOURCES), pins(inputs), pins(artifacts)
    errors = []
    if sources != after_sources: errors.append("sources changed")
    if before != after_inputs: errors.append("inputs changed")
    if artifacts != after_artifacts: errors.append("artifacts changed")
    result = dict(schema="mhgp8_q4_seed_cells_candidate_readback_v1", status="passed" if error is None and not errors else "failed",
        error=error, started_utc=started, finished_utc=utc_stamp(), commands=len(records), records=records, results=results,
        native_reexecutions=0, source_count=len(sources), small_measurements=51, release_ctests=96,
        sanitizer_native_gates=4, sanitizer_probe_measurements=24, disabled_scalar_probes=3,
        reader_corruptions_per_selftest=145, normal_optimized_identical=len(results) == 2 and results[0] == results[1],
        captures=captures, r1_promoted_to_r2=False, changed_source_files=sorted(expected_changes),
        cpp_engine_and_probe_sources_unchanged=True, archived_r1_sources_verified=len(previous_sources),
        r1_archive_sha256=digest(archive), r1_r2_native_artifact_comparisons=comparisons,
        r1_r2_geometry_and_complete_payloads_equal=True, source_sha256=sources, source_sha256_after=after_sources,
        input_sha256=before, input_sha256_after=after_inputs, artifact_sha256=artifacts, artifact_sha256_after=after_artifacts,
        closing_errors=errors, full_contract_qualified=False, universal_subquadratic_claim=False, gcp_used=False)
    write_json(output, result)
    print(json.dumps(dict(path=str(output), status=result["status"], error=error, commands=len(records),
        sources=len(sources), inputs=len(before), artifacts=len(artifacts), closing_errors=errors)))
    require(result["status"] == "passed", "r2 candidate closure failed")


if __name__ == "__main__":
    main()
