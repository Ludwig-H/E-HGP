#!/usr/bin/env python3
"""Pilote des microbancs de la tour v12 (MES-M3, MES-M4) et du vidage de la v11. Bibliotheque standard seule.

Etapes (dans l'ordre si plusieurs) :
  construire   cmake + make (-j fils) des binaires mhgp12_* liees a la construction v11 donnee
  portes       portes des microbancs : temoins graves, mutants causaux (doivent etre tues)
  vider        mhgp12_vidage sur chaque cas (trame:K) ; empreinte MHGP11FUL1 comparee a la reference (MESURE.md § 4)
  m3           MES-M3 sur chaque vidage (identite exigee, routes, temps a un fil)
  m3var        MES-M3, variante « mere puis Welzl » sur le premier cas : binaire normal (identite) et mutant sans le
               test S dans F (doit etre tue sur donnees reelles)
  m4           MES-M4 sur chaque vidage (identite exigee, LEM-T6, temps a un fil) ; mutant joue sur un vidage
  rapport      synthese JSON (rapport_mes_m3_m4.json) des etapes jouees, et tableaux Markdown (tableaux.md)
  tout         toutes les etapes

Exemple (codespace ; sur G4, adapter les chemins, et --fils 48 pour le vidage seulement) :
  python3 pilote.py --v11-source /workspaces/E-HGP/morsehgp3D_v11 --v11-build <build-v11> \\
      --donnees /workspaces/E-HGP/build/v11-full-data-20261002 --sortie out --cas ng00:5,ng01:5,ng02:5,ng00:10 tout

Les vidages derivent de donnees SemanticKITTI (CC BY-NC-SA) : ils restent dans --sortie, jamais dans un depot.
Codes : 0 conforme ; 1 ecart d'identite, porte en echec ou mutant survivant ; 2 erreur d'usage ou d'execution.
"""
import argparse
import datetime
import hashlib
import json
import os
import platform
import subprocess
import sys
import time

ICI = os.path.dirname(os.path.abspath(__file__))

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


def maintenant():
    return datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def sha256(chemin):
    h = hashlib.sha256()
    with open(chemin, "rb") as f:
        for bloc in iter(lambda: f.read(1 << 20), b""):
            h.update(bloc)
    return h.hexdigest()


def jouer(commande, journal=None, cwd=None):
    """Execute une commande ; rend (code, lignes JSON, duree). La sortie brute est gardee dans `journal`."""
    debut = time.monotonic()
    proc = subprocess.run(commande, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=False)
    duree = time.monotonic() - debut
    if journal:
        with open(journal, "w", encoding="utf-8") as f:
            f.write(proc.stdout)
            if proc.stderr:
                f.write("\n# stderr\n" + proc.stderr)
    lignes = []
    for ligne in proc.stdout.splitlines():
        ligne = ligne.strip()
        if ligne.startswith("{"):
            try:
                lignes.append(json.loads(ligne))
            except json.JSONDecodeError:
                lignes.append({"phase": "ligne_illisible", "texte": ligne[:200]})
    return proc.returncode, lignes, duree, proc.stderr[-2000:]


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
        trame, k = item.split(":")
        if not trame or "/" in trame or not 1 <= int(k) <= 12:
            raise SystemExit("cas invalide : " + item)
        cas.append((trame, int(k)))
    return cas


def dossier_cas(sortie, trame, k):
    return os.path.join(sortie, "%s_k%d" % (trame, k))


# Sections attendues par genre de fichier (format version 1, README.md § 4) : etiquette -> taille d'un element
# (None : variable, 4k pour PARTS).
SECTIONS = {
    1: {"SITEXYZ": 12, "BALLS": 32, "POPOFF": 8, "POPVAL": 4, "NLEVELS": 8},
    2: {"BIRTHS": 16, "BCENTER": 64, "CELLS": 24, "CELLOFF": 8, "TRACEA": 8, "SEEDS": 12, "PARTOFF": 8,
        "PARTS": None, "PARTINF": 8},
    3: {"FNODES": 24, "FEDGES": 4, "FLOWER": 4, "FMETA": 8},
}


