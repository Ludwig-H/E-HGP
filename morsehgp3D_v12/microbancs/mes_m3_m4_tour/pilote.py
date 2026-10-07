#!/usr/bin/env python3
"""Pilote des microbancs de la tour v12 (MES-M3, MES-M4) et du vidage de la v11. Bibliotheque standard seule.

Etapes (dans l'ordre si plusieurs) :
  construire   cmake + make (-j fils) des binaires mhgp12_* liees a la construction v11 donnee ; empreintes de tous
               les binaires et de libmhgp11.a : toute execution ulterieure est rattachee a ces empreintes
  portes       portes des microbancs : temoins graves (WIT-T1-CARRE, borne CST-0212, plateaux), mutants causaux ; un
               mutant n'est tue que par sa reponse geometrique (temoin faux, ecarts), jamais par un simple code
  vider        mhgp12_vidage sur chaque cas (trame:K) ; empreinte MHGP11FUL1 comparee a la reference (MESURE.md § 4) ;
               vidages inventories (sections exigees, FLOWER a k >= 2) et haches : references des etapes suivantes
  resolution   mesure de resolution a un fil (trois bras, regle de MES-M3) REPLIQUEE : --processus processus neufs
               par cas ; chaque prise reecrit les vidages, qui doivent etre identiques a l'octet a ceux de « vider »
               (memes traces, memes parties), puis sont effaces ; chaque campagne est conservee, jamais ecrasee
  m3           MES-M3 sur chaque vidage (identite exigee sur toutes les parties de tous les ordres, temps a un fil)
  m3var        MES-M3, variante « mere puis Welzl » sur le premier cas : binaire normal (identite, test S dans F
               decisif) et mutant sans le test S dans F (doit etre tue sur donnees reelles par des ecarts de sphere)
  m4           MES-M4 sur chaque vidage (identite, LEM-T6 sur toutes les naissances des ordres 2..K, temps a un fil) ;
               mutant joue sur un vidage (doit sortir en ecart d'identite)
  rapport      synthese, verdicts (MES-M3 : adoption ; MES-M4 et portes : conformite), tableaux Markdown
  tout         construire portes vider resolution m3 m3var m4 rapport

Preuves (constats CST-0018, CST-0213, CST-0214 de l'auditeur Codex) : aucune conformite sans preuve. Chaque execution
est rattachee au binaire lance (empreinte avant et apres, egale a celle de « construire »), aux vidages lus (empreintes
egales a celles de « vider », avant et apres) et a son journal (empreinte) ; chaque sortie doit porter toutes ses
phases et tous ses ordres ; chaque verdict cite les fichiers et empreintes sur lesquels il repose. Rien n'est ecrase :
un bloc remplace passe dans « historique », chaque campagne a son identifiant et ses journaux.

Regle de MES-M3 (PLAN.md, ecrite avant la mesure) : CPU de resolution a K10 reduit d'au moins 40 % a un fil, resultats
identiques. Statistique (MESURE.md § 5, comme MES-M2) : par processus, rapport des sommes sur les ordres 2..K des temps
de la replique v12 et de la replique v11 (chaque temps est le MINIMUM de R passes dans le processus) ; moyenne
geometrique des rapports par processus et IC 95 % par bootstrap sur les processus (10 000 tirages, graine fixe) ;
« adopte » si la borne haute est au plus 0,60 sur chacun des cas ng00, ng01, ng02 a K10, avec au moins 5 prises par
cas ; « rejete » si une borne haute depasse 0,60 ou si l'identite est en defaut ; « refuse » si une preuve manque.

Exemple (codespace ; sur G4, adapter les chemins, et --fils 48 pour le vidage seulement) :
  python3 pilote.py --v11-source /workspaces/E-HGP/morsehgp3D_v11 --v11-build <build-v11> \\
      --donnees /workspaces/E-HGP/build/v11-full-data-20261002 --sortie out --cas ng00:5,ng01:5,ng02:5,ng00:10 tout

Les vidages derivent de donnees SemanticKITTI (CC BY-NC-SA) : ils restent dans --sortie, jamais dans un depot.
Codes : 0 conforme ; 1 ecart d'identite, porte en echec ou mutant survivant ; 2 usage ; 3 refus (preuve absente,
perimee ou non rattachee). Le verdict d'adoption de MES-M3 est dans le rapport et ne change pas le code.
"""
import argparse
import datetime
import hashlib
import json
import math
import os
import platform
import random
import shutil
import struct
import subprocess
import sys
import time

ICI = os.path.dirname(os.path.abspath(__file__))
PROFIL = 21  # profil de la construction v11 liee (MHGP11_COORD_BITS)

# Empreintes SHA-256 des vidages MHGP11FUL1 de reference (morsehgp3D_v12/docs/MESURE.md, § 4 ; moteur ac081a06f, u21).
EMPREINTES = {
    ("ng00", 5): "3a2bfb4f9f48b4b0cc5b0318d9fcf4887906638e034b2c97dda1918e3170a6fe",
    ("ng01", 5): "5212a2ced81bf69bd2abfb935a3b14a35158df25339285a0d25a9d5d5c09e091",
    ("ng02", 5): "78feb765e21c8e4582762a25bd0dd36a455747d3f0df80523e19f7ba4be0e207",
    ("ng00", 10): "61a4245b91d9a4fdad012f0a2e26a63c3e48db1d46a180db756f2c4e4aa77295",
    ("ng01", 10): "838a447e0b92e13e42f7f69a84fd536d5d46d26463d3f4cc7c688475878fe0de",
    ("ng02", 10): "81f89995eaccb497cfe92abb81213bebd0d4ce5076210570d86aa9456cf7ff2e",
    ("u8000", 5): "f87dbb19dd928311fcb65d5a98d30b4fb50eddf7de1d26708bbc278f46dcc7cf",
    ("u16000", 5): "141bc7d6523154cff1dd22b288e4155259e3d97a82877205887642bad8286889",
    ("u32000", 5): "a7563907a5c9f1b273776b811d8a559eb80713eb161460edd948664f5433811b",
}
FICHIERS = {
    "ng00": "lidar_ng00", "ng01": "lidar_ng01", "ng02": "lidar_ng02",
    "u8000": "uniform_u18_n8000", "u16000": "uniform_u18_n16000", "u32000": "uniform_u18_n32000",
}
BINAIRES = ["mhgp12_vidage", "mhgp12_mes_m3", "mhgp12_mes_m3_mutant_sans_s_dans_f", "mhgp12_mes_m4",
            "mhgp12_mes_m4_mutant_sans_contraction"]
PORTES = [  # (nom, binaire, options, code attendu)
    ("mes_m3", "mhgp12_mes_m3", ["--porte"], 0),
    ("mes_m3_mutant_sans_s_dans_f", "mhgp12_mes_m3_mutant_sans_s_dans_f", ["--porte"], 1),
    ("mes_m4", "mhgp12_mes_m4", ["--porte", "6000"], 0),
    ("mes_m4_mutant_sans_contraction", "mhgp12_mes_m4_mutant_sans_contraction", ["--porte", "6000"], 1),
]
# Regle de MES-M3, ecrite avant la mesure (PLAN.md § 1 ; statistique de MESURE.md § 5, comme MES-M2).
REGLE_M3 = {"seuil": 0.60, "cas_decisifs": ("ng00_k10", "ng01_k10", "ng02_k10"), "processus_min": 5,
            "bootstrap": 10000, "graine": 20261007,
            "statistique": "par processus : somme sur les ordres 2..K du minimum de R passes de la replique v12, "
                           "rapportee a la meme somme pour la replique v11 ; moyenne geometrique des rapports par "
                           "processus, IC 95 % par bootstrap sur les processus (10 000 tirages, graine fixe)",
            "adoption": "borne haute <= 0,60 sur chaque cas qui decide (ng00, ng01, ng02 a K10), au moins 5 prises "
                        "par cas, identite de MES-M3 et graines identiques sur les trois bras"}
# Seuils de MES-M4 (PLAN.md § 1) : publies et compares, sans verdict d'adoption automatique (statistique non ecrite).
SEUILS_M4 = {"noyau_ms_k5": 10.0, "noyau_ms_k10": 35.0, "contraction_ms": 3.0}


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


def jouer(commande, journal=None, cwd=None):
    """Execute une commande ; rend (code, lignes JSON, duree, fin de l'erreur standard). La sortie brute est gardee
    dans `journal` (jamais ecrase : le nom porte l'identifiant de campagne)."""
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


def machine():
    info = {"plateforme": platform.platform(), "python": platform.python_version(), "coeurs": os.cpu_count()}
    try:
        with open("/proc/cpuinfo", encoding="utf-8") as f:
            for ligne in f:
                if ligne.startswith("model name"):
                    info["processeur"] = ligne.split(":", 1)[1].strip()
                    break
    except OSError:
        pass
    try:
        info["compilateur"] = subprocess.run(["c++", "--version"], stdout=subprocess.PIPE, text=True,
                                             check=False).stdout.splitlines()[0]
    except (OSError, IndexError):
        pass
    try:
        info["charge"] = os.getloadavg()
    except OSError:
        pass
    return info


def cas_liste(texte):
    """Cas `trame:K`. Une trame hors de FICHIERS designe <donnees>/<trame>.u32le et <trame>.ids.u32le (nouvelles trames
    de plusieurs sequences, decision D7) ; elle n'a pas d'empreinte de reference."""
    cas = []
    for item in texte.split(","):
        try:
            trame, k = item.split(":")
            k = int(k)
        except ValueError:
            raise SystemExit("cas invalide : " + item)
        if not trame or "/" in trame or not 1 <= k <= 12:
            raise SystemExit("cas invalide : " + item)
        cas.append((trame, k))
    return cas


def nom_cas(trame, k):
    return "%s_k%d" % (trame, k)


def dossier_cas(sortie, trame, k):
    return os.path.join(sortie, nom_cas(trame, k))


