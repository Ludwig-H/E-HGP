#!/usr/bin/env python3
"""Porte du validateur de campagne (bench/pentes.py) — FAIL-CLOSED prouvé.

Construit dans un répertoire temporaire une campagne synthétique minimale
conforme (matrice 1 famille × 3 tailles × 3 graines, compteurs complets),
vérifie que pentes.py l'accepte (code 0, table imprimée), puis grave les
falsifications exigées par l'audit du 31 août : chacune doit être REFUSÉE
(code 3) avec un stdout VIDE (aucune table partielle).

Codes : 0 conforme ; 1 une falsification passe ou le nominal échoue.
"""

from __future__ import annotations

import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
PENTES = HERE.parent / "bench" / "pentes.py"
SIZES = [8000, 16000, 32000]
SEEDS = [3, 4, 5]
FAM = "uniform"

OUTPUT_TEMPLATE = """payload=mhgp6-forests-horizontal-v1 authority=status_terminal callbacks=provisional vertical_maps=none
backend=cpu_reference
tower_scope=profile_complete_k10 smax_requested=11 smax_effective=11
famille={fam} n={n} coord=100 s=8 smax=11 seed={seed} threads=8 emis={base} boules_uniques={base} mortes_profondeur=10 survivantes={surv} census_int=50 census_shell=20 evenements={base} facettes={base} fusions=10 deltas=10 noeuds=10
generation rect_alive={base}/{base}/{base} rect_visites_fusionnes={base} ancres={base}/{base}/{base} candidats={base}/{base}/{base} tues_profondeur=1/{base}/{base} ancres_w4=1 ancres_w3=1 ancres_secteurs=1/1 ancres_cellules=0/0 seeds_cellules=0/0 grilles=0/0 seeds={base}/{base} completions_q4={base} seeds_core_tues={base} seeds_corde_tues={base} float_cert=1/1 repli={base} ancres_hist=1/1/1 hist_lignes=1/1/1 hist_seuil=1/1/1 hist_survivants={base}/{base}/{base} jung=1/1/1
sweep tests_coeur={base} tests_prof_q3={base} tests_passe2={base} tri_comparaisons={base} seeds_passe2={base} racines_corde={base} groupes={base} racines_hors_corde={base} temoins_constants=1 rejets=lens:1/owner:1/once:1/i64:1/face:1/det:0/centre:1
vwspd nœuds_temoins={base} coins={base} h_rect={base}/{base}/{base} m_anchor={base}/{base}/{base} iters_coeur={base} iters_passe2={base}
octaves_q4 ancres=1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 seeds=1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 w1=1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 (octave = log2 de la taille du cover de l'ancre)
vcensus nœuds={base} feuilles=1 range_add=1
p_factor={base}/{base}/{base} (evaluations d'auto-produits des histogrammes)
ledger_paires emis=1/1/1 tues=1/1/1
ouvriers wspd=8 rects=8 rle=8 prefiltre=8 census=8 expansion=8 fold=8
digest_all=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
"""


def build_campaign(root: Path) -> None:
    out = root / "out"
    out.mkdir(parents=True)
    status_lines = []
    for n in SIZES:
        for seed in SEEDS:
            base = n // 4  # croissance lineaire : pentes ~1 partout
            (out / f"{FAM}_{n}_s{seed}.txt").write_text(
                OUTPUT_TEMPLATE.format(fam=FAM, n=n, seed=seed, base=base, surv=base - 10))
            (out / f"{FAM}_{n}_s{seed}.txt.err").write_text("")
            status_lines.append(f"code=0 fam={FAM} n={n} seed={seed} secs=1")
    (root / "STATUS.txt").write_text("\n".join(status_lines) + "\nDONE")
    (root / "META.txt").write_text(
        f"recu=campagne synthetique de porte\nfamilles={FAM} ; n=8000 16000 32000 ; graines=3 4 5\n")


def run_pentes(root: Path) -> tuple[int, str, str]:
    r = subprocess.run([sys.executable, str(PENTES), str(root / "out")], capture_output=True, text=True)
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
        build_campaign(root)
        rc, out, err = run_pentes(root)
        check("nominal accepte", rc == 0 and f"== {FAM}" in out, f"rc={rc} err={err[:120]}")

        # Chaque falsification : rejetee (3) avec stdout VIDE.
        def falsify(name: str, mutate) -> None:
            with tempfile.TemporaryDirectory() as td2:
                r2 = Path(td2) / "campagne"
                shutil.copytree(root, r2)
                mutate(r2)
                rc2, out2, err2 = run_pentes(r2)
                check(name, rc2 == 3 and out2 == "", f"rc={rc2} stdout={len(out2)}o err={err2[:100]}")

        falsify("STATUS absent", lambda r: (r / "STATUS.txt").unlink())
        falsify("STATUS reduit a DONE", lambda r: (r / "STATUS.txt").write_text("DONE"))
        falsify("STATUS sans DONE terminal",
                lambda r: (r / "STATUS.txt").write_text((r / "STATUS.txt").read_text().replace("\nDONE", "\nNOT_DONE_YET")))
        falsify("tuple STATUS en trop (code=1)",
                lambda r: (r / "STATUS.txt").write_text((r / "STATUS.txt").read_text().replace(
                    "\nDONE", f"\ncode=1 fam={FAM} n=64000 seed=3 secs=1\nDONE")))
        falsify("famille entiere manquante (matrice elargie)",
                lambda r: (r / "META.txt").write_text((r / "META.txt").read_text().replace(
                    f"familles={FAM} ;", f"familles={FAM} fantome ;")))
        falsify("fichier .txt manquant", lambda r: (r / "out" / f"{FAM}_16000_s4.txt").unlink())
        falsify("fichier .err manquant", lambda r: (r / "out" / f"{FAM}_16000_s4.txt.err").unlink())
        falsify("fichier .txt en trop",
                lambda r: (r / "out" / f"{FAM}_64000_s3.txt").write_text("x"))
        falsify("stderr non vide", lambda r: (r / "out" / f"{FAM}_8000_s3.txt.err").write_text("boom"))
        falsify("compteur absent d'une graine",
                lambda r: (r / "out" / f"{FAM}_32000_s5.txt").write_text(
                    re.sub(r"tri_comparaisons=\d+ ", "", (r / "out" / f"{FAM}_32000_s5.txt").read_text())))
        falsify("identite discordante (seed)",
                lambda r: (r / "out" / f"{FAM}_8000_s4.txt").write_text(
                    (r / "out" / f"{FAM}_8000_s4.txt").read_text().replace("seed=4", "seed=9")))
        falsify("mode digest absent",
                lambda r: (r / "out" / f"{FAM}_8000_s3.txt").write_text(
                    (r / "out" / f"{FAM}_8000_s3.txt").read_text().replace("digest_all=", "digest_nope=")))

        # Zero legitime : accepte, pente indefinie affichee `-`, jamais un echec.
        with tempfile.TemporaryDirectory() as td3:
            r3 = Path(td3) / "campagne"
            shutil.copytree(root, r3)
            for n in SIZES:
                for seed in SEEDS:
                    p = r3 / "out" / f"{FAM}_{n}_s{seed}.txt"
                    p.write_text(re.sub(r"seeds_cellules=\d+/\d+", "seeds_cellules=0/0", p.read_text()))
            rc3, out3, _ = run_pentes(r3)
            check("zero legitime accepte", rc3 == 0, f"rc={rc3}")

    if failures:
        return 1
    print("pentes_gate : validateur fail-closed conforme (nominal + 12 falsifications + zero legitime)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
