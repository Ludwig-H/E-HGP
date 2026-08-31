#!/usr/bin/env python3
"""Porte du validateur de campagne (bench/pentes.py) — FAIL-CLOSED prouvé.

Construit dans un répertoire temporaire une campagne synthétique minimale
conforme (matrice 1 famille × 3 tailles × 3 graines, compteurs complets,
identités fermantes des octaves SATISFAITES), vérifie que pentes.py
l'accepte (code 0, table imprimée), puis grave les falsifications exigées
par les audits du 31 août (deuxième recette + cinquième cycle) : chacune
doit être REFUSÉE (code 3) avec un stdout VIDE (aucune table partielle) et
sans traceback. Le cas « zéro légitime » porte sur un compteur RÉELLEMENT
parsé (racines_hors_corde), mis à zéro sur les trois tailles : code 0 exigé
ET pente indéfinie `-` affichée sur sa ligne.

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

# Identités fermantes satisfaites par construction : seeds_q4 = base, toutes
# les issues sur l'octave 0 en passe 2 (cellules/cœur/corde = 0, légitimes) ;
# Σ ancres == entrees_ancres_q4 == base ; Σ w1 == tests_coeur == base.
OCT_BASE = "{base},0,0,0,0,0,0,0,0,0,0,0,0,0,0,0"
OCT_ZERO = "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0"
OUTPUT_TEMPLATE = ("""payload=mhgp6-forests-horizontal-v1 authority=status_terminal callbacks=provisional vertical_maps=none
backend=cpu_reference
tower_scope=profile_complete_k10 smax_requested=11 smax_effective=11
famille={fam} n={n} coord=100 s=8 smax=11 seed={seed} threads=8 emis={base} boules_uniques={base} mortes_profondeur=10 survivantes={surv} census_int=50 census_shell=20 evenements={base} facettes={base} fusions=10 deltas=10 noeuds=10
generation rect_alive={base}/{base}/{base} rect_visites_fusionnes={base} ancres={base}/{base}/{base} candidats={base}/{base}/{base} tues_profondeur=1/{base}/{base} ancres_w4=1 ancres_w3=1 ancres_secteurs=1/1 ancres_cellules=0/0 seeds_cellules=0/0 grilles=0/0 seeds={base}/{base} completions_q4={base} seeds_core_tues=0 seeds_corde_tues=0 float_cert=1/1 repli={base} ancres_hist=1/1/1 hist_lignes=1/1/1 hist_seuil=1/1/1 hist_survivants={base}/{base}/{base} jung=1/1/1
sweep tests_coeur={base} tests_prof_q3={base} tests_passe2={base} tri_comparaisons={base} seeds_passe2={base} racines_corde={base} groupes={base} racines_hors_corde={base} temoins_constants=1 rejets=lens:1/owner:1/once:1/i64:1/face:1/det:0/centre:1
vwspd nœuds_temoins={base} coins={base} h_rect=1/{base}/{base} h_scan=0/{base}/{base} m_anchor=1/1/{base} entrees_ancres=0/{base}/{base} iters_coeur={base} iters_passe2={base}
octaves_q4 ancres=""" + OCT_BASE + " seeds=" + OCT_BASE + " w1=" + OCT_BASE +
                   """ (octave = log2 de la taille du cover de l'ancre)
octaves_q4_seeds cellules=""" + OCT_ZERO + " coeur=" + OCT_ZERO + " corde=" + OCT_ZERO + " passe2=" + OCT_BASE + """
vcensus prefiltre_nœuds={base} prefiltre_feuilles=0 range_add=1 census_nœuds={base} census_feuilles=1
p_factor={base}/{base}/{base} (evaluations d'auto-produits des histogrammes)
ledger_paires emis=1/1/1 tues=1/1/1
ouvriers wspd=8 rects=8 rle=8 prefiltre=8 census=8 expansion=8 fold=8
digest_all=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
""")


def sha256_bytes(data: bytes) -> str:
    import hashlib
    return hashlib.sha256(data).hexdigest()


def build_campaign(root: Path) -> None:
    out = root / "out"
    out.mkdir(parents=True)
    (root / "bin").mkdir()
    binary = b"#!/bin/sh\nexit 0\n"
    (root / "bin" / "mhgp6").write_bytes(binary)
    bin_sha = sha256_bytes(binary)
    profile = f"profil=porte_synthetique\nfamilles={FAM}\nn=8000 16000 32000\ngraines=3 4 5\n"
    (root / "PROFIL.txt").write_text(profile)
    (root / "PROFIL_AUTORITE.txt").write_text(profile)
    status_lines, hash_lines, out_hash_lines = [], [], []
    for n in SIZES:
        for seed in SEEDS:
            base = n // 4  # croissance lineaire : pentes ~1 partout
            body = OUTPUT_TEMPLATE.replace("{base}", str(base)).format(
                fam=FAM, n=n, seed=seed, base=base, surv=base - 10)
            (out / f"{FAM}_{n}_s{seed}.txt").write_text(body)
            (out / f"{FAM}_{n}_s{seed}.txt.err").write_text("")
            status_lines.append(f"code=0 fam={FAM} n={n} seed={seed} secs=1")
            hash_lines.append(f"avant={bin_sha} apres={bin_sha} run={FAM}_{n}_s{seed}")
            out_hash_lines.append(f"{sha256_bytes(body.encode())}  out/{FAM}_{n}_s{seed}.txt")
    (root / "STATUS.txt").write_text("\n".join(status_lines) + "\nDONE")
    (root / "HASHES.txt").write_text("\n".join(hash_lines) + "\n")
    (root / "META.txt").write_text(
        "recu=campagne synthetique de porte\npin=0000000000000000000000000000000000000000\n"
        f"sha256_binaire_prive={bin_sha}\n"
        "commande=bin/mhgp6 --family=<fam> --n=<n>\n"
        f"familles={FAM} ; n=8000 16000 32000 ; graines=3 4 5\n"
        + "\n".join(out_hash_lines) + "\n")


def rehash_outputs(root: Path) -> None:
    """Apres une mutation LEGITIME des sorties, remet les hashes du META en
    coherence (les falsifications de hash, elles, ne l'appellent pas)."""
    mtext = (root / "META.txt").read_text()
    for p in sorted((root / "out").glob("*.txt")):
        mtext = re.sub(rf"^[0-9a-f]{{64}}  out/{re.escape(p.name)}$",
                       f"{sha256_bytes(p.read_bytes())}  out/{p.name}", mtext, flags=re.M)
    (root / "META.txt").write_text(mtext)


def run_pentes(root: Path) -> tuple[int, str, str]:
    r = subprocess.run([sys.executable, str(PENTES), str(root / "out"), str(root / "PROFIL_AUTORITE.txt")],
                       capture_output=True, text=True)
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
        check("nominal accepte", rc == 0 and f"== {FAM}" in out, f"rc={rc} err={err[:200]}")

        # Chaque falsification : rejetee (3), stdout VIDE, sans traceback.
        # rehash=True remet les hashes de sorties du META en coherence apres
        # une mutation de contenu, pour que la falsification echoue sur SA
        # cause declaree et pas sur le recoupement de hash.
        def falsify(name: str, mutate, rehash: bool = False) -> None:
            with tempfile.TemporaryDirectory() as td2:
                r2 = Path(td2) / "campagne"
                shutil.copytree(root, r2)
                mutate(r2)
                if rehash:
                    rehash_outputs(r2)
                rc2, out2, err2 = run_pentes(r2)
                check(name, rc2 == 3 and out2 == "" and "Traceback" not in err2,
                      f"rc={rc2} stdout={len(out2)}o err={err2[:100]}")

        def edit(r: Path, fname: str, fn) -> None:
            p = r / "out" / fname
            p.write_text(fn(p.read_text()))

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
        falsify("famille dupliquee dans le META",
                lambda r: (r / "META.txt").write_text((r / "META.txt").read_text().replace(
                    f"familles={FAM} ;", f"familles={FAM} {FAM} ;")))
        falsify("entier invalide dans le META (sans traceback)",
                lambda r: (r / "META.txt").write_text((r / "META.txt").read_text().replace(
                    "n=8000 16000 32000 ;", "n=8000 16000 32000x ;")))
        falsify("fichier .txt manquant", lambda r: (r / "out" / f"{FAM}_16000_s4.txt").unlink())
        falsify("fichier .err manquant", lambda r: (r / "out" / f"{FAM}_16000_s4.txt.err").unlink())
        falsify("fichier .txt en trop",
                lambda r: (r / "out" / f"{FAM}_64000_s3.txt").write_text("x"))
        falsify("fichier d'extension inattendue",
                lambda r: (r / "out" / "intrus.xyz").write_text("x"))
        falsify("stderr non vide", lambda r: (r / "out" / f"{FAM}_8000_s3.txt.err").write_text("boom"))
        falsify("compteur absent d'une graine",
                lambda r: edit(r, f"{FAM}_32000_s5.txt", lambda t: re.sub(r"tri_comparaisons=\d+ ", "", t)), True)
        falsify("compteur duplique (ligne sweep doublee)",
                lambda r: edit(r, f"{FAM}_8000_s3.txt", lambda t: t.replace(
                    "p_factor=", "sweep tests_coeur=7 tests_prof_q3=7 tests_passe2=7 tri_comparaisons=7 "
                    "seeds_passe2=7 racines_corde=7 groupes=7 racines_hors_corde=7 temoins_constants=1 "
                    "rejets=lens:1/owner:1/once:1/i64:1/face:1/det:0/centre:1\np_factor=", 1)), True)
        falsify("identite discordante (seed)",
                lambda r: edit(r, f"{FAM}_8000_s4.txt", lambda t: t.replace("seed=4", "seed=9")), True)
        falsify("mode digest absent",
                lambda r: edit(r, f"{FAM}_8000_s3.txt", lambda t: t.replace("digest_all=", "digest_nope=")), True)
        falsify("digest_all duplique",
                lambda r: edit(r, f"{FAM}_8000_s3.txt", lambda t: t + "digest_all=" + "b" * 64 + "\n"), True)
        falsify("digest_all non hexadecimal",
                lambda r: edit(r, f"{FAM}_8000_s3.txt", lambda t: t.replace("a" * 64, "z" * 64)), True)
        falsify("identite fermante violee (somme des octaves)",
                lambda r: edit(r, f"{FAM}_8000_s5.txt", lambda t: t.replace(
                    " w1=2000,0", " w1=2001,0", 1)), True)
        falsify("identite par octave violee (seeds != cel+coeur+corde+passe2)",
                lambda r: edit(r, f"{FAM}_8000_s5.txt", lambda t: t.replace(
                    " seeds=2000,0", " seeds=2000,0".replace("2000,0", "1999,1"), 1)), True)

        falsify("profil d'autorite absent", lambda r: (r / "PROFIL_AUTORITE.txt").unlink())
        falsify("PROFIL.txt divergent de l'autorite",
                lambda r: (r / "PROFIL.txt").write_text((r / "PROFIL.txt").read_text() + "x\n"))
        falsify("familles du profil vides",
                lambda r: [p.write_text(re.sub(r"^familles=.*$", "familles=", p.read_text(), flags=re.M))
                           for p in (r / "PROFIL.txt", r / "PROFIL_AUTORITE.txt")] and None)
        falsify("seconde ligne de matrice au META",
                lambda r: (r / "META.txt").write_text(
                    (r / "META.txt").read_text() + "familles=fantome ; n=8000 16000 32000 ; graines=3 4 5\n"))
        falsify("lien symbolique dans out/",
                lambda r: ((r / "out" / f"{FAM}_8000_s3.txt.err").unlink(),
                           (r / "out" / f"{FAM}_8000_s3.txt.err").symlink_to("/dev/null"))[-1])
        falsify("sous-repertoire dans out/", lambda r: (r / "out" / "intrusdir").mkdir())
        falsify("hash de sortie falsifie (META non recoupe)",
                lambda r: edit(r, f"{FAM}_8000_s3.txt", lambda t: t.replace("coord=100", "coord=101")))
        falsify("HASHES.txt heterogene",
                lambda r: (r / "HASHES.txt").write_text(
                    (r / "HASHES.txt").read_text().replace("avant=", "avant=0deadbeef", 1)))
        falsify("binaire prive absent", lambda r: (r / "bin" / "mhgp6").unlink())
        falsify("binaire prive discordant",
                lambda r: (r / "bin" / "mhgp6").write_bytes(b"autre binaire"))
        falsify("STATUT_TERMINAL present (campagne invalidee)",
                lambda r: (r / "STATUT_TERMINAL.txt").write_text("statut_terminal=invalide"))

        # Zero legitime sur un compteur REELLEMENT parse (audit du cinquieme
        # cycle : l'ancien cas portait sur seeds_cellules, absent de la liste,
        # et ne verifiait pas le `-`) : racines_hors_corde mis a zero sur les
        # trois tailles -> code 0 ET pente indefinie `-` sur sa ligne.
        with tempfile.TemporaryDirectory() as td3:
            r3 = Path(td3) / "campagne"
            shutil.copytree(root, r3)
            for n in SIZES:
                for seed in SEEDS:
                    p = r3 / "out" / f"{FAM}_{n}_s{seed}.txt"
                    p.write_text(re.sub(r"racines_hors_corde=\d+", "racines_hors_corde=0", p.read_text()))
            rehash_outputs(r3)
            rc3, out3, _ = run_pentes(r3)
            row = next((ln for ln in out3.splitlines() if ln.startswith("sweep_hors_corde")), "")
            check("zero legitime accepte avec pente `-` affichee",
                  rc3 == 0 and "  -  " in row, f"rc={rc3} row={row!r}")

    if failures:
        return 1
    print("pentes_gate : validateur fail-closed conforme (nominal + 31 falsifications dont provenance + zero legitime avec `-`)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