# Sections par genre de fichier (format version 1, README.md § 4) : etiquette -> taille d'un element (None : 4k pour
# PARTS). Toutes sont exigees, sauf FLOWER : exigee a l'ordre k >= 2, absente a l'ordre 1 (CST-0214).
SECTIONS = {
    1: {"SITEXYZ": 12, "BALLS": 32, "POPOFF": 8, "POPVAL": 4, "NLEVELS": 8},
    2: {"BIRTHS": 16, "BCENTER": 64, "CELLS": 24, "CELLOFF": 8, "TRACEA": 8, "SEEDS": 12, "PARTOFF": 8,
        "PARTS": None, "PARTINF": 8},
    3: {"FNODES": 24, "FEDGES": 4, "FLOWER": 4, "FMETA": 8},
}


def lire_vidage(chemin):
    """Lecteur strict d'un fichier MHGP12DP version 1 : en-tete et inventaire des sections (sans les donnees).
    Refuse (ValueError) une magie ou une version inconnue, une section tronquee, inattendue, en double, ABSENTE ou de
    taille fausse, des octets en trop."""
    taille = os.path.getsize(chemin)
    with open(chemin, "rb") as f:
        tete = f.read(64)
        if len(tete) != 64 or tete[:8] != b"MHGP12DP":
            raise ValueError("magie inconnue : " + chemin)
        version, genre, bits, kmax, ordre, nsec = struct.unpack_from("<6I", tete, 8)
        sites, = struct.unpack_from("<Q", tete, 32)
        trame = tete[40:64].rstrip(b"\0").decode("ascii", "replace")
        if version != 1 or genre not in SECTIONS:
            raise ValueError("version ou genre non pris en charge : " + chemin)
        pos = 64
        inventaire = {}
        for _ in range(nsec):
            f.seek(pos)
            sh = f.read(24)
            if len(sh) != 24:
                raise ValueError("section tronquee : " + chemin)
            tag = sh[:8].rstrip(b"\0").decode("ascii", "replace")
            elem, _res, nombre = struct.unpack_from("<IIQ", sh, 8)
            attendu = SECTIONS[genre].get(tag, -1)
            if attendu == -1:
                raise ValueError("section inattendue %s : %s" % (tag, chemin))
            if tag == "PARTS":
                attendu = 4 * ordre
            if attendu is not None and elem != attendu:
                raise ValueError("taille d'element %d pour %s : %s" % (elem, tag, chemin))
            if tag in inventaire:
                raise ValueError("section en double %s : %s" % (tag, chemin))
            octets = elem * nombre
            pos += 24 + octets + (-octets) % 8
            if pos > taille:
                raise ValueError("donnees tronquees %s : %s" % (tag, chemin))
            inventaire[tag] = {"element": elem, "nombre": nombre}
        if pos != taille:
            raise ValueError("octets en trop : " + chemin)
    exigees = set(SECTIONS[genre]) - ({"FLOWER"} if genre == 3 and ordre < 2 else set())
    absentes = sorted(exigees - set(inventaire))
    if absentes:
        raise ValueError("sections absentes %s : %s" % (",".join(absentes), chemin))
    if genre == 3 and ordre < 2 and "FLOWER" in inventaire:
        raise ValueError("FLOWER a l'ordre 1 : " + chemin)
    return {"version": version, "genre": genre, "coord_bits": bits, "kmax": kmax, "ordre": ordre, "sites": sites,
            "trame": trame, "sections": inventaire}


def fichiers_attendus(k_max):
    return ["cat.bin"] + ["ordre_%d.bin" % k for k in range(1, k_max + 1)] + \
        ["foret_%d.bin" % k for k in range(1, k_max + 1)]


def inventorier(dossier, trame, k_max):
    """Inventaire et empreintes des vidages d'un cas ; rend (fichiers, problemes)."""
    fichiers, problemes = {}, []
    for nom in fichiers_attendus(k_max):
        chemin = os.path.join(dossier, nom)
        if not os.path.isfile(chemin):
            problemes.append("vidage absent : " + nom)
            continue
        try:
            inv = lire_vidage(chemin)
        except (OSError, ValueError) as e:
            problemes.append("vidage illisible : %s" % e)
            continue
        genre, ordre = (1, 0) if nom == "cat.bin" else (2 if nom.startswith("ordre") else 3,
                                                         int(nom.split("_")[1].split(".")[0]))
        if inv["genre"] != genre or inv["ordre"] != ordre or inv["kmax"] != k_max or inv["trame"] != trame or \
                inv["coord_bits"] != PROFIL:
            problemes.append("%s : genre, ordre, K, trame ou profil differents du cas" % nom)
        fichiers[nom] = {"octets": os.path.getsize(chemin), "sha256": sha256(chemin), "inventaire": inv}
    return fichiers, problemes


def nombre(fichiers, nom, section):
    try:
        return fichiers[nom]["inventaire"]["sections"][section]["nombre"]
    except (KeyError, TypeError):
        return None


def empreintes(fichiers):
    return {nom: f["sha256"] for nom, f in fichiers.items()}


def verifier_vidages(rapport, trame, k, dossier):
    """Vidages de reference d'un cas (etape « vider » conforme) et leurs empreintes actuelles ; rend (ref, problemes)."""
    cas = nom_cas(trame, k)
    bloc = rapport.get("vidages", {}).get(cas)
    if not bloc or not bloc.get("conforme"):
        return None, ["%s : vidage de reference absent ou non conforme (etape vider)" % cas]
    ref = empreintes(bloc["fichiers"])
    problemes = []
    for nom, attendu in sorted(ref.items()):
        if sha256_ou_rien(os.path.join(dossier, nom)) != attendu:
            problemes.append("%s/%s : empreinte differente du vidage de reference" % (cas, nom))
    return ref, problemes


def ranger(rapport, section, cle, bloc):
    """Ecrit rapport[section][cle] sans perte : un bloc precedent passe dans rapport['historique'] (jamais ecrase)."""
    table = rapport.setdefault(section, {})
    if cle in table:
        rapport.setdefault("historique", {}).setdefault(section, {}).setdefault(cle, []).append(table[cle])
    table[cle] = bloc


def entier(x):
    return isinstance(x, int) and not isinstance(x, bool)


def fini_positif(x):
    return isinstance(x, (int, float)) and not isinstance(x, bool) and math.isfinite(x) and x > 0


# ---- Provenance : construction, binaires, sources --------------------------------------------------------------------
def binaire(args, nom):
    chemin = os.path.join(os.path.abspath(args.construction), nom)
    return chemin if os.path.isfile(chemin) else None


def lancer(args, rapport, nom, options, journal):
    """Lance un binaire de la construction ; rend (code, lignes, duree, erreur, provenance). La provenance porte
    l'empreinte du binaire avant et apres, son accord avec « construire », la commande et l'empreinte du journal."""
    chemin = binaire(args, nom)
    attendu = rapport.get("construction", {}).get("binaires", {}).get(nom)
    if chemin is None:
        return 127, [], 0.0, "binaire absent", {"binaire": nom, "sha256": None, "inchange": False,
                                                 "construit": False, "commande": options}
    avant = sha256(chemin)
    code, lignes, duree, erreur = jouer([chemin] + options, journal)
    apres = sha256_ou_rien(chemin)
    prov = {"binaire": nom, "sha256": avant, "inchange": avant == apres, "construit": attendu == avant,
            "commande": [chemin] + options, "journal": os.path.relpath(journal, args.sortie) if journal else None,
            "journal_sha256": sha256_ou_rien(journal)}
    return code, lignes, duree, erreur, prov


def problemes_provenance(prov):
    out = []
    if prov.get("sha256") is None:
        out.append("binaire %s absent" % prov.get("binaire"))
    else:
        if not prov.get("construit"):
            out.append("binaire %s sans provenance (empreinte differente de « construire » ou construction absente)"
                       % prov.get("binaire"))
        if not prov.get("inchange"):
            out.append("binaire %s modifie pendant l'execution" % prov.get("binaire"))
    return out


def hacher_sources(exclus=()):
    """Empreintes des sources du microbanc (hors sorties, constructions et caches)."""
    out = {}
    exclus = {os.path.realpath(e) for e in exclus if e}
    for racine, dossiers, fichiers in os.walk(ICI):
        dossiers[:] = sorted(d for d in dossiers if d not in ("out", "build", "__pycache__") and
                             os.path.realpath(os.path.join(racine, d)) not in exclus)
        for nom in sorted(fichiers):
            if nom.endswith((".py", ".cpp", ".hpp", ".txt", ".md")):
                chemin = os.path.join(racine, nom)
                out[os.path.relpath(chemin, ICI)] = sha256(chemin)
    return out


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
    bloc = enregistrer_binaires(args, {"dossier": construction, "etapes": etapes, "code": code,
                                       "erreur": erreur if code else ""})
    ancien = rapport.get("construction")
    if ancien:  # une construction precedente n'est jamais effacee : elle passe dans l'historique
        rapport.setdefault("historique", {}).setdefault("construction", []).append(ancien)
    rapport["construction"] = bloc
    absents = [n for n in BINAIRES if bloc["binaires"].get(n) is None]
    if code != 0 or absents or bloc["libmhgp11_sha256"] is None:
        print("construction en echec ou incomplete (voir %s) : toute execution sera refusee" % construction,
              flush=True)
        return 3
    return 0


def dependances(construction, racines):
    """Dependances locales effectivement compilees : fichiers .d du compilateur sous <construction>/CMakeFiles,
    chemins sous les racines (sources du microbanc, v11), haches ; en-tetes systeme hors du releve."""
    trouves = set()
    racines = [os.path.realpath(r) for r in racines if r]
    for dossier, _, noms in os.walk(os.path.join(construction, "CMakeFiles")):
        for nom in noms:
            if not nom.endswith(".d"):
                continue
            try:
                with open(os.path.join(dossier, nom), encoding="utf-8", errors="replace") as f:
                    texte = f.read().replace("\\\n", " ")
            except OSError:
                continue
            for jeton in texte.split():
                chemin = os.path.realpath(jeton)
                if not jeton.endswith(":") and any(chemin.startswith(r + os.sep) for r in racines) and \
                        os.path.isfile(chemin):
                    trouves.add(chemin)
    out = {}
    for chemin in sorted(trouves):
        etiquette = next(("%d:%s" % (i, os.path.relpath(chemin, r)) for i, r in enumerate(racines)
                          if chemin.startswith(r + os.sep)), chemin)
        out[etiquette] = sha256(chemin)
    return out


