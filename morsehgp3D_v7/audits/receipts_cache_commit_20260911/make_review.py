#!/usr/bin/env python3
"""Écrit review.json : sha256 de tous les artefacts du reçu, des sources commitées
consultées, HEAD, et un résumé tiré des enregistrements. Sans assert."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import subprocess

REVIEWED_COMMIT = "ad7ffd28b35e153a20bd8cf42534d1cd29160bcd"

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
SOURCES = [
    "morsehgp3D_v7/src/forest/full_ball_tower.hpp", "morsehgp3D_v7/src/forest/full_coverage_certificate.hpp",
    "morsehgp3D_v7/src/forest/anchor_meb.hpp", "morsehgp3D_v7/src/pipeline/witness_front.hpp",
    "morsehgp3D_v7/src/spindle/witness_batch.hpp", "morsehgp3D_v7/tests/facet_resolver_cache_gate.cpp",
    "morsehgp3D_v7/tests/full_ball_tower_gate.cpp", "morsehgp3D_v7/tests/full_ball_work_gate.cpp",
    "morsehgp3D_v7/tests/witness_front_gate.cpp", "morsehgp3D_v7/bench/full_ball_tower_probe.cpp",
    "morsehgp3D_v7/bench/full_gabriel_semantic_digest.hpp", "morsehgp3D_v7/CMakeLists.txt",
    "morsehgp3D_v7/receipts/full_ball_named_blocks_20260910/capture/observer.hpp",
    "morsehgp3D_v7/receipts/full_ball_named_blocks_20260910/capture/observer_gate.cpp",
    "morsehgp3D_v7/receipts/full_ball_named_blocks_20260910/capture/observer_overlay.patch",
    "morsehgp3D_v7/receipts/full_ball_named_blocks_20260910/capture/expectations.json",
    "morsehgp3D_v7/receipts/full_ball_named_blocks_20260910/capture/source_before.json",
    "morsehgp3D_v7/receipts/full_ball_named_blocks_20260910/capture/binary_o2_r2.sha256",
    "morsehgp3D_v7/receipts/ball_resolver_residence_20260910/README.md",
    "morsehgp3D_v7/receipts/ball_resolver_residence_20260910/combined/mutant.py",
    "morsehgp3D_v7/audits/receipts_raccord_ancres_20260910/suite_cache_20260910/README.md",
    "morsehgp3D_v7/audits/receipts_raccord_ancres_20260910/suite_cache_20260910/snapshots/SHA256SUMS",
]


def sha256_of(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def sha256_blob(rel: str) -> str:
    """sha256 du blob de <rel> (relatif au dépôt) au commit revu."""
    data = subprocess.check_output(["git", "-C", str(ROOT), "show", f"{REVIEWED_COMMIT}:{rel}"])
    return hashlib.sha256(data).hexdigest()


def main() -> int:
    records = []
    for meta in sorted(HERE.rglob("*.json")):
        if meta.name == "review.json":
            continue
        try:
            row = json.loads(meta.read_text(encoding="utf-8"))
        except ValueError:
            continue
        if isinstance(row, dict) and row.get("schema") == "mhgp7-audit-record-v1":
            records.append({"record": str(meta.relative_to(HERE)), "returncode": row["returncode"],
                            "expected": row["expected_returncode"], "conforming": row["conforming"],
                            "wall_seconds": row["wall_seconds"], "child_maxrss_kib": row["child_maxrss_kib"]})
    artefacts = {str(p.relative_to(HERE)): sha256_of(p) for p in sorted(HERE.rglob("*"))
                 if p.is_file() and p.name != "review.json" and "__pycache__" not in p.parts}
    review = {
        "schema": "mhgp7-second-auditor-receipt-review-v1",
        "receipt": "audits/receipts_cache_commit_20260911",
        "head": subprocess.check_output(["git", "-C", str(ROOT), "rev-parse", "HEAD"], text=True).strip(),
        "scope": "Replay by the second auditor, on the committed bytes of ad7ffd28, of the resolver-cache receipt gates, "
                 "the grouped-lot mutants, the payload fingerprints per family, the random-corpus rational judge, the "
                 "batched witness front, all CTests, and the constructor's named 50k block observer executed on local CPU. "
                 "Relative to supplied complete exact censuses; no timing claim, no GPU, no public promotion.",
        "gcp_used": False,
        "public_status": "not_claimed",
        "reviewed_commit": REVIEWED_COMMIT,
        "sources_sha256": {rel: sha256_blob(rel) for rel in SOURCES},
        "records": records,
        "artefacts_sha256": artefacts,
    }
    (HERE / "review.json").write_text(json.dumps(review, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"review.json : {len(records)} enregistrements, {len(artefacts)} artefacts, {len(SOURCES)} sources")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
