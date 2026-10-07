#!/usr/bin/env python3
"""Pilote et juge de MES-G1 (levier G-L3 : saut certifie sans census), seconde moitie de la regle. Bibliotheque standard
seule, jouable sous python3 -S -O (aucune garde par assert).

Etapes (dans l'ordre si plusieurs) :
  construire  cmake + build de ce microbanc (mhgp12_mes_g1, son mutant, mhgp12_vidage et le mutant de son controle de
              foret) lie a la construction v11 donnee ; empreintes des binaires, de libmhgp11.a et des sources
  portes      porte gravee de mes_g1 (0) et son mutant « cote nul admis » (1, tue par ses ecarts) ; quatrieme bras sur le
              premier cas (--chrono-resolution 1 --bras-saut) : foret identique a chaque ordre (0), et mutant des
              cibles decalees (1, tue par une foret differente)
  campagne    --processus processus neufs par cas, chacun : mhgp12_vidage --journal aucun --chrono-resolution R
              --bras-saut (quatre bras entrelaces a un fil, minimum de R passes par bras et par ordre) ; journaux gardes
              sous l'identifiant de campagne, jamais ecrases ; vidages haches (memes entrees d'une prise a l'autre)
              puis effaces
  rapport     juge (REGLE_G1) et tableaux Markdown
  tout        construire portes campagne rapport

REGLE_G1 (contrat de la tour, paragraphe 4.3, ecrite avant la mesure) : « adopte » si, sur chacun des cas ng00, ng01,
ng02 a K5, (1) au moins la moitie des censuses satures disparaissent (somme sur les ordres 2..K des censuses satures du
bras replique_v12 moins ceux du bras replique_v12_saut, rapportee aux premiers), (2) la foret du bras est identique a
celle de la v11 a chaque ordre de chaque prise, et (3) la borne haute de l'IC 95 % du rapport des temps est sous 1 :
par processus, somme sur les ordres 2..K du minimum de R passes de replique_v12_saut, rapportee a la meme somme pour
replique_v12 ; moyenne geometrique des rapports par processus et IC par bootstrap sur les processus (10 000 tirages,
graine fixe), comme REGLE_M3. « rejete » si une borne haute est au moins 1, si la premiere moitie manque ou si une
foret differe ; « refuse » si une preuve manque (construction, portes, binaires haches avant et apres et egaux a la
construction, journaux gardes et inchanges, au moins cinq prises valides par cas, memes vidages d'une prise a l'autre).

Exemple (codespace ; sur G4 : --fils 48 --fils-construction 48) :
  python3 pilote_g1.py --v11-source <v11>/morsehgp3D_v11 --v11-build <build v11> --donnees <donnees> \\
      --sortie out_g1 --construction build_g1 --cas ng00:5,ng01:5,ng02:5 --processus 5 --passes 3 tout

Les vidages derivent de donnees SemanticKITTI (CC BY-NC-SA) : effaces apres hachage, jamais publies.
Codes : 0 conforme ; 1 ecart (porte en echec, mutant survivant, foret differente) ; 2 usage ; 3 refus (preuve absente,
perimee ou non rattachee). Le verdict d'adoption est dans le rapport et ne change pas le code.
"""
import argparse
import datetime
import hashlib
import json
import math
import os
import platform
import random
import secrets
import subprocess
import sys
import time

ICI = os.path.dirname(os.path.abspath(__file__))
MICROBANCS = os.path.dirname(ICI)
V12 = os.path.dirname(MICROBANCS)
FICHIERS = {"ng00": "lidar_ng00", "ng01": "lidar_ng01", "ng02": "lidar_ng02",
            "u8000": "uniform_u18_n8000", "u16000": "uniform_u18_n16000", "u32000": "uniform_u18_n32000"}
# Porte gravee de mes_g1 et ses mutants : (nom de porte, binaire, mutant_cote_nul, mutant_garde, code attendu).
PORTES_G1 = [("mes_g1", "mhgp12_mes_g1", False, "aucun", 0),
             ("mes_g1_mutant_cote_nul", "mhgp12_mes_g1_mutant_cote_nul", True, "aucun", 1),
             ("mes_g1_mutant_sans_garde_route", "mhgp12_mes_g1_mutant_sans_garde_route", False, "route", 1),
             ("mes_g1_mutant_sans_garde_ordres", "mhgp12_mes_g1_mutant_sans_garde_ordres", False, "ordres", 1),
             ("mes_g1_mutant_sans_garde_bilan", "mhgp12_mes_g1_mutant_sans_garde_bilan", False, "bilan", 1)]