def enregistrer_binaires(args, bloc):
    """Empreintes des binaires de la construction et de libmhgp11.a ; references de toutes les executions."""
    bloc = dict(bloc)
    bloc["dossier"] = os.path.abspath(args.construction)
    bloc["binaires"] = {n: sha256_ou_rien(binaire(args, n)) for n in BINAIRES}
    bloc["libmhgp11_sha256"] = sha256_ou_rien(os.path.join(args.v11_build, "libmhgp11.a")) if args.v11_build else None
    bloc["dependances"] = {"racines": ["0: microbanc", "1: morsehgp3D_v11"],
                           "fichiers": dependances(bloc["dossier"], [ICI, args.v11_source])}
    bloc["date"] = maintenant()
    return bloc


# ---- Validation des sorties (fonctions pures : lignes JSON d'un processus -> problemes) -----------------------------
def lignes_de(lignes, phase):
    return [l for l in lignes if isinstance(l, dict) and l.get("phase") == phase]


def ordres_uniques(lignes, attendus):
    """Lignes « ordre » : exactement une par ordre attendu ; rend (par ordre, problemes)."""
    par, problemes = {}, []
    for l in lignes_de(lignes, "ordre"):
        k = l.get("k")
        if k in par:
            problemes.append("ordre %s en double" % k)
        par[k] = l
    manquants = [k for k in attendus if k not in par]
    if manquants:
        problemes.append("ordres absents : %s" % manquants)
    if set(par) - set(attendus):
        problemes.append("ordres inattendus : %s" % sorted(set(par) - set(attendus), key=str))
    return {k: par[k] for k in attendus if k in par}, problemes


def fin_conforme(lignes, code):
    """Fin explicite exigee : une seule ligne « fin » dont le code egale celui du processus, aucune exception."""
    problemes = []
    fins = lignes_de(lignes, "fin")
    if len(fins) != 1 or fins[0].get("code") != code:
        problemes.append("ligne de fin absente, multiple ou discordante (code %s)" % code)
    for phase in ("exception", "refus", "ligne_illisible"):
        if lignes_de(lignes, phase):
            problemes.append("phase %s : %s" % (phase, json.dumps(lignes_de(lignes, phase)[0])[:200]))
    return problemes


def valider_m3(lignes, code, trame, k_max, fichiers):
    """Un processus de MES-M3 : entree, un ordre par k = 2..K (parties = PARTS du vidage, routes completes), fin.
    Rend (problemes de preuve, identite)."""
    problemes = []
    entrees = lignes_de(lignes, "entree")
    if len(entrees) != 1 or entrees[0].get("K") != k_max or entrees[0].get("trame") != trame or \
            entrees[0].get("mutant_sans_s_dans_f") is not False:
        problemes.append("entree absente ou d'un autre cas")
    par, p = ordres_uniques(lignes, list(range(2, k_max + 1)))
    problemes += p
    identite = bool(par)
    for k, l in par.items():
        parties = nombre(fichiers, "ordre_%d.bin" % k, "PARTS")
        routes = l.get("routes")
        if not entier(l.get("parties")) or l.get("parties") != parties:
            problemes.append("ordre %d : parties %s, vidage %s" % (k, l.get("parties"), parties))
        if not isinstance(routes, dict) or sum(v for v in routes.values() if entier(v)) != l.get("parties"):
            problemes.append("ordre %d : routes incompletes" % k)
        ident = l.get("identite")
        if not isinstance(ident, dict) or not isinstance(ident.get("identiques"), bool):
            problemes.append("ordre %d : identite illisible" % k)
        else:
            identite = identite and ident["identiques"]
    if par and sum(l.get("parties") for l in par.values() if entier(l.get("parties"))) == 0:
        problemes.append("aucune partie jugee : identite vide, jamais une preuve")
    if code not in (0, 1) or (code == 0) != identite:
        problemes.append("code %s discordant avec l'identite" % code)
    problemes += fin_conforme(lignes, code)
    return problemes, identite


def signature_m3(lignes):
    return json.dumps([(l.get("k"), l.get("parties"), l.get("routes"), l.get("raisons"), l.get("comptes"),
                        l.get("identite")) for l in lignes_de(lignes, "ordre")], sort_keys=True)


def valider_m4(lignes, code, trame, k_max, fichiers, fils):
    """Un processus de MES-M4 : entree, un ordre par k = 1..K (comptes egaux au vidage), LEM-T6 sur toutes les
    naissances des ordres k >= 2 (au moins une), contraction parallele identique, fin. Rend (problemes, identite)."""
    problemes = []
    entrees = lignes_de(lignes, "entree")
    if len(entrees) != 1 or entrees[0].get("K") != k_max or entrees[0].get("trame") != trame or \
            entrees[0].get("mutant_sans_contraction") is not False:
        problemes.append("entree absente ou d'un autre cas")
    par, p = ordres_uniques(lignes, list(range(1, k_max + 1)))
    problemes += p
    identite = bool(par)
    for k, l in par.items():
        comptes = {"naissances": nombre(fichiers, "ordre_%d.bin" % k, "BIRTHS"),
                   "cellules": nombre(fichiers, "ordre_%d.bin" % k, "CELLS"),
                   "representants": nombre(fichiers, "ordre_%d.bin" % k, "SEEDS"),
                   "noeuds_v11": nombre(fichiers, "foret_%d.bin" % k, "FNODES")}
        for cle, attendu in comptes.items():
            if l.get(cle) != attendu or attendu is None:
                problemes.append("ordre %d : %s %s, vidage %s" % (k, cle, l.get(cle), attendu))
        t6 = l.get("lem_t6")
        if not isinstance(t6, dict) or not entier(t6.get("naissances_jugees")) or not entier(t6.get("ecarts")):
            problemes.append("ordre %d : LEM-T6 illisible" % k)
            continue
        if k >= 2 and (t6["naissances_jugees"] != l.get("naissances") or not t6["naissances_jugees"]):
            problemes.append("ordre %d : %s naissances jugees par LEM-T6 sur %s (CST-0214)" % (
                k, t6["naissances_jugees"], l.get("naissances")))
        par_c = l.get("contraction_parallele")
        if not isinstance(par_c, dict) or par_c.get("fils") != fils or not isinstance(par_c.get("identique"), bool):
            problemes.append("ordre %d : contraction parallele illisible ou d'un autre nombre de fils" % k)
            continue
        ident = l.get("identite")
        if not isinstance(ident, dict) or not isinstance(ident.get("identiques"), bool):
            problemes.append("ordre %d : identite illisible" % k)
            continue
        identite = identite and ident["identiques"] and l.get("racine_unique") is True and \
            l.get("noeuds") == l.get("noeuds_v11") and t6["ecarts"] == 0 and par_c["identique"]
    if code not in (0, 1) or (code == 0) != identite:
        problemes.append("code %s discordant avec l'identite" % code)
    problemes += fin_conforme(lignes, code)
    return problemes, identite


def signature_m4(lignes):
    return json.dumps([(l.get("k"), l.get("naissances"), l.get("noeuds"), l.get("fusions"), l.get("identite"),
                        l.get("lem_t6")) for l in lignes_de(lignes, "ordre")], sort_keys=True)


def valider_variante(nom, code, lignes, k_max, fichiers):
    """Variante « mere puis Welzl » : normal = identite sur tous les ordres et test S dans F decisif (meres
    ecartees) ; mutant = ecarts de sphere (tue par sa reponse geometrique). Rend (problemes, reponse, par ordre)."""
    problemes = []
    entrees = lignes_de(lignes, "entree")
    if len(entrees) != 1 or entrees[0].get("proposition") != "mere_puis_welzl" or \
            entrees[0].get("mutant_sans_s_dans_f") is not (nom != "normal"):
        problemes.append("entree absente ou d'un autre binaire")
    par, p = ordres_uniques(lignes, list(range(2, k_max + 1)))
    problemes += p + fin_conforme(lignes, code)
    for kk, l in par.items():
        if l.get("parties") != nombre(fichiers, "ordre_%d.bin" % kk, "PARTS"):
            problemes.append("ordre %d : parties differentes du vidage" % kk)
    ecartees = sum(l.get("comptes", {}).get("mere_ecartee_par_s_dans_f", 0) for l in par.values()
                   if isinstance(l.get("comptes"), dict))
    ecarts = sum(l.get("identite", {}).get("sphere_ecarts", 0) for l in par.values()
                 if isinstance(l.get("identite"), dict))
    if nom == "normal":
        reponse = code == 0 and bool(par) and ecartees > 0 and \
            all(isinstance(l.get("identite"), dict) and l["identite"].get("identiques") is True for l in par.values())
    else:
        reponse = code == 1 and ecarts > 0
    return problemes, reponse, {"mere_ecartee_par_s_dans_f": ecartees, "ecarts_de_sphere": ecarts}, par


def valider_mutant_m4(code, lignes):
    """Mutant sans contraction sur un vidage : tue seulement par un ecart d'identite (code 1, sortie complete, aucun
    refus ni exception). Rend (tue, reponse geometrique, sortie complete)."""
    entrees = lignes_de(lignes, "entree")
    geometrique = any(isinstance(l.get("identite"), dict) and l["identite"].get("identiques") is False
                      for l in lignes_de(lignes, "ordre"))
    complete = len(entrees) == 1 and entrees[0].get("mutant_sans_contraction") is True and \
        not fin_conforme(lignes, code)
    return code == 1 and geometrique and complete, geometrique, complete


