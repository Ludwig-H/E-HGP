#!/usr/bin/env python3
"""Agrégateur inter-graines PRÉENREGISTRÉ (exigence de l'audit du 31 août :
« aucune conclusion mono-graine ; aucun seuil choisi après lecture »).

RÈGLE FIGÉE AVANT LA CAMPAGNE QU'IL JUGE (campagne_sonde_octaves_20260831
et suivantes) — provenance des seuils : GRAND_LIVRE.md § 3 (< 2 strictement,
gravé avant toute campagne) et la lecture post-hoc de la capture INVALIDE
518e2706 (RECU.md : la queue vit aux octaves ≥ 10) — antérieure à toute
sortie de la campagne jugée ici, jamais ajustée après.

1. Le dossier doit d'abord passer `bench/pentes.py` (fail-closed, identités
   fermantes) — l'agrégateur refuse sinon (code 3).
2. Termes jugés par famille : `W_sweep1_evals_coeur`, `m_anchor_q4`, et le
   terme de QUEUE `T_lourde = Σ_{octave >= 10} w1[o]` (défini ici, publié
   par graine).
3. Pente sécante par graine aux deux pas (log2 des rapports) ; agrégat
   inter-graines = MIN / MÉDIANE / MAX des trois graines (médiane de 3 =
   valeur centrale) ; une pente indéfinie (zéro) rend l'agrégat de ce terme
   indéfini `-` et ne déclenche JAMAIS.
4. VERDICT (binaire, sans interprétation) : pour chaque famille STATIONNAIRE
   ANNONCÉE (terrain_stationnaire, scanline_stationnaire), `E6_active=oui`
   ssi MÉDIANE inter-graines de la pente au pas 16000→32000 >= 2,0 pour AU
   MOINS UN des trois termes. Pour uniform et eight_clusters (garde-fou
   GO § 3) : `garde_fou_viole=oui` ssi une médiane >= 2,0.
5. Sortie : AGREGAT.txt dans le dossier parent de out/ (bufferisé, écrit
   seulement après validation complète), et le même contenu sur stdout.

Usage : agregateur.py <dossier out/ de campagne>
Codes : 0 = agrégat publié (quel que soit le verdict) ; 3 = campagne refusée.
"""

from __future__ import annotations

import importlib.util
import math
import re
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
PENTES = HERE / "pentes.py"
SIZES = [8000, 16000, 32000]
SEEDS = [3, 4, 5]
STATIONARY = ("terrain_stationnaire", "scanline_stationnaire")
GUARDED = ("uniform", "eight_clusters")
OCTAVE_LOURDE_MIN = 10  # fige AVANT la campagne jugee (voir en-tete)
SEUIL_PENTE = 2.0       # GRAND_LIVRE.md § 3, fige avant toute campagne


def med3(xs):
    xs = sorted(xs)
    return xs[1]


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: agregateur.py <dossier out/ de campagne>", file=sys.stderr)
        return 2
    out_dir = Path(sys.argv[1])
    # 1. La campagne doit passer le validateur fail-closed INTEGRALEMENT.
    r = subprocess.run([sys.executable, str(PENTES), str(out_dir)], capture_output=True, text=True)
    if r.returncode != 0:
        print(f"REFUS : pentes.py a rendu {r.returncode} — agrégat impossible sur une campagne non validée",
              file=sys.stderr)
        sys.stderr.write(r.stderr)
        return 3
    spec = importlib.util.spec_from_file_location("pentes", PENTES)
    pentes = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(pentes)

    meta = (out_dir.parent / "META.txt").read_text()
    families = re.search(r"^familles=(.+?) ;", meta, re.M).group(1).split()

    def counters_of(fam: str, n: int, seed: int) -> dict:
        text = (out_dir / f"{fam}_{n}_s{seed}.txt").read_text()
        vals = {}
        for cname, pat in pentes.PATTERNS:
            vals[cname] = int(re.search(pat, text).group(1))
        w1 = [int(x) for x in re.search(r" w1=([0-9,]+) \(octave", text).group(1).split(",")]
        vals["T_lourde"] = sum(w1[OCTAVE_LOURDE_MIN:])
        return vals

    lines = [
        "# AGREGAT inter-graines — règle préenregistrée (bench/agregateur.py, seuils figés avant la campagne).",
        f"# termes juges : W_sweep1_evals_coeur, m_anchor_q4, T_lourde (= somme w1 des octaves >= {OCTAVE_LOURDE_MIN}).",
        f"# verdict : mediane inter-graines de la pente 16000->32000 >= {SEUIL_PENTE} sur au moins un terme.",
    ]
    verdicts = []
    for fam in families:
        data = {(n, s): counters_of(fam, n, s) for n in SIZES for s in SEEDS}
        lines.append(f"== {fam}")
        med_by_term = {}
        for term in ("W_sweep1_evals_coeur", "m_anchor_q4", "T_lourde"):
            slopes2 = []
            rows = []
            for s in SEEDS:
                v = [data[(n, s)][term] for n in SIZES]
                s1 = math.log2(v[1] / v[0]) if v[0] > 0 and v[1] > 0 else None
                s2 = math.log2(v[2] / v[1]) if v[1] > 0 and v[2] > 0 else None
                rows.append((s, v, s1, s2))
                slopes2.append(s2)
            for s, v, s1, s2 in rows:
                fmt = lambda x: "  -  " if x is None else f"{x:5.2f}"
                lines.append(f"  {term:22s} g{s}: {v[0]:>13} {v[1]:>13} {v[2]:>13}  pentes {fmt(s1)} | {fmt(s2)}")
            if any(x is None for x in slopes2):
                med_by_term[term] = None
                lines.append(f"  {term:22s} agrégat pas2 : indéfini (zéro légitime) — ne déclenche pas")
            else:
                mn, md, mx = min(slopes2), med3(slopes2), max(slopes2)
                med_by_term[term] = md
                lines.append(f"  {term:22s} agrégat pas2 : min={mn:.2f} médiane={md:.2f} max={mx:.2f}")
        trig = [t for t, m in med_by_term.items() if m is not None and m >= SEUIL_PENTE]
        if fam in STATIONARY:
            verdict = f"E6_active={'oui' if trig else 'non'} famille={fam}" + \
                (f" termes={','.join(trig)}" if trig else "")
        elif fam in GUARDED:
            verdict = f"garde_fou_viole={'oui' if trig else 'non'} famille={fam}" + \
                (f" termes={','.join(trig)}" if trig else "")
        else:
            verdict = f"hors_perimetre famille={fam}"
        verdicts.append(verdict)
        lines.append(f"  VERDICT : {verdict}")
        lines.append("")
    lines.extend(verdicts)
    content = "\n".join(lines) + "\n"
    (out_dir.parent / "AGREGAT.txt").write_text(content)
    print(content, end="")
    return 0


if __name__ == "__main__":
    sys.exit(main())
