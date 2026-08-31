#!/usr/bin/env python3
"""PORTE E6 BORNÉE inter-graines — trois termes, pas 2 seulement (nommage
imposé par l'alerte du 31 août : ceci n'est PAS le garde-fou GO complet du
GRAND_LIVRE § 3, qui exige chaque terme payé sur les deux pas).

RÈGLE PRÉENREGISTRÉE (figée avant la campagne qu'elle juge ; provenance des
seuils : GRAND_LIVRE.md § 3, gravé avant toute campagne, et la lecture
post-hoc des captures antérieures — `T_lourde` est une HYPOTHÈSE DÉRIVÉE de
la première capture, dont la stabilité doit être testée sur une campagne
indépendante) :

1. Le dossier doit d'abord passer `bench/pentes.py` avec le MÊME profil
   d'autorité (fail-closed, provenance, identités fermantes) — refus sinon
   (code 3), et tout `AGREGAT.txt` préexistant est SUPPRIMÉ (jamais un
   agrégat périmé à côté d'un refus).
2. Termes jugés par famille : `W_sweep1_evals_coeur`, `m_anchor_q4`, et
   `T_lourde = Σ_{octave >= 10} w1[o]`.
3. Pente sécante par graine au pas 16000→32000 ; par terme :
   - toutes les valeurs > 0 : agrégat = MIN/MÉDIANE/MAX des trois graines ;
   - une transition 0 → positif chez AU MOINS une graine : le terme est
     classé `EMERGENCE` (indéterminé) — ce n'est NI un déclencheur NI une
     preuve négative (alerte : « une émergence de bin n'est pas encore une
     loi d'échelle ») ;
   - zéro aux deux tailles chez toutes les graines : `-` (indéfini, ne
     déclenche pas).
4. VERDICT (binaire, sans interprétation) : famille stationnaire annoncée ⟹
   `E6_active=oui` ssi la MÉDIANE inter-graines >= 2,0 sur au moins un terme
   NON indéterminé (la médiane exige donc au moins deux graines sur trois) ;
   les termes en émergence sont LISTÉS à part (`emergences=`). uniform /
   eight_clusters ⟹ `garde_fou_borne_viole` par la même médiane — garde-fou
   BORNÉ à ces trois termes et à ce pas, jamais le § 3 entier.
5. Sortie : AGREGAT.txt dans le dossier parent de out/ + stdout.

Usage : agregateur.py <dossier out/ de campagne> <profil d'autorite>
Codes : 0 = agrégat publié (quel que soit le verdict) ; 3 = campagne refusée
(AGREGAT.txt supprimé s'il existait).
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
# La matrice (tailles, graines) vient du profil d'autorite, comme pour
# pentes.py (profil de confirmation hors echantillon a tailles decalees).
STATIONARY = ("terrain_stationnaire", "scanline_stationnaire")
GUARDED = ("uniform", "eight_clusters")
OCTAVE_LOURDE_MIN = 10  # hypothese derivee de la premiere capture, figee ici
SEUIL_PENTE = 2.0       # GRAND_LIVRE.md § 3, fige avant toute campagne


def med3(xs):
    xs = sorted(xs)
    return xs[1]


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: agregateur.py <dossier out/ de campagne> <profil d'autorite>", file=sys.stderr)
        return 2
    out_dir = Path(sys.argv[1])
    authority = Path(sys.argv[2])
    agregat_path = out_dir.parent / "AGREGAT.txt"
    # 1. La campagne doit passer le validateur fail-closed INTEGRALEMENT,
    # avec le meme profil d'autorite ; un refus supprime l'agregat perime.
    r = subprocess.run([sys.executable, str(PENTES), str(out_dir), str(authority)],
                       capture_output=True, text=True)
    if r.returncode != 0:
        if agregat_path.exists():
            agregat_path.unlink()
        print(f"REFUS : pentes.py a rendu {r.returncode} — agrégat impossible sur une campagne non validée "
              f"(AGREGAT.txt supprimé s'il existait)", file=sys.stderr)
        sys.stderr.write(r.stderr)
        return 3
    spec = importlib.util.spec_from_file_location("pentes", PENTES)
    pentes = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(pentes)

    atext = authority.read_text()
    families = re.search(r"^familles=(.+)$", atext, re.M).group(1).split()
    sizes = [int(x) for x in re.search(r"^n=(.+)$", atext, re.M).group(1).split()]
    seeds = [int(x) for x in re.search(r"^graines=(.+)$", atext, re.M).group(1).split()]

    def counters_of(fam: str, n: int, seed: int) -> dict:
        text = (out_dir / f"{fam}_{n}_s{seed}.txt").read_text()
        vals = {}
        for cname, pat in pentes.PATTERNS:
            vals[cname] = int(re.search(pat, text).group(1))
        w1 = [int(x) for x in re.search(r" w1=([0-9,]+) \(octave", text).group(1).split(",")]
        vals["T_lourde"] = sum(w1[OCTAVE_LOURDE_MIN:])
        return vals

    lines = [
        "# AGREGAT inter-graines — PORTE E6 BORNEE (trois termes, pas 16000->32000 seulement ;",
        "# jamais le garde-fou GO complet du GRAND_LIVRE paragraphe 3). Regle preenregistree.",
        f"# termes : W_sweep1_evals_coeur, m_anchor_q4, T_lourde (= somme w1 des octaves >= {OCTAVE_LOURDE_MIN},",
        "# hypothese derivee de la premiere capture, stabilite a tester sur campagne independante).",
        f"# verdict : mediane inter-graines >= {SEUIL_PENTE} sur au moins un terme non indetermine ;",
        "# une transition 0->positif est une EMERGENCE (indeterminee), ni declencheur ni preuve negative.",
    ]
    verdicts = []
    for fam in families:
        data = {(n, s): counters_of(fam, n, s) for n in sizes for s in seeds}
        lines.append(f"== {fam}")
        med_by_term = {}
        emergences = []
        for term in ("W_sweep1_evals_coeur", "m_anchor_q4", "T_lourde"):
            slopes2 = []
            emergence = False
            rows = []
            for s in seeds:
                v = [data[(n, s)][term] for n in sizes]
                s1 = math.log2(v[1] / v[0]) if v[0] > 0 and v[1] > 0 else None
                s2 = math.log2(v[2] / v[1]) if v[1] > 0 and v[2] > 0 else None
                if v[1] == 0 and v[2] > 0:
                    emergence = True
                rows.append((s, v, s1, s2))
                slopes2.append(s2)
            for s, v, s1, s2 in rows:
                fmt = lambda x: "  -  " if x is None else f"{x:5.2f}"
                lines.append(f"  {term:22s} g{s}: {v[0]:>13} {v[1]:>13} {v[2]:>13}  pentes {fmt(s1)} | {fmt(s2)}")
            if emergence:
                med_by_term[term] = None
                emergences.append(term)
                lines.append(f"  {term:22s} agrégat pas2 : EMERGENCE (0 -> positif) — indéterminé, à classer par une campagne de tailles supérieures")
            elif any(x is None for x in slopes2):
                med_by_term[term] = None
                lines.append(f"  {term:22s} agrégat pas2 : indéfini (zéro des deux côtés) — ne déclenche pas")
            else:
                mn, md, mx = min(slopes2), med3(slopes2), max(slopes2)
                med_by_term[term] = md
                lines.append(f"  {term:22s} agrégat pas2 : min={mn:.2f} médiane={md:.2f} max={mx:.2f}")
        trig = [t for t, m in med_by_term.items() if m is not None and m >= SEUIL_PENTE]
        suffix = (f" termes={','.join(trig)}" if trig else "") + \
            (f" emergences={','.join(emergences)}" if emergences else "")
        if fam in STATIONARY:
            verdict = f"E6_active={'oui' if trig else 'non'} famille={fam}" + suffix
        elif fam in GUARDED:
            verdict = f"garde_fou_borne_viole={'oui' if trig else 'non'} famille={fam}" + suffix
        else:
            verdict = f"hors_perimetre famille={fam}"
        verdicts.append(verdict)
        lines.append(f"  VERDICT : {verdict}")
        lines.append("")
    lines.extend(verdicts)
    content = "\n".join(lines) + "\n"
    agregat_path.write_text(content)
    print(content, end="")
    return 0


if __name__ == "__main__":
    sys.exit(main())
