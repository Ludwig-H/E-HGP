"""Short independent countercheck of frozen benchmark fixes, not a cloud benchmark.

Usage: python3 [-O] record.py NEW_CAPTURE_DIR
Fixtures borrow the developer's dev-plan generator only; expectations and
mutations are checked here. Does not generate point clouds or rebuild C++.
"""
import copy
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import time

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
SOURCE = HERE / "source/morsehgp3D_v10"


def check(ok, message):
    if not ok:
        raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    check(len(sys.argv) == 2, "provide a NEW capture directory")
    dest = Path(sys.argv[1]).resolve()
    check(not dest.exists(), "must not overwrite a closed receipt")
    paths = sorted(p for p in SOURCE.rglob("*") if p.is_file()) + [Path(__file__).resolve()]
    before = {str(p): sha(p) for p in paths}
    dest.mkdir(parents=True)
    helper = SOURCE / "tests/regression/test_decide_completeness.py"
    spec = importlib.util.spec_from_file_location("fixture_generator_only", helper)
    fixture = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(fixture)
    specs, digest = fixture.oracle_plan(fixture.MINI_PLAN)
    prereg = dict(id="AUDIT_DEV_METADATA_ONLY", plan=dict(fixture.MINI_PLAN, manifest_sha256=digest),
                  methods=[dict(name=m) for m in fixture.METHODS],
                  decision=dict(alpha=0.05, delta_min=0.02, permutations=8, bootstrap=8, refusal_cap=0.01,
                                pairs=[dict(name="tour_hdb", method="tour", adversary="hdb")]))
    ppath = dest / "PREREG_DEV.json"
    ppath.write_text(json.dumps(prereg, indent=2, sort_keys=True) + "\n")
    info = dict(prereg=ppath.name, prereg_sha256=sha(ppath), plan_sha256=digest,
                scenes=len(specs), computed=len(specs), complete=True)
    rows = fixture.as_text(fixture.mini_rows(specs))
    check(len(specs) == 32 and len(rows) == 96, "fixture changed")
    records = []
    env = dict(os.environ, OPENBLAS_NUM_THREADS="1", OMP_NUM_THREADS="1")
    python = [sys.executable, "-B"] + (["-O"] if sys.flags.optimize else [])

    def run(name, argv, want):
        start = time.monotonic()
        result = subprocess.run(argv, capture_output=True, timeout=45, env=env, check=False)
        (dest / (name + ".stdout")).write_bytes(result.stdout)
        (dest / (name + ".stderr")).write_bytes(result.stderr)
        records.append(dict(name=name, argv=argv, exit_code=result.returncode,
                            wall_seconds=time.monotonic() - start,
                            stdout_sha256=sha(dest / (name + ".stdout")),
                            stderr_sha256=sha(dest / (name + ".stderr"))))
        check(result.returncode == want, name + ": unexpected exit code")
        return result

    gate = run("timeout_gate", python + [str(SOURCE / "tests/regression/test_scale_run_timeout.py"),
                                         str(dest / "unused_build")], 0)
    check(b"scale_run_timeout_ok" in gate.stdout, "timeout gate did not finish")

    def decide(name, values, want, complete=True):
        folder = dest / name
        fixture.write_lot(str(folder), values, info)
        argv = python + ([] if not complete else ["-S"]) + [str(SOURCE / "bench/synthetic/decide.py"),
                "--prereg", str(ppath), "--run", str(folder)]
        if complete:
            argv.append("--check-only")
        result = run(name, argv, want)
        if want == 2:
            check(b"REFUS" in result.stdout, name + ": refusal not explained")
            check(not (folder / "DECISION.json").exists(), name + ": partial decision leaked")
        elif complete:
            check(b"lot_conforme_au_plan" in result.stdout, name + ": not accepted by checker")
        return folder

    decide("valid", rows, 0)
    decide("one_missing", rows[:-1], 2)
    decide("duplicate", rows + [dict(rows[1])], 2)
    outside = copy.deepcopy(rows)
    outside[1]["unit"] = "outside_the_pinned_plan"
    decide("outside", outside, 2)
    nonfinite = copy.deepcopy(rows)
    nonfinite[1]["ari_s"] = "nan"  # row is not refused
    decide("nonfinite", nonfinite, 2)
    impossible = copy.deepcopy(rows)
    for row in impossible:
        if row["method"] == "tour":
            check(row["refused"] == "0", "mutated refusal row")
            row["ari_s"] = "1.25"
    decide("ari_above_one_check", impossible, 0)
    full = decide("ari_above_one_full", impossible, 0, complete=False)
    decision = json.loads((full / "DECISION.json").read_text())
    check(abs(decision["mean_ari_s"]["tour"] - 1.25) < 1e-12, "impossible ARI did not reach decision")
    refusal = copy.deepcopy(rows)
    refusal[2]["ari_s"] = "nan"  # refused=1; converted to zero by the declared policy
    check(refusal[2]["refused"] == "1", "refusal fixture changed")
    decide("refused_placeholder", refusal, 0)
    after = {str(p): sha(p) for p in paths}
    check(before == after, "frozen source changed")
    summary = dict(status="PASS", scope="benchmark_metadata_and_process_cleanup_only",
                   commands=len(records), dev_scenes=32, clouds_generated=0,
                   timeout_gate_cases=8, missing_duplicate_outside_nonfinite_refused=True,
                   impossible_ARI_check_code=0, impossible_ARI_full_code=0,
                   impossible_ARI_full_mean=decision["mean_ari_s"]["tour"],
                   refused_placeholder_intentionally_accepted=True,
                   engine_modified=False, GCP_used=False)
    receipt = dict(summary, python=sys.version, optimized=bool(sys.flags.optimize), records=records,
                   hashes_before=before, hashes_after=after)
    (dest / "receipt.json").write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n")
    print(json.dumps(summary, sort_keys=True))


if __name__ == "__main__":
    main()
