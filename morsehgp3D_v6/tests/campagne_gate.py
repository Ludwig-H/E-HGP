#!/usr/bin/env python3
"""Porte du lanceur de campagne locale (bench/campagne_locale.sh) — la
provenance d'executable est PROUVEE (fixture de reconstruction concurrente
exigee par l'alerte auditeur « campagne CPU mixte » du 31 août).

Scenarios :
  1. nominal (faux binaire conforme, matrice 1x1x2) : DONE terminal, un
     statut code=0 par tuple, HASHES.txt avec avant==apres par run, META
     avec sha256 du binaire prive ;
  2. reconstruction concurrente simulee : le faux binaire ALTERE la copie
     privee pendant son execution (chmod 755 puis append) — la campagne
     doit s'arreter SANS DONE avec le marqueur `INVALID hash apres ...` ;
  3. l'agregateur refuse : le STATUS du scenario 2 ne finit pas par DONE.

Codes : 0 conforme ; 1 un scenario en echec.
"""

from __future__ import annotations

import os
import stat
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
LAUNCHER = HERE.parent / "bench" / "campagne_locale.sh"

FAKE_OK = """#!/usr/bin/env bash
echo "sortie factice fam=$1"
exit 0
"""

FAKE_TAMPER = """#!/usr/bin/env bash
# Simule une reconstruction concurrente : altere la copie privee.
chmod 755 "$0" && echo "# altere" >> "$0"
echo "sortie factice"
exit 0
"""


def run_launcher(out: Path, bin_src: Path, seeds: str = "3 4") -> int:
    r = subprocess.run(["bash", str(LAUNCHER), str(out), str(bin_src),
                        "campagne de porte", "uniform", "400", seeds],
                       capture_output=True, text=True)
    return r.returncode


def main() -> int:
    failures = 0

    def check(name: str, ok: bool, detail: str = "") -> None:
        nonlocal failures
        if not ok:
            failures += 1
            print(f"ECHEC porte : {name} {detail}", file=sys.stderr)

    with tempfile.TemporaryDirectory() as td:
        base = Path(td)
        src = base / "faux_mhgp6"
        src.write_text(FAKE_OK)
        src.chmod(src.stat().st_mode | stat.S_IXUSR)

        out1 = base / "nominal"
        rc = run_launcher(out1, src)
        status = (out1 / "STATUS.txt").read_text().splitlines()
        hashes = (out1 / "HASHES.txt").read_text().splitlines()
        meta = (out1 / "META.txt").read_text()
        check("nominal : rc=0 et DONE terminal", rc == 0 and status and status[-1] == "DONE",
              f"rc={rc} fin={status[-1] if status else '?'}")
        check("nominal : deux tuples code=0",
              sum(1 for line in status if line.startswith("code=0 ")) == 2)
        check("nominal : hashes avant==apres par run",
              len(hashes) == 2 and all("avant=" in h and h.split()[0][6:] == h.split()[1][6:] for h in hashes))
        check("nominal : sha256 du binaire prive au META", "sha256_binaire_prive=" in meta)
        check("nominal : copie privee non modifiable",
              not os.access(out1 / "bin" / "mhgp6", os.W_OK))

        src2 = base / "faux_tamper"
        src2.write_text(FAKE_TAMPER)
        src2.chmod(src2.stat().st_mode | stat.S_IXUSR)
        out2 = base / "tamper"
        rc2 = run_launcher(out2, src2)
        status2 = (out2 / "STATUS.txt").read_text().splitlines()
        check("reconstruction concurrente : rc=3", rc2 == 3, f"rc={rc2}")
        check("reconstruction concurrente : marqueur INVALID hash apres",
              any(line.startswith("INVALID hash apres") for line in status2))
        check("reconstruction concurrente : PAS de DONE (l'agregateur refuse)",
              "DONE" not in status2)
        check("reconstruction concurrente : arret au premier tuple altere",
              sum(1 for line in status2 if line.startswith("code=")) == 0)

    if failures:
        return 1
    print("campagne_gate : provenance d'executable prouvee (nominal + reconstruction concurrente refusee)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
