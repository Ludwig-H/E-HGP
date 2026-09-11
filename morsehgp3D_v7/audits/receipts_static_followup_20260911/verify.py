"""Verify immutable constructor evidence, new SAN replay and exact seed witnesses."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import subprocess
import sys

import seed_fixture


HERE = Path(__file__).resolve().parent
V7 = HERE.parents[1]


def need(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def compute() -> dict:
    context = json.loads((HERE / "context_pins.json").read_text())
    for name, expected in context["files"].items():
        need(sha(V7 / name) == expected, "constructor input: " + name)
    readers = {}
    for name in context["files"]:
        if not name.endswith("/verify.py"):
            continue
        argv = [sys.executable, "-B"] + (["-O"] if sys.flags.optimize else [])
        # All invoked code is pinned, read-only receipt verification, not a runner.
        run = subprocess.run(argv + [str(V7 / name)], capture_output=True, text=True, timeout=60)
        need(run.returncode == 0 and not run.stderr, "constructor reader: " + name)
        readers[name] = json.loads(run.stdout)["status"]
    need(len(readers) == 7, "seven completed package readers")

    base = V7 / "receipts/static_worker_failure_20260911"
    mapping = json.loads((base / "storage_map.json").read_text())

    def raw(name: str) -> bytes:
        row = mapping[name]
        data = (base / row["physical"]).read_bytes()
        need(hashlib.sha256(data).hexdigest() == row["sha256"], "logical input: " + name)
        return data

    replay = json.loads((HERE / "worker_san.json").read_text())
    previous = json.loads(raw("san/commands.json"))
    need(replay["status"] == "passed" and replay["exit_code"] == 0 and not replay["stderr"] and
         replay["gcp_used"] is False and replay["public_status"] == "not_claimed", "new SAN success")
    need(replay["argv"] == previous[1]["argv"] and replay["binary_sha256"] ==
         raw("san/binary.sha256").decode().strip() ==
         "70379dd9dfeb3d7643debca660eb2239ecc9f1d7206222bfee27258113ce20cf", "same compiled binary")
    need(replay["environment_overlay"] == {"ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1",
         "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"}, "sanitizer options")
    need(replay["constructor_manifest_sha256"] == sha(base / "MANIFEST.json") and
         replay["stable"] and replay["before"] == replay["after"] and
         replay["ended_ns"] >= replay["started_ns"], "closed stable capture")
    binary = Path(replay["argv"][0])
    old_repo = binary.parents[3]
    expected = {str(binary): replay["binary_sha256"],
                str(old_repo / "morsehgp3D_v7/receipts/static_worker_failure_20260911/MANIFEST.json"):
                    sha(base / "MANIFEST.json"),
                str(old_repo / "morsehgp3D_v7/audits/receipts_static_followup_20260911/worker_replay.py"):
                    sha(HERE / "worker_replay.py")}
    for path, logical in zip(replay["argv"][1:], ("san/fixtures.txt", "san/expected.txt")):
        expected[path] = hashlib.sha256(raw(logical)).hexdigest()
    for name, digest in json.loads(raw("san/sources_before.json")).items():
        expected[str(binary.parent / "source" / name)] = digest
    expected[str(binary.parent / "driver.cpp")] = hashlib.sha256(raw("san/driver.cpp")).hexdigest()
    need(replay["before"] == expected, "all new replay pins tied to original sources")
    lines = [json.loads(line) for line in replay["stdout"].splitlines()]
    original_o2 = [json.loads(line) for line in raw("o2/post_admission_and_reuse.stdout").splitlines()]
    need(len(lines) == 2 and lines[0]["status"] == "passed_post_admission_failure" and
         lines[0]["failed_runs"] == 2 and lines[0]["retained_MEB_calls"] >= 2, "non-vacuous failure after admission")
    need(lines[1] == original_o2[1] and lines[1]["rows"] == 2524 and lines[1]["vertical"] == 1506,
         "nominal reuse exactly matches O2")

    scale = []
    for n in (8000, 16000, 32000):
        directory = V7 / "receipts/static_resolution_scale_20260911" / f"n{n}_s8_static4"
        current = json.loads((directory / "stdout").read_text())
        old = json.loads((directory / "reference.stdout").read_text())
        unique = sum(r["unique"] for r in current["static_orders"])
        seeded = sum(r["seeded_unique"] for r in current["static_orders"])
        descents = current["intruder_queries"]
        need(current["anchor_hits"] == unique - seeded, "one initial MEB per unseeded group")
        need(current["resolver_meb_calls"] == unique - seeded + descents, "static M=U-S+D")
        initial_saved = old["anchor_hits"] - current["anchor_hits"]
        descents_saved = old["intruder_queries"] - descents
        need(initial_saved + descents_saved == old["resolver_meb_calls"] - current["resolver_meb_calls"],
             "decomposition of saved work")
        scale.append(dict(n=n, unique=unique, seeded_initial=seeded, descents=descents,
                          initial_saved=initial_saved, descents_saved=descents_saved,
                          MEB_calls=current["resolver_meb_calls"]))
    witness = seed_fixture.compute()
    return dict(status="passed_static_followup", constructor_commit=context["constructor_commit"],
                constructor_readers=readers, worker_SAN=lines, saved_work=scale,
                seed_exchange_witnesses=witness, seed_shortcut_implemented=False,
                seed_hit_count_measured=False, public_status="not_claimed", gcp_used=False)


def main() -> None:
    need(sys.argv[1:] in ([], ["--record"]), "arguments")
    if not sys.argv[1:]:
        entries = (HERE / "SHA256SUMS").read_text().splitlines()
        found = set()
        for line in entries:
            expected, name = line.split("  ", 1)
            need(Path(name).name == name and name not in found and sha(HERE / name) == expected,
                 "packet pin: " + name)
            found.add(name)
        need(found == {"README.md", "context_pins.json", "worker_replay.py", "worker_san.json",
                       "seed_fixture.py", "verify.py", "review.json"}, "packet inventory")
    result = compute()
    if sys.argv[1:]:
        need(not (HERE / "review.json").exists(), "create-only review")
        (HERE / "review.json").write_text(json.dumps(result, indent=2) + "\n")
    else:
        need(result == json.loads((HERE / "review.json").read_text()), "derived result")
    print(json.dumps(dict(status=result["status"], readers=7, worker_SAN_failed_runs=2,
                         Gamma_reuse_rows=2524, exact_seed_branches=2,
                         equal_radius_counterfixture=True, shortcut_implemented=False,
                         public_status="not_claimed", gcp_used=False)))


if __name__ == "__main__":
    main()