def valider_resolution(lignes, k_max):
    """Lignes « resolution_un_fil » d'un processus : une par ordre k = 2..K, graines identiques sur les trois bras,
    temps finis positifs ; rend (problemes, ordres, totaux)."""
    problemes, ordres = [], {}
    par = {}
    for l in lignes_de(lignes, "resolution_un_fil"):
        if l.get("k") in par:
            problemes.append("resolution : ordre %s en double" % l.get("k"))
        par[l.get("k")] = l
    attendus = list(range(2, k_max + 1))
    if sorted(k for k in par if entier(k)) != attendus or len(par) != len(attendus):
        problemes.append("resolution : ordres %s, attendus 2..%d" % (sorted(par, key=str), k_max))
    totaux = {"v11": 0.0, "replique_v11": 0.0, "replique_v12": 0.0}
    for k in attendus:
        l = par.get(k)
        if l is None:
            continue
        sec = l.get("secondes")
        if l.get("graines_identiques") is not True or not entier(l.get("traces")) or l.get("traces") <= 0 or \
                not isinstance(sec, dict) or not all(fini_positif(sec.get(b)) for b in totaux):
            problemes.append("resolution : ordre %d sans graines identiques, traces ou temps finis positifs" % k)
            continue
        ordres[k] = {"traces": l["traces"], "secondes": {b: sec[b] for b in totaux}}
        for b in totaux:
            totaux[b] += sec[b]
    if not lignes_de(lignes, "resolution_fin"):
        problemes.append("resolution : phase resolution_fin absente")
    return problemes, ordres, totaux


def valider_vidage(lignes, code, trame, k_max, journal_compare, chrono, ful1):
    """Sortie de mhgp12_vidage : toutes ses phases et tous ses ordres, controles internes vrais, sortie ok."""
    problemes = []
    if code != 0:
        problemes.append("code %d" % code)
    dom = lignes_de(lignes, "domaine")
    if len(dom) != 1 or dom[0].get("K") != k_max or dom[0].get("trame") != trame or dom[0].get("coord_bits") != PROFIL:
        problemes.append("phase domaine absente ou d'un autre cas")
    for phase in ("forets_publiees", "catalogue_vide", "fin") + (("ful1",) if ful1 else ()):
        if len(lignes_de(lignes, phase)) != 1:
            problemes.append("phase %s absente ou multiple" % phase)
    journaux = lignes_de(lignes, "journaux_v11")
    if len(journaux) != 1 or journaux[0].get("forets_serie_identiques") is not True:
        problemes.append("journaux v11 absents ou forets serie differentes")
    _, p = ordres_uniques(lignes, list(range(1, k_max + 1)))
    problemes += p
    graines = lignes_de(lignes, "graines")
    if sorted(str(l.get("k")) for l in graines) != sorted(str(k) for k in range(1, k_max + 1)):
        problemes.append("graines : un controle par ordre attendu")
    for l in graines:
        k = l.get("k")
        if journal_compare and (l.get("journal_v11_compare") is not True or l.get("identiques") is not True):
            problemes.append("graines de l'ordre %s differentes du journal v11 ou non comparees" % k)
    sorties = lignes_de(lignes, "exit")
    if len(sorties) != 1 or sorties[0].get("status") != "ok":
        problemes.append("sortie du vidage non ok")
    if chrono:
        problemes += valider_resolution(lignes, k_max)[0]
    for phase in ("exception", "ligne_illisible"):
        if lignes_de(lignes, phase):
            problemes.append("phase " + phase)
    return problemes


def valider_porte(nom, code, lignes):
    """Porte d'un microbanc : resume et temoins attendus ; un mutant est tue par sa reponse (temoin faux, ecarts)."""
    problemes = []
    if nom.startswith("mes_m3"):
        mutant = nom != "mes_m3"
        resume = [l for l in lignes if l.get("porte") == "mes_m3"]
        temoins = {l.get("temoin"): l for l in lignes if "temoin" in l}
        if len(resume) != 1 or resume[0].get("mutant_sans_s_dans_f") is not mutant or \
                not entier(resume[0].get("temoins")) or resume[0]["temoins"] < 10 or \
                resume[0]["temoins"] != len(temoins):
            problemes.append("resume de porte absent, d'un autre binaire ou temoins manquants")
        carre = temoins.get("WIT-T1-CARRE", {})
        if mutant:
            if code != 1 or carre.get("conforme") is not False or not resume or not resume[0].get("ecarts"):
                problemes.append("mutant non tue par WIT-T1-CARRE (code %s)" % code)
        elif code != 0 or carre.get("conforme") is not True or not resume or resume[0].get("ecarts") != 0 or \
                not all(l.get("conforme") is True for l in temoins.values()):
            problemes.append("temoins non conformes (code %s)" % code)
    else:
        mutant = nom != "mes_m4"
        resume = [l for l in lignes if l.get("porte") == "mes_m4"]
        borne = [l for l in lignes if l.get("temoin") == "borne_operandes_cst0212"]
        if len(resume) != 1 or resume[0].get("mutant_sans_contraction") is not mutant or \
                resume[0].get("cas_aleatoires") != 6000 or len(borne) != 1 or borne[0].get("conforme") is not True:
            problemes.append("resume, borne CST-0212 ou cas aleatoires absents")
        elif mutant:
            if code != 1 or not resume[0].get("ecarts"):
                problemes.append("mutant non tue (code %s, ecarts %s)" % (code, resume[0].get("ecarts")))
        elif code != 0 or resume[0].get("ecarts") != 0 or resume[0].get("temoins_ecarts") != 0:
            problemes.append("porte en echec (code %s)" % code)
    return problemes


# ---- Etapes ----------------------------------------------------------------------------------------------------------
def portes(args, rapport):
    os.makedirs(args.sortie, exist_ok=True)
    pire = 0
    for nom, exe, options, code_attendu in PORTES:
        journal = os.path.join(args.sortie, "porte_%s_%s.jsonl" % (nom, args.campagne))
        code, lignes, duree, _, prov = lancer(args, rapport, exe, options, journal)
        problemes = problemes_provenance(prov)
        refus = bool(problemes)
        problemes += valider_porte(nom, code, lignes)
        conforme = not problemes
        ranger(rapport, "portes", nom, {
            "campagne": args.campagne, "code": code, "code_attendu": code_attendu, "conforme": conforme,
            "verdict": ("conforme" if code_attendu == 0 else "mutant_tue") if conforme else
            ("REFUS" if refus else ("ECHEC" if code_attendu == 0 else "MUTANT_SURVIVANT")),
            "problemes": problemes, "secondes": round(duree, 3), "lignes": lignes, "binaire": prov,
            "journal": prov["journal"], "journal_sha256": prov["journal_sha256"]})
        pire = max(pire, 0 if conforme else (3 if refus else 1))
    return pire


def vider(args, rapport):
    pire = 0
    for trame, k in cas_liste(args.cas):
        dossier = dossier_cas(args.sortie, trame, k)
        os.makedirs(dossier, exist_ok=True)
        for nom in os.listdir(dossier):  # aucun vidage perime ne survit a un nouveau vidage
            if nom.endswith(".bin") or nom.endswith(".ful1"):
                os.remove(os.path.join(dossier, nom))
        base = os.path.join(os.path.abspath(args.donnees), FICHIERS.get(trame, trame))
        ful1 = os.path.join(dossier, "%s_k%d.ful1" % (trame, k))
        feuille = 16 if k <= 5 else 24
        chrono = (args.chrono_k5 if k <= 5 else args.chrono_k10) if args.chrono_vidage == "oui" else 0
        options = [base + ".u32le", base + ".ids.u32le", trame, str(k), str(feuille), str(args.fils), dossier,
                   "--ful1", ful1, "--journal", args.journal, "--chrono-resolution", str(chrono)]
        print("[%s] vidage %s K%d ..." % (maintenant(), trame, k), flush=True)
        journal = os.path.join(dossier, "vidage_%s.jsonl" % args.campagne)
        code, lignes, duree, erreur, prov = lancer(args, rapport, "mhgp12_vidage", options, journal)
        empreinte = sha256_ou_rien(ful1)
        attendue = EMPREINTES.get((trame, k))
        identique = (empreinte is not None and empreinte == attendue) if attendue else None  # None : sans reference
        if not args.garder_ful1 and os.path.exists(ful1):
            os.remove(ful1)
        fichiers, problemes_inv = inventorier(dossier, trame, k)
        refus = problemes_provenance(prov) + problemes_inv + \
            valider_vidage(lignes, code, trame, k, args.journal != "aucun", chrono, True)
        if empreinte is None:
            refus.append("MHGP11FUL1 absent")
        ecart = identique is False
        conforme = not refus and not ecart
        ranger(rapport, "vidages", nom_cas(trame, k), {
            "campagne": args.campagne, "commande": prov.get("commande"), "code": code, "secondes": round(duree, 2),
            "ful1_sha256": empreinte, "ful1_reference": attendue, "ful1_identique": identique, "fichiers": fichiers,
            "lignes": lignes, "erreur": erreur if code else "", "binaire": prov, "journal": prov.get("journal"),
            "journal_sha256": prov.get("journal_sha256"), "prise_de_resolution": "informative" if chrono else None,
            "refus": refus, "conforme": conforme})
        pire = max(pire, 3 if refus else (1 if ecart else 0))
        print("    code %d, %.1f s, MHGP11FUL1 %s%s" % (code, duree, {True: "identique", False: "DIFFERENT",
                                                                   None: "sans empreinte de reference"}[identique],
                                                     ", REFUS : " + "; ".join(refus[:3]) if refus else ""),
              flush=True)
    return pire