TEMOINS_G1 = {"temoins": 4, "admission": 6}  # temoins geometriques et temoins d'admission de la porte gravee
BINAIRES = [p[1] for p in PORTES_G1] + ["mhgp12_vidage", "mhgp12_vidage_mutant_cibles_decalees"]
# Sources dont dependent les binaires de ce microbanc (hors v11, hachee par sa bibliotheque) : hachees au debut et a la
# fin de chaque invocation, et a la construction.
SOURCES = [os.path.join(ICI, n) for n in ("CMakeLists.txt", "mes_g1.cpp", "voisins.hpp", "pilote_g1.py")] + [
    os.path.join(MICROBANCS, "mes_m3_m4_tour", "vidage", "vidage_v11.cpp"),
    os.path.join(MICROBANCS, "mes_m3_m4_tour", "common", "format.hpp"),
    os.path.join(MICROBANCS, "mes_m3_m4_tour", "mes_m3", "meb_cert.hpp"),
    os.path.join(MICROBANCS, "mes_m3_m4_tour", "mes_m3", "welzl_proposal.hpp"),
    os.path.join(V12, "cmake", "run_expect.cmake")]
REGLE_G1 = {"cas_decisifs": ("ng00_k5", "ng01_k5", "ng02_k5"), "processus_min": 5, "bootstrap": 10000,
            "graine": 20261007, "seuil_satures_evites": 0.5, "seuil_borne_haute": 1.0,
            "statistique": "par processus : somme sur les ordres 2..K du minimum de R passes du bras replique_v12_saut, "
                           "rapportee a la meme somme pour le bras replique_v12 (quatre bras entrelaces, un fil) ; "
                           "moyenne geometrique des rapports par processus, IC 95 % par bootstrap sur les processus "
                           "(10 000 tirages, graine fixe)",
            "adoption": "sur chaque cas qui decide (ng00, ng01, ng02 a K5) : au moins 50 % des censuses satures "
                        "evites (ordres 2..K), foret identique a la v11 a chaque ordre de chaque prise, borne haute de "
                        "l'IC sous 1, au moins 5 prises valides, portes conformes et mutants tues"}


def maintenant():
    return datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def sha256(chemin):
    h = hashlib.sha256()
    with open(chemin, "rb") as f:
        for bloc in iter(lambda: f.read(1 << 20), b""):
            h.update(bloc)
    return h.hexdigest()


def sha256_ou_rien(chemin):
    return sha256(chemin) if chemin and os.path.isfile(chemin) else None


def entier(x):
    return isinstance(x, int) and not isinstance(x, bool)


def fini_positif(x):
    return isinstance(x, (int, float)) and not isinstance(x, bool) and math.isfinite(x) and x > 0


def jouer(commande, journal=None, cwd=None):
    """Execute une commande ; rend (code, lignes JSON, duree, fin de l'erreur standard) ; sortie brute dans journal."""
    debut = time.monotonic()
    try:
        proc = subprocess.run(commande, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
                              check=False)
        code, sortie, erreur = proc.returncode, proc.stdout, proc.stderr
    except OSError as e:
        code, sortie, erreur = 127, "", str(e)
    duree = time.monotonic() - debut
    if journal:
        with open(journal, "w", encoding="utf-8") as f:
            f.write(sortie)
            if erreur:
                f.write("\n# stderr\n" + erreur)
    lignes = []
    for ligne in sortie.splitlines():
        ligne = ligne.strip()
        if ligne.startswith("{"):
            try:
                lignes.append(json.loads(ligne))
            except json.JSONDecodeError:
                lignes.append({"phase": "ligne_illisible", "texte": ligne[:200]})
    return code, lignes, duree, erreur[-2000:]


def lignes_de(lignes, phase):
    return [l for l in lignes if isinstance(l, dict) and l.get("phase") == phase]


def cas_liste(texte):
    out = []
    for item in texte.split(","):
        trame, _, k = item.partition(":")
        if not trame or not k.isdigit() or not 2 <= int(k) <= 12:
            raise SystemExit("cas invalide : %r (attendu trame:K, 2 <= K <= 12)" % item)
        out.append((trame, int(k)))
    return out


def nom_cas(trame, k):
    return "%s_k%d" % (trame, k)


def hacher_sources():
    return {os.path.relpath(p, MICROBANCS): sha256_ou_rien(p) for p in SOURCES}


def charger(sortie):
    chemin = os.path.join(sortie, "rapport_g1.json")
    if os.path.isfile(chemin):
        with open(chemin, encoding="utf-8") as f:
            return json.load(f)
    return {"schema": "ehgp.v12.mes_g1_pilote.v1", "regle": REGLE_G1}


def sauver(sortie, rapport):
    os.makedirs(sortie, exist_ok=True)
    chemin = os.path.join(sortie, "rapport_g1.json")
    with open(chemin + ".tmp", "w", encoding="utf-8") as f:
        json.dump(rapport, f, indent=1, ensure_ascii=False, sort_keys=True)
        f.write("\n")
    os.replace(chemin + ".tmp", chemin)


def ranger(rapport, section, cle, bloc):
    """Un bloc remplace n'est jamais efface : il passe dans l'historique."""
    table = rapport.setdefault(section, {})
    if cle in table:
        rapport.setdefault("historique", {}).setdefault(section, {}).setdefault(cle, []).append(table[cle])
    table[cle] = bloc


