"""Audit-only short decision probes on frozen R2 sources and fabricated scores.

No engine, cloud generator, GCP, or signal experiment is invoked. Exit zero
means collection succeeded, never that the product accepted only valid inputs.
The normal and optimized runs have separate fresh fixture directories.
"""
import datetime
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    root = Path(sys.argv[1]).resolve()
    mode = sys.argv[2]
    base = root / "runs" / mode
    frozen = root / "source_snapshot"
    decide = frozen / "bench/synthetic/decide.py"
    merge = frozen / "bench/g4/merge_sessions.py"
    cases = sorted(p for p in base.iterdir() if p.is_dir())
    dependencies = sorted(p for p in frozen.rglob("*") if p.is_file())
    before = {str(p.relative_to(root)): digest(p) for p in dependencies}
    env = dict(os.environ, OPENBLAS_NUM_THREADS="1", OMP_NUM_THREADS="1", MKL_NUM_THREADS="1")
    prefix = [sys.executable, "-B"] + (["-O"] if sys.flags.optimize else [])
    records = []
    def call(name, kind, command, run_dir=None):
        start = time.monotonic()
        r = subprocess.run(command, capture_output=True, text=True, timeout=5, env=env)
        output = dict(case=name, kind=kind, argv=command, exit_code=r.returncode,
                      seconds=time.monotonic() - start, stdout=r.stdout, stderr=r.stderr)
        if run_dir is not None:
            path = run_dir / "DECISION.json"
            output["decision_written"] = path.exists()
            if path.exists():
                output["decision"] = json.loads(path.read_text())
        records.append(output)
    for case in cases:
        call(case.name, "check_only", prefix + [str(decide), "--prereg", str(case / "prereg.json"),
             "--run", str(case / "run"), "--check-only"], case / "run")
    full = ("baseline", "alpha_two", "alpha_nan", "missing_pair_method", "missing_bootstrap",
            "duplicate_ari_header", "refused_nan", "unused_metrics_invalid")
    for name in full:
        case = base / name
        call(name, "full_decision", prefix + [str(decide), "--prereg", str(case / "prereg.json"),
             "--run", str(case / "run")], case / "run")
    poison = base / "ari_125"
    fusion = base / "fusion_poison"
    call("fusion_ari_125", "merge", prefix + [str(merge), "--prereg", str(poison / "prereg.json"),
         "--out", str(fusion), str(poison / "run")])
    merged_info = json.loads((fusion / "run.json").read_text()) if (fusion / "run.json").exists() else None
    call("fusion_ari_125", "downstream_check_only", prefix + [str(decide), "--prereg",
         str(poison / "prereg.json"), "--run", str(fusion), "--check-only"], fusion)
    after = {str(p.relative_to(root)): digest(p) for p in dependencies}
    if before != after:
        raise RuntimeError("frozen source changed during audit")
    print(json.dumps(dict(status="SHORT_ADVERSES_COLLECTED_NOT_ENGINE_QUALIFICATION",
          observed_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(), mode=mode,
          python=sys.version, interpreter=sys.executable, calls=len(records), scores_are_fabricated=True,
          scene_generator_calls=0, engine_calls=0, GCP_used=False, GPU_used=False,
          source_hashes_before=before, source_hashes_after=after,
          fusion_poison_run_info=merged_info, records=records), sort_keys=True))
if __name__ == "__main__":
    main()
