#!/usr/bin/env python3
"""Agregateur FAIL-CLOSED de la sonde d'ablation du reduce
(bench/sonde_ablation_reduce.sh ; fermeture minimale exigee par
audits/ALERTE_SONDE_ABLATION_REDUCE_20260902.md).

Autorite : META.txt (n_list, reps, ablations, statut) et plan.txt (bloc,
position, bras — carre de Williams). Le resume n'est produit que si :
  - la matrice bras x tailles x repetitions est EXACTE (chaque .status
    present avec code=0, aucun tuple hors matrice, aucun doublon, position
    executee egale a celle du plan, plan equilibre : chaque bras une fois a
    chaque position par groupe de quatre blocs) ;
  - chaque sortie porte exactement dix lignes `profil_reduce K=1..10` (une
    par K, aucune autre), toutes les fenetres requises presentes et FINIES,
    `somme` coherente avec ses composantes (seuil 0.0051 du § 5.13),
    `temps_mur_ms` et `rss_max_kb` presents et finis.
Une mesure absente n'est JAMAIS remplacee par zero : elle est un refus.

Tableau principal : differences APPARIEES PAR BLOC (bras − aucune au sein
du meme bloc et de la meme taille), mediane [min ; max] sur les blocs.
Second tableau : medianes brutes par bras. Le bras ablation-post-cle-factice
est une BORNE COMPOSITE (lecture keys[] + tri de cles egales), jamais
« lecture seule ». Bornes exploratoires non causales sur binaire
instrumente : jamais un benchmark, jamais un mur, jamais un choix de palier.

Codes : 0 resume sur stdout ; 1 refus (motif sur stderr) ; 2 usage.
Aucun assert (doit tenir sous python3 -O).
"""
import math
import os
import re
import statistics
import sys

WINDOWS = ("init", "touch", "pre", "unite", "post_remplissage",
           "materialisation_tri_copie", "partition", "liberation", "somme")
# Composantes dont `somme` est la somme (liveness figure dans reduce_v2 ; si
# elle est absente elle ne compte pas — mais une composante requise absente
# reste un refus).
COMPOSANTES = ("init", "touch", "pre", "unite", "post_remplissage",
               "materialisation_tri_copie", "liveness", "partition",
               "liberation")
CHAMPS_REQUIS = WINDOWS + ("mur_reduce_interne", "residuel")
KS = tuple(range(1, 11))
KS_SHOWN = (8, 9, 10)
BRAS_TEMOIN = "aucune"
SEUIL_SOMME = 0.0051
ETIQUETTES = {
    "ablation-mat-sans-copie": "borne : copie profonde retiree (objet change)",
    "ablation-mat-sans-tris": "borne : tris retires (objet change)",
    "ablation-post-cle-factice":
        "borne composite (lecture keys[] + tri de cles egales)",
}
PARTS = ("materialisation_tri_copie", "post_remplissage")


class Refus(Exception):
    """Motif de refus (code 1) — jamais un zero substitue."""


def lire_kv(path, requis=()):
    kv = {}
    with open(path, encoding="utf-8") as f:
        for line in f:
            line = line.rstrip("\n")
            if not line or line.startswith("#") or "=" not in line:
                continue
            k, v = line.split("=", 1)
            kv.setdefault(k, v)
    for k in requis:
        if k not in kv:
            raise Refus(f"{os.path.basename(path)} sans champ {k}=")
    return kv


def lire_meta(work):
    meta = lire_kv(os.path.join(work, "META.txt"),
                   ("n_list", "reps", "ablations", "statut"))
    try:
        ns = [int(x) for x in meta["n_list"].split()]
        reps = int(meta["reps"])
    except ValueError as e:
        raise Refus(f"META.txt : n_list/reps non entiers ({e})")
    bras = meta["ablations"].split()
    if not ns or reps <= 0:
        raise Refus("META.txt : matrice vide (n_list vide ou reps <= 0)")
    if BRAS_TEMOIN not in bras or len(bras) != len(set(bras)):
        raise Refus(f"META.txt : bras temoin {BRAS_TEMOIN} absent ou bras dupliques")
    return meta, ns, reps, bras