# ---- Construction et provenance ---------------------------------------------------------------------------------------
def binaire(args, nom):
    chemin = os.path.join(os.path.abspath(args.construction), nom)
    return chemin if os.path.isfile(chemin) else None


def construire(args, rapport):
    construction = os.path.abspath(args.construction)
    os.makedirs(construction, exist_ok=True)
    etapes = []
    code, _, duree, erreur = jouer(["cmake", "-S", ICI, "-B", construction, "-DCMAKE_BUILD_TYPE=Release",
                                    "-DMHGP11_SOURCE=" + os.path.abspath(args.v11_source),
                                    "-DMHGP11_BUILD=" + os.path.abspath(args.v11_build)],
                                   os.path.join(construction, "cmake.log"))
    etapes.append({"cmake": code, "secondes": round(duree, 2)})
    if code == 0:
        code, _, duree, erreur = jouer(["cmake", "--build", construction, "--parallel", str(args.fils_construction)],
                                       os.path.join(construction, "build.log"))
        etapes.append({"build": code, "secondes": round(duree, 2)})
    bloc = {"dossier": construction, "etapes": etapes, "code": code, "erreur": erreur if code else "",
            "binaires": {n: sha256_ou_rien(binaire(args, n)) for n in BINAIRES},
            "libmhgp11_sha256": sha256_ou_rien(os.path.join(args.v11_build, "libmhgp11.a")),
            "sources": hacher_sources(), "date": maintenant(), "machine": machine()}
    ancien = rapport.get("construction")
    if ancien:
        rapport.setdefault("historique", {}).setdefault("construction", []).append(ancien)
    rapport["construction"] = bloc
    if code != 0 or any(v is None for v in bloc["binaires"].values()) or bloc["libmhgp11_sha256"] is None:
        print("construction en echec ou incomplete (voir %s) : toute execution sera refusee" % construction,
              flush=True)
        return 3
    return 0


def machine():
    info = {"plateforme": platform.platform(), "python": platform.python_version(), "coeurs": os.cpu_count()}
    try:
        with open("/proc/cpuinfo", encoding="utf-8", errors="replace") as f:
            for ligne in f:
                if ligne.startswith("model name"):
                    info["processeur"] = ligne.split(":", 1)[1].strip()
                    break
    except OSError:
        pass
    return info


def lancer(args, rapport, nom, options, journal):
    """Lance un binaire de la construction ; provenance : empreinte avant et apres, accord avec « construire »."""
    chemin = binaire(args, nom)
    attendu = rapport.get("construction", {}).get("binaires", {}).get(nom)
    if chemin is None:
        return 127, [], 0.0, "binaire absent", {"binaire": nom, "sha256": None, "inchange": False, "construit": False}
    avant = sha256(chemin)
    code, lignes, duree, erreur = jouer([chemin] + options, journal)
    apres = sha256_ou_rien(chemin)
    prov = {"binaire": nom, "sha256": avant, "inchange": avant == apres, "construit": attendu == avant,
            "commande": [nom] + options,
            "journal": os.path.relpath(journal, args.sortie) if journal else None,
            "journal_sha256": sha256_ou_rien(journal)}
    return code, lignes, duree, erreur, prov


def problemes_provenance(prov):
    out = []
    if prov.get("sha256") is None:
        out.append("binaire %s absent" % prov.get("binaire"))
    else:
        if not prov.get("construit"):
            out.append("binaire %s sans provenance (empreinte differente de « construire »)" % prov.get("binaire"))
        if not prov.get("inchange"):
            out.append("binaire %s modifie pendant l'execution" % prov.get("binaire"))
    if not prov.get("journal_sha256"):
        out.append("journal absent")
    return out