def lire_vidage(chemin):
    """Lecteur strict d'un fichier MHGP12DP version 1 : en-tete et inventaire des sections (sans les donnees).
    Refuse (ValueError) une magie ou une version inconnue, une section tronquee, inattendue ou de taille fausse,
    des octets en trop."""
    import struct
    taille = os.path.getsize(chemin)
    with open(chemin, "rb") as f:
        tete = f.read(64)
        if len(tete) != 64 or tete[:8] != b"MHGP12DP":
            raise ValueError("magie inconnue : " + chemin)
        version, genre, bits, kmax, ordre, nsec = struct.unpack_from("<6I", tete, 8)
        sites, = struct.unpack_from("<Q", tete, 32)
        trame = tete[40:64].rstrip(b"\0").decode("ascii")
        if version != 1 or genre not in SECTIONS:
            raise ValueError("version ou genre non pris en charge : " + chemin)
        pos = 64
        inventaire = {}
        for _ in range(nsec):
            f.seek(pos)
            sh = f.read(24)
            if len(sh) != 24:
                raise ValueError("section tronquee : " + chemin)
            tag = sh[:8].rstrip(b"\0").decode("ascii")
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
    return {"version": version, "genre": genre, "coord_bits": bits, "kmax": kmax, "ordre": ordre, "sites": sites,
            "trame": trame, "sections": inventaire}


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
    rapport["construction"] = {"dossier": construction, "etapes": etapes, "code": code,
                               "libmhgp11_sha256": sha256(os.path.join(args.v11_build, "libmhgp11.a")),
                               "erreur": erreur if code else ""}
    if code != 0:
        raise SystemExit("construction en echec (voir %s)" % construction)
    return code


def binaire(args, nom):
    chemin = os.path.join(os.path.abspath(args.construction), nom)
    if not os.path.exists(chemin):
        raise SystemExit("binaire absent : %s (etape construire)" % chemin)
    return chemin


def portes(args, rapport):
    os.makedirs(args.sortie, exist_ok=True)
    resultat = {}
    attendus = [("mes_m3", "mhgp12_mes_m3", ["--porte"], 0), ("mes_m3_mutant_sans_s_dans_f",
                                                                "mhgp12_mes_m3_mutant_sans_s_dans_f", ["--porte"], 1),
                ("mes_m4", "mhgp12_mes_m4", ["--porte", "6000"], 0),
                ("mes_m4_mutant_sans_contraction", "mhgp12_mes_m4_mutant_sans_contraction", ["--porte", "6000"], 1)]
    echec = False
    for nom, exe, options, code_attendu in attendus:
        code, lignes, duree, _ = jouer([binaire(args, exe)] + options, os.path.join(args.sortie, "porte_%s.jsonl" % nom))
        conforme = code == code_attendu
        echec |= not conforme
        resultat[nom] = {"code": code, "code_attendu": code_attendu,
                         "verdict": ("conforme" if code_attendu == 0 else "mutant_tue") if conforme else
                         ("ECHEC" if code_attendu == 0 else "MUTANT_SURVIVANT"),
                         "secondes": round(duree, 3), "lignes": lignes}
    rapport["portes"] = resultat
    return 1 if echec else 0


def vider(args, rapport):
    resultat = rapport.setdefault("vidages", {})
    echec = False
    for trame, k in cas_liste(args.cas):
        dossier = dossier_cas(args.sortie, trame, k)
        os.makedirs(dossier, exist_ok=True)
        base = os.path.join(os.path.abspath(args.donnees), FICHIERS.get(trame, trame))
        ful1 = os.path.join(dossier, "%s_k%d.ful1" % (trame, k))
        feuille = 16 if k <= 5 else 24
        chrono = args.chrono_k5 if k <= 5 else args.chrono_k10
        commande = [binaire(args, "mhgp12_vidage"), base + ".u32le", base + ".ids.u32le", trame, str(k), str(feuille),
                    str(args.fils), dossier, "--ful1", ful1, "--journal", args.journal,
                    "--chrono-resolution", str(chrono)]
        print("[%s] vidage %s K%d ..." % (maintenant(), trame, k), flush=True)
        code, lignes, duree, erreur = jouer(commande, os.path.join(dossier, "vidage.jsonl"))
        empreinte = sha256(ful1) if os.path.exists(ful1) else None
        attendue = EMPREINTES.get((trame, k))
        identique = (empreinte is not None and empreinte == attendue) if attendue else None  # None : sans reference
        if not args.garder_ful1 and os.path.exists(ful1):
            os.remove(ful1)
        fichiers = {}
        for nom in sorted(os.listdir(dossier)):
            if nom.endswith(".bin"):
                chemin = os.path.join(dossier, nom)
                fichiers[nom] = {"octets": os.path.getsize(chemin), "sha256": sha256(chemin),
                                 "inventaire": lire_vidage(chemin)}
        ok = code == 0 and identique is not False
        echec |= not ok
        resultat["%s_k%d" % (trame, k)] = {
            "commande": commande, "code": code, "secondes": round(duree, 2), "ful1_sha256": empreinte,
            "ful1_reference": attendue, "ful1_identique": identique, "fichiers": fichiers, "lignes": lignes,
            "erreur": erreur if code else ""}
        print("    code %d, %.1f s, MHGP11FUL1 %s" % (code, duree, {True: "identique", False: "DIFFERENT",
                                                                  None: "sans empreinte de reference"}[identique]),
              flush=True)
    return 1 if echec else 0


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


