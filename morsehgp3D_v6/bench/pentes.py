#!/usr/bin/env python3
"""Pentes sécantes locales par terme du grand-livre (doctrine REGIMES.md § 4).

Lit les sorties d'une campagne (receipts/campagne_*/out/<fam>_<n>_s<seed>.txt),
extrait les compteurs déterministes et imprime, par famille et par compteur,
les pentes sécantes des deux pas 8000→16000→32000 pour chaque graine, plus
l'étendue inter-graines. Aucune somme de termes, aucun ajustement global.
Les temps ne sont jamais lus : compteurs seulement.
"""

from __future__ import annotations

import math
import re
import sys
from pathlib import Path

SIZES = [8000, 16000, 32000]
SEEDS = [3, 4, 5]

# (nom publié, regex d'extraction) — groupes u64 ; les champs a/b/c des
# triplets par lane sont éclatés en colonnes distinctes.
PATTERNS = [
    ("rect_visites", r"rect_visites_fusionnes=(\d+)"),
    ("rect_alive_q2", r"rect_alive=(\d+)/\d+/\d+"),
    ("rect_alive_q3", r"rect_alive=\d+/(\d+)/\d+"),
    ("rect_alive_q4", r"rect_alive=\d+/\d+/(\d+)"),
    ("ancres_q3", r"ancres=\d+/(\d+)/\d+"),
    ("ancres_q4", r"ancres=\d+/\d+/(\d+)"),
    ("hist_survivants_q3", r"hist_survivants=\d+/(\d+)/\d+"),
    ("hist_survivants_q4", r"hist_survivants=\d+/\d+/(\d+)"),
    ("seeds_q3", r"seeds=(\d+)/\d+ "),
    ("seeds_q4", r"seeds=\d+/(\d+) "),
    ("seeds_core_tues", r"seeds_core_tues=(\d+)"),
    ("seeds_corde_tues", r"seeds_corde_tues=(\d+)"),
    ("W_sweep1_tests_coeur", r"tests_coeur=(\d+)"),
    ("W_scan_q3", r"tests_prof_q3=(\d+)"),
    ("W_sweep2_tests_passe2", r"tests_passe2=(\d+)"),
    ("tri_comparaisons", r"tri_comparaisons=(\d+)"),
    ("P_factor_q3", r"p_factor=\d+/(\d+)/\d+"),
    ("P_factor_q4", r"p_factor=\d+/\d+/(\d+)"),
    ("sweep_seeds_p2", r"seeds_passe2=(\d+)"),
    ("sweep_racines", r"racines_corde=(\d+)"),
    ("sweep_hors_corde", r"racines_hors_corde=(\d+)"),
    ("P_role", r"completions_q4=(\d+)"),
    ("candidats_q3", r"candidats=\d+/(\d+)/\d+"),
    ("candidats_q4", r"candidats=\d+/\d+/(\d+)"),
    ("tues_profondeur_q4", r"tues_profondeur=\d+/\d+/(\d+)"),
    ("emis", r"emis=(\d+)"),
    ("boules_uniques", r"boules_uniques=(\d+)"),
    ("evenements", r"evenements=(\d+)"),
    ("facettes", r"facettes=(\d+)"),
    ("float_repli", r"repli=(\d+) "),
]