# ---- Validation d'une execution du vidage avec le quatrieme bras -----------------------------------------------------
def valider_prise(lignes, code, k_max):
    """Lignes d'un processus mhgp12_vidage --chrono-resolution R --bras-saut ; rend (problemes, foret_differente,
    resume). La foret differente (code 1, ligne exit « ecart ») n'est pas un defaut de preuve : c'est un ecart."""
    problemes, ordres = [], {}
    attendus = list(range(2, k_max + 1))
    if len(lignes_de(lignes, "voisins_etage_p")) != 1:
        problemes.append("ligne voisins_etage_p absente ou multiple")
    un_fil = {l.get("k"): l for l in lignes_de(lignes, "resolution_un_fil")}
    saut = {}
    for l in lignes_de(lignes, "resolution_saut"):
        if l.get("k") in saut:
            problemes.append("ordre %s en double" % l.get("k"))
        saut[l.get("k")] = l
    if sorted(saut, key=str) != sorted(attendus, key=str) or sorted(un_fil, key=str) != sorted(attendus, key=str):
        problemes.append("ordres 2..%d attendus (resolution_un_fil et resolution_saut)" % k_max)
    foret_differente = False
    for k in attendus:
        l, u = saut.get(k), un_fil.get(k)
        if l is None or u is None:
            continue
        sec = l.get("secondes", {})
        census = l.get("census", {})
        foret = l.get("foret", {})
        if u.get("graines_identiques") is not True:
            problemes.append("ordre %d : graines des trois premiers bras non identiques" % k)
        if not all(fini_positif(sec.get(b)) for b in ("v11", "replique_v11", "replique_v12", "replique_v12_saut")):
            problemes.append("ordre %d : temps des quatre bras non finis ou non positifs" % k)
        v12 = census.get("replique_v12", {})
        sa = census.get("replique_v12_saut", {})
        if not all(entier(x) and x >= 0 for x in (v12.get("satures"), v12.get("complets"), sa.get("satures"),
                                                     sa.get("complets"))):
            problemes.append("ordre %d : comptes de census absents" % k)
        if foret.get("controle_v11_reproduit") is not True or not isinstance(foret.get("identique"), bool):
            problemes.append("ordre %d : controle de foret absent ou non reproduit sur la v11" % k)
        elif not foret["identique"]:
            foret_differente = True
        ordres[k] = {"secondes": {b: sec.get(b) for b in ("v11", "replique_v11", "replique_v12", "replique_v12_saut")},
                     "census": census, "sauts": l.get("sauts"), "pas": l.get("pas"), "chaines": l.get("chaines"),
                     "graines_differentes_de_la_v11": l.get("graines_differentes_de_la_v11"), "foret": foret}
    sorties = lignes_de(lignes, "exit")
    if len(sorties) != 1:
        problemes.append("ligne exit absente ou multiple")
    else:
        etat = sorties[0].get("status")
        if foret_differente:
            if code != 1 or etat != "ecart":
                problemes.append("foret differente sans code 1 ni sortie « ecart »")
        elif code != 0 or etat != "ok":
            problemes.append("sortie du vidage non ok (code %s, %s)" % (code, etat))
    if len(lignes_de(lignes, "fin")) != 1:
        problemes.append("ligne fin absente ou multiple")
    for phase in ("exception", "ligne_illisible"):
        if lignes_de(lignes, phase):
            problemes.append("phase %s" % phase)
    resume = None
    if not problemes:
        t12 = sum(ordres[k]["secondes"]["replique_v12"] for k in attendus)
        tsa = sum(ordres[k]["secondes"]["replique_v12_saut"] for k in attendus)
        sat12 = sum(ordres[k]["census"]["replique_v12"]["satures"] for k in attendus)
        satsa = sum(ordres[k]["census"]["replique_v12_saut"]["satures"] for k in attendus)
        resume = {"replique_v12": t12, "replique_v12_saut": tsa, "rapport_total": tsa / t12,
                  "satures_v12": sat12, "satures_saut": satsa,
                  "part_satures_evites": (sat12 - satsa) / sat12 if sat12 else 0.0,
                  "complets_v12": sum(ordres[k]["census"]["replique_v12"]["complets"] for k in attendus),
                  "complets_saut": sum(ordres[k]["census"]["replique_v12_saut"]["complets"] for k in attendus)}
    return problemes, foret_differente, {"ordres": ordres, "totaux": resume}


def options_vidage(args, trame, k, dossier, passes):
    base = os.path.join(os.path.abspath(args.donnees), FICHIERS.get(trame, trame))
    feuille = 16 if k <= 5 else 24
    return [base + ".u32le", base + ".ids.u32le", trame, str(k), str(feuille), str(args.fils), dossier, "--journal",
            "aucun", "--chrono-resolution", str(passes), "--bras-saut"]


def effacer_vidages(dossier):
    """Empreintes des vidages ecrits par une prise, puis effacement (donnees KITTI, jamais publiees)."""
    out = {}
    for nom in sorted(os.listdir(dossier)):
        if nom.endswith(".bin"):
            chemin = os.path.join(dossier, nom)
            out[nom] = sha256(chemin)
            os.remove(chemin)
    return out