def resolution(args, rapport):
    """Mesure de resolution a un fil repliquee (CST-0213) : --processus processus neufs par cas, chacun rattache aux
    vidages de reference ; les prises d'une campagne s'ajoutent, jamais ne remplacent."""
    pire = 0
    for trame, k in cas_liste(args.cas):
        cas = nom_cas(trame, k)
        dossier = dossier_cas(args.sortie, trame, k)
        ref, problemes = verifier_vidages(rapport, trame, k, dossier)
        passes = args.chrono_k5 if k <= 5 else args.chrono_k10
        campagne = {"debut": maintenant(), "processus_demandes": args.processus, "passes_par_processus": passes,
                    "statistique_intra_processus": "minimum de %d passe(s) dans le processus, par bras et par ordre"
                                                   % passes,
                    "vidages_reference": ref, "prises": [], "refus": list(problemes)}
        bloc = rapport.setdefault("resolution", {}).setdefault(cas, {"campagnes": {}})
        if args.campagne in bloc["campagnes"]:
            raise SystemExit("campagne %s deja presente pour %s : jamais ecrasee" % (args.campagne, cas))
        bloc["campagnes"][args.campagne] = campagne
        if problemes or passes < 1:
            if passes < 1:
                campagne["refus"].append("aucune passe de resolution (--chrono-k5/--chrono-k10 = 0)")
            pire = 3
            continue
        racine = os.path.join(dossier, "resolution", args.campagne)
        base = os.path.join(os.path.abspath(args.donnees), FICHIERS.get(trame, trame))
        feuille = 16 if k <= 5 else 24
        for n in range(args.processus):
            prise_dir = os.path.join(racine, "p%d" % n)
            os.makedirs(prise_dir)  # dossier neuf : refuse s'il existe deja
            print("[%s] resolution %s K%d (processus %d/%d) ..." % (maintenant(), trame, k, n + 1, args.processus),
                  flush=True)
            options = [base + ".u32le", base + ".ids.u32le", trame, str(k), str(feuille), str(args.fils), prise_dir,
                       "--journal", "aucun", "--chrono-resolution", str(passes)]
            journal = os.path.join(prise_dir, "resolution.jsonl")
            code, lignes, duree, erreur, prov = lancer(args, rapport, "mhgp12_vidage", options, journal)
            raisons = problemes_provenance(prov)
            if code != 0:
                raisons.append("code %d" % code)
            p, ordres, totaux = valider_resolution(lignes, k)
            raisons += p
            sorties = lignes_de(lignes, "exit")
            if len(sorties) != 1 or sorties[0].get("status") != "ok" or len(lignes_de(lignes, "fin")) != 1:
                raisons.append("sortie du vidage non ok ou fin absente")
            # Rattachement aux memes traces : vidages reecrits identiques a l'octet a ceux de « vider », puis effaces.
            reecrits = {nom: sha256(os.path.join(prise_dir, nom)) for nom in sorted(os.listdir(prise_dir))
                        if nom.endswith(".bin")}
            if reecrits != ref:
                raisons.append("vidages reecrits differents des vidages de reference (autres traces ou parties)")
            for nom in reecrits:
                os.remove(os.path.join(prise_dir, nom))
            valide = not raisons and totaux["replique_v11"] > 0
            campagne["prises"].append({
                "processus": n, "code": code, "secondes": round(duree, 2), "valide": valide, "raisons": raisons,
                "binaire": prov, "journal": prov.get("journal"), "journal_sha256": prov.get("journal_sha256"),
                "vidages_identiques_a_la_reference": reecrits == ref, "ordres": ordres, "totaux": totaux,
                "rapport_total": totaux["replique_v12"] / totaux["replique_v11"] if valide else None,
                "erreur": erreur if code else ""})
            if not valide:
                pire = 3
            print("    code %d, %.1f s%s" % (code, duree, "" if valide else ", REFUS : " + "; ".join(raisons[:3])),
                  flush=True)
        campagne["fin"] = maintenant()
        valides = sum(p["valide"] for p in campagne["prises"])
        campagne["prises_valides"] = valides
        if valides != args.processus:
            campagne["refus"].append("%d prises valides sur %d" % (valides, args.processus))
    return pire


def m3(args, rapport):
    pire = 0
    for trame, k in cas_liste(args.cas):
        cas = nom_cas(trame, k)
        dossier = dossier_cas(args.sortie, trame, k)
        ref, refus = verifier_vidages(rapport, trame, k, dossier)
        fichiers = rapport.get("vidages", {}).get(cas, {}).get("fichiers", {})
        repetitions = args.repetitions_k5 if k <= 5 else args.repetitions_k10
        passages, identite = [], True
        for n in range(args.processus if not refus else 0):
            print("[%s] MES-M3 %s K%d (processus %d/%d) ..." % (maintenant(), trame, k, n + 1, args.processus),
                  flush=True)
            journal = os.path.join(dossier, "mes_m3_%s_p%d.jsonl" % (args.campagne, n))
            code, lignes, duree, erreur, prov = lancer(args, rapport, "mhgp12_mes_m3",
                                                       [dossier, "--repetitions", str(repetitions)], journal)
            p, ident = valider_m3(lignes, code, trame, k, fichiers)
            p = problemes_provenance(prov) + p
            refus += ["processus %d : %s" % (n, x) for x in p]
            identite = identite and ident
            passages.append({"code": code, "secondes": round(duree, 2), "lignes": lignes, "erreur": erreur if code else "",
                             "binaire": prov, "journal": prov.get("journal"), "journal_sha256": prov.get("journal_sha256"),
                             "problemes": p})
            print("    code %d, %.1f s" % (code, duree), flush=True)
        if ref is not None and passages:
            _, apres = verifier_vidages(rapport, trame, k, dossier)
            refus += apres
        # Determinisme : routes, comptes et identites identiques d'un processus a l'autre, sur des sorties completes.
        signatures = {signature_m3(p["lignes"]) for p in passages}
        deterministe = len(passages) == args.processus and len(signatures) == 1 and not any(p["problemes"]
                                                                                            for p in passages)
        if passages and len(signatures) != 1:
            refus.append("routes ou comptes differents entre processus")
        complet = not refus and deterministe
        codes = [p["code"] for p in passages]
        ranger(rapport, "mes_m3", cas, {
            "campagne": args.campagne, "processus": passages, "codes": codes, "code": max(codes) if codes else None,
            "routes_identiques_entre_processus": deterministe, "vidages_sha256": ref, "refus": refus,
            "complet": complet, "identique": complet and identite, "conforme": complet and identite})
        pire = max(pire, 3 if not complet else (0 if identite else 1))
    return pire


def m3_variante(args, rapport):
    """Variante « mere puis Welzl » sur le premier cas : la proposition S*(mere) n'est jamais dans F ; avec le test
    S dans F l'identite tient et le test est decisif, le mutant sans ce test doit etre tue par des ecarts de sphere."""
    trame, k = cas_liste(args.cas)[0]
    cas = nom_cas(trame, k)
    dossier = dossier_cas(args.sortie, trame, k)
    ref, refus = verifier_vidages(rapport, trame, k, dossier)
    fichiers = rapport.get("vidages", {}).get(cas, {}).get("fichiers", {})
    variante, pire = {"vidages_sha256": ref}, 3 if refus else 0
    for nom, exe, attendu in (("normal", "mhgp12_mes_m3", 0),
                              ("mutant_sans_s_dans_f", "mhgp12_mes_m3_mutant_sans_s_dans_f", 1)):
        if refus:
            break
        journal = os.path.join(dossier, "mes_m3_mere_%s_%s.jsonl" % (nom, args.campagne))
        code, lignes, duree, _, prov = lancer(args, rapport, exe, [dossier, "--repetitions", "1", "--proposition",
                                                                   "mere_puis_welzl"], journal)
        problemes, reponse, comptes, par = valider_variante(nom, code, lignes, k, fichiers)
        problemes = problemes_provenance(prov) + problemes
        conforme = not problemes and reponse
        variante[nom] = {"code": code, "code_attendu": attendu, "conforme": conforme, "secondes": round(duree, 2),
                         "problemes": problemes, "binaire": prov, "journal": prov.get("journal"),
                         "journal_sha256": prov.get("journal_sha256"),
                         "ordres": {kk: {"identite": l.get("identite"), "routes": l.get("routes"),
                                         "mere_ecartee_par_s_dans_f": l.get("comptes", {}).get(
                                             "mere_ecartee_par_s_dans_f")} for kk, l in par.items()}}
        variante[nom].update(comptes)
        pire = max(pire, 3 if problemes else (0 if conforme else 1))
    variante["refus"] = refus
    ranger(rapport, "mes_m3", "variante_mere_puis_welzl_" + cas, variante)
    return pire


def m4(args, rapport):
    pire = 0
    premiers = cas_liste(args.cas)
    for trame, k in premiers:
        cas = nom_cas(trame, k)
        dossier = dossier_cas(args.sortie, trame, k)
        ref, refus = verifier_vidages(rapport, trame, k, dossier)
        fichiers = rapport.get("vidages", {}).get(cas, {}).get("fichiers", {})
        passages, identite = [], True
        for n in range(args.processus if not refus else 0):
            print("[%s] MES-M4 %s K%d (processus %d/%d) ..." % (maintenant(), trame, k, n + 1, args.processus),
                  flush=True)
            journal = os.path.join(dossier, "mes_m4_%s_p%d.jsonl" % (args.campagne, n))
            code, lignes, duree, erreur, prov = lancer(args, rapport, "mhgp12_mes_m4",
                                                       [dossier, "--repetitions", str(args.repetitions_m4),
                                                        "--fils-contraction", str(args.fils_contraction)], journal)
            p, ident = valider_m4(lignes, code, trame, k, fichiers, args.fils_contraction)
            p = problemes_provenance(prov) + p
            refus += ["processus %d : %s" % (n, x) for x in p]
            identite = identite and ident
            passages.append({"code": code, "secondes": round(duree, 2), "lignes": lignes, "erreur": erreur if code else "",
                             "binaire": prov, "journal": prov.get("journal"), "journal_sha256": prov.get("journal_sha256"),
                             "problemes": p})
            print("    code %d, %.1f s" % (code, duree), flush=True)
        if ref is not None and passages:
            refus += verifier_vidages(rapport, trame, k, dossier)[1]
        signatures = {signature_m4(p["lignes"]) for p in passages}
        if passages and len(signatures) != 1:
            refus.append("forets ou LEM-T6 differents entre processus")
        complet = not refus and len(passages) == args.processus
        codes = [p["code"] for p in passages]
        ranger(rapport, "mes_m4", cas, {
            "campagne": args.campagne, "processus": passages, "codes": codes, "code": max(codes) if codes else None,
            "vidages_sha256": ref, "refus": refus, "complet": complet, "identique": complet and identite,
            "conforme": complet and identite})
        pire = max(pire, 3 if not complet else (0 if identite else 1))
    # Mutant sans contraction sur le premier vidage : il doit sortir en ecart d'identite (code 1), sans refus.
    trame, k = premiers[0]
    cas = nom_cas(trame, k)
    dossier = dossier_cas(args.sortie, trame, k)
    ref, refus = verifier_vidages(rapport, trame, k, dossier)
    bloc = {"vidages_sha256": ref, "refus": refus, "tue": False}
    if not refus:
        journal = os.path.join(dossier, "mes_m4_mutant_%s.jsonl" % args.campagne)
        code, lignes, duree, _, prov = lancer(args, rapport, "mhgp12_mes_m4_mutant_sans_contraction",
                                              [dossier, "--repetitions", "1"], journal)
        refus += problemes_provenance(prov)
        tue, geometrique, complete = valider_mutant_m4(code, lignes)
        bloc.update({"code": code, "reponse_geometrique": geometrique, "sortie_complete": complete, "binaire": prov,
                     "journal": prov.get("journal"), "journal_sha256": prov.get("journal_sha256"),
                     "tue": tue and not refus})
    ranger(rapport, "mes_m4", "mutant_sans_contraction_" + cas, bloc)
    pire = max(pire, 3 if refus else (0 if bloc["tue"] else 1))
    return pire


