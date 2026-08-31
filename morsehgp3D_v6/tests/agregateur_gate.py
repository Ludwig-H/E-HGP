#!/usr/bin/env python3
"""Porte de la PORTE E6 BORNÉE inter-graines (bench/agregateur.py) — la
règle préenregistrée est prouvée sur les scénarios exigés par l'alerte du
31 août :

  1. nominal (croissance linéaire, aucune masse aux octaves lourds) :
     agrégat publié, `garde_fou_borne_viole=non` (uniform), T_lourde
     indéfini ne déclenche pas ;
  2. campagne invalide : REFUS code 3 ET l'AGREGAT.txt préexistant est
     SUPPRIMÉ (jamais un agrégat périmé à côté d'un refus) ;
  3. majorité 1/3 : une seule graine à pente >= 2 => médiane < 2 =>
     `E6_active=non` ;
  4. majorité 2/3 : deux graines à pente >= 2 => `E6_active=oui` par la
     médiane ;
  5. seuil exact : médiane == 2,0 => déclenche (>= est contractuel) ;
  6. émergence 0 -> positif chez une graine : terme classé EMERGENCE
     (indéterminé), listé `emergences=`, ni déclencheur ni preuve négative ;
  7. violation du seul pas 1 : la porte (bornée au pas 2) rend `non` —
     périmètre documenté, jamais un garde-fou complet.

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


def run_agreg(root: Path) -> tuple[int, str, str]:
    r = subprocess.run([sys.executable, str(AGREG), str(root / "out"), str(root / "PROFIL_AUTORITE.txt")],
                       capture_output=True, text=True)
    return r.returncode, r.stdout, r.stderr


def build_stationary(root: Path, k_of) -> None:
    """Campagne synthétique sur terrain_stationnaire dont la masse w1 à
    l'octave 10 vaut k_of(n, seed) — identités fermantes PRÉSERVÉES
    (Σ w1 == tests_coeur) puis hashes de sorties remis en cohérence."""
    old = pg.FAM
    pg.FAM = "terrain_stationnaire"
    try:
        pg.build_campaign(root)
    finally:
        pg.FAM = old
    for n in pg.SIZES:
        for seed in pg.SEEDS:
            base, k = n // 4, k_of(n, seed)
            assert 0 <= k <= base
            new_w1 = f"{base - k}," + "0," * 9 + f"{k}," + "0,0,0,0,0"
            p = root / "out" / f"terrain_stationnaire_{n}_s{seed}.txt"
            p.write_text(re.sub(r" w1=[0-9,]+ \(octave", f" w1={new_w1} (octave", p.read_text()))
    pg.rehash_outputs(root)


def main() -> int:
    failures = 0

    def check(name: str, ok: bool, detail: str = "") -> None:
        nonlocal failures
        if not ok:
            failures += 1
            print(f"ECHEC porte : {name} {detail}", file=sys.stderr)

    # 1. Nominal uniforme.
    with tempfile.TemporaryDirectory() as td:
        root = Path(td) / "campagne"
        pg.build_campaign(root)
        rc, out, err = run_agreg(root)
        check("nominal : agrégat publié (rc=0)", rc == 0, f"rc={rc} err={err[:200]}")
        check("nominal : garde-fou borné non violé (uniform)", "garde_fou_borne_viole=non famille=uniform" in out)
        check("nominal : T_lourde indéfini ne déclenche pas", "indéfini (zéro des deux côtés)" in out)
        check("nominal : AGREGAT.txt écrit", (root / "AGREGAT.txt").is_file())

    # 2. Refus + suppression de l'agrégat périmé.
    with tempfile.TemporaryDirectory() as td:
        root = Path(td) / "campagne"
        pg.build_campaign(root)
        rc, _, _ = run_agreg(root)
        check("refus : agrégat nominal préalable publié", rc == 0 and (root / "AGREGAT.txt").is_file())
        (root / "out" / f"{pg.FAM}_16000_s4.txt").unlink()
        rc, out, err = run_agreg(root)
        check("campagne invalide : refus code 3, stdout vide", rc == 3 and out == "", f"rc={rc}")
        check("campagne invalide : AGREGAT.txt préexistant SUPPRIMÉ", not (root / "AGREGAT.txt").exists())

    def scenario(name: str, k_of, expect_active: bool, expect_marks=()) -> None:
        with tempfile.TemporaryDirectory() as td:
            root = Path(td) / "campagne"
            build_stationary(root, k_of)
            rc, out, err = run_agreg(root)
            check(f"{name} : agrégat publié", rc == 0, f"rc={rc} err={err[:200]}")
            want = "E6_active=oui famille=terrain_stationnaire" if expect_active \
                else "E6_active=non famille=terrain_stationnaire"
            check(f"{name} : verdict attendu", want in out, out.splitlines()[-3:] if out else "vide")
            for mark in expect_marks:
                check(f"{name} : {mark!r} présent", mark in out)

    # 3. Majorité 1/3 : seed 3 pente 3, seeds 4/5 pente 1 => médiane 1 => non.
    scenario("majorité 1/3", lambda n, s: {8000: 100, 16000: 200, 32000: 1600 if s == 3 else 400}[n],
             expect_active=False)
    # 4. Majorité 2/3 : seeds 3/4 pente 3 => médiane 3 => oui par T_lourde.
    scenario("majorité 2/3", lambda n, s: {8000: 100, 16000: 200, 32000: 1600 if s in (3, 4) else 400}[n],
             expect_active=True, expect_marks=("termes=T_lourde",))
    # 5. Seuil exact : toutes pentes pas2 == 2,0 => déclenche (>= contractuel).
    scenario("seuil exact médiane == 2,0", lambda n, s: {8000: 100, 16000: 200, 32000: 800}[n],
             expect_active=True)
    # 6. Émergence : seed 3 à 0 jusqu'a 16000 puis positif => indéterminé
    # listé, pas un déclencheur (les autres graines restent à pente 1).
    scenario("émergence 0->positif", lambda n, s: 0 if (s == 3 and n < 32000) else
             {8000: 100, 16000: 200, 32000: 6400 if s == 3 else 400}[n],
             expect_active=False, expect_marks=("EMERGENCE", "emergences=T_lourde"))
    # 7. Violation du seul pas 1 : pentes (3 ; 1) => la porte bornée au pas 2
    # rend non — périmètre documenté.
    scenario("violation du seul pas 1 (périmètre borné)",
             lambda n, s: {8000: 100, 16000: 800, 32000: 1600}[n], expect_active=False)

    if failures:
        return 1
    print("agregateur_gate : porte E6 bornée conforme (nominal, refus+suppression, majorités 1/3 et 2/3, "
          "seuil exact, émergence indéterminée, périmètre pas2)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