# ---- Etapes -----------------------------------------------------------------------------------------------------------
def portes(args, rapport):
    pire = 0
    racine = os.path.join(args.sortie, "portes", args.campagne)
    os.makedirs(racine)
    # Porte gravee (temoins geometriques et temoins d'admission) et ses mutants : « cote nul admis » et un mutant par
    # garde d'admission (route, ordres, bilan), chacun tue par ses ecarts, jamais par un simple code.
    for nom, exe, cote_nul, garde, attendu in PORTES_G1:
        journal = os.path.join(racine, nom + ".jsonl")
        code, lignes, duree, _, prov = lancer(args, rapport, exe, ["--porte"], journal)
        raisons = problemes_provenance(prov)
        resume = [l for l in lignes if l.get("porte") == "mes_g1"]
        if len(resume) != 1 or any(resume[0].get(c) != v for c, v in TEMOINS_G1.items()) or \
                resume[0].get("mutant_cote_nul") is not cote_nul or resume[0].get("mutant_garde") != garde:
            raisons.append("resume de porte absent, d'un autre binaire ou temoins manquants")
        elif attendu == 0 and (code != 0 or resume[0].get("ecarts") != 0):
            raisons.append("porte en echec (code %d)" % code)
        elif attendu == 1 and (code != 1 or not entier(resume[0].get("ecarts")) or resume[0]["ecarts"] < 1):
            raisons.append("mutant survivant (code %d)" % code)
        ranger(rapport, "portes", nom, {"code": code, "attendu": attendu, "conforme": not raisons, "raisons": raisons,
                                        "binaire": prov, "journal": prov.get("journal"),
                                        "journal_sha256": prov.get("journal_sha256"), "secondes": round(duree, 2)})
        if raisons:
            pire = max(pire, 3 if any("provenance" in r or "absent" in r for r in raisons) else 1)
    # 3, 4 : quatrieme bras sur le premier cas, et mutant du controle de foret (tue par une foret differente).
    trame, k = cas_liste(args.cas)[0]
    for nom, exe, attendu in (("bras_saut_" + nom_cas(trame, k), "mhgp12_vidage", 0),
                              ("bras_saut_mutant_cibles_decalees_" + nom_cas(trame, k),
                               "mhgp12_vidage_mutant_cibles_decalees", 1)):
        dossier = os.path.join(racine, nom)
        os.makedirs(dossier)
        journal = os.path.join(dossier, "vidage.jsonl")
        print("[%s] porte %s ..." % (maintenant(), nom), flush=True)
        code, lignes, duree, erreur, prov = lancer(args, rapport, exe, options_vidage(args, trame, k, dossier, 1),
                                                   journal)
        vidages = effacer_vidages(dossier)
        raisons = problemes_provenance(prov)
        p, differente, _ = valider_prise(lignes, code, k)
        raisons += p
        if attendu == 0 and differente:
            raisons.append("foret du bras differente de celle de la v11")
        if attendu == 1 and not differente:
            raisons.append("mutant survivant : foret declaree identique")
        ranger(rapport, "portes", nom, {"code": code, "attendu": attendu, "conforme": not raisons, "raisons": raisons,
                                        "binaire": prov, "journal": prov.get("journal"),
                                        "journal_sha256": prov.get("journal_sha256"), "vidages_sha256": vidages,
                                        "secondes": round(duree, 2), "erreur": erreur if code not in (0, 1) else ""})
        if raisons:
            pire = max(pire, 3 if any("provenance" in r or "absent" in r for r in raisons) else 1)
        print("    code %d, %.1f s%s" % (code, duree, "" if not raisons else ", " + "; ".join(raisons[:3])), flush=True)
    return pire


def campagne(args, rapport):
    pire = 0
    for trame, k in cas_liste(args.cas):
        cas = nom_cas(trame, k)
        bloc = rapport.setdefault("campagnes", {}).setdefault(cas, {})
        if args.campagne in bloc:
            raise SystemExit("campagne %s deja presente pour %s : jamais ecrasee" % (args.campagne, cas))
        camp = {"debut": maintenant(), "processus_demandes": args.processus, "passes_par_processus": args.passes,
                "statistique_intra_processus": "minimum de %d passe(s) dans le processus, par bras et par ordre"
                                               % args.passes, "prises": [], "refus": []}
        bloc[args.campagne] = camp
        racine = os.path.join(args.sortie, cas, "campagnes", args.campagne)
        reference = None
        for n in range(args.processus):
            dossier = os.path.join(racine, "p%d" % n)
            os.makedirs(dossier)  # dossier neuf : refuse s'il existe deja
            print("[%s] campagne %s %s (processus %d/%d) ..." % (maintenant(), args.campagne, cas, n + 1,
                                                                 args.processus), flush=True)
            journal = os.path.join(dossier, "vidage.jsonl")
            code, lignes, duree, erreur, prov = lancer(args, rapport, "mhgp12_vidage",
                                                       options_vidage(args, trame, k, dossier, args.passes), journal)
            vidages = effacer_vidages(dossier)
            raisons = problemes_provenance(prov)
            p, differente, donnees = valider_prise(lignes, code, k)
            raisons += p
            if reference is None and vidages:
                reference = vidages
            if not vidages or vidages != reference:
                raisons.append("vidages absents ou differents de ceux de la premiere prise (autres entrees)")
            valide = not raisons
            camp["prises"].append({"processus": n, "code": code, "secondes": round(duree, 2), "valide": valide,
                                   "raisons": raisons, "binaire": prov, "journal": prov.get("journal"),
                                   "journal_sha256": prov.get("journal_sha256"), "vidages_sha256": vidages,
                                   "foret_differente": differente, "ordres": donnees["ordres"],
                                   "totaux": donnees["totaux"], "erreur": erreur if code not in (0, 1) else ""})
            if differente:
                pire = max(pire, 1)
            if not valide:
                pire = 3
            t = donnees["totaux"]
            print("    code %d, %.1f s%s" % (code, duree, (", rapport %.3f, satures evites %.1f %%" % (
                t["rapport_total"], 100 * t["part_satures_evites"])) if valide else ", REFUS : " +
                "; ".join(raisons[:3])), flush=True)
        camp["fin"] = maintenant()
        camp["vidages_reference"] = reference
        camp["prises_valides"] = sum(p["valide"] for p in camp["prises"])
        if camp["prises_valides"] != args.processus:
            camp["refus"].append("%d prises valides sur %d" % (camp["prises_valides"], args.processus))
    return pire


