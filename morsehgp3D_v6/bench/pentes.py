#!/usr/bin/env python3
"""Pentes sécantes locales par terme du grand-livre (doctrine REGIMES.md § 4).

FAIL-CLOSED (audit du 31 août, deuxième recette) : la matrice exacte
familles × tailles × graines vient du META ; STATUS.txt et le contenu du
dossier de sorties doivent correspondre EXACTEMENT à cette matrice (aucun
tuple ni fichier supplémentaire, aucun manquant) ; l'identité de chaque
sortie est recoupée (famille, n, seed, s, smax, threads, présence des
digests) ; chaque compteur est exigé dans chaque sortie (absence = échec,
zéro légitime = pente indéfinie affichée `-`) ; TOUTE la validation précède
le premier octet de table sur stdout (les tables sont bufferisées).
Les temps ne sont jamais lus : compteurs déterministes seulement.
"""

from __future__ import annotations

import math
import re
import sys
from pathlib import Path

SIZES = [8000, 16000, 32000]
SEEDS = [3, 4, 5]
EXPECTED_S = 8
EXPECTED_SMAX = 11
EXPECTED_THREADS = 8

PATTERNS = [
    ("rect_visites", r"rect_visites_fusionnes=(\d+)"),
    ("vwspd_noeuds", r"vwspd nœuds_temoins=(\d+)"),
    ("vwspd_coins", r"coins=(\d+) h_rect"),
    ("h_rect_q3", r"h_rect=\d+/(\d+)/\d+"),
    ("h_rect_q4", r"h_rect=\d+/\d+/(\d+)"),
    ("m_anchor_q3", r"m_anchor=\d+/(\d+)/\d+"),
    ("m_anchor_q4", r"m_anchor=\d+/\d+/(\d+)"),
    ("iters_coeur", r"iters_coeur=(\d+)"),
    ("iters_passe2", r"iters_passe2=(\d+)"),
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
    ("W_sweep1_evals_coeur", r"tests_coeur=(\d+)"),
    ("W_scan_q3", r"tests_prof_q3=(\d+)"),
    ("W_sweep2_evals_passe2", r"tests_passe2=(\d+)"),
    ("tri_comparaisons", r"tri_comparaisons=(\d+)"),
    ("P_factor_q2", r"p_factor=(\d+)/\d+/\d+"),
    ("P_factor_q3", r"p_factor=\d+/(\d+)/\d+"),
    ("P_factor_q4", r"p_factor=\d+/\d+/(\d+)"),
    ("V_census_noeuds", r"vcensus nœuds=(\d+)"),
    ("sweep_seeds_p2", r"seeds_passe2=(\d+)"),
    ("sweep_racines", r"racines_corde=(\d+)"),
    ("sweep_groupes", r"groupes=(\d+)"),
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


def fail(msg: str) -> int:
    print(f"ECHEC : {msg}", file=sys.stderr)
    return 3


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: pentes.py <dossier out/ de campagne>", file=sys.stderr)
        return 2
    out_dir = Path(sys.argv[1])
    meta = out_dir.parent / "META.txt"
    status = out_dir.parent / "STATUS.txt"
    if not meta.is_file() or not status.is_file():
        return fail("META.txt ou STATUS.txt absent")
    mfam = re.search(r"^familles=(.+?) ; n=(.+?) ; graines=(.+)$", meta.read_text(), re.M)
    if not mfam:
        return fail("matrice familles/n/graines absente du META")
    families = mfam.group(1).split()
    sizes = [int(x) for x in mfam.group(2).split()]
    seeds = [int(x) for x in mfam.group(3).split()]
    if sizes != SIZES or seeds != SEEDS:
        return fail("matrice du META differente de celle de l'analyse")
    expected = {(f, n, s) for f in families for n in sizes for s in seeds}

    # STATUS : derniere ligne exactement DONE, puis bijection avec la matrice.
    st_lines = status.read_text().splitlines()
    if not st_lines or st_lines[-1] != "DONE":
        return fail("STATUS.txt ne finit pas par DONE")
    seen: dict[tuple[str, int, int], int] = {}
    for line in st_lines[:-1]:
        m = re.match(r"^code=(\d+) fam=(\S+) n=(\d+) seed=(\d+) secs=\d+$", line)
        if not m:
            return fail(f"ligne STATUS invalide : {line}")
        key = (m.group(2), int(m.group(3)), int(m.group(4)))
        if key in seen:
            return fail(f"tuple duplique dans STATUS : {key}")
        seen[key] = int(m.group(1))
    if set(seen) != expected:
        extra = set(seen) - expected
        missing = expected - set(seen)
        return fail(f"STATUS != matrice (en trop : {sorted(extra)[:3]}, manquants : {sorted(missing)[:3]})")
    bad = [k for k, c in seen.items() if c != 0]
    if bad:
        return fail(f"{len(bad)} tuple(s) a code non nul : {sorted(bad)[:3]}")

    # Dossier de sorties : bijection exacte des fichiers avec la matrice.
    txts = {p.name for p in out_dir.glob("*.txt")}
    errs = {p.name for p in out_dir.glob("*.err")}
    want_txts = {f"{f}_{n}_s{s}.txt" for (f, n, s) in expected}
    want_errs = {f"{f}_{n}_s{s}.txt.err" for (f, n, s) in expected}
    if txts != want_txts:
        return fail(f"fichiers .txt != matrice (en trop : {sorted(txts - want_txts)[:3]}, "
                    f"manquants : {sorted(want_txts - txts)[:3]})")
    if errs != want_errs:
        return fail(f"fichiers .err != matrice (en trop : {sorted(errs - want_errs)[:3]}, "
                    f"manquants : {sorted(want_errs - errs)[:3]})")
    for name in want_errs:
        if (out_dir / name).stat().st_size > 0:
            return fail(f"stderr non vide : {name}")

    # Identites et compteurs : TOUT valide avant le premier octet de table.
    data: dict[str, dict[tuple[int, int], dict[str, int]]] = {f: {} for f in families}
    for fam, n, seed in sorted(expected):
        path = out_dir / f"{fam}_{n}_s{seed}.txt"
        text = path.read_text()
        ident = re.search(r"famille=(\S+) n=(\d+) coord=\d+ s=(\d+) smax=(\d+) seed=(\d+) threads=(\d+)", text)
        if not ident:
            return fail(f"ligne d'identite absente : {path.name}")
        got = (ident.group(1), int(ident.group(2)), int(ident.group(5)))
        if got != (fam, n, seed):
            return fail(f"identite discordante dans {path.name} : {got}")
        if (int(ident.group(3)), int(ident.group(4)), int(ident.group(6))) != (EXPECTED_S, EXPECTED_SMAX,
                                                                               EXPECTED_THREADS):
            return fail(f"parametres s/smax/threads inattendus dans {path.name}")
        if "digest_all=" not in text:
            return fail(f"mode digest absent de {path.name}")
        counters: dict[str, int] = {}
        for cname, pat in PATTERNS:
            m = re.search(pat, text)
            if m is None:
                return fail(f"compteur {cname} absent de {path.name}")
            counters[cname] = int(m.group(1))
        data[fam][(n, seed)] = counters

    # Tables : bufferisees, imprimees seulement apres validation complete.
    out: list[str] = []
    for fam in families:
        out.append(f"== {fam} — pentes sécantes 8000→16000 | 16000→32000 (graines {SEEDS})")
        out.append(f"{'compteur':24s} {'pas1 (par graine)':26s} {'pas2 (par graine)':26s} {'étendues p1|p2':12s}")
        for cname, _ in PATTERNS:
            rows1, rows2 = [], []
            for seed in SEEDS:
                v = [data[fam][(n, seed)][cname] for n in SIZES]
                rows1.append(math.log2(v[1] / v[0]) if v[0] > 0 and v[1] > 0 else None)
                rows2.append(math.log2(v[2] / v[1]) if v[1] > 0 and v[2] > 0 else None)
            fmt = lambda rows: "/".join("  -  " if r is None else f"{r:5.2f}" for r in rows)
            s1 = [r for r in rows1 if r is not None]
            s2 = [r for r in rows2 if r is not None]
            sp1 = max(s1) - min(s1) if s1 else 0.0
            sp2 = max(s2) - min(s2) if s2 else 0.0
            out.append(f"{cname:24s} {fmt(rows1):26s} {fmt(rows2):26s} {sp1:5.2f}|{sp2:5.2f}")
        out.append("")
    print("\n".join(out))
    return 0


if __name__ == "__main__":
    sys.exit(main())
