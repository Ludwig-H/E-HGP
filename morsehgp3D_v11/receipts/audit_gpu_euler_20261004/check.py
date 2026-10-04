"""Read-only closure and portable model replays; never run native/GPU tools."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parent
JOBS = (
    ("euler", "check.py", "normal.json"),
    ("device_geometry", "check_device_geometry.py", "checks.normal.json"),
    ("device_published_bindings", "check_bindings.py", "checks.normal.json"),
    ("batch_transport_live", "check_frontier_count.py", "stdout_normal.json"),
    ("centre_leaf_counterexample", "check.py", "normal.json"),
    ("gpu_bench_current", "check.py", "normal.json"),
    ("mesure_protocol_live", "check.py", "normal.json"),
    ("lecteurs_published", "check.py", "normal.json"),
    ("mesures_bindings", "review_measures.py", "stdout_normal.json"),
)


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


inventories = {}
for folder in sorted(p for p in ROOT.iterdir() if p.is_dir()):
    capsule = folder.name
    entries = {}
    for line in (folder / "SHA256SUMS").read_text().splitlines():
        digest, relative = line.split("  ", 1)
        require(relative not in entries, "duplicate payload")
        require(not Path(relative).is_absolute() and ".." not in Path(relative).parts,
                "unsafe manifest path")
        require(hashlib.sha256((folder / relative).read_bytes()).hexdigest() == digest,
                "changed payload " + capsule + "/" + relative)
        entries[relative] = digest
    actual = {p.relative_to(folder).as_posix() for p in folder.rglob("*")
              if p.is_file() and p.relative_to(folder).as_posix() != "SHA256SUMS"}
    require(actual == set(entries), "incomplete payload inventory " + capsule)
    inventories[capsule] = len(entries)

results = {}
for capsule, script, expected in JOBS:
    folder = ROOT / capsule
    output = (folder / expected).read_bytes()
    for options in (("-B", "-S"), ("-B", "-O", "-S")):
        args = [sys.executable, *options, script]
        if capsule == "mesures_bindings":
            repo = subprocess.check_output(["git", "rev-parse", "--show-toplevel"],
                                           cwd=ROOT, text=True).strip()
            args += ["--repo", repo]
        replay = subprocess.run(args, cwd=folder,
                                capture_output=True, timeout=30)
        require(replay.returncode == 0 and not replay.stderr, "replay failure " + capsule)
        require(replay.stdout == output, "replay differs " + capsule)
    model = json.loads(output)
    field = "metadata_checks" if capsule == "mesures_bindings" else "checks"
    results[capsule] = {"payloads": inventories[capsule], "checks": model[field],
                        "normal_optimized_identical": True}

print(json.dumps({"status": "PASS", "commit": "61da03749344a6acc4fea2b9eee875606cc857e8",
                  "capsules": results, "inventories": inventories,
                  "native_runs": 0, "gcp_actions": 0},
                 sort_keys=True, indent=2))