# ---- Juge -------------------------------------------------------------------------------------------------------------
def bootstrap_gm(logs, rng, tirages):
    """Moyenne geometrique et IC 95 % par bootstrap (memes conventions que REGLE_M3 et le juge de MES-M2)."""
    gm = math.exp(sum(logs) / len(logs))
    boot = []
    for _ in range(tirages):
        echantillon = [logs[rng.randrange(len(logs))] for _ in logs]
        boot.append(sum(echantillon) / len(echantillon))
    boot.sort()
    return gm, math.exp(boot[int(0.025 * tirages)]), math.exp(boot[int(0.975 * tirages) - 1])


def derniere_campagne(rapport, cas):
    campagnes = rapport.get("campagnes", {}).get(cas, {})
    if not campagnes:
        return None, None
    ident = max(campagnes, key=lambda c: (campagnes[c].get("debut", ""), c))
    return ident, campagnes[ident]


def juger(rapport, sortie):
    """Verdict de REGLE_G1 ; cite les journaux et leurs empreintes, re-hachees ici (un journal modifie est refuse)."""
    binaires = rapport.get("construction", {}).get("binaires", {})
    refus, rejets, cas_out, preuves = [], [], {}, {}
    if not binaires or any(binaires.get(n) is None for n in BINAIRES):
        refus.append("construction absente ou incomplete")

    def journal_intact(chemin, empreinte):
        if not chemin or not empreinte:
            return False
        complet = os.path.join(sortie, chemin)
        return os.path.isfile(complet) and sha256(complet) == empreinte

    portes_ = rapport.get("portes", {})
    noms_portes = [p[0] for p in PORTES_G1]
    noms_portes += [n for n in portes_ if n.startswith("bras_saut_")]
    if not any(n.startswith("bras_saut_mutant_") for n in noms_portes) or \
            not any(n.startswith("bras_saut_") and "mutant" not in n for n in noms_portes):
        refus.append("portes du quatrieme bras absentes (bras et mutant du controle de foret)")
    for nom in noms_portes:
        b = portes_.get(nom)
        exe = (b or {}).get("binaire", {}).get("binaire")
        if not b or not b.get("conforme") or not exe or b["binaire"].get("sha256") != binaires.get(exe):
            refus.append("porte %s absente, non conforme ou d'un autre binaire" % nom)
        elif not journal_intact(b.get("journal"), b.get("journal_sha256")):
            refus.append("porte %s : journal absent ou modifie" % nom)
        else:
            preuves[b["journal"]] = b["journal_sha256"]
    rng = random.Random(REGLE_G1["graine"])
    for cas in REGLE_G1["cas_decisifs"]:
        ident, camp = derniere_campagne(rapport, cas)
        if camp is None:
            refus.append("%s : aucune campagne" % cas)
            continue
        prises = camp.get("prises", [])
        valides = [p for p in prises if p.get("valide")]
        exigees = max(REGLE_G1["processus_min"], camp.get("processus_demandes", 0))
        if camp.get("refus") or len(valides) != len(prises) or len(valides) < exigees:
            refus.append("%s : campagne %s : %d prises valides sur %d exigees" % (cas, ident, len(valides), exigees))
            continue
        if any(p["binaire"].get("sha256") != binaires.get("mhgp12_vidage") for p in valides):
            refus.append("%s : prise d'un autre binaire de vidage" % cas)
            continue
        if any(not journal_intact(p.get("journal"), p.get("journal_sha256")) for p in valides):
            refus.append("%s : journal de prise absent ou modifie" % cas)
            continue
        if any(p.get("vidages_sha256") != camp.get("vidages_reference") for p in valides):
            refus.append("%s : vidages differents d'une prise a l'autre" % cas)
            continue
        evites = {round(p["totaux"]["part_satures_evites"], 12) for p in valides}
        if len(evites) != 1:
            refus.append("%s : comptes de census differents d'une prise a l'autre (non deterministes)" % cas)
            continue
        preuves.update({p["journal"]: p["journal_sha256"] for p in valides})
        logs = [math.log(p["totaux"]["rapport_total"]) for p in valides]
        gm, bas, haut = bootstrap_gm(logs, rng, REGLE_G1["bootstrap"])
        part = valides[0]["totaux"]["part_satures_evites"]
        differente = any(p.get("foret_differente") for p in valides)
        cas_out[cas] = {"campagne": ident, "prises": len(valides),
                        "rapports": [p["totaux"]["rapport_total"] for p in valides], "moyenne_geometrique": gm,
                        "ic95": [bas, haut], "gain": 1 - gm, "part_satures_evites": part,
                        "satures_v12": valides[0]["totaux"]["satures_v12"],
                        "satures_saut": valides[0]["totaux"]["satures_saut"], "foret_differente": differente,
                        "passes_par_processus": camp.get("passes_par_processus")}
        if part < REGLE_G1["seuil_satures_evites"]:
            rejets.append("%s : %.1f %% des censuses satures evites, sous 50 %%" % (cas, 100 * part))
        if differente:
            rejets.append("%s : foret du bras differente de celle de la v11" % cas)
        if haut >= REGLE_G1["seuil_borne_haute"]:
            rejets.append("%s : borne haute %.3f >= 1" % (cas, haut))
    verdict = "refuse" if refus else ("rejete" if rejets else "adopte")
    informatifs, rng_info = {}, random.Random(REGLE_G1["graine"])
    for cas in sorted(rapport.get("campagnes", {})):
        if cas in REGLE_G1["cas_decisifs"]:
            continue
        ident, camp = derniere_campagne(rapport, cas)
        valides = [p for p in camp.get("prises", []) if p.get("valide")] if camp else []
        if not valides or len(valides) != len(camp.get("prises", [])):
            informatifs[cas] = {"campagne": ident, "prises_valides": len(valides), "complete": False}
            continue
        gm, bas, haut = bootstrap_gm([math.log(p["totaux"]["rapport_total"]) for p in valides], rng_info,
                                     REGLE_G1["bootstrap"])
        informatifs[cas] = {"campagne": ident, "prises_valides": len(valides), "complete": True,
                            "moyenne_geometrique": gm, "ic95": [bas, haut],
                            "part_satures_evites": valides[0]["totaux"]["part_satures_evites"],
                            "foret_differente": any(p.get("foret_differente") for p in valides)}
    return {"verdict": verdict, "regle": REGLE_G1, "refus": refus, "rejets": rejets, "cas": cas_out,
            "cas_publies_sans_decision": informatifs,
            "binaires": {n: binaires.get(n) for n in BINAIRES}, "preuves": dict(sorted(preuves.items()))}