def m3(args, rapport):
    resultat = rapport.setdefault("mes_m3", {})
    echec = False
    for trame, k in cas_liste(args.cas):
        dossier = dossier_cas(args.sortie, trame, k)
        repetitions = args.repetitions_k5 if k <= 5 else args.repetitions_k10
        passages = []
        for n in range(args.processus):
            print("[%s] MES-M3 %s K%d (processus %d/%d) ..." % (maintenant(), trame, k, n + 1, args.processus),
                  flush=True)
            code, lignes, duree, erreur = jouer([binaire(args, "mhgp12_mes_m3"), dossier, "--repetitions",
                                                 str(repetitions)], os.path.join(dossier, "mes_m3_%d.jsonl" % n))
            echec |= code != 0
            passages.append({"code": code, "secondes": round(duree, 2), "lignes": lignes,
                             "erreur": erreur if code else ""})
            print("    code %d, %.1f s" % (code, duree), flush=True)
        # Determinisme : routes et identites identiques d'un processus a l'autre.
        signatures = {json.dumps([(l["k"], l["routes"], l["identite"]) for l in p["lignes"]
                                  if l.get("phase") == "ordre"], sort_keys=True) for p in passages}
        deterministe = len(signatures) == 1
        echec |= not deterministe
        resultat["%s_k%d" % (trame, k)] = {"processus": passages, "code": max(p["code"] for p in passages),
                                           "routes_identiques_entre_processus": deterministe}
    return 1 if echec else 0


def m3_variante(args, rapport):
    """Variante « mere puis Welzl » sur le premier cas : la proposition S*(mere) n'est jamais dans F ; avec le test
    S dans F l'identite tient (code 0), le mutant sans ce test doit etre tue sur donnees reelles (code 1)."""
    resultat = rapport.setdefault("mes_m3", {})
    echec = False
    trame, k = cas_liste(args.cas)[0]
    dossier = dossier_cas(args.sortie, trame, k)
    variante = {}
    for nom, exe, attendu in (("normal", "mhgp12_mes_m3", 0),
                              ("mutant_sans_s_dans_f", "mhgp12_mes_m3_mutant_sans_s_dans_f", 1)):
        code, lignes, duree, _ = jouer([binaire(args, exe), dossier, "--repetitions", "1", "--proposition",
                                        "mere_puis_welzl"], os.path.join(dossier, "mes_m3_mere_%s.jsonl" % nom))
        ordres = {l["k"]: {"identite": l["identite"], "routes": l["routes"],
                           "mere_ecartee_par_s_dans_f": l["comptes"]["mere_ecartee_par_s_dans_f"]}
                  for l in lignes if l.get("phase") == "ordre"}
        conforme = code == attendu
        echec |= not conforme
        variante[nom] = {"code": code, "code_attendu": attendu, "conforme": conforme, "secondes": round(duree, 2),
                         "ordres": ordres}
    resultat["variante_mere_puis_welzl_%s_k%d" % (trame, k)] = variante
    return 1 if echec else 0


