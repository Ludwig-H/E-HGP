#!/usr/bin/env python3
"""Pentes sécantes locales par terme du grand-livre (doctrine REGIMES.md § 4).

FAIL-CLOSED (audits du 31 août, deuxième recette puis cinquième cycle) : la
matrice exacte familles × tailles × graines vient du META (familles sans
doublon, entiers stricts, échec propre sans traceback) ; STATUS.txt et le
contenu du dossier de sorties doivent correspondre EXACTEMENT à cette matrice
(aucun tuple ni fichier supplémentaire — quelle que soit son extension —,
aucun manquant) ; l'identité de chaque sortie est recoupée (famille, n, seed,
s, smax, threads, digest_all unique et hexadécimal) ; chaque compteur est
exigé dans chaque sortie et doit y apparaître EXACTEMENT une fois (absence ou
doublon = échec, zéro légitime = pente indéfinie affichée `-`) ; les vecteurs
d'octaves de la sonde q4 sont parsés et leurs IDENTITÉS FERMANTES vérifiées
(Σ ancres == entrées_ancres_q4, Σ w1 == tests_cœur, Σ seeds == seeds_q4, et
par octave seeds == cellules + cœur + corde + passe2) ; TOUTE la validation
précède le premier octet de table sur stdout (les tables sont bufferisées).
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
    ("h_rect_q4", r"h_rect=\d+/\d+/(\d+) h_scan"),
    ("h_scan_q3", r"h_scan=\d+/(\d+)/\d+"),
    ("h_scan_q4", r"h_scan=\d+/\d+/(\d+) m_anchor"),
    ("m_anchor_q3", r"m_anchor=\d+/(\d+)/\d+"),
    ("m_anchor_q4", r"m_anchor=\d+/\d+/(\d+) entrees_ancres"),
    ("entrees_ancres_q3", r"entrees_ancres=\d+/(\d+)/\d+"),
    ("entrees_ancres_q4", r"entrees_ancres=\d+/\d+/(\d+) iters_coeur"),
    ("iters_coeur", r"iters_coeur=(\d+)"),
    ("iters_passe2", r"iters_passe2=(\d+)"),
    ("rect_alive_q2", r"rect_alive=(\d+)/\d+/\d+"),
    ("rect_alive_q3", r"rect_alive=\d+/(\d+)/\d+"),
    ("rect_alive_q4", r"rect_alive=\d+/\d+/(\d+)"),
    ("ancres_q3", r" ancres=\d+/(\d+)/\d+"),
    ("ancres_q4", r" ancres=\d+/\d+/(\d+)"),
    ("hist_survivants_q3", r"hist_survivants=\d+/(\d+)/\d+"),
    ("hist_survivants_q4", r"hist_survivants=\d+/\d+/(\d+)"),
    ("seeds_q3", r"seeds=(\d+)/\d+ "),
    ("seeds_q4", r"seeds=\d+/(\d+) "),
    ("seeds_cellules_q4", r"seeds_cellules=\d+/(\d+)"),
    ("seeds_core_tues", r"seeds_core_tues=(\d+)"),
    ("seeds_corde_tues", r"seeds_corde_tues=(\d+)"),
    ("W_sweep1_evals_coeur", r"tests_coeur=(\d+)"),
    ("W_scan_q3", r"tests_prof_q3=(\d+)"),
    ("W_sweep2_evals_passe2", r"tests_passe2=(\d+)"),
    ("tri_comparaisons", r"tri_comparaisons=(\d+)"),
    ("P_factor_q2", r"p_factor=(\d+)/\d+/\d+"),
    ("P_factor_q3", r"p_factor=\d+/(\d+)/\d+"),
    ("P_factor_q4", r"p_factor=\d+/\d+/(\d+)"),
    ("V_prefiltre_noeuds", r"vcensus prefiltre_nœuds=(\d+)"),
    ("V_prefiltre_range_add", r"range_add=(\d+)"),
    ("V_census_noeuds", r"census_nœuds=(\d+)"),
    ("V_census_feuilles", r"census_feuilles=(\d+)"),
    ("sweep_seeds_p2", r"seeds_passe2=(\d+)"),
    ("sweep_racines", r"racines_corde=(\d+)"),
    ("sweep_groupes", r"groupes=(\d+)"),
    ("sweep_hors_corde", r"racines_hors_corde=(\d+)"),
    ("P_role", r"completions_q4=(\d+)"),
    ("candidats_q3", r"candidats=\d+/(\d+)/\d+"),
    ("candidats_q4", r"candidats=\d+/\d+/(\d+)"),
    ("tues_profondeur_q4", r"tues_profondeur=\d+/\d+/(\d+)"),
    ("emis", r"emis=(\d+) boules_uniques"),
    ("boules_uniques", r"boules_uniques=(\d+)"),
    ("evenements", r"census_shell=\d+ evenements=(\d+)"),
    ("facettes", r"facettes=(\d+) fusions"),
    ("float_repli", r"repli=(\d+) "),
]

# Vecteurs d'octaves de la sonde q4 (16 composantes chacun) et leurs
# identités fermantes contre les scalaires ci-dessus.
OCTAVE_VECTORS = [
    ("oct_ancres", r"octaves_q4 ancres=([0-9,]+) seeds="),
    ("oct_seeds", r"octaves_q4 ancres=[0-9,]+ seeds=([0-9,]+) w1="),
    ("oct_w1", r" w1=([0-9,]+) \(octave"),
    ("oct_cellules", r"octaves_q4_seeds cellules=([0-9,]+) coeur="),
    ("oct_coeur", r" coeur=([0-9,]+) corde="),
    ("oct_corde", r" corde=([0-9,]+) passe2="),
    ("oct_passe2", r" passe2=([0-9,]+)"),
]
OCTAVE_SUM_IDENTITIES = [
    ("oct_ancres", "entrees_ancres_q4"),
    ("oct_seeds", "seeds_q4"),
    ("oct_w1", "W_sweep1_evals_coeur"),
    ("oct_cellules", "seeds_cellules_q4"),
    ("oct_coeur", "seeds_core_tues"),
    ("oct_corde", "seeds_corde_tues"),
    ("oct_passe2", "sweep_seeds_p2"),
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
    if len(families) != len(set(families)):
        return fail("famille dupliquee dans le META")
    try:
        sizes = [int(x) for x in mfam.group(2).split()]
        seeds = [int(x) for x in mfam.group(3).split()]
    except ValueError:
        return fail("entier invalide dans la matrice du META")
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

    # Dossier de sorties : bijection exacte, TOUTE extension confondue (un
    # fichier d'extension inattendue est refuse, audit du cinquieme cycle).
    all_files = {p.name for p in out_dir.iterdir() if p.is_file()}
    want_txts = {f"{f}_{n}_s{s}.txt" for (f, n, s) in expected}
    want_errs = {f"{f}_{n}_s{s}.txt.err" for (f, n, s) in expected}
    want_all = want_txts | want_errs
    if all_files != want_all:
        return fail(f"fichiers != matrice (en trop : {sorted(all_files - want_all)[:3]}, "
                    f"manquants : {sorted(want_all - all_files)[:3]})")
    for name in want_errs:
        if (out_dir / name).stat().st_size > 0:
            return fail(f"stderr non vide : {name}")

    # Identites, compteurs (exactement une occurrence chacun) et identites
    # fermantes des octaves : TOUT valide avant le premier octet de table.
    data: dict[str, dict[tuple[int, int], dict[str, int]]] = {f: {} for f in families}
    for fam, n, seed in sorted(expected):
        path = out_dir / f"{fam}_{n}_s{seed}.txt"
        text = path.read_text()
        idents = re.findall(r"famille=(\S+) n=(\d+) coord=\d+ s=(\d+) smax=(\d+) seed=(\d+) threads=(\d+)", text)
        if len(idents) != 1:
            return fail(f"ligne d'identite absente ou dupliquee : {path.name}")
        ident = idents[0]
        if (ident[0], int(ident[1]), int(ident[4])) != (fam, n, seed):
            return fail(f"identite discordante dans {path.name} : {ident[:2] + ident[4:5]}")
        if (int(ident[2]), int(ident[3]), int(ident[5])) != (EXPECTED_S, EXPECTED_SMAX, EXPECTED_THREADS):
            return fail(f"parametres s/smax/threads inattendus dans {path.name}")
        digests = re.findall(r"^digest_all=([0-9a-f]{64})$", text, re.M)
        if len(digests) != 1 or len(re.findall(r"^digest_all=", text, re.M)) != 1:
            return fail(f"digest_all absent, duplique ou non hexadecimal dans {path.name}")
        counters: dict[str, int] = {}
        for cname, pat in PATTERNS:
            ms = re.findall(pat, text)
            if len(ms) != 1:
                return fail(f"compteur {cname} absent ou duplique ({len(ms)} occurrences) dans {path.name}")
            counters[cname] = int(ms[0])
        vectors: dict[str, list[int]] = {}
        for vname, pat in OCTAVE_VECTORS:
            ms = re.findall(pat, text)
            if len(ms) != 1:
                return fail(f"vecteur {vname} absent ou duplique dans {path.name}")
            vec = [int(x) for x in ms[0].split(",")]
            if len(vec) != 16:
                return fail(f"vecteur {vname} a {len(vec)} composantes (16 attendues) dans {path.name}")
            vectors[vname] = vec
        for vname, cname in OCTAVE_SUM_IDENTITIES:
            if sum(vectors[vname]) != counters[cname]:
                return fail(f"identite fermante violee : somme({vname}) != {cname} dans {path.name}")
        for o in range(16):
            total = vectors["oct_cellules"][o] + vectors["oct_coeur"][o] + vectors["oct_corde"][o] + \
                vectors["oct_passe2"][o]
            if vectors["oct_seeds"][o] != total:
                return fail(f"identite par octave violee (octave {o}) dans {path.name}")
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