# ---- Juges -----------------------------------------------------------------------------------------------------------
def bootstrap_gm(logs, rng, tirages):
    """Moyenne geometrique et IC 95 % par bootstrap (memes conventions que le juge de MES-M2)."""
    gm = math.exp(sum(logs) / len(logs))
    boot = []
    for _ in range(tirages):
        echantillon = [logs[rng.randrange(len(logs))] for _ in logs]
        boot.append(sum(echantillon) / len(echantillon))
    boot.sort()
    return gm, math.exp(boot[int(0.025 * tirages)]), math.exp(boot[int(0.975 * tirages) - 1])


def derniere_campagne(rapport, cas):
    campagnes = rapport.get("resolution", {}).get(cas, {}).get("campagnes", {})
    if not campagnes:
        return None, None
    ident = max(campagnes, key=lambda c: (campagnes[c].get("debut", ""), c))
    return ident, campagnes[ident]


def juger_m3(rapport):
    """Verdict d'adoption de MES-M3 (REGLE_M3) ; cite les fichiers et empreintes de chaque preuve."""
    binaires = rapport.get("construction", {}).get("binaires", {})
    refus, rejets, cas_out, preuves = [], [], {}, {}
    portes_ = rapport.get("portes", {})
    for nom, exe in (("mes_m3", "mhgp12_mes_m3"), ("mes_m3_mutant_sans_s_dans_f", "mhgp12_mes_m3_mutant_sans_s_dans_f")):
        b = portes_.get(nom)
        if not b or not b.get("conforme") or b.get("binaire", {}).get("sha256") != binaires.get(exe) or \
                not binaires.get(exe):
            refus.append("porte %s absente, non conforme ou d'un autre binaire" % nom)
        else:
            preuves[b["journal"]] = b["journal_sha256"]
    rng = random.Random(REGLE_M3["graine"])
    for cas in REGLE_M3["cas_decisifs"]:
        vid = rapport.get("vidages", {}).get(cas)
        if not vid or not vid.get("conforme"):
            refus.append("%s : vidage de reference absent ou non conforme" % cas)
            continue
        ref = empreintes(vid["fichiers"])
        preuves.update({"%s/%s" % (cas, n): h for n, h in ref.items()})
        bloc = rapport.get("mes_m3", {}).get(cas)
        if not bloc or not bloc.get("complet") or bloc.get("vidages_sha256") != ref or \
                any(p["binaire"].get("sha256") != binaires.get("mhgp12_mes_m3") for p in bloc.get("processus", [])):
            refus.append("%s : identite MES-M3 absente, incomplete ou non rattachee" % cas)
        else:
            preuves.update({p["journal"]: p["journal_sha256"] for p in bloc["processus"]})
            if not bloc.get("identique"):
                rejets.append("%s : identite MES-M3 en defaut" % cas)
        ident, camp = derniere_campagne(rapport, cas)
        if camp is None:
            refus.append("%s : aucune prise de resolution repliquee (etape resolution)" % cas)
            continue
        prises = [p for p in camp.get("prises", []) if p.get("valide")]
        exigees = max(REGLE_M3["processus_min"], camp.get("processus_demandes", 0))
        if camp.get("refus") or len(prises) != len(camp.get("prises", [])) or len(prises) < exigees or \
                camp.get("vidages_reference") != ref:
            refus.append("%s : campagne %s : %d prises valides sur %d exigees%s" % (
                cas, ident, len(prises), exigees, "" if camp.get("vidages_reference") == ref else
                ", vidages de reference differents"))
            continue
        if any(p["binaire"].get("sha256") != binaires.get("mhgp12_vidage") or not binaires.get("mhgp12_vidage")
               for p in prises):
            refus.append("%s : prise d'un autre binaire de vidage" % cas)
            continue
        logs = [math.log(p["rapport_total"]) for p in prises]
        gm, bas, haut = bootstrap_gm(logs, rng, REGLE_M3["bootstrap"])
        cas_out[cas] = {"campagne": ident, "prises": len(prises), "rapports": [p["rapport_total"] for p in prises],
                        "moyenne_geometrique": gm, "ic95": [bas, haut], "reduction": 1 - gm,
                        "passes_par_processus": camp.get("passes_par_processus"),
                        "statistique_intra_processus": camp.get("statistique_intra_processus")}
        preuves.update({p["journal"]: p["journal_sha256"] for p in prises})
        if haut > REGLE_M3["seuil"]:
            rejets.append("%s : borne haute %.3f > %.2f" % (cas, haut, REGLE_M3["seuil"]))
    verdict = "refuse" if refus else ("rejete" if rejets else "adopte")
    # Cas qui ne decident pas (K5, autres trames) : meme statistique, publiee, sans effet sur le verdict (generateur
    # distinct : les nombres des cas qui decident ne dependent pas des cas publies).
    informatifs, rng_info = {}, random.Random(REGLE_M3["graine"])
    for cas in sorted(rapport.get("resolution", {})):
        if cas in REGLE_M3["cas_decisifs"]:
            continue
        ident, camp = derniere_campagne(rapport, cas)
        prises = [p for p in camp.get("prises", []) if p.get("valide")] if camp else []
        if not prises or camp.get("refus") or len(prises) != len(camp.get("prises", [])):
            informatifs[cas] = {"campagne": ident, "prises_valides": len(prises), "complete": False}
            continue
        gm, bas, haut = bootstrap_gm([math.log(p["rapport_total"]) for p in prises], rng_info, REGLE_M3["bootstrap"])
        informatifs[cas] = {"campagne": ident, "prises_valides": len(prises), "complete": True,
                            "moyenne_geometrique": gm, "ic95": [bas, haut],
                            "rapports": [p["rapport_total"] for p in prises]}
    return {"verdict": verdict, "regle": REGLE_M3, "refus": refus, "rejets": rejets, "cas": cas_out,
            "cas_publies_sans_decision": informatifs,
            "binaires": {n: binaires.get(n) for n in ("mhgp12_vidage", "mhgp12_mes_m3",
                                                      "mhgp12_mes_m3_mutant_sans_s_dans_f")},
            "preuves": dict(sorted(preuves.items()))}


def juger_m4(rapport):
    """Conformite de MES-M4 par cas (identite, LEM-T6 complet, mutant tue, porte), avec ses preuves ; temps publies
    et compares aux seuils (aucune adoption automatique : la statistique de MES-M4 n'est pas ecrite)."""
    binaires = rapport.get("construction", {}).get("binaires", {})
    out, preuves, refus = {}, {}, []
    for nom, exe in (("mes_m4", "mhgp12_mes_m4"), ("mes_m4_mutant_sans_contraction",
                                                   "mhgp12_mes_m4_mutant_sans_contraction")):
        b = rapport.get("portes", {}).get(nom)
        if not b or not b.get("conforme") or b.get("binaire", {}).get("sha256") != binaires.get(exe):
            refus.append("porte %s absente, non conforme ou d'un autre binaire" % nom)
        else:
            preuves[b["journal"]] = b["journal_sha256"]
    mutants = {}
    for c, b in rapport.get("mes_m4", {}).items():
        cas_mutant = c[len("mutant_sans_contraction_"):]
        if c.startswith("mutant") and b.get("tue") and \
                b.get("binaire", {}).get("sha256") == binaires.get("mhgp12_mes_m4_mutant_sans_contraction") and \
                b.get("vidages_sha256") == empreintes(rapport.get("vidages", {}).get(cas_mutant, {}).get("fichiers", {})):
            mutants[c] = b
    if not mutants:
        refus.append("mutant sans contraction non joue, non tue ou non rattache a ses binaire et vidages")
    else:
        preuves.update({b["journal"]: b["journal_sha256"] for b in mutants.values()})
    for cas, bloc in sorted(rapport.get("mes_m4", {}).items()):
        if cas.startswith("mutant"):
            continue
        vid = rapport.get("vidages", {}).get(cas, {})
        rattache = bloc.get("vidages_sha256") is not None and bloc.get("vidages_sha256") == \
            empreintes(vid.get("fichiers", {})) and all(p["binaire"].get("sha256") == binaires.get("mhgp12_mes_m4")
                                                        for p in bloc.get("processus", []))
        if not bloc.get("complet") or not rattache:
            etat = "refuse"
        else:
            etat = "conforme" if bloc.get("identique") else "non_conforme"
        par_ordre = lignes_ordres(processus_de(bloc))
        temps = {}
        try:
            k = max(par_ordre)
            ls = par_ordre[k]
            temps = {"ordre": k, "processus": len(ls),
                     "noyau_ms_mediane": 1e3 * mediane([x["temps_un_fil_s"]["noyau"] for x in ls]),
                     "contraction_ms_mediane": 1e3 * mediane([x["temps_un_fil_s"]["contraction"] for x in ls]),
                     "contraction_parallele_ms_mediane": 1e3 * mediane(
                         [x["contraction_parallele"]["secondes"] for x in ls]),
                     "fils_contraction": ls[0]["contraction_parallele"]["fils"],
                     "seuils": {"noyau_ms": SEUILS_M4["noyau_ms_k5"] if k <= 5 else SEUILS_M4["noyau_ms_k10"],
                                "contraction_ms": SEUILS_M4["contraction_ms"]},
                     "statistique": "mediane entre processus du minimum de R passes (publiee, non jugee)"}
        except (KeyError, TypeError, ValueError):
            temps = {}
        out[cas] = {"etat": etat, "refus": bloc.get("refus", []), "temps": temps,
                    "preuves": {p["journal"]: p["journal_sha256"] for p in bloc.get("processus", [])},
                    "vidages_sha256": bloc.get("vidages_sha256")}
    etats = [v["etat"] for v in out.values()]
    verdict = "refuse" if refus or not etats or "refuse" in etats else (
        "non_conforme" if "non_conforme" in etats else "conforme")
    return {"verdict": verdict, "refus": refus, "cas": out, "preuves": dict(sorted(preuves.items())),
            "note": "conformite (forets identiques, LEM-T6 sur toutes les naissances, mutant tue, porte) ; les temps "
                    "sont publies et compares aux seuils de PLAN.md sans verdict d'adoption automatique"}