def m4(args, rapport):
    resultat = rapport.setdefault("mes_m4", {})
    echec = False
    premiers = cas_liste(args.cas)
    for trame, k in premiers:
        dossier = dossier_cas(args.sortie, trame, k)
        passages = []
        for n in range(args.processus):
            print("[%s] MES-M4 %s K%d (processus %d/%d) ..." % (maintenant(), trame, k, n + 1, args.processus),
                  flush=True)
            code, lignes, duree, erreur = jouer([binaire(args, "mhgp12_mes_m4"), dossier, "--repetitions",
                                                 str(args.repetitions_m4), "--fils-contraction",
                                                 str(args.fils_contraction)],
                                                os.path.join(dossier, "mes_m4_%d.jsonl" % n))
            echec |= code != 0
            passages.append({"code": code, "secondes": round(duree, 2), "lignes": lignes,
                             "erreur": erreur if code else ""})
            print("    code %d, %.1f s" % (code, duree), flush=True)
        resultat["%s_k%d" % (trame, k)] = {"processus": passages, "code": max(p["code"] for p in passages)}
    # Mutant sans contraction sur le premier vidage : il doit sortir en ecart (code 1).
    trame, k = premiers[0]
    dossier = dossier_cas(args.sortie, trame, k)
    code, lignes, duree, _ = jouer([binaire(args, "mhgp12_mes_m4_mutant_sans_contraction"), dossier, "--repetitions",
                                    "1"], os.path.join(dossier, "mes_m4_mutant.jsonl"))
    tue = code == 1
    echec |= not tue
    resultat["mutant_sans_contraction_%s_k%d" % (trame, k)] = {"code": code, "tue": tue}
    return 1 if echec else 0


def lignes_ordres(passages):
    """Par ordre : la ligne du premier passage, et les temps de tous les passages."""
    par_ordre = {}
    for p in passages:
        for l in p.get("lignes", []):
            if l.get("phase") == "ordre":
                par_ordre.setdefault(l["k"], []).append(l)
    return par_ordre


def synthese(rapport):
    """Tableaux compacts : routes MES-M3 par ordre, temps medians entre processus, identites ; MES-M4 par ordre."""
    s = {"mes_m3": {}, "mes_m4": {}, "resolution": {}}
    for cas, bloc in rapport.get("mes_m3", {}).items():
        if cas.startswith("variante"):
            s.setdefault("variantes_mes_m3", {})[cas] = bloc
            continue
        ordres = {}
        for k, ls in lignes_ordres(processus_de(bloc)).items():
            l = ls[0]
            ordres[k] = {"parties": l["parties"], "identiques": all(x["identite"]["identiques"] for x in ls),
                         "routes": l["routes"], "part_t1": l["parts"]["t1"],
                         "part_sphere_au_catalogue": l["parts"]["sphere_au_catalogue"],
                         "part_eligible_test_complet": l["parts"]["t1_eligible_test_complet"],
                         "processus": len(ls),
                         "ns_reference": round(mediane([x["temps_un_fil_s"]["ns_par_partie_reference"] for x in ls]), 1),
                         "ns_voie_nouvelle": round(mediane([x["temps_un_fil_s"]["ns_par_partie_voie_nouvelle"]
                                                            for x in ls]), 1),
                         "rapport_temps": round(mediane([x["temps_un_fil_s"]["rapport"] for x in ls]), 3)}
        s["mes_m3"][cas] = ordres
    for cas, bloc in rapport.get("mes_m4", {}).items():
        ordres = {}
        for k, ls in lignes_ordres(processus_de(bloc)).items():
            l = ls[0]
            ordres[k] = {"identiques": all(x["identite"]["identiques"] for x in ls), "noeuds": l["noeuds"],
                         "fusions": l["fusions"], "lem_t6_ecarts": max(x["lem_t6"]["ecarts"] for x in ls),
                         "processus": len(ls),
                         "ms_naissances": round(1e3 * mediane([x["temps_un_fil_s"]["naissances"] for x in ls]), 3),
                         "ms_noyau": round(1e3 * mediane([x["temps_un_fil_s"]["noyau"] for x in ls]), 3),
                         "ms_contraction": round(1e3 * mediane([x["temps_un_fil_s"]["contraction"] for x in ls]), 3)}
        if ordres:
            s["mes_m4"][cas] = ordres
    for cas, bloc in rapport.get("vidages", {}).items():
        ordres = {}
        for l in bloc.get("lignes", []):
            if l.get("phase") == "resolution_un_fil":
                ordres[l["k"]] = {"traces": l["traces"], "secondes": l["secondes"],
                                  "rapport_v12_sur_replique_v11": round(l["rapport_v12_sur_replique_v11"], 3)}
        if ordres:
            tot = {b: sum(o["secondes"][b] for o in ordres.values()) for b in ("v11", "replique_v11", "replique_v12")}
            s["resolution"][cas] = {"ordres": ordres, "total_secondes": tot,
                                    "rapport_total_v12_sur_replique_v11": round(tot["replique_v12"] /
                                                                               tot["replique_v11"], 3)
                                    if tot["replique_v11"] else None}
    return s


