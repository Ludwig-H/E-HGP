#!/usr/bin/env python3
"""Agregateur FAIL-CLOSED de la sonde d'ablation du reduce
(bench/sonde_ablation_reduce.sh ; fermeture minimale exigee par
audits/ALERTE_SONDE_ABLATION_REDUCE_20260902.md, puis les cinq
contre-fixtures encore vertes de son « Etat du WIP » et du § 5.21 de
REPONSE_AUDITEURS_MULTICPU_V6 : taille dupliquee, hash vide, champ duplique,
carre latin non Williams, artefact inattendu dans out/).

Autorite : META.txt (n_list, reps, ablations, statut, binaire_sha256) et
plan.txt (bloc, position, bras — carre de Williams). Le resume n'est produit
que si :
  - META.txt porte le schema v2, aucune ligne `campagne INVALIDE`, des
    tailles distinctes (deux tuples de meme taille porteraient le meme tag),
    des hash a la grammaire EXACTE ^[0-9a-f]{64}$ (binaire, lanceur,
    agregateur), et `runs_effectues` egal au cardinal de la matrice ;
  - tout champ est UNIQUE : un champ present deux fois dans META.txt, un
    .status, une ligne de plan.txt ou une ligne profil_reduce est un refus
    (jamais la premiere ni la derniere valeur), une ligne de plan ou de
    profil malformee (jeton sans `=`) aussi ;
  - la matrice bras x tailles x repetitions est EXACTE : out/ contient
    exactement le triplet <tag>.txt|.err|.status de chaque tuple attendu et
    RIEN d'autre (ni intrus, ni repertoire, ni lien, ni out/SHA256SUMS),
    chaque .status a code=0, un contenu coherent avec son nom, la position du
    plan, et des sha256_avant/apres 64-hex egaux au binaire_sha256 du META ;
    HASHES.txt porte exactement une ligne 64-hex par tuple ; la copie privee
    bin/mhgp6_profile_sonde a le hash du META ;
  - plan.txt est un carre de WILLIAMS : par groupe de quatre blocs chaque
    bras occupe chaque position une fois ET chaque succession ordonnee X→Y
    (bras adjacents dans un bloc) apparait exactement une fois — un carre
    latin cyclique (ABCD/BCDA/CDAB/DABC) est refuse ;
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
import hashlib
import math
import os
import re
import statistics
import sys

SCHEMA = "e-hgp.sonde-ablation-reduce.v2"
HEX64 = re.compile(r"^[0-9a-f]{64}$")
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
EXTENSIONS = ("txt", "err", "status")
CHAMPS_STATUS = ("ablation", "n", "rep", "position", "code",
                 "sha256_avant", "sha256_apres")
CHAMPS_META = ("schema", "n_list", "reps", "ablations", "statut",
               "binaire_sha256", "sha256_lanceur", "sha256_agregateur")


class Refus(Exception):
    """Motif de refus (code 1) — jamais un zero substitue."""


def exiger_hex64(valeur, contexte):
    """Grammaire EXACTE d'un sha256 : une chaine vide n'est jamais « egale »."""
    if not HEX64.match(valeur):
        raise Refus(f"{contexte} : sha256 vide ou hors grammaire 64-hex ({valeur!r})")
    return valeur


def tokens_kv(tokens, contexte):
    """Jetons `cle=valeur` -> dict ; jeton sans `=` ou cle dupliquee = refus."""
    kv = {}
    for tok in tokens:
        if "=" not in tok:
            raise Refus(f"{contexte} : jeton malforme sans `=` ({tok!r})")
        k, v = tok.split("=", 1)
        if k in kv:
            raise Refus(f"{contexte} : champ {k}= duplique ({kv[k]!r} puis {v!r}) — "
                        "jamais la premiere ni la derniere valeur")
        kv[k] = v
    return kv


def lire_kv(path, requis=()):
    """Fichier `cle=valeur` par ligne ; un champ present deux fois est un refus."""
    kv = {}
    base = os.path.basename(path)
    with open(path, encoding="utf-8") as f:
        for no, line in enumerate(f, 1):
            line = line.rstrip("\n")
            if not line or line.startswith("#") or "=" not in line:
                continue
            k, v = line.split("=", 1)
            if k in kv:
                raise Refus(f"{base} : champ {k}= duplique (ligne {no}, {kv[k]!r} puis {v!r})"
                            " — jamais la premiere ni la derniere valeur")
            kv[k] = v
    for k in requis:
        if k not in kv:
            raise Refus(f"{base} sans champ {k}=")
    return kv


def lire_meta(work):
    path = os.path.join(work, "META.txt")
    if not os.path.isfile(path):
        raise Refus("META.txt absent")
    with open(path, encoding="utf-8") as f:
        for line in f:
            if line.startswith("campagne INVALIDE"):
                raise Refus(f"META.txt porte « {line.strip()} » — reçu invalide, jamais agrege")
    meta = lire_kv(path, CHAMPS_META)
    if meta["schema"] != SCHEMA:
        raise Refus(f"META.txt : schema {meta['schema']!r} != {SCHEMA}")
    for k in ("binaire_sha256", "sha256_lanceur", "sha256_agregateur"):
        exiger_hex64(meta[k], f"META.txt {k}=")
    try:
        ns = [int(x) for x in meta["n_list"].split()]
        reps = int(meta["reps"])
    except ValueError as e:
        raise Refus(f"META.txt : n_list/reps non entiers ({e})")
    bras = meta["ablations"].split()
    if not ns or reps <= 0:
        raise Refus("META.txt : matrice vide (n_list vide ou reps <= 0)")
    if any(n <= 0 for n in ns):
        raise Refus(f"META.txt : taille non positive dans n_list ({ns})")
    if len(ns) != len(set(ns)):
        doublons = sorted({n for n in ns if ns.count(n) > 1})
        raise Refus(f"META.txt : n_list avec taille dupliquee {doublons} — les tags "
                    "<bras>_n<n>_r<bloc> de deux tuples s'ecraseraient")
    if BRAS_TEMOIN not in bras or len(bras) != len(set(bras)):
        raise Refus(f"META.txt : bras temoin {BRAS_TEMOIN} absent ou bras dupliques")
    cardinal = len(bras) * len(ns) * reps
    for k in ("runs_effectues", "runs_attendus"):
        if k in meta:
            try:
                v = int(meta[k])
            except ValueError:
                raise Refus(f"META.txt : {k}={meta[k]!r} non entier")
            if v != cardinal:
                raise Refus(f"META.txt : {k}={v} != cardinal de la matrice {cardinal} "
                            f"({len(bras)} bras x {len(ns)} tailles x {reps} blocs)")
    return meta, ns, reps, bras


def verifier_williams(plan, bras, blocs):
    """Chaque succession ordonnee X→Y (bras adjacents dans un bloc) exactement
    une fois sur le groupe de blocs : m blocs x (m-1) successions = m(m-1)
    paires ordonnees. Un carre latin cyclique n'y satisfait pas."""
    comptes = {}
    for bloc in blocs:
        ordre = sorted(bras, key=lambda b: plan[(bloc, b)])
        for s in zip(ordre, ordre[1:]):
            comptes[s] = comptes.get(s, 0) + 1
    attendues = {(x, y) for x in bras for y in bras if x != y}
    repetees = sorted(s for s, c in comptes.items() if c > 1)
    absentes = sorted(attendues - set(comptes))
    if repetees or absentes:
        detail_rep = ", ".join(f"{x}->{y} x{comptes[(x, y)]}" for x, y in repetees[:3])
        detail_abs = ", ".join(f"{x}->{y}" for x, y in absentes[:3])
        raise Refus(f"plan.txt : blocs {list(blocs)} equilibres par position mais NON Williams"
                    f" — succession(s) repetee(s) [{detail_rep}], absente(s) [{detail_abs}] ;"
                    " chaque succession ordonnee X->Y de bras adjacents doit apparaitre"
                    f" exactement une fois par groupe de {len(bras)} blocs")