def mediane(valeurs):
    v = sorted(valeurs)
    n = len(v)
    return v[n // 2] if n % 2 else 0.5 * (v[n // 2 - 1] + v[n // 2])


def tableaux(rapport):
    out = ["# MES-G1, seconde moitie : quatrieme bras replique_v12_saut\n",
           "Tables tirees de `rapport_g1.json` (aucune valeur recopiee a la main). Verdict : **%s**.\n"
           % rapport.get("verdict", {}).get("verdict", "absent")]
    v = rapport.get("verdict", {})
    for titre, cle in (("Refus", "refus"), ("Rejets", "rejets")):
        if v.get(cle):
            out.append("%s : %s\n" % (titre, " ; ".join(v[cle])))
    out.append("| Cas | Prises | Rapport saut / v12 (moyenne geometrique) | IC 95 % | Censuses satures evites | Foret |")
    out.append("| --- | ---: | ---: | --- | ---: | --- |")
    toutes = dict(v.get("cas", {}))
    toutes.update({c: x for c, x in v.get("cas_publies_sans_decision", {}).items() if x.get("complete")})
    for cas in sorted(toutes):
        x = toutes[cas]
        out.append("| %s | %d | %.3f | [%.3f ; %.3f] | %.1f %% | %s |" % (
            cas, x.get("prises", x.get("prises_valides", 0)), x["moyenne_geometrique"], x["ic95"][0], x["ic95"][1],
            100 * x["part_satures_evites"], "differente" if x.get("foret_differente") else "identique"))
    out.append("")
    for cas in sorted(rapport.get("campagnes", {})):
        ident, camp = derniere_campagne(rapport, cas)
        valides = [p for p in camp.get("prises", []) if p.get("valide")]
        if not valides:
            continue
        out.append("## %s (campagne %s, %d prises valides, minimum de %s passe(s) par processus)\n" % (
            cas, ident, len(valides), camp.get("passes_par_processus")))
        out.append("| k | v11 (s) | replique v11 (s) | replique v12 (s) | replique v12 saut (s) | satures v12 | satures "
                   "saut | complets v12 | complets saut | sauts certifies / tentatives | tests par tentative | pas v12 | "
                   "pas saut | chaine max v12 / saut | graines differentes | foret |")
        out.append("| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- | ---: | ---: | ---: | --- | ---: "
                   "| --- |")
        ordres = sorted(valides[0]["ordres"], key=lambda x: int(x))
        for k in ordres:
            o = valides[0]["ordres"][k]
            med = {b: mediane([p["ordres"][k]["secondes"][b] for p in valides])
                   for b in ("v11", "replique_v11", "replique_v12", "replique_v12_saut")}
            c, s = o["census"], o["sauts"]
            out.append("| %s | %.3f | %.3f | %.3f | %.3f | %d | %d | %d | %d | %d / %d | %.1f | %d | %d | %d / %d | %d "
                       "| %s |" % (
                           k, med["v11"], med["replique_v11"], med["replique_v12"], med["replique_v12_saut"],
                           c["replique_v12"]["satures"], c["replique_v12_saut"]["satures"],
                           c["replique_v12"]["complets"], c["replique_v12_saut"]["complets"], s["certifies"],
                           s["tentatives"], s["tests_par_tentative"], o["pas"]["replique_v12"],
                           o["pas"]["replique_v12_saut"], o["chaines"]["replique_v12"]["max"],
                           o["chaines"]["replique_v12_saut"]["max"], o["graines_differentes_de_la_v11"],
                           "identique" if o["foret"].get("identique") else "differente"))
        out.append("\nTemps : mediane des prises du minimum de R passes par processus (informatif ; le juge porte sur "
                   "les rapports par processus).\n")
    return "\n".join(out) + "\n"


def rapport_etape(args, rapport):
    rapport["verdict"] = juger(rapport, os.path.abspath(args.sortie))
    with open(os.path.join(args.sortie, "tableaux_g1.md"), "w", encoding="utf-8") as f:
        f.write(tableaux(rapport))
    v = rapport["verdict"]
    print("verdict MES-G1 (seconde moitie) : %s%s" % (v["verdict"], "" if v["verdict"] == "adopte" else
                                                      " (" + " ; ".join((v["refus"] or v["rejets"])[:4]) + ")"),
          flush=True)
    return 0


def main(argv):
    p = argparse.ArgumentParser(description="pilote et juge de MES-G1, seconde moitie (quatrieme bras)")
    p.add_argument("--v11-source", required=True)
    p.add_argument("--v11-build", required=True)
    p.add_argument("--donnees", required=True)
    p.add_argument("--sortie", required=True)
    p.add_argument("--construction", required=True)
    p.add_argument("--cas", default="ng00:5,ng01:5,ng02:5")
    p.add_argument("--processus", type=int, default=5)
    p.add_argument("--passes", type=int, default=3)
    p.add_argument("--fils", type=int, default=3, help="fils du vidage (les mesures sont a un fil)")
    p.add_argument("--fils-construction", type=int, default=3)
    p.add_argument("--campagne", default=None, help="identifiant de campagne (defaut : date et hasard)")
    p.add_argument("etapes", nargs="+", choices=["construire", "portes", "campagne", "rapport", "tout"])
    try:
        args = p.parse_args(argv[1:])
    except SystemExit as e:
        return 0 if e.code == 0 else 2
    if args.processus < 1 or args.passes < 1 or args.fils < 1 or args.fils_construction < 1:
        print("processus, passes et fils : entiers positifs", file=sys.stderr)
        return 2
    cas_liste(args.cas)
    args.sortie = os.path.abspath(args.sortie)
    args.campagne = args.campagne or "c%s_%s" % (datetime.datetime.now(datetime.timezone.utc).strftime(
        "%Y%m%dT%H%M%SZ"), secrets.token_hex(3))
    etapes = ["construire", "portes", "campagne", "rapport"] if "tout" in args.etapes else args.etapes
    rapport = charger(args.sortie)
    sources_debut = hacher_sources()
    invocation = {"campagne": args.campagne, "etapes": etapes, "debut": maintenant(), "sources_debut": sources_debut,
                  "argv": argv[1:], "codes": {}}
    rapport.setdefault("invocations", []).append(invocation)
    code = 0
    for etape in etapes:
        if etape == "construire":
            c = construire(args, rapport)
        elif etape == "portes":
            c = portes(args, rapport)
        elif etape == "campagne":
            c = campagne(args, rapport)
        else:
            c = rapport_etape(args, rapport)
        invocation["codes"][etape] = c
        code = max(code, c)
        sauver(args.sortie, rapport)
    invocation["fin"] = maintenant()
    invocation["sources_fin"] = hacher_sources()
    construites = rapport.get("construction", {}).get("sources")
    if invocation["sources_fin"] != sources_debut or (construites and construites != sources_debut):
        invocation["refus_sources"] = "sources du microbanc modifiees pendant l'invocation ou depuis la construction"
        code = max(code, 3)
    if "rapport" in etapes and invocation.get("refus_sources"):
        rapport["verdict"]["verdict"] = "refuse"
        rapport["verdict"]["refus"].append(invocation["refus_sources"])
        with open(os.path.join(args.sortie, "tableaux_g1.md"), "w", encoding="utf-8") as f:
            f.write(tableaux(rapport))
    sauver(args.sortie, rapport)
    print(json.dumps({"pilote": "mes_g1", "campagne": args.campagne, "code": code,
                      "verdict": rapport.get("verdict", {}).get("verdict")}), flush=True)
    return code


if __name__ == "__main__":
    sys.exit(main(sys.argv))
