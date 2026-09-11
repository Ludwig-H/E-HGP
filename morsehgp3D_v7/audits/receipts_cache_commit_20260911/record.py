#!/usr/bin/env python3
"""Enregistre une commande du reçu : argv, cwd, code, durée, RSS max de l'enfant,
sha256 des sorties. Aucun assert ; identique sous python3 et python3 -O ; refuse
d'écraser un enregistrement existant. Ne compile rien et ne touche jamais GCP."""
from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import resource
import subprocess
import sys
import time


def sha256_of(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    argv = sys.argv[1:]
    if len(argv) < 5 or argv[3] != "--":
        print("usage: record.py <dossier> <nom> <code_attendu> -- <commande...>", file=sys.stderr)
        return 2
    out_dir = Path(argv[0])
    name = argv[1]
    try:
        expected = int(argv[2])
    except ValueError:
        print("code attendu invalide", file=sys.stderr)
        return 2
    command = argv[4:]
    out_dir.mkdir(parents=True, exist_ok=True)
    meta = out_dir / f"{name}.json"
    if meta.exists():
        print(f"refus : {meta} existe déjà", file=sys.stderr)
        return 2
    overrides = {k: os.environ[k] for k in ("ASAN_OPTIONS", "UBSAN_OPTIONS", "MHGP7_AUDIT_FAMILY") if k in os.environ}
    stdout_path = out_dir / f"{name}.stdout"
    stderr_path = out_dir / f"{name}.stderr"
    start = time.time()
    signal = None
    with stdout_path.open("wb") as out, stderr_path.open("wb") as err:
        try:
            rc = subprocess.run(command, stdout=out, stderr=err, check=False).returncode
        except OSError as error:
            err.write(f"record.py: {error}\n".encode())
            rc = 127
    end = time.time()
    if rc < 0:
        signal = -rc
    usage = resource.getrusage(resource.RUSAGE_CHILDREN)
    row = {
        "schema": "mhgp7-audit-record-v1",
        "name": name,
        "argv": command,
        "cwd": os.getcwd(),
        "start_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime(start)),
        "end_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime(end)),
        "wall_seconds": round(end - start, 3),
        "returncode": rc,
        "expected_returncode": expected,
        "signal": signal,
        "conforming": rc == expected and signal is None,
        "child_maxrss_kib": usage.ru_maxrss,
        "child_user_seconds": round(usage.ru_utime, 3),
        "child_system_seconds": round(usage.ru_stime, 3),
        "stdout_sha256": sha256_of(stdout_path),
        "stderr_sha256": sha256_of(stderr_path),
        "environment_overrides": overrides,
        "gcp_used": False,
    }
    meta.write_text(json.dumps(row, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"{name}: rc={rc} attendu={expected} {end - start:.3f}s maxrss_kib={usage.ru_maxrss}")
    return 0 if row["conforming"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