def lire_plan(work, reps, bras):
    """plan.txt -> {(bloc, bras): position} ; refuse un plan non equilibre."""
    plan = {}
    path = os.path.join(work, "plan.txt")
    if not os.path.isfile(path):
        raise Refus("plan.txt absent")
    with open(path, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            kv = dict(t.split("=", 1) for t in line.split() if "=" in t)
            try:
                bloc, pos, b = int(kv["bloc"]), int(kv["position"]), kv["bras"]
            except (KeyError, ValueError):
                raise Refus(f"plan.txt : ligne illisible ({line})")
            hors = b not in bras or not 1 <= bloc <= reps or not 1 <= pos <= len(bras)
            if hors:
                raise Refus(f"plan.txt : ligne hors matrice ({line})")
            if (bloc, b) in plan:
                raise Refus(f"plan.txt : bras {b} duplique au bloc {bloc}")
            plan[(bloc, b)] = pos
    for bloc in range(1, reps + 1):
        positions = sorted(plan.get((bloc, b), 0) for b in bras)
        if positions != list(range(1, len(bras) + 1)):
            raise Refus(f"plan.txt : bloc {bloc} incomplet ou positions non distinctes")
    if reps % len(bras) != 0:
        raise Refus(f"plan.txt : {reps} blocs ne forment pas des groupes de {len(bras)}")
    for g in range(0, reps, len(bras)):
        blocs = range(g + 1, g + len(bras) + 1)
        for pos in range(1, len(bras) + 1):
            occupants = sorted(b for bloc in blocs for b in bras if plan[(bloc, b)] == pos)
            if occupants != sorted(bras):
                raise Refus(f"plan.txt : position {pos} non equilibree sur les blocs {list(blocs)}")
    return plan


def lire_status(path):
    with open(path, encoding="utf-8") as f:
        head = f.readline().split()
    kv = dict(t.split("=", 1) for t in head if "=" in t)
    try:
        return (kv["ablation"], int(kv["n"]), int(kv["rep"]),
                int(kv["position"]), int(kv["code"]))
    except (KeyError, ValueError):
        raise Refus(f"{os.path.basename(path)} : premiere ligne illisible")


def parse_profile(path):
    """-> ({K: {champ: float}}, temps_mur_ms, rss_max_kb) ; refus sinon."""
    with open(path, encoding="utf-8") as f:
        text = f.read()
    per_k = {}
    for m in re.finditer(r"^profil_reduce K=(\d+) (.*)$", text, re.M):
        k = int(m.group(1))
        if k not in KS:
            raise Refus(f"{os.path.basename(path)} : K={k} hors 1..10")
        if k in per_k:
            raise Refus(f"{os.path.basename(path)} : ligne K={k} dupliquee")
        fields = {}
        for tok in m.group(2).split():
            if "=" not in tok:
                continue
            name, v = tok.split("=", 1)
            try:
                fields[name] = float(v)
            except ValueError:
                fields[name] = float("nan")
        for name in CHAMPS_REQUIS:
            if name not in fields:
                raise Refus(f"{os.path.basename(path)} : K={k} sans fenetre {name}")
            if not math.isfinite(fields[name]):
                raise Refus(f"{os.path.basename(path)} : K={k} fenetre {name} non finie")
        recalc = sum(fields[c] for c in COMPOSANTES if c in fields)
        if not math.isfinite(recalc) or abs(recalc - fields["somme"]) > SEUIL_SOMME:
            raise Refus(f"{os.path.basename(path)} : K={k} somme imprimee {fields['somme']:.3f}"
                        f" != somme des composantes {recalc:.3f}")
        per_k[k] = fields
    manquants = [k for k in KS if k not in per_k]
    if manquants:
        raise Refus(f"{os.path.basename(path)} : ligne(s) K manquante(s) {manquants}")
    mur = re.search(r"^temps_mur_ms=(\S+)", text, re.M)
    rss = re.search(r"^rss_max_kb=(\d+)$", text, re.M)
    if not mur:
        raise Refus(f"{os.path.basename(path)} : temps_mur_ms absent")
    try:
        mur_ms = float(mur.group(1))
    except ValueError:
        raise Refus(f"{os.path.basename(path)} : temps_mur_ms illisible")
    if not math.isfinite(mur_ms):
        raise Refus(f"{os.path.basename(path)} : temps_mur_ms non fini")
    if not rss:
        raise Refus(f"{os.path.basename(path)} : rss_max_kb absent")
    return per_k, mur_ms, int(rss.group(1))


def charger(work):
    meta, ns, reps, bras = lire_meta(work)
    plan = lire_plan(work, reps, bras)
    out = os.path.join(work, "out")
    if not os.path.isdir(out):
        raise Refus("dossier out/ absent")
    attendus = {(a, n, r) for a in bras for n in ns for r in range(1, reps + 1)}
    mesures = {}
    fichiers = sorted(os.listdir(out))
    for f in fichiers:
        if f.endswith(".txt") and f[:-4] + ".status" not in fichiers:
            raise Refus(f"sortie sans statut : {f}")
        if not f.endswith(".status"):
            continue
        abl, n, rep, pos, code = lire_status(os.path.join(out, f))
        key = (abl, n, rep)
        if key not in attendus:
            raise Refus(f"tuple hors matrice : {f}")
        if f != f"{abl}_n{n}_r{rep}.status":
            raise Refus(f"nom de statut incoherent avec son contenu : {f}")
        if key in mesures:
            raise Refus(f"tuple duplique : {f}")
        if code != 0:
            raise Refus(f"run en echec (code {code}) : {f}")
        if plan[(rep, abl)] != pos:
            raise Refus(f"position executee {pos} != plan {plan[(rep, abl)]} : {f}")
        txt = os.path.join(out, f[:-len(".status")] + ".txt")
        if not os.path.isfile(txt):
            raise Refus(f"sortie absente : {os.path.basename(txt)}")
        mesures[key] = parse_profile(txt)
    manquants = sorted(attendus - set(mesures))
    if manquants:
        raise Refus(f"matrice incomplete : {len(manquants)} tuple(s) absent(s), "
                    f"premiers {manquants[:4]}")
    return meta, ns, reps, bras, mesures


def valeurs(mesures, abl, n, rep):
    """Colonnes d'un tuple : fenetres par K, sommes sur K, mur, rss."""
    per_k, mur, rss = mesures[(abl, n, rep)]
    cols = {}
    for K in KS_SHOWN:
        for w in WINDOWS:
            cols[(K, w)] = per_k[K][w]
    for w in WINDOWS:
        cols[("sigma", w)] = sum(per_k[K][w] for K in KS)
    cols[("sigma", "mur")] = mur
    cols[("sigma", "rss")] = float(rss)
    return cols


def etiquette(abl):
    return f"{abl} ({ETIQUETTES[abl]})" if abl in ETIQUETTES else abl


def fmt3(vals, prec=1):
    med, mn, mx = statistics.median(vals), min(vals), max(vals)
    return f"{med:+.{prec}f} [{mn:+.{prec}f};{mx:+.{prec}f}]"


def main():
    if len(sys.argv) != 2:
        print("usage: sonde_ablation_reduce.py <dossier de reçu (.partial ou publie)>",
              file=sys.stderr)
        return 2
    try:
        meta, ns, reps, bras, mesures = charger(sys.argv[1])
    except Refus as e:
        print(f"REFUS : {e}", file=sys.stderr)
        return 1
    autres = [a for a in bras if a != BRAS_TEMOIN]
    print(f"# sonde ablation reduce — statut={meta['statut']}")
    print("# bornes exploratoires NON CAUSALES sur binaire instrumente (join=1) : jamais un "
          "benchmark, jamais un mur, jamais un choix de palier.")
    print("# bras : " + " ; ".join([f"{BRAS_TEMOIN} (temoin)"] + [etiquette(a) for a in autres]))
    print(f"# plan : williams_4x4, blocs={reps}, tailles={ns} ; appariement PAR BLOC : "
          f"d_b = bras(b) − {BRAS_TEMOIN}(b), meme bloc, meme taille.")
    print("# tableau principal : mediane [min;max] sur les blocs des differences appariees d_b "
          "(ms ; negatif = fenetre reduite). Second tableau : medianes brutes par bras.")
    sigma_cols = list(WINDOWS) + ["mur", "rss"]
    sigma_head = "\t".join(WINDOWS) + "\tmur_instrumente_ms\trss_max_kb"
    for n in ns:
        print(f"\n## n={n}")
        cols = {(a, r): valeurs(mesures, a, n, r) for a in bras for r in range(1, reps + 1)}
        print(f"\n### differences appariees par bloc (blocs={reps})")
        for K in KS_SHOWN:
            print(f"K={K}\t" + "\t".join(WINDOWS))
            for a in autres:
                d = {w: [cols[(a, r)][(K, w)] - cols[(BRAS_TEMOIN, r)][(K, w)]
                         for r in range(1, reps + 1)] for w in WINDOWS}
                print(f"{etiquette(a)}\t" + "\t".join(fmt3(d[w]) for w in WINDOWS))
        print("Σ_K\t" + sigma_head)
        for a in autres:
            d = {w: [cols[(a, r)][("sigma", w)] - cols[(BRAS_TEMOIN, r)][("sigma", w)]
                     for r in range(1, reps + 1)] for w in sigma_cols}
            print(f"{etiquette(a)}\t" + "\t".join(fmt3(d[w]) for w in WINDOWS)
                  + f"\t{fmt3(d['mur'])}\t{fmt3(d['rss'], 0)}")
        print("\n### parts Σ_K retirees de la fenetre du temoin, appariees par bloc "
              "(mediane [min;max], % ; borne non causale)")
        for a in autres:
            morceaux = []
            for w in PARTS:
                parts = []
                for r in range(1, reps + 1):
                    ref = cols[(BRAS_TEMOIN, r)][("sigma", w)]
                    if ref <= 0:
                        continue
                    parts.append(100.0 * (ref - cols[(a, r)][("sigma", w)]) / ref)
                morceaux.append(f"{w} {fmt3(parts)} %" if parts else f"{w} indefini (temoin nul)")
            print(f"  {etiquette(a)} : " + " ; ".join(morceaux))
        print("\n### medianes brutes sur blocs (second tableau, non apparie)")
        for K in KS_SHOWN:
            print(f"K={K}\t" + "\t".join(WINDOWS))
            for a in bras:
                print(f"{etiquette(a)}\t" + "\t".join(
                    f"{statistics.median(cols[(a, r)][(K, w)] for r in range(1, reps + 1)):.1f}"
                    for w in WINDOWS))
        print("Σ_K\t" + sigma_head)
        for a in bras:
            med = {w: statistics.median(cols[(a, r)][("sigma", w)] for r in range(1, reps + 1))
                   for w in sigma_cols}
            print(f"{etiquette(a)}\t" + "\t".join(f"{med[w]:.1f}" for w in WINDOWS)
                  + f"\t{med['mur']:.1f}\t{med['rss']:.0f}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
