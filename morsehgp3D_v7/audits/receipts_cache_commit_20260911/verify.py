#!/usr/bin/env python3
"""Lecteur du reçu `receipts_cache_commit_20260911` (second auditeur).

Vérifie, sans compiler ni exécuter aucun moteur et sans GCP : que chaque
enregistrement `record.py` est conforme (sauf l'essai 50k déclaré interrompu
par SIGTERM sous pression mémoire) et que ses sorties n'ont pas changé ; que les
fichiers de comparaison ne portent aucune différence ; que la sortie 50k `_r2`
porte les quatre blocs avec les cardinalités épinglées ; que le journal CTest
est vert, à l'unique exception tolérée du dépassement de délai sous charge de
`mhgp7_e6_grille_objet`, lequel doit alors passer dans son rejeu isolé ; que
`review.json` épingle tous les artefacts. Aucun `assert` ; identique sous
`python3 -B` et `python3 -B -O`. Code 0 conforme, 1 sinon, 2 lecture impossible."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import re
import sys

HERE = Path(__file__).resolve().parent
V7 = HERE.parents[1]
EXPECTATIONS = V7 / "receipts/full_ball_named_blocks_20260910/capture/expectations.json"
# Essai 50k interrompu par SIGTERM sous pression mémoire concurrente (CTest 32k) ;
# conservé comme enregistrement honnête, non conforme par construction.
DECLARED_TERMINATED = {"blocs_50k/pinned_50000_threads8_o2"}
# Unique test dont le dépassement de délai sous charge est toléré s'il passe isolé.
CTEST_ALLOWED_TIMEOUT = "mhgp7_e6_grille_objet"


def sha256_of(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    problems: list[str] = []
    records = 0
    for meta in sorted(HERE.rglob("*.json")):
        if meta.name in ("review.json", "contre_lecture.json"):
            continue
        try:
            row = json.loads(meta.read_text(encoding="utf-8"))
        except ValueError:
            continue
        if not isinstance(row, dict) or row.get("schema") != "mhgp7-audit-record-v1":
            continue
        records += 1
        base = meta.with_suffix("")
        rel = str(meta.relative_to(HERE).with_suffix(""))
        if rel in DECLARED_TERMINATED:
            if row.get("signal") != 15:
                problems.append(f"essai déclaré interrompu mais signal != 15 : {rel}")
        elif not row.get("conforming") or row.get("signal") is not None:
            problems.append(f"enregistrement non conforme : {meta.relative_to(HERE)}")
        for stream in ("stdout", "stderr"):
            path = Path(f"{base}.{stream}")
            if not path.is_file() or sha256_of(path) != row.get(f"{stream}_sha256"):
                problems.append(f"sortie {stream} altérée ou absente : {meta.relative_to(HERE)}")
        if row.get("gcp_used") is not False:
            problems.append(f"gcp_used doit être false : {meta.relative_to(HERE)}")
    if records < 50:
        problems.append(f"trop peu d'enregistrements : {records}")

    comparisons = 0
    comparison_files = sorted(HERE.rglob("comparaison_avec_recu_20260910.txt")) + [HERE / "portes_commit/identite_o2_san.txt"]
    for text in comparison_files:
        if not text.is_file():
            problems.append(f"fichier de comparaison absent : {text.relative_to(HERE)}")
            continue
        lines = [line for line in text.read_text(encoding="utf-8").splitlines() if line.strip()]
        if not lines:
            problems.append(f"fichier de comparaison vide : {text.relative_to(HERE)}")
        for line in lines:
            comparisons += 1
            if "!=" in line or "DIFFERENT" in line or "ILLISIBLE" in line:
                problems.append(f"différence : {text.relative_to(HERE)} : {line}")

    named = HERE / "blocs_50k/pinned_50000_threads8_o2_r2.stdout"
    blocks_checked = 0
    try:
        out = json.loads(named.read_text(encoding="utf-8"))
        expectations = json.loads(EXPECTATIONS.read_text(encoding="utf-8"))
    except (OSError, ValueError) as error:
        problems.append(f"sortie 50k _r2 ou attentes illisibles : {error}")
        out, expectations = {}, {}
    if out:
        if out.get("status") != "passed" or out.get("n") != 50000 or out.get("s") != 8 or out.get("kmax") != 10:
            problems.append("sortie 50k : statut ou configuration inattendus")
        if out.get("input_digest") != expectations.get("input", {}).get("digest"):
            problems.append("sortie 50k : digest d'entrée différent des attentes épinglées")
        if out.get("raw_anchor_mutant_killed") is not True or out.get("raw_anchor_mutant_reason") != "named.pre_root_not_live":
            problems.append("sortie 50k : mutant d'ancre brute non tué par la raison attendue")
        if out.get("benchmark") is not False or out.get("contract_qualified") is not False or out.get("public_status") != "not_claimed":
            problems.append("sortie 50k : drapeaux de non-promotion absents")
        got_blocks = out.get("blocks", [])
        want_blocks = expectations.get("blocks", [])
        if len(got_blocks) != len(want_blocks) or len(want_blocks) != 4:
            problems.append("sortie 50k : nombre de blocs différent de quatre")
        for got, want in zip(got_blocks, want_blocks):
            exp = got.get("expected", {})
            roots = got.get("pre_lot_roots", [])
            if exp.get("capture_label") != want.get("capture_label") or exp.get("k") != want.get("k"):
                problems.append(f"bloc {want.get('capture_label')} : identité différente")
            if len(roots) != want.get("expected_roots") or got.get("contribution_mask") != want.get("expected_contribution_mask") \
                    or got.get("contribution_interior") != want.get("expected_contribution_interior"):
                problems.append(f"bloc {want.get('capture_label')} : racines ou contribution différentes des attentes")
            if got.get("before_observations") != 1 or got.get("after_observations") != 1:
                problems.append(f"bloc {want.get('capture_label')} : observation non unique")
            blocks_checked += 1

    ctest = HERE / "ctest/ctest_release.log"
    ctest_ok = False
    if ctest.is_file():
        text = ctest.read_text(encoding="utf-8", errors="replace")
        failed = set(re.findall(r"Test\s+#\d+:\s+(\S+)\s+\.+\*\*\*", text))
        fully_green = "100% tests passed, 0 tests failed" in text
        extra = failed - {CTEST_ALLOWED_TIMEOUT}
        if not failed and fully_green:
            ctest_ok = True
        elif failed <= {CTEST_ALLOWED_TIMEOUT} and not extra:
            isolated = HERE / "ctest/e6_grille_objet_isolated.json"
            try:
                rec = json.loads(isolated.read_text(encoding="utf-8"))
                ctest_ok = rec.get("conforming") is True and rec.get("returncode") == 0
            except (OSError, ValueError):
                ctest_ok = False
            if not ctest_ok:
                problems.append("CTest : timeout e6 toléré mais rejeu isolé absent ou non conforme")
        else:
            problems.append(f"CTest : échecs non tolérés {sorted(extra)}")
    else:
        problems.append("journal CTest absent")
    if not ctest_ok and not any(p.startswith("CTest") for p in problems):
        problems.append("journal CTest non vert")

    review = HERE / "review.json"
    pinned = 0
    try:
        pins = json.loads(review.read_text(encoding="utf-8")).get("artefacts_sha256", {})
    except (OSError, ValueError):
        pins = {}
        problems.append("review.json illisible")
    for rel, expected in pins.items():
        path = HERE / rel
        if not path.is_file() or sha256_of(path) != expected:
            problems.append(f"artefact altéré : {rel}")
        pinned += 1
    for path in sorted(HERE.rglob("*")):
        if path.is_file() and path.name != "review.json" and "__pycache__" not in path.parts:
            if str(path.relative_to(HERE)) not in pins:
                problems.append(f"artefact non épinglé : {path.relative_to(HERE)}")

    summary = {
        "records": records, "comparison_lines": comparisons, "named_blocks_checked": blocks_checked,
        "ctest_green_or_tolerated": ctest_ok, "artefacts_pinned": pinned, "problems": problems,
        "gcp_used": False, "public_status": "not_claimed",
        "status": "passed" if not problems else "failed",
    }
    print(json.dumps(summary, indent=2, ensure_ascii=False))
    return 0 if not problems else 1


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except OSError as error:
        print(f"lecture impossible : {error}", file=sys.stderr)
        raise SystemExit(2)
