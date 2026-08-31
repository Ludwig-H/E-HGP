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


def read_counters(path: Path) -> dict[str, int]:
    text = path.read_text()
    out: dict[str, int] = {}
    for name, pat in PATTERNS:
        m = re.search(pat, text)
        if m:
            out[name] = int(m.group(1))
    return out


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: pentes.py <dossier out/ de campagne>", file=sys.stderr)
        return 2
    out_dir = Path(sys.argv[1])
    families = sorted({p.name.rsplit("_", 2)[0] for p in out_dir.glob("*_s*.txt")})
    if not families:
        print("aucune sortie trouvée", file=sys.stderr)
        return 2
    for fam in families:
        data: dict[tuple[int, int], dict[str, int]] = {}
        for n in SIZES:
            for seed in SEEDS:
                p = out_dir / f"{fam}_{n}_s{seed}.txt"
                if p.is_file():
                    data[(n, seed)] = read_counters(p)
        if len(data) < 6:
            print(f"== {fam} : campagne incomplète ({len(data)}/9 runs), ignorée")
            continue
        print(f"== {fam} — pentes sécantes 8000→16000 | 16000→32000 (graines {SEEDS})")
        header = f"{'compteur':24s} {'pas1 (par graine)':26s} {'pas2 (par graine)':26s} {'étendue':8s}"
        print(header)
        for name, _ in PATTERNS:
            rows1, rows2 = [], []
            for seed in SEEDS:
                v = [data.get((n, seed), {}).get(name) for n in SIZES]
                if None in v or 0 in v[:2]:
                    rows1.append(None)
                    rows2.append(None)
                    continue
                rows1.append(math.log2(v[1] / v[0]) if v[0] else None)
                rows2.append(math.log2(v[2] / v[1]) if v[1] and v[2] else None)
            if all(r is None for r in rows1):
                continue
            fmt = lambda rows: "/".join("  -  " if r is None else f"{r:5.2f}" for r in rows)
            vals = [r for r in rows1 + rows2 if r is not None]
            spread = max(vals) - min(vals) if vals else 0.0
            print(f"{name:24s} {fmt(rows1):26s} {fmt(rows2):26s} {spread:6.2f}")
        print()
    return 0


if __name__ == "__main__":
    sys.exit(main())