def processus_de(bloc):
    """Liste des passages d'un bloc m3/m4 (ancien format : un seul passage a plat)."""
    if isinstance(bloc, dict) and "processus" in bloc:
        return bloc["processus"]
    return [bloc] if isinstance(bloc, dict) and "lignes" in bloc else []


def mediane(valeurs):
    v = sorted(valeurs)
    if not v:
        return 0.0
    m = len(v) // 2
    return v[m] if len(v) % 2 else 0.5 * (v[m - 1] + v[m])


def lignes_ordres(passages):
    """Par ordre : les lignes de tous les passages."""
    par_ordre = {}
    for p in passages:
        for l in p.get("lignes", []):
            if l.get("phase") == "ordre":
                par_ordre.setdefault(l["k"], []).append(l)
    return par_ordre


def synthese(rapport):
    """Tableaux compacts : routes MES-M3 par ordre, temps medians entre processus, identites ; MES-M4 par ordre ;
    resolution par campagne (prises par processus)."""
    s = {"mes_m3": {}, "mes_m4": {}, "resolution": {}}
    for cas, bloc in rapport.get("mes_m3", {}).items():
        if cas.startswith("variante"):
            s.setdefault("variantes_mes_m3", {})[cas] = {k: v for k, v in bloc.items() if k != "vidages_sha256"}
            continue
        ordres = {}
        for k, ls in lignes_ordres(processus_de(bloc)).items():
            l = ls[0]
            try:
                ordres[k] = {"parties": l["parties"], "identiques": all(x["identite"]["identiques"] for x in ls),
                             "routes": l["routes"], "part_t1": l["parts"]["t1"],
                             "part_sphere_au_catalogue": l["parts"]["sphere_au_catalogue"],
                             "part_eligible_test_complet": l["parts"]["t1_eligible_test_complet"],
                             "processus": len(ls),
                             "ns_reference": round(mediane([x["temps_un_fil_s"]["ns_par_partie_reference"]
                                                            for x in ls]), 1),
                             "ns_voie_nouvelle": round(mediane([x["temps_un_fil_s"]["ns_par_partie_voie_nouvelle"]
                                                                for x in ls]), 1),
                             "rapport_temps": round(mediane([x["temps_un_fil_s"]["rapport"] for x in ls]), 3)}
            except (KeyError, TypeError):
                ordres[k] = {"illisible": True}
        s["mes_m3"][cas] = {"conforme": bloc.get("conforme"), "ordres": ordres}
    for cas, bloc in rapport.get("mes_m4", {}).items():
        if cas.startswith("mutant"):
            s["mes_m4"][cas] = {"tue": bloc.get("tue")}
            continue
        ordres = {}
        for k, ls in lignes_ordres(processus_de(bloc)).items():
            l = ls[0]
            try:
                ordres[k] = {"identiques": all(x["identite"]["identiques"] for x in ls), "noeuds": l["noeuds"],
                             "fusions": l["fusions"], "lem_t6_ecarts": max(x["lem_t6"]["ecarts"] for x in ls),
                             "lem_t6_naissances_jugees": min(x["lem_t6"]["naissances_jugees"] for x in ls),
                             "naissances": l["naissances"], "processus": len(ls),
                             "ms_naissances": round(1e3 * mediane([x["temps_un_fil_s"]["naissances"] for x in ls]), 3),
                             "ms_noyau": round(1e3 * mediane([x["temps_un_fil_s"]["noyau"] for x in ls]), 3),
                             "ms_contraction": round(1e3 * mediane([x["temps_un_fil_s"]["contraction"]
                                                                    for x in ls]), 3)}
            except (KeyError, TypeError):
                ordres[k] = {"illisible": True}
        s["mes_m4"][cas] = {"conforme": bloc.get("conforme"), "ordres": ordres}
    for cas, bloc in rapport.get("resolution", {}).items():
        campagnes = {}
        for ident, camp in bloc.get("campagnes", {}).items():
            prises = camp.get("prises", [])
            valides = [p for p in prises if p.get("valide")]
            campagnes[ident] = {"debut": camp.get("debut"), "prises": len(prises), "valides": len(valides),
                                "passes_par_processus": camp.get("passes_par_processus"),
                                "rapports_par_processus": [p["rapport_total"] for p in valides],
                                "rapport_median": round(mediane([p["rapport_total"] for p in valides]), 4)
                                if valides else None, "refus": camp.get("refus", [])}
        s["resolution"][cas] = campagnes
    for cas, bloc in rapport.get("vidages", {}).items():  # prise informative du vidage (non decisive)
        p, ordres, tot = valider_resolution(bloc.get("lignes", []), int(cas.rsplit("_k", 1)[1]))
        if ordres:
            s.setdefault("resolution_vidage_informative", {})[cas] = {
                "rapport_total_v12_sur_replique_v11": round(tot["replique_v12"] / tot["replique_v11"], 3)
                if tot["replique_v11"] else None, "complete": not p}
    return s