def read_counters(path: Path) -> dict[str, int] | None:
    """Chaque compteur du grand-livre DOIT etre present (zero legitime accepte,
    absence = echec) — exigence fail-closed de l'audit du 31 aout."""
    text = path.read_text()
    out: dict[str, int] = {}
    for name, pat in PATTERNS:
        m = re.search(pat, text)
        if m is None:
            print(f"ECHEC : compteur {name} absent de {path.name}", file=sys.stderr)
            return None
        out[name] = int(m.group(1))
    return out


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: pentes.py <dossier out/ de campagne>", file=sys.stderr)
        return 2
    out_dir = Path(sys.argv[1])
    # VALIDATEUR FAIL-CLOSED (audit du 31 aout) : la matrice attendue vient du
    # META (`familles=`), jamais des sorties presentes ; STATUS.txt doit
    # exister, finir par DONE et porter EXACTEMENT un code=0 par tuple ; un
    # .txt ET un .err vide par tuple, identite recoupee dans le .txt. Tout
    # manquement echoue AVANT le moindre stdout de table.
    meta = out_dir.parent / "META.txt"
    status = out_dir.parent / "STATUS.txt"
    if not meta.is_file() or not status.is_file():
        print("ECHEC : META.txt ou STATUS.txt absent", file=sys.stderr)
        return 3
    mfam = re.search(r"^familles=(.+?) ; n=(.+?) ; graines=(.+)$", meta.read_text(), re.M)
    if not mfam:
        print("ECHEC : matrice familles/n/graines absente du META", file=sys.stderr)
        return 3
    families = mfam.group(1).split()
    sizes = [int(x) for x in mfam.group(2).split()]
    seeds = [int(x) for x in mfam.group(3).split()]
    if sizes != SIZES or seeds != SEEDS:
        print("ECHEC : matrice du META differente de celle de l'analyse", file=sys.stderr)
        return 3
    st_lines = status.read_text().splitlines()
    if not st_lines or st_lines[-1] != "DONE":
        print("ECHEC : STATUS.txt ne finit pas par DONE", file=sys.stderr)
        return 3
    codes = {}
    for line in st_lines[:-1]:
        m = re.match(r"^code=(\d+) fam=(\S+) n=(\d+) seed=(\d+) ", line)
        if not m:
            print(f"ECHEC : ligne STATUS invalide : {line}", file=sys.stderr)
            return 3
        key = (m.group(2), int(m.group(3)), int(m.group(4)))
        if key in codes:
            print(f"ECHEC : tuple duplique dans STATUS : {key}", file=sys.stderr)
            return 3
        codes[key] = int(m.group(1))
    for fam in families:
        for n in sizes:
            for seed in seeds:
                key = (fam, n, seed)
                if codes.get(key) != 0:
                    print(f"ECHEC : code absent ou non nul pour {key}", file=sys.stderr)
                    return 3
                txt = out_dir / f"{fam}_{n}_s{seed}.txt"
                err = out_dir / f"{fam}_{n}_s{seed}.txt.err"
                if not txt.is_file() or not err.is_file():
                    print(f"ECHEC : sortie ou stderr manquant pour {key}", file=sys.stderr)
                    return 3
                if err.stat().st_size > 0:
                    print(f"ECHEC : stderr non vide pour {key}", file=sys.stderr)
                    return 3
                ident = re.search(r"famille=(\S+) n=(\d+) .* seed=(\d+)", txt.read_text())
                if not ident or ident.group(1) != fam or int(ident.group(2)) != n or int(ident.group(3)) != seed:
                    print(f"ECHEC : identite de la sortie discordante pour {key}", file=sys.stderr)
                    return 3
    if not families:
        print("aucune sortie trouvée", file=sys.stderr)
        return 2
    for fam in families:
        data: dict[tuple[int, int], dict[str, int]] = {}
        for n in SIZES:
            for seed in SEEDS:
                counters = read_counters(out_dir / f"{fam}_{n}_s{seed}.txt")
                if counters is None:
                    return 3
                data[(n, seed)] = counters
        print(f"== {fam} — pentes sécantes 8000→16000 | 16000→32000 (graines {SEEDS})")
        header = f"{'compteur':24s} {'pas1 (par graine)':26s} {'pas2 (par graine)':26s} {'étendues p1|p2':12s}"
        print(header)
        for name, _ in PATTERNS:
            rows1, rows2 = [], []
            for seed in SEEDS:
                v = [data[(n, seed)][name] for n in SIZES]
                # Zero legitime : la pente est mathematiquement indefinie —
                # affichee `-`, jamais confondue avec une absence (echec plus haut).
                rows1.append(math.log2(v[1] / v[0]) if v[0] > 0 and v[1] > 0 else None)
                rows2.append(math.log2(v[2] / v[1]) if v[1] > 0 and v[2] > 0 else None)
            fmt = lambda rows: "/".join("  -  " if r is None else f"{r:5.2f}" for r in rows)
            s1 = [r for r in rows1 if r is not None]
            s2 = [r for r in rows2 if r is not None]
            sp1 = max(s1) - min(s1) if s1 else 0.0
            sp2 = max(s2) - min(s2) if s2 else 0.0
            print(f"{name:24s} {fmt(rows1):26s} {fmt(rows2):26s} {sp1:5.2f}|{sp2:5.2f}")
        print()
    return 0


if __name__ == "__main__":
    sys.exit(main())