def lire_plan(work, reps, bras):
    """plan.txt -> {(bloc, bras): position} ; refuse un plan non Williams."""
    plan = {}
    path = os.path.join(work, "plan.txt")
    if not os.path.isfile(path):
        raise Refus("plan.txt absent")
    with open(path, encoding="utf-8") as f:
        for no, line in enumerate(f, 1):
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            kv = tokens_kv(line.split(), f"plan.txt ligne {no}")
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
        verifier_williams(plan, bras, blocs)
    return plan


def lire_status(path):
    """-> (ablation, n, rep, position, code, sha256_avant, sha256_apres).
    Premiere ligne = jetons ; lignes suivantes = cle=valeur ; tout champ
    present deux fois (sur la ligne 1 ou entre lignes) est un refus."""
    base = os.path.basename(path)
    with open(path, encoding="utf-8") as f:
        lignes = f.read().splitlines()
    if not lignes:
        raise Refus(f"{base} : statut vide")
    kv = tokens_kv(lignes[0].split(), f"{base} ligne 1")
    for no, line in enumerate(lignes[1:], 2):
        if not line or "=" not in line:
            continue
        k, v = line.split("=", 1)
        if k in kv:
            raise Refus(f"{base} : champ {k}= duplique (ligne {no}, {kv[k]!r} puis {v!r})"
                        " — jamais la premiere ni la derniere valeur")
        kv[k] = v
    for k in CHAMPS_STATUS:
        if k not in kv:
            raise Refus(f"{base} : champ {k}= absent")
    try:
        return (kv["ablation"], int(kv["n"]), int(kv["rep"]), int(kv["position"]),
                int(kv["code"]), kv["sha256_avant"], kv["sha256_apres"])
    except ValueError:
        raise Refus(f"{base} : n/rep/position/code non entiers")