def tableaux(rapport):
    """Tableaux Markdown tires du rapport (aucune valeur recopiee a la main)."""
    out = []

    def ligne(cols):
        out.append("| " + " | ".join(str(c) for c in cols) + " |")

    verdicts = rapport.get("verdicts", {})
    out.append("### Verdicts\n")
    ligne(["mesure", "verdict", "refus", "rejets"])
    ligne(["---"] * 4)
    for nom in ("mes_m3", "mes_m4"):
        v = verdicts.get(nom, {})
        ligne([nom, v.get("verdict", "non juge"), "; ".join(v.get("refus", [])[:4]) or "-",
               "; ".join(v.get("rejets", [])[:4]) or "-"])
    out.append("\n### Vidages\n")
    ligne(["cas", "code", "MHGP11FUL1 identique", "conforme", "secondes", "octets des .bin"])
    ligne(["---"] * 6)
    for cas, v in sorted(rapport.get("vidages", {}).items()):
        octets = sum(f["octets"] for f in v.get("fichiers", {}).values())
        ligne([cas, v["code"], {True: "oui", False: "NON", None: "sans reference"}[v["ful1_identique"]],
               "oui" if v.get("conforme") else "NON", v["secondes"], octets])
    out.append("\n### Profil des descentes de la v11 (vidage)\n")
    ligne(["cas", "k", "traces", "pas", "parties (MEB)", "table sur trace", "table apres pas", "catalogue",
           "census sature", "census complet", "sphere au catalogue", "S* dans F", "plus longue chaine"])
    ligne(["---"] * 13)
    for cas, v in sorted(rapport.get("vidages", {}).items()):
        for l in v.get("lignes", []):
            if l.get("phase") == "ordre" and l.get("k", 0) >= 2:
                ligne([cas, l["k"], l.get("traces"), l.get("steps"), l.get("parts"), l.get("table_on_trace"),
                       l.get("table_after_steps"), l.get("route_catalogue"), l.get("route_census_saturated"),
                       l.get("route_census_complete"), l.get("sphere_in_catalogue"), l.get("sstar_in_f"),
                       l.get("max_parts_per_trace")])
    out.append("\n### Resolution a un fil repliquee (trois bras ; prises par processus ; MES-M3)\n")
    ligne(["cas", "campagne", "processus", "passes par processus", "v12 / replique v11 par processus",
           "moyenne geometrique", "IC 95 %", "refus"])
    ligne(["---"] * 8)
    jug = dict(verdicts.get("mes_m3", {}).get("cas_publies_sans_decision", {}))
    jug.update(verdicts.get("mes_m3", {}).get("cas", {}))
    for cas, bloc in sorted(rapport.get("resolution", {}).items()):
        for ident, camp in sorted(bloc.get("campagnes", {}).items()):
            valides = [p for p in camp.get("prises", []) if p.get("valide")]
            j = jug.get(cas, {}) if jug.get(cas, {}).get("campagne") == ident and \
                "moyenne_geometrique" in jug.get(cas, {}) else {}
            ligne([cas, ident, "%d / %d" % (len(valides), camp.get("processus_demandes", 0)),
                   camp.get("passes_par_processus"), ", ".join("%.3f" % p["rapport_total"] for p in valides) or "-",
                   "%.3f" % j["moyenne_geometrique"] if j else "-",
                   "[%.3f, %.3f]" % tuple(j["ic95"]) if j else "-", "; ".join(camp.get("refus", [])[:2]) or "-"])
    out.append("\n### MES-M3 : routes, parts et temps a un fil (ns par partie)\n")
    ligne(["cas", "k", "parties", "identite", "t1", "cert_table", "cert_census", "repli", "part t1",
           "sphere au cat.", "eligible (test complet)", "ns ref", "ns v12", "rapport"])
    ligne(["---"] * 14)
    for cas, v in sorted(rapport.get("mes_m3", {}).items()):
        if cas.startswith("variante"):
            continue
        for k, ls in sorted(lignes_ordres(processus_de(v)).items()):
            l = ls[0]
            try:
                r = l["routes"]
                t = {cle: mediane([x["temps_un_fil_s"][cle] for x in ls]) for cle in
                     ("rapport", "ns_par_partie_reference", "ns_par_partie_voie_nouvelle")}
                ligne([cas, l["k"], l["parties"], "oui" if all(x["identite"]["identiques"] for x in ls) else "NON",
                       r["t1"], r["cert_table"], r["cert_census"], r["repli_table"] + r["repli_census"],
                       "%.3f" % l["parts"]["t1"], "%.3f" % l["parts"]["sphere_au_catalogue"],
                       "%.3f" % l["parts"]["t1_eligible_test_complet"], "%.0f" % t["ns_par_partie_reference"],
                       "%.0f" % t["ns_par_partie_voie_nouvelle"], "%.3f" % t["rapport"]])
            except (KeyError, TypeError):
                ligne([cas, k, "illisible"] + [""] * 11)
    out.append("\n### MES-M4 : identite, LEM-T6 et temps a un fil (ms, medianes entre processus)\n")
    ligne(["cas", "k", "naissances", "naissances jugees LEM-T6", "fusions", "identite", "LEM-T6 ecarts",
           "noyau (ms)", "contraction (ms)", "contraction parallele (ms, fils)"])
    ligne(["---"] * 10)
    for cas, v in sorted(rapport.get("mes_m4", {}).items()):
        if cas.startswith("mutant"):
            continue
        for k, ls in sorted(lignes_ordres(processus_de(v)).items()):
            l = ls[0]
            try:
                par = [x["contraction_parallele"] for x in ls]
                fils = par[0].get("fils", 1)
                tpar = ("%.2f (%d)" % (1e3 * mediane([q["secondes"] for q in par]), fils)
                        if fils > 1 and all(q.get("identique") for q in par) else ("ECART" if fils > 1 else "-"))
                ligne([cas, l["k"], l["naissances"], min(x["lem_t6"]["naissances_jugees"] for x in ls), l["fusions"],
                       "oui" if all(x["identite"]["identiques"] for x in ls) else "NON",
                       max(x["lem_t6"]["ecarts"] for x in ls),
                       "%.2f" % (1e3 * mediane([x["temps_un_fil_s"]["noyau"] for x in ls])),
                       "%.2f" % (1e3 * mediane([x["temps_un_fil_s"]["contraction"] for x in ls])), tpar])
            except (KeyError, TypeError):
                ligne([cas, k, "illisible"] + [""] * 7)
    out.append("\n### Portes et mutants\n")
    ligne(["porte", "code", "attendu", "verdict"])
    ligne(["---"] * 4)
    for nom, v in sorted(rapport.get("portes", {}).items()):
        ligne([nom, v["code"], v["code_attendu"], v["verdict"]])
    for nom, v in sorted(rapport.get("mes_m4", {}).items()):
        if nom.startswith("mutant"):
            ligne([nom, v.get("code"), 1, "mutant_tue" if v.get("tue") else "MUTANT_SURVIVANT_OU_SANS_PREUVE"])
    for nom, v in sorted(rapport.get("mes_m3", {}).items()):
        if nom.startswith("variante"):
            for cle in ("normal", "mutant_sans_s_dans_f"):
                if cle in v:
                    ligne([nom + "/" + cle, v[cle]["code"], v[cle]["code_attendu"],
                           "conforme" if v[cle]["conforme"] else "NON CONFORME"])
    return "\n".join(out) + "\n"


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("etapes", nargs="+",
                   choices=["construire", "portes", "vider", "resolution", "m3", "m3var", "m4", "rapport", "tout"])
    p.add_argument("--v11-source", default="/workspaces/E-HGP/morsehgp3D_v11")
    p.add_argument("--v11-build", required=False, default=os.environ.get("MHGP11_BUILD", ""))
    p.add_argument("--donnees", default="/workspaces/E-HGP/build/v11-full-data-20261002")
    p.add_argument("--sortie", default=os.path.join(ICI, "out"))
    p.add_argument("--construction", default=os.path.join(ICI, "build"))
    p.add_argument("--cas", default="ng00:5,ng01:5,ng02:5,ng00:10")
    p.add_argument("--fils", type=int, default=3, help="fils du vidage (les microbancs sont a un fil)")
    p.add_argument("--fils-construction", type=int, default=3)
    p.add_argument("--journal", default="tous", help="ordres dont le journal de graines v11 est compare")
    p.add_argument("--chrono-k5", type=int, default=3, help="passes de la mesure de resolution a K<=5 (par processus)")
    p.add_argument("--chrono-k10", type=int, default=1, help="passes de la mesure de resolution a K>5 (par processus)")
    p.add_argument("--chrono-vidage", choices=["oui", "non"], default="non",
                   help="prise de resolution informative dans l'etape vider (les prises decisives viennent de "
                        "l'etape resolution)")
    p.add_argument("--repetitions-k5", type=int, default=3)
    p.add_argument("--repetitions-k10", type=int, default=1)
    p.add_argument("--repetitions-m4", type=int, default=5)
    p.add_argument("--garder-ful1", action="store_true")
    p.add_argument("--fils-contraction", type=int, default=1,
                   help="MES-M4 : fils de la contraction parallele (1 : sequentielle seule ; G4 : 48)")
    p.add_argument("--processus", type=int, default=1,
                   help="processus par microbanc et par prise de resolution (MESURE.md § 5 : l'unite de replication "
                        "est le processus ; MES-M3 exige au moins 5 prises par cas)")
    args = p.parse_args()
    if args.processus < 1 or args.fils < 1 or args.fils_contraction < 1:
        p.error("--processus, --fils et --fils-contraction : au moins 1")
    etapes = args.etapes
    if "tout" in etapes:
        etapes = ["construire", "portes", "vider", "resolution", "m3", "m3var", "m4", "rapport"]
    args.campagne = "c%s_%s" % (datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%dT%H%M%SZ"),
                                os.urandom(4).hex())
    args.sortie = os.path.abspath(args.sortie)
    os.makedirs(args.sortie, exist_ok=True)
    chemin_rapport = os.path.join(args.sortie, "rapport_mes_m3_m4.json")
    rapport = {}
    if os.path.exists(chemin_rapport):
        with open(chemin_rapport, encoding="utf-8") as f:
            rapport = json.load(f)
    rapport["cadre"] = {"phase": "exploration_v12_hors_registre", "backend": "cpu_reference",
                        "quantification": "quantized_u21_input_only", "public_status": "not_claimed",
                        "mesures": ["MES-M3", "MES-M4"], "gcp": "GCP non utilise par ce pilote"}
    sources = hacher_sources((args.sortie, args.construction))
    execution = {"campagne": args.campagne, "debut": maintenant(), "etapes": etapes, "machine": machine(),
                 "arguments": {k: v for k, v in vars(args).items()}, "sources_sha256": sources}
    rapport.setdefault("executions", []).append(execution)
    if "construire" not in etapes and args.v11_build and rapport.get("construction", {}).get("binaires") is None:
        # Rapport sans construction enregistree : les empreintes actuelles ne prouvent rien (aucune provenance).
        print("aucune construction enregistree dans ce rapport : les executions seront refusees (etape construire)",
              flush=True)
    code = 0
    for etape in etapes:
        if etape == "construire":
            if not args.v11_build:
                raise SystemExit("--v11-build requis")
            code = max(code, construire(args, rapport))
        elif etape == "portes":
            code = max(code, portes(args, rapport))
        elif etape == "vider":
            code = max(code, vider(args, rapport))
        elif etape == "resolution":
            code = max(code, resolution(args, rapport))
        elif etape == "m3":
            code = max(code, m3(args, rapport))
        elif etape == "m3var":
            code = max(code, m3_variante(args, rapport))
        elif etape == "m4":
            code = max(code, m4(args, rapport))
        elif etape == "rapport":
            rapport["verdicts"] = {"mes_m3": juger_m3(rapport), "mes_m4": juger_m4(rapport), "date": maintenant(),
                                   "campagne": args.campagne}
            rapport["synthese"] = synthese(rapport)
            with open(os.path.join(args.sortie, "tableaux.md"), "w", encoding="utf-8") as f:
                f.write(tableaux(rapport))
        with open(chemin_rapport, "w", encoding="utf-8") as f:
            json.dump(rapport, f, indent=1, sort_keys=True)
    fin_sources = hacher_sources((args.sortie, args.construction))
    if fin_sources != sources:
        code = 3
        execution["sources_modifiees"] = sorted(k for k in set(sources) | set(fin_sources)
                                                if sources.get(k) != fin_sources.get(k))
    releve = rapport.get("construction", {}).get("dependances", {}).get("fichiers", {})
    if releve:
        actuel = dependances(rapport["construction"]["dossier"], [ICI, args.v11_source])
        modifiees = sorted(k for k, v in releve.items() if actuel.get(k) != v)
        if modifiees:
            code = 3
            execution["dependances_modifiees"] = modifiees
    if rapport.get("construction", {}).get("libmhgp11_sha256") and args.v11_build:
        if sha256_ou_rien(os.path.join(args.v11_build, "libmhgp11.a")) != rapport["construction"]["libmhgp11_sha256"]:
            code = 3
            execution["libmhgp11_modifiee"] = True
    execution["fin"] = maintenant()
    execution["code"] = code
    with open(chemin_rapport, "w", encoding="utf-8") as f:
        json.dump(rapport, f, indent=1, sort_keys=True)
    print("rapport : %s (code %d)" % (chemin_rapport, code))
    return code


if __name__ == "__main__":
    sys.exit(main())
