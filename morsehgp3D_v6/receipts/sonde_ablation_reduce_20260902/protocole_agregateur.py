#!/usr/bin/env python3
"""Agregateur de la sonde d'ablation du reduce (bench/sonde_ablation_reduce.sh).

Lit out/<ablation>_n<n>_r<rep>.txt (sortie de mhgp6_profile_sonde), extrait
les lignes `profil_reduce K=...` et publie, par (n, K) et par bras, la
MEDIANE sur les repetitions de chaque fenetre, plus l'ecart au bras
`aucune`. Faits seulement : une attribution sur binaire instrumente, jamais
un mur ; aucune ligne n'est une decision. Une porte Python ne repose jamais
sur assert (python3 -O).
"""
import glob
import os
import re
import statistics
import sys

WINDOWS = ("init", "touch", "pre", "unite", "post_remplissage",
           "materialisation_tri_copie", "partition", "liberation", "somme")
KS_SHOWN = (8, 9, 10)


def parse_profile(path):
    per_k = {}
    text = open(path, encoding="utf-8").read()
    for m in re.finditer(r"^profil_reduce K=(\d+) (.*)$", text, re.M):
        fields = {}
        for tok in m.group(2).split():
            if "=" in tok:
                k, v = tok.split("=", 1)
                try:
                    fields[k] = float(v)
                except ValueError:
                    pass
        per_k[int(m.group(1))] = fields
    mur = re.search(r"^temps_mur_ms=([0-9.]+)", text, re.M)
    rss = re.search(r"^rss_max_kb=(\d+)", text, re.M)
    return per_k, (float(mur.group(1)) if mur else None), (int(rss.group(1)) if rss else None)


def main():
    if len(sys.argv) != 2:
        print("usage: sonde_ablation_reduce.py <dossier de reçu (.partial ou publie)>", file=sys.stderr)
        return 2
    work = sys.argv[1]
    runs = {}  # (abl, n) -> list of (per_k, mur, rss)
    codes = {}
    for st in sorted(glob.glob(os.path.join(work, "out", "*.status"))):
        head = open(st, encoding="utf-8").readline().split()
        kv = dict(t.split("=", 1) for t in head if "=" in t)
        abl, n, rep, code = kv["ablation"], int(kv["n"]), int(kv["rep"]), int(kv["code"])
        codes[(abl, n, rep)] = code
        txt = st[:-len(".status")] + ".txt"
        if code != 0 or not os.path.exists(txt):
            continue
        runs.setdefault((abl, n), []).append(parse_profile(txt))
    bad = [k for k, c in codes.items() if c != 0]
    if bad:
        print(f"RUNS EN ECHEC : {bad}")
    ns = sorted({n for (_a, n) in runs})
    abls = ["aucune"] + sorted({a for (a, _n) in runs if a != "aucune"})
    print("# sonde ablation reduce — medianes sur repetitions, ms ; ecart = bras - aucune "
          "(negatif = fenetre reduite). Attribution sur binaire instrumente, join=1 : JAMAIS un mur.")
    for n in ns:
        print(f"\n## n={n}")
        base = runs.get(("aucune", n), [])
        nrep = {a: len(runs.get((a, n), [])) for a in abls}
        print("repetitions : " + " ".join(f"{a}={c}" for a, c in nrep.items()))
        for K in KS_SHOWN:
            print(f"\nK={K}\t" + "\t".join(WINDOWS))
            ref = {}
            for a in abls:
                rows = [r[0].get(K, {}) for r in runs.get((a, n), []) if K in r[0]]
                if not rows:
                    continue
                med = {w: statistics.median([row.get(w, 0.0) for row in rows]) for w in WINDOWS}
                if a == "aucune":
                    ref = med
                    print(f"{a}\t" + "\t".join(f"{med[w]:.1f}" for w in WINDOWS))
                else:
                    print(f"{a}\t" + "\t".join(
                        f"{med[w]:.1f} ({med[w] - ref.get(w, 0.0):+.1f})" for w in WINDOWS))
        # Somme sur tous les K de chaque fenetre.
        print("\nΣ_K\t" + "\t".join(WINDOWS) + "\tmur_instrumente_ms\trss_max_kb")
        ref = {}
        for a in abls:
            rs = runs.get((a, n), [])
            if not rs:
                continue
            sums = []
            for per_k, mur, rss in rs:
                s = {w: sum(f.get(w, 0.0) for f in per_k.values()) for w in WINDOWS}
                s["mur"] = mur or 0.0
                s["rss"] = float(rss or 0)
                sums.append(s)
            med = {w: statistics.median([s[w] for s in sums]) for w in list(WINDOWS) + ["mur", "rss"]}
            if a == "aucune":
                ref = med
                print(f"{a}\t" + "\t".join(f"{med[w]:.1f}" for w in WINDOWS)
                      + f"\t{med['mur']:.1f}\t{med['rss']:.0f}")
            else:
                print(f"{a}\t" + "\t".join(f"{med[w]:.1f} ({med[w] - ref.get(w, 0.0):+.1f})" for w in WINDOWS)
                      + f"\t{med['mur']:.1f} ({med['mur'] - ref.get('mur', 0.0):+.1f})\t{med['rss']:.0f}")
        # Parts de la fenetre materialisation_tri_copie expliquees par chaque
        # retrait (Σ_K), en % de la fenetre du bras aucune.
        if base:
            mat_ref = statistics.median([sum(f.get("materialisation_tri_copie", 0.0) for f in r[0].values())
                                         for r in base])
            post_ref = statistics.median([sum(f.get("post_remplissage", 0.0) for f in r[0].values())
                                          for r in base])
            print("\nparts Σ_K (en % de la fenetre du bras aucune) :")
            for a in abls[1:]:
                rs = runs.get((a, n), [])
                if not rs or mat_ref <= 0:
                    continue
                mat = statistics.median([sum(f.get("materialisation_tri_copie", 0.0) for f in r[0].values())
                                         for r in rs])
                post = statistics.median([sum(f.get("post_remplissage", 0.0) for f in r[0].values())
                                          for r in rs])
                print(f"  {a}: materialisation_tri_copie {100.0 * (mat_ref - mat) / mat_ref:+.1f} %"
                      f" ; post_remplissage {100.0 * (post_ref - post) / post_ref if post_ref > 0 else 0.0:+.1f} %")
    return 0


if __name__ == "__main__":
    sys.exit(main())