def lire_hashes(work, tags, h_meta):
    """HASHES.txt : exactement une ligne `<tag> avant=<64hex> apres=<64hex>`
    par tuple attendu, chaque hash egal au binaire_sha256 du META."""
    path = os.path.join(work, "HASHES.txt")
    if not os.path.isfile(path):
        raise Refus("HASHES.txt absent")
    vus = set()
    with open(path, encoding="utf-8") as f:
        for no, line in enumerate(f, 1):
            tokens = line.split()
            if not tokens:
                continue
            if len(tokens) != 3:
                raise Refus(f"HASHES.txt ligne {no} : attendu `<tag> avant=<h> apres=<h>` ({line.strip()!r})")
            tag = tokens[0]
            if tag not in tags:
                raise Refus(f"HASHES.txt ligne {no} : tag hors matrice ({tag})")
            if tag in vus:
                raise Refus(f"HASHES.txt ligne {no} : tag duplique ({tag})")
            vus.add(tag)
            kv = tokens_kv(tokens[1:], f"HASHES.txt ligne {no}")
            for k in ("avant", "apres"):
                if k not in kv:
                    raise Refus(f"HASHES.txt ligne {no} : champ {k}= absent")
                exiger_hex64(kv[k], f"HASHES.txt {tag} {k}=")
                if kv[k] != h_meta:
                    raise Refus(f"HASHES.txt {tag} {k}={kv[k]} != binaire_sha256 du META {h_meta}")
    manquants = sorted(tags - vus)
    if manquants:
        raise Refus(f"HASHES.txt : {len(manquants)} tuple(s) sans ligne de hash, premiers {manquants[:4]}")