def tableaux(rapport):
    """Tableaux Markdown tires du rapport (pour RAPPORT.md ; aucune valeur recopiee a la main)."""
    out = []
    def ligne(cols):
        out.append("| " + " | ".join(str(c) for c in cols) + " |")
    out.append("### Vidages\n")
    ligne(["cas", "code", "MHGP11FUL1 identique", "secondes", "octets des .bin"])
    ligne(["---"] * 5)
    for cas, v in sorted(rapport.get("vidages", {}).items()):
        octets = sum(f["octets"] for f in v.get("fichiers", {}).values())
        ligne([cas, v["code"], {True: "oui", False: "NON", None: "sans reference"}[v["ful1_identique"]], v["secondes"],
               octets])
        for l in v.get("lignes", []):
            if l.get("phase") == "graines" and not l.get("identiques", True):
                ligne([cas, "graines k=%d" % l["k"], "ECART", "", ""])
    out.append("\n### Profil des descentes de la v11 (vidage)\n")
    ligne(["cas", "k", "traces", "pas", "parties (MEB)", "table sur trace", "table apres pas", "catalogue",
           "census sature", "census complet", "sphere au catalogue", "S* dans F", "plus longue chaine"])
    ligne(["---"] * 13)
    for cas, v in sorted(rapport.get("vidages", {}).items()):
        for l in v.get("lignes", []):
            if l.get("phase") == "ordre" and l["k"] >= 2:
                ligne([cas, l["k"], l["traces"], l["steps"], l["parts"], l["table_on_trace"], l["table_after_steps"],
                       l["route_catalogue"], l["route_census_saturated"], l["route_census_complete"],
                       l["sphere_in_catalogue"], l["sstar_in_f"], l["max_parts_per_trace"]])
    out.append("\n### MES-M3 : routes, parts et temps a un fil (ns par partie)\n")
    ligne(["cas", "k", "parties", "identite", "t1", "cert_table", "cert_census", "repli", "part t1",
           "sphere au cat.", "eligible (test complet)", "ns ref", "ns v12", "rapport"])
    ligne(["---"] * 14)
    tot = {}
    for cas, v in sorted(rapport.get("mes_m3", {}).items()):
        if cas.startswith("variante"):
            continue
        for k, ls in sorted(lignes_ordres(processus_de(v)).items()):
            l = ls[0]
            r = l["routes"]
            t = {cle: mediane([x["temps_un_fil_s"][cle] for x in ls]) for cle in
                 ("reference", "voie_nouvelle", "rapport", "ns_par_partie_reference", "ns_par_partie_voie_nouvelle")}
            repli = r["repli_table"] + r["repli_census"]
            ligne([cas, l["k"], l["parties"], "oui" if all(x["identite"]["identiques"] for x in ls) else "NON", r["t1"],
                   r["cert_table"],
                   r["cert_census"], repli, "%.3f" % l["parts"]["t1"], "%.3f" % l["parts"]["sphere_au_catalogue"],
                   "%.3f" % l["parts"]["t1_eligible_test_complet"], "%.0f" % t["ns_par_partie_reference"],
                   "%.0f" % t["ns_par_partie_voie_nouvelle"], "%.3f" % t["rapport"]])
            a = tot.setdefault(cas, [0, 0, 0, 0, 0.0, 0.0])
            a[0] += l["parties"]
            a[1] += r["t1"]
            a[2] += l["comptes"]["sphere_au_catalogue"]
            a[3] += l["comptes"]["t1_eligible_test_complet"]
            a[4] += t["reference"]
            a[5] += t["voie_nouvelle"]
    out.append("\n### MES-M3 : totaux par cas (tous ordres)\n")
    ligne(["cas", "parties", "part t1", "part sphere au catalogue", "part eligible (test complet)",
           "temps ref (s)", "temps v12 (s)", "rapport"])
    ligne(["---"] * 8)
    for cas, a in sorted(tot.items()):
        ligne([cas, a[0], "%.4f" % (a[1] / a[0]), "%.4f" % (a[2] / a[0]), "%.4f" % (a[3] / a[0]), "%.3f" % a[4],
               "%.3f" % a[5], "%.3f" % (a[5] / a[4] if a[4] else 0)])
    out.append("\n### Resolution a un fil (vidage, trois bras ; graines identiques exigees)\n")
    ligne(["cas", "k", "traces", "v11 (s)", "replique v11 (s)", "replique v12 (s)", "v12 / replique v11"])
    ligne(["---"] * 7)
    for cas, v in sorted(rapport.get("vidages", {}).items()):
        somme = [0.0, 0.0, 0.0]
        for l in v.get("lignes", []):
            if l.get("phase") == "resolution_un_fil":
                sec = l["secondes"]
                somme = [somme[0] + sec["v11"], somme[1] + sec["replique_v11"], somme[2] + sec["replique_v12"]]
                ligne([cas, l["k"], l["traces"], "%.3f" % sec["v11"], "%.3f" % sec["replique_v11"],
                       "%.3f" % sec["replique_v12"], "%.3f" % l["rapport_v12_sur_replique_v11"]])
        if somme[1] > 0:
            ligne([cas, "tous", "", "%.3f" % somme[0], "%.3f" % somme[1], "%.3f" % somme[2],
                   "%.3f" % (somme[2] / somme[1])])
    out.append("\n### MES-M4 : identite et temps a un fil (ms)\n")
    ligne(["cas", "k", "naissances", "representants", "evenements", "fusions", "identite", "LEM-T6 ecarts",
           "naissances (ms)", "noyau (ms)", "contraction (ms)", "contraction parallele (ms, fils)"])
    ligne(["---"] * 12)
    for cas, v in sorted(rapport.get("mes_m4", {}).items()):
        if cas.startswith("mutant"):
            continue
        for k, ls in sorted(lignes_ordres(processus_de(v)).items()):
            l = ls[0]
            t = {cle: mediane([x["temps_un_fil_s"][cle] for x in ls]) for cle in ("naissances", "noyau", "contraction")}
            par = [x.get("contraction_parallele", {}) for x in ls]
            fils = par[0].get("fils", 1) if par else 1
            tpar = ("%.2f (%d)" % (1e3 * mediane([q["secondes"] for q in par]), fils)
                    if fils > 1 and all(q.get("identique") for q in par) else ("ECART" if fils > 1 else "-"))
            ligne([cas, l["k"], l["naissances"], l["representants"], l["evenements"], l["fusions"],
                   "oui" if all(x["identite"]["identiques"] for x in ls) else "NON",
                   max(x["lem_t6"]["ecarts"] for x in ls), "%.2f" % (1e3 * t["naissances"]),
                   "%.2f" % (1e3 * t["noyau"]), "%.2f" % (1e3 * t["contraction"]), tpar])
    out.append("\n### MES-M3, variante « mere puis Welzl » (S*(mere) jamais dans F)\n")
    ligne(["cas", "binaire", "code", "attendu", "k", "mere ecartee par S dans F", "ecarts de sphere", "identite"])
    ligne(["---"] * 8)
    for cas, v in sorted(rapport.get("mes_m3", {}).items()):
        if not cas.startswith("variante"):
            continue
        for nom, b in sorted(v.items()):
            for k, o in sorted(b["ordres"].items(), key=lambda x: int(x[0])):
                ligne([cas, nom, b["code"], b["code_attendu"], k, o["mere_ecartee_par_s_dans_f"],
                       o["identite"]["sphere_ecarts"], "oui" if o["identite"]["identiques"] else "NON"])
    out.append("\n### Portes et mutants\n")
    ligne(["porte", "code", "attendu", "verdict"])
    ligne(["---"] * 4)
    for nom, v in sorted(rapport.get("portes", {}).items()):
        ligne([nom, v["code"], v["code_attendu"], v["verdict"]])
    for nom, v in sorted(rapport.get("mes_m4", {}).items()):
        if nom.startswith("mutant"):
            ligne([nom, v["code"], 1, "mutant_tue" if v["tue"] else "MUTANT_SURVIVANT"])
    return "\n".join(out) + "\n"


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("etapes", nargs="+",
                   choices=["construire", "portes", "vider", "m3", "m3var", "m4", "rapport", "tout"])
    p.add_argument("--v11-source", default="/workspaces/E-HGP/morsehgp3D_v11")
    p.add_argument("--v11-build", required=False, default=os.environ.get("MHGP11_BUILD", ""))
    p.add_argument("--donnees", default="/workspaces/E-HGP/build/v11-full-data-20261002")
    p.add_argument("--sortie", default=os.path.join(ICI, "out"))
    p.add_argument("--construction", default=os.path.join(ICI, "build"))
    p.add_argument("--cas", default="ng00:5,ng01:5,ng02:5,ng00:10")
    p.add_argument("--fils", type=int, default=3, help="fils du vidage (les microbancs sont a un fil)")
    p.add_argument("--fils-construction", type=int, default=3)
    p.add_argument("--journal", default="tous", help="ordres dont le journal de graines v11 est compare")
    p.add_argument("--chrono-k5", type=int, default=3, help="repetitions de la mesure de resolution a K<=5")
    p.add_argument("--chrono-k10", type=int, default=1, help="repetitions de la mesure de resolution a K>5")
    p.add_argument("--repetitions-k5", type=int, default=3)
    p.add_argument("--repetitions-k10", type=int, default=1)
    p.add_argument("--repetitions-m4", type=int, default=5)
    p.add_argument("--garder-ful1", action="store_true")
    p.add_argument("--fils-contraction", type=int, default=1,
                   help="MES-M4 : fils de la contraction parallele (1 : sequentielle seule ; G4 : 48)")
    p.add_argument("--processus", type=int, default=1,
                   help="processus par microbanc (MESURE.md § 5 : l'unite de replication est le processus)")
    args = p.parse_args()
    etapes = args.etapes
    if "tout" in etapes:
        etapes = ["construire", "portes", "vider", "m3", "m3var", "m4", "rapport"]
    os.makedirs(args.sortie, exist_ok=True)
    chemin_rapport = os.path.join(args.sortie, "rapport_mes_m3_m4.json")
    rapport = {}
    if os.path.exists(chemin_rapport):
        with open(chemin_rapport, encoding="utf-8") as f:
            rapport = json.load(f)
    rapport["cadre"] = {"phase": "exploration_v12_hors_registre", "backend": "cpu_reference",
                        "quantification": "quantized_u21_input_only", "public_status": "not_claimed",
                        "mesures": ["MES-M3", "MES-M4"], "gcp": "GCP non utilise par ce pilote"}
    rapport.setdefault("executions", []).append({"debut": maintenant(), "etapes": etapes, "machine": machine(),
                                                 "arguments": vars(args)})
    code = 0
    for etape in etapes:
        if etape == "construire":
            if not args.v11_build:
                raise SystemExit("--v11-build requis")
            code |= construire(args, rapport)
        elif etape == "portes":
            code |= portes(args, rapport)
        elif etape == "vider":
            code |= vider(args, rapport)
        elif etape == "m3":
            code |= m3(args, rapport)
        elif etape == "m3var":
            code |= m3_variante(args, rapport)
        elif etape == "m4":
            code |= m4(args, rapport)
        elif etape == "rapport":
            rapport["synthese"] = synthese(rapport)
            with open(os.path.join(args.sortie, "tableaux.md"), "w", encoding="utf-8") as f:
                f.write(tableaux(rapport))
        with open(chemin_rapport, "w", encoding="utf-8") as f:
            json.dump(rapport, f, indent=1, sort_keys=True)
    rapport["executions"][-1]["fin"] = maintenant()
    rapport["executions"][-1]["code"] = code
    with open(chemin_rapport, "w", encoding="utf-8") as f:
        json.dump(rapport, f, indent=1, sort_keys=True)
    print("rapport : %s (code %d)" % (chemin_rapport, code))
    return code


if __name__ == "__main__":
    sys.exit(main())
