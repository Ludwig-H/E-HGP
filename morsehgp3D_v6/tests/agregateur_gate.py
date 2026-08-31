#!/usr/bin/env python3
"""Porte de l'agrégateur inter-graines (bench/agregateur.py) — la règle
préenregistrée est prouvée sur trois scénarios synthétiques :

  1. nominal (croissance linéaire, aucune masse aux octaves lourds) :
     agrégat publié, verdict `garde_fou_viole=non` (uniform), T_lourde
     indéfini ne déclenche pas ;
  2. campagne invalide (fichier manquant) : REFUS code 3, aucun AGREGAT.txt
     (jamais un agrégat sur une campagne non validée) ;
  3. activation : famille stationnaire déclarée + masse w1 à l'octave 10
     quadruplant par pas (identités fermantes PRÉSERVÉES : Σ w1 ==
     tests_coeur) — verdict `E6_active=oui ... termes=T_lourde` par la
     MÉDIANE des trois graines.

Codes : 0 conforme ; 1 un scénario en échec.
"""

from __future__ import annotations

import importlib.util
import re
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
AGREG = HERE.parent / "bench" / "agregateur.py"

spec = importlib.util.spec_from_file_location("pentes_gate", HERE / "pentes_gate.py")
pg = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pg)

K_BY_N = {8000: 100, 16000: 400, 32000: 6400}  # pentes T_lourde : 2 puis 4


def run_agreg(root: Path) -> tuple[int, str, str]:
    r = subprocess.run([sys.executable, str(AGREG), str(root / "out")], capture_output=True, text=True)
    return r.returncode, r.stdout, r.stderr


def main() -> int:
    failures = 0

    def check(name: str, ok: bool, detail: str = "") -> None:
        nonlocal failures
        if not ok:
            failures += 1
            print(f"ECHEC porte : {name} {detail}", file=sys.stderr)

    with tempfile.TemporaryDirectory() as td:
        root = Path(td) / "campagne"
        pg.build_campaign(root)
        rc, out, err = run_agreg(root)
        check("nominal : agrégat publié (rc=0)", rc == 0, f"rc={rc} err={err[:200]}")
        check("nominal : garde-fou non violé (uniform)", "garde_fou_viole=non famille=uniform" in out)
        check("nominal : T_lourde indéfini ne déclenche pas", "indéfini (zéro légitime)" in out)
        check("nominal : AGREGAT.txt écrit", (root / "AGREGAT.txt").is_file())

    with tempfile.TemporaryDirectory() as td:
        root = Path(td) / "campagne"
        pg.build_campaign(root)
        (root / "out" / f"{pg.FAM}_16000_s4.txt").unlink()
        rc, out, err = run_agreg(root)
        check("campagne invalide : refus code 3", rc == 3 and out == "", f"rc={rc}")
        check("campagne invalide : aucun AGREGAT.txt", not (root / "AGREGAT.txt").exists())

    with tempfile.TemporaryDirectory() as td:
        old_fam = pg.FAM
        pg.FAM = "terrain_stationnaire"
        root = Path(td) / "campagne"
        try:
            pg.build_campaign(root)
        finally:
            pg.FAM = old_fam
        for n in pg.SIZES:
            base, k = n // 4, K_BY_N[n]
            new_w1 = f"{base - k}," + "0," * 9 + f"{k}," + "0,0,0,0,0"
            for seed in pg.SEEDS:
                p = root / "out" / f"terrain_stationnaire_{n}_s{seed}.txt"
                p.write_text(re.sub(r" w1=[0-9,]+ \(octave", f" w1={new_w1} (octave", p.read_text()))
        rc, out, err = run_agreg(root)
        check("activation : agrégat publié (rc=0)", rc == 0, f"rc={rc} err={err[:200]}")
        check("activation : E6_active=oui par T_lourde (médiane des graines)",
              bool(re.search(r"E6_active=oui famille=terrain_stationnaire termes=.*T_lourde", out)))

    if failures:
        return 1
    print("agregateur_gate : règle inter-graines préenregistrée conforme (nominal + refus + activation)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