def sha256_fichier(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for bloc in iter(lambda: f.read(1 << 16), b""):
            h.update(bloc)
    return h.hexdigest()


def parse_profile(path):
    """-> ({K: {champ: float}}, temps_mur_ms, rss_max_kb) ; refus sinon."""
    base = os.path.basename(path)
    with open(path, encoding="utf-8") as f:
        text = f.read()
    per_k = {}
    for m in re.finditer(r"^profil_reduce K=(\d+) (.*)$", text, re.M):
        k = int(m.group(1))
        if k not in KS:
            raise Refus(f"{base} : K={k} hors 1..10")
        if k in per_k:
            raise Refus(f"{base} : ligne K={k} dupliquee")
        fields = {}
        for name, v in tokens_kv(m.group(2).split(), f"{base} profil_reduce K={k}").items():
            try:
                fields[name] = float(v)
            except ValueError:
                fields[name] = float("nan")
        for name in CHAMPS_REQUIS:
            if name not in fields:
                raise Refus(f"{base} : K={k} sans fenetre {name}")
            if not math.isfinite(fields[name]):
                raise Refus(f"{base} : K={k} fenetre {name} non finie")
        recalc = sum(fields[c] for c in COMPOSANTES if c in fields)
        if not math.isfinite(recalc) or abs(recalc - fields["somme"]) > SEUIL_SOMME:
            raise Refus(f"{base} : K={k} somme imprimee {fields['somme']:.3f}"
                        f" != somme des composantes {recalc:.3f}")
        per_k[k] = fields
    manquants = [k for k in KS if k not in per_k]
    if manquants:
        raise Refus(f"{base} : ligne(s) K manquante(s) {manquants}")
    mur = re.search(r"^temps_mur_ms=(\S+)", text, re.M)
    rss = re.search(r"^rss_max_kb=(\d+)$", text, re.M)
    if not mur:
        raise Refus(f"{base} : temps_mur_ms absent")
    try:
        mur_ms = float(mur.group(1))
    except ValueError:
        raise Refus(f"{base} : temps_mur_ms illisible")
    if not math.isfinite(mur_ms):
        raise Refus(f"{base} : temps_mur_ms non fini")
    if not rss:
        raise Refus(f"{base} : rss_max_kb absent")
    return per_k, mur_ms, int(rss.group(1))


def inventaire_out(out, attendus):
    """out/ = EXACTEMENT le triplet <tag>.txt|.err|.status de chaque tuple."""
    attendus_fichiers = {f"{a}_n{n}_r{r}.{ext}" for (a, n, r) in attendus for ext in EXTENSIONS}
    entrees = sorted(os.listdir(out))
    for e in entrees:
        chemin = os.path.join(out, e)
        if os.path.islink(chemin) or not os.path.isfile(chemin):
            raise Refus(f"out/{e} : entree non reguliere (repertoire, lien ou special) — "
                        "out/ ne contient que des fichiers <tag>.txt|.err|.status")
    inattendus = sorted(set(entrees) - attendus_fichiers)
    if inattendus:
        raise Refus(f"out/ : {len(inattendus)} artefact(s) inattendu(s) hors de l'ensemble exact "
                    f"<tag>.txt|.err|.status des tuples attendus : {inattendus[:4]}")
    manquants = sorted(attendus_fichiers - set(entrees))
    if manquants:
        raise Refus(f"out/ : {len(manquants)} artefact(s) attendu(s) absent(s) : {manquants[:4]}")


def charger(work):
    meta, ns, reps, bras = lire_meta(work)
    plan = lire_plan(work, reps, bras)
    h_meta = meta["binaire_sha256"]
    copie = os.path.join(work, "bin", "mhgp6_profile_sonde")
    if not os.path.isfile(copie) or os.path.islink(copie):
        raise Refus("bin/mhgp6_profile_sonde absent ou non regulier")
    h_copie = sha256_fichier(copie)
    if h_copie != h_meta:
        raise Refus(f"bin/mhgp6_profile_sonde : sha256 {h_copie} != binaire_sha256 du META {h_meta}")
    out = os.path.join(work, "out")
    if not os.path.isdir(out):
        raise Refus("dossier out/ absent")
    attendus = sorted((a, n, r) for a in bras for n in ns for r in range(1, reps + 1))
    lire_hashes(work, {f"{a}_n{n}_r{r}" for (a, n, r) in attendus}, h_meta)
    inventaire_out(out, attendus)
    mesures = {}
    for key in attendus:
        a, n, r = key
        tag = f"{a}_n{n}_r{r}"
        f = f"{tag}.status"
        abl, n_lu, rep, pos, code, h_avant, h_apres = lire_status(os.path.join(out, f))
        if (abl, n_lu, rep) != key:
            raise Refus(f"nom de statut incoherent avec son contenu : {f} "
                        f"(contenu {abl}_n{n_lu}_r{rep})")
        if code != 0:
            raise Refus(f"run en echec (code {code}) : {f}")
        if plan[(rep, abl)] != pos:
            raise Refus(f"position executee {pos} != plan {plan[(rep, abl)]} : {f}")
        for k, h in (("sha256_avant", h_avant), ("sha256_apres", h_apres)):
            exiger_hex64(h, f"{f} {k}=")
            if h != h_meta:
                raise Refus(f"{f} : {k}={h} != binaire_sha256 du META {h_meta}")
        mesures[key] = parse_profile(os.path.join(out, f"{tag}.txt"))
    manquants = sorted(set(attendus) - set(mesures))
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
    except OSError as e:
        print(f"REFUS : lecture impossible ({e})", file=sys.stderr)
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
