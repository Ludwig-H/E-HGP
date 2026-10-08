#!/usr/bin/env python3
"""Pilote et juge du levier R1 (raccourci du registre R pour les classes a cellule unique, critere q = d + 1 de
l'auditeur Codex, recu registre_classe_unique), avant / avant_bis / apres en processus alternes, mur FULL de la voie
par defaut de bench/full_probe.cpp (Session recouverte), voie appareil. Bibliotheque standard seule, jouable sous
python3 -S -O (aucune garde par assert). Chaque journal est lu par le lecteur strict partage
microbancs/outils/lecteur_full.py (schema 'recouvert').

Bras (sondes mhgp12_full_probe, Release, profil 21, MHGP12_ENABLE_CUDA=ON) :
  avant      archive epinglee des sources de la tete de main juste avant R1 (--base, --avant-archive,
             --avant-sha256 ; sha256 verifie avant deballage)
  avant_bis  le MEME binaire que avant, joue comme un bras distinct (A/A : bruit de la session ; veto)
  apres      les sources du paquet ({src}) : cette base + levier R1
Les deux sondes ouvrent leur Session avec le cache de blocs de 8 Gio par defaut de la sonde (le pilote ne passe pas
--cache). L'ordre des bras tourne d'un cran par tour, sans inversion.

Etapes (dans l'ordre si plusieurs) :
  auto-test   le juge sur des campagnes synthetiques ; tout autre resultat que l'attendu : code 1
  construire  deballage, configuration et construction des deux sondes ; empreintes, extraits du CMakeCache
  identite    processus --digest, bras avant et apres : ng00-02 a K5 (2 passes) et K10 (1 passe) ; ng00 a K5 a un fil ;
              uniformes de 8 000, 16 000 et 32 000 sites a K5 ({donnees}/uniform_u18_n*.u32le) ; les 37 trames v12set
              en Session (une passe par trame). FUL1 n'ecrit pas les tableaux du registre : leur identite mot pour mot
              sur ces memes entrees est etablie hors chrono avant la session (rapport du chantier R1)
  grandes     DECISIF : les trames v12set de plus de 60 000 sites, en Session, K5 : --tours-grandes tours ; dans chaque
              tour, un processus neuf par bras, qui joue les grandes trames deux fois (ordre tourne d'un tour a
              l'autre) ; le second tour du processus fait foi
  ng          GARDE : ng00, ng01, ng02 a K5 : --tours tours ; dans chaque tour, trames en ordre tourne et, pour chacune,
              un processus neuf par bras ; --passes passes (la premiere, a froid, ecartee)
  k10         INFORMATION : ng00, ng01, ng02 a K10, --tours-k10 tours de --passes-k10 passes, memes alternances ;
              publie sans jugement (une prise manquante n'y est pas un refus)
  rapport     exige --archive-v12set (manifeste de la cohorte), puis auto-test, cohorte fermee contre la commande
              avant toute statistique (fermeture de l'auditeur, recu a6_pilote_admission), juge (REGLE_R1), rapport
              JSON et tableaux Markdown
  tout        auto-test construire identite grandes ng k10 rapport

REGLE_R1 (ecrite le 8 octobre 2026 a 12:31 UTC, avant toute mesure de ces bras sur G4) :
  identite  toutes les empreintes FUL1 des processus d'identite identiques d'un bras et d'une passe a l'autre (par cle ;
            v12set trame par trame) ; sinon REJETE ;
  grandes   par tour, par grande trame : rapport des murs (second tour du processus) apres / avant ; par tour : moyenne
            geometrique sur les trames ; IC 95 % par bootstrap sur les tours (10 000 tirages, graine 20261008) ;
  ng        par tour et par trame : rapport des medianes des passes 2..P du mur, apres / avant ; meme IC ;
  A/A       memes statistiques pour avant_bis / avant ;
  ADOPTE    si l'identite tient, si la borne haute de l'IC des grandes trames est sous 0,99 ET si la borne haute de
            l'IC de chacune de ng00, ng01, ng02 est sous 1,02 ; REJETE sinon ;
  REFUSE    si une prise manque (processus en echec, journal illisible pour le lecteur strict, moins de tours valides
            que demandes, hors etape k10), si un binaire change, si un journal manque ou a change, si le GPU n'est pas
            vide avant ou apres, si l'auto-test echoue, ou si la moyenne geometrique A/A sort de [0,985 ; 1,015] sur les
            grandes trames ou sur l'une de ng00, ng01, ng02 (veto : appariement invalide).
Publies sans jugement : queue de la Session, fins de l'ordre K (G, noyau, M, V, R) et R(K) - max(M(K), V(K)), CPU par
passe, pic du budget, par bras ; K10 de ng00-02 (meme statistique, sans verdict).

Amendements du 8 octobre 2026, avant toute mesure (seuils, statistiques et veto inchanges) : 12:32 UTC, base fd84039c0
(30a69104a + correctif de deux mutants du manifeste, produit identique) au lieu de 30a69104a ; 13:11 UTC, base
6497ed3b5 (pont de publication CST-0242 et juge A6 ferme ; sortie identique), pour que le bras avant soit la base
pontee sur laquelle R1 s'applique ; 13:23 UTC : la base devient un parametre de la commande (--base, archive et
empreinte du bras avant) : la tete de main juste avant R1 (6497ed3b5, ou 6497ed3b5 + lot T2-d-B2 si B2 est adopte et
integre avant), fixee par le coordinateur avant toute mesure ; le juge exige la meme base a la construction et au
jugement ; 13:33 UTC : A6 rejete sur G4 et retire de main, R1 rebase sur bdfca8fb1 : la base est la tete de main
juste avant R1 apres ce retrait (sources src/ et tests/ de bdfca8fb1, plus le lot T2-d-B2 s'il est integre avant).
13:57 UTC : base fixee par le coordinateur : 4171b2653 (A6 retire en ab5614c2a, lot T2-d-B2 integre, non encore
adopte ; archive git archive --format=tar 4171b2653 morsehgp3D_v12 | gzip -n -9). Si B2 est retire de main, la
base sera redonnee avant toute mesure.
14:09 UTC : base fixee par le coordinateur : 5f8e777cf (4171b2653 + T1-d, catalogue en flux, module du catalogue
seulement ; archive git archive --format=tar 5f8e777cf morsehgp3D_v12 | gzip -n -9, sha256 d07df84f).

Mode --essai (local, voie CPU, sans CUDA, moins de prises, deux grandes trames) : verdict force a « essai ».

Codes : 0 campagne complete et jugee (le verdict est dans le rapport) ; 1 auto-test en echec ; 2 usage ; 3 refus.
"""
import argparse
import datetime
import json
import math
import os
import random
import shutil
import sys
import tarfile

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "outils"))
import banc_full as bf  # noqa: E402  lancement borne, environnement, empreintes, extrait du CMakeCache
import lecteur_full as lf  # noqa: E402  lecteur strict partage

SONDE = "mhgp12_full_probe"
TRAMES = {"ng00": 39885, "ng01": 35551, "ng02": 45845}
UNIFORMES = (8000, 16000, 32000)
SEUIL_GRANDES = 60000
BRAS = ("avant", "avant_bis", "apres")
REGLE_R1 = {"base": "tete de main juste avant R1 (--base, --avant-archive, --avant-sha256)", "k": 5,
            "seuil_grandes_sites": SEUIL_GRANDES, "bootstrap": 10000,
            "graine": 20261008, "borne_grandes": 0.99, "borne_ng": 1.02, "fenetre_aa": 0.015,
            "tours_min": 5, "passes_min": 6,
            "statistique": "grandes : par tour, moyenne geometrique sur les grandes trames du rapport des murs "
                           "(second tour du processus) apres / avant ; ng : par tour, rapport des medianes des "
                           "passes 2..P ; IC 95 % par bootstrap sur les tours",
            "adoption": "identite FUL1 ; borne haute grandes < 0,99 ; borne haute < 1,02 sur chacune de ng00-02",
            "veto_aa": "refus si la moyenne geometrique avant_bis / avant sort de 1 +/- 0,015 (grandes, ng00-02)",
            "cache_blocs": "8 Gio, defaut de la sonde, pour les deux bras",
            "k10": "information sans jugement (ng00-02, memes statistiques)",
            "ecrite": "8 octobre 2026 a 12:31 UTC, avant toute mesure G4 de ces bras ; amendee a 12:32 UTC (base "
                      "fd84039c0), a 13:11 UTC (base 6497ed3b5), a 13:23 UTC (base en parametre), a 13:33 UTC (A6 "
                      "retire), a 13:57 UTC (base 4171b2653) et a 14:09 UTC (base 5f8e777cf), seuils "
                      "inchanges"}


def maintenant():
    return datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def mediane(valeurs):
    v = sorted(valeurs)
    n = len(v)
    return v[n // 2] if n % 2 else 0.5 * (v[n // 2 - 1] + v[n // 2])


def ic_bootstrap(logs, graine, tirages):
    """Moyenne geometrique et IC 95 % par bootstrap des logarithmes (une valeur par tour)."""
    rng = random.Random(graine)
    moyenne = math.exp(sum(logs) / len(logs))
    tires = sorted(sum(logs[rng.randrange(len(logs))] for _ in logs) / len(logs) for _ in range(tirages))
    return moyenne, math.exp(tires[int(0.025 * tirages)]), math.exp(tires[int(0.975 * tirages) - 1])


# ---- Construction -----------------------------------------------------------------------------------------------------
def deballer(archive, empreinte, dest):
    if not os.path.isfile(archive) or bf.sha256_file(archive) != empreinte:
        raise RuntimeError("archive de la base absente ou d'une autre empreinte : %s" % archive)
    with tarfile.open(archive, "r:gz") as tar:
        membres = tar.getmembers()
        for m in membres:
            nom = m.name
            if os.path.isabs(nom) or ".." in nom.split("/") or not (m.isfile() or m.isdir()) or \
                    not nom.startswith("morsehgp3D_v12"):
                raise RuntimeError("membre refuse dans l'archive : %r" % nom)
        tar.extractall(dest, members=membres)
    return dest


def construire(nom, racine, travail, jobs, journaux, nvcc):
    """Sonde du bras ; nvcc None : voie CPU (essai). Rend le chemin du binaire."""
    build = os.path.join(travail, nom, "build")
    os.makedirs(build, exist_ok=True)
    journal = os.path.join(journaux, "construction_%s.txt" % nom)
    configure = ["cmake", "-S", os.path.join(racine, "morsehgp3D_v12"), "-B", build, "-DCMAKE_BUILD_TYPE=Release",
                 "-DMHGP12_COORD_BITS=21"]
    if nvcc is not None:
        configure += ["-DMHGP12_ENABLE_CUDA=ON", "-DCMAKE_CUDA_COMPILER=" + nvcc]
    for argv in (configure, ["cmake", "--build", build, "-j", str(jobs), "--target", SONDE]):
        code, out, err, _s = bf.run(argv, 3600)
        with open(journal, "a", encoding="utf-8") as f:
            f.write("$ %s\n%s%s" % (" ".join(argv), out, err))
        if code != 0:
            raise RuntimeError("construction du bras %s en echec (%s)" % (nom, journal))
    binaire = os.path.join(build, SONDE)
    if not os.path.isfile(binaire):
        raise RuntimeError("sonde du bras %s absente" % nom)
    return binaire


def etape_construire(args, rapport):
    travail = os.path.abspath(args.travail)
    journaux = os.path.join(args.sortie, "construction")
    os.makedirs(journaux, exist_ok=True)
    nvcc = None
    if not args.essai:
        nvcc = shutil.which("nvcc") or ("/usr/local/cuda/bin/nvcc" if os.path.isfile("/usr/local/cuda/bin/nvcc")
                                        else None)
        if nvcc is None:
            raise RuntimeError("nvcc introuvable")
    base = os.path.join(travail, "avant", "src")
    if os.path.isdir(base):
        shutil.rmtree(base)
    os.makedirs(base)
    racines = {"avant": deballer(args.avant_archive, args.avant_sha256, base), "apres": args.src}
    binaires = {n: construire(n, racines[n], travail, args.jobs, journaux, nvcc) for n in ("avant", "apres")}
    binaires["avant_bis"] = binaires["avant"]  # A/A : le meme binaire, joue comme un bras distinct
    rapport["construction"] = {"debut": maintenant(), "base": args.base,
                               "archive_avant": os.path.basename(args.avant_archive),
                               "archive_avant_sha256": args.avant_sha256, "nvcc": nvcc,
                               "binaires": {n: {"chemin": b, "sha256": bf.sha256_file(b),
                                                "cmake": bf.cmake_extract(os.path.dirname(b))}
                                            for n, b in binaires.items()}}


# ---- Prises -----------------------------------------------------------------------------------------------------------
def attendu_de(args, k, fils, passes, empreinte, trames):
    return {"voie": "cpu" if args.essai else "appareil", "k": k, "fils": fils, "passes": passes,
            "empreinte": empreinte, "trames": [list(t) for t in trames], "budget_appareil": "partage", "bits": 21,
            "schema": "recouvert"}


def resume(passes):
    """Ce que le juge et les tableaux lisent d'une prise admise."""
    out = {"murs_ns": [], "empreintes": [], "queues_ns": [], "cpu_ns": [], "pics_octets": [], "fins_k": []}
    for r in passes:
        out["murs_ns"].append(r["wall_ns"])
        out["empreintes"].append(r.get("full_sha256"))
        out["queues_ns"].append(r["recouvrement"]["queue_ns"])
        out["cpu_ns"].append(r["cpu_ns"])
        out["pics_octets"].append(r["pic_octets"])
        out["fins_k"].append(r["fins_par_ordre_ns"][-1])
    return out


def admettre(journal, code, attendu):
    with open(journal, encoding="ascii", errors="replace") as f:
        texte = f.read()
    lu = lf.parse_output(code, texte, {**attendu, "trames": [tuple(t) for t in attendu["trames"]]})
    return lu["etat"], lu["raison"], (resume(lu["passes"]) if lu["etat"] == "ok" else None)


def prise(args, bras, entrees, attendu, journal):
    """Un processus neuf de la sonde du bras ; journal brut ; admission par le lecteur strict."""
    argv = [args.binaires[bras]] + entrees + ["--k=%d" % attendu["k"], "--threads=%d" % attendu["fils"],
                                              "--passes=%d" % attendu["passes"]]
    argv += ([] if args.essai else ["--device"]) + (["--digest"] if attendu["empreinte"] else [])
    code, _out, _err, secondes = bf.run(argv, args.delai, raw_out=journal, raw_err=journal + ".err")
    etat, raison, resu = admettre(journal, code, attendu)
    return {"journal": os.path.relpath(journal, args.sortie), "journal_sha256": bf.sha256_file(journal),
            "code": code, "bras": bras, "attendu": attendu, "secondes": round(secondes, 3), "etat": etat,
            "raison": raison, "resume": resu}


def trame_args(args, nom):
    return ["--trame=%s,%s,%s" % (os.path.join(args.donnees, "lidar_%s.u32le" % nom),
                                  os.path.join(args.donnees, "lidar_%s.ids.u32le" % nom), nom)]


def v12set(args):
    dossier = os.path.join(os.path.abspath(args.travail), "v12set")
    manifeste = bf.unpack(args.archive_v12set, dossier)
    if manifeste is None:
        raise RuntimeError("archive v12set refusee")
    cas = [(c["name"], c["count"]) for c in manifeste["cases"]]
    return dossier, cas


def session_args(dossier, cas):
    return ["--trame=%s,%s,%s" % (os.path.join(dossier, n + ".u32le"), os.path.join(dossier, n + ".ids.u32le"),
                                  n[-23:]) for n, _ in cas]


def etape_identite(args, rapport):
    dossier = os.path.join(args.sortie, "journaux", "identite")
    os.makedirs(dossier, exist_ok=True)
    v_dossier, cas = v12set(args)
    if args.essai:
        cas = cas[:2]
    ident = {"debut": maintenant(), "prises": {}}
    for bras in ("avant", "apres"):
        cles = []
        for trame, sites in TRAMES.items():
            for k, passes in ((5, 2),) if args.essai else ((5, 2), (10, 1)):
                cles.append(("%s_k%d" % (trame, k), trame_args(args, trame),
                             attendu_de(args, k, args.fils, passes, True, [(trame, sites)] * passes)))
        cles.append(("ng00_k5_1fil", trame_args(args, "ng00"), attendu_de(args, 5, 1, 1, True, [("ng00", 39885)])))
        for n in UNIFORMES[:2] if args.essai else UNIFORMES:
            nom = "uniform_u18_n%d" % n
            entree = ["--trame=%s,%s,u%d" % (os.path.join(args.donnees, nom + ".u32le"),
                                             os.path.join(args.donnees, nom + ".ids.u32le"), n)]
            cles.append(("u%d_k5" % n, entree, attendu_de(args, 5, args.fils, 1, True, [("u%d" % n, n)])))
        cles.append(("v12set_k5", session_args(v_dossier, cas),
                     attendu_de(args, 5, args.fils, len(cas), True, [(n[-23:], s) for n, s in cas])))
        for cle, entrees, attendu in cles:
            ident["prises"].setdefault(cle, {})[bras] = prise(args, bras, entrees, attendu,
                                                              os.path.join(dossier, "%s_%s.jsonl" % (cle, bras)))
    ident["fin"] = maintenant()
    rapport["identite"] = ident


def ordre_des_bras(tour):
    k = tour % len(BRAS)
    return BRAS[k:] + BRAS[:k]


def etape_grandes(args, rapport):
    dossier = os.path.join(args.sortie, "journaux", "grandes")
    os.makedirs(dossier, exist_ok=True)
    v_dossier, cas = v12set(args)
    grandes = [c for c in cas if c[1] > SEUIL_GRANDES]
    if args.essai:
        grandes = grandes[-2:]
    camp = {"debut": maintenant(), "trames": [n for n, _ in grandes], "tours": []}
    for t in range(args.tours_grandes):
        ordre = grandes[t % len(grandes):] + grandes[:t % len(grandes)]
        tour = {"ordre": [n for n, _ in ordre]}
        for bras in ordre_des_bras(t):
            attendu = attendu_de(args, 5, args.fils, 2 * len(ordre), False, [(n[-23:], s) for n, s in ordre] * 2)
            tour[bras] = prise(args, bras, session_args(v_dossier, ordre), attendu,
                               os.path.join(dossier, "%s_t%02d.jsonl" % (bras, t)))
        camp["tours"].append(tour)
    camp["fin"] = maintenant()
    rapport["grandes"] = camp


def etape_ng(args, rapport):
    dossier = os.path.join(args.sortie, "journaux", "ng")
    os.makedirs(dossier, exist_ok=True)
    noms = list(TRAMES)
    camp = {"debut": maintenant(), "passes": args.passes, "trames": {n: [] for n in noms}}
    for t in range(args.tours):
        for trame in noms[t % 3:] + noms[:t % 3]:
            tour = {}
            for bras in ordre_des_bras(t):
                attendu = attendu_de(args, 5, args.fils, args.passes, False, [(trame, TRAMES[trame])] * args.passes)
                tour[bras] = prise(args, bras, trame_args(args, trame), attendu,
                                   os.path.join(dossier, "%s_%s_t%02d.jsonl" % (trame, bras, t)))
            camp["trames"][trame].append(tour)
    camp["fin"] = maintenant()
    camp["binaires_apres"] = {n: bf.sha256_file(b) for n, b in args.binaires.items()}
    rapport["ng"] = camp


def etape_k10(args, rapport):
    """Information : ng00-02 a K10, memes alternances que ng ; aucune prise de cette etape ne decide du verdict."""
    dossier = os.path.join(args.sortie, "journaux", "k10")
    os.makedirs(dossier, exist_ok=True)
    noms = list(TRAMES)
    camp = {"debut": maintenant(), "passes": args.passes_k10, "trames": {n: [] for n in noms}}
    for t in range(args.tours_k10):
        for trame in noms[t % 3:] + noms[:t % 3]:
            tour = {}
            for bras in ordre_des_bras(t):
                attendu = attendu_de(args, 10, args.fils, args.passes_k10, False,
                                     [(trame, TRAMES[trame])] * args.passes_k10)
                tour[bras] = prise(args, bras, trame_args(args, trame), attendu,
                                   os.path.join(dossier, "%s_%s_t%02d.jsonl" % (trame, bras, t)))
            camp["trames"][trame].append(tour)
    camp["fin"] = maintenant()
    camp["binaires_apres"] = {n: bf.sha256_file(b) for n, b in args.binaires.items()}
    rapport["k10"] = camp


# ---- Juge -------------------------------------------------------------------------------------------------------------
def prises_du_rapport(rapport):
    out = []
    for groupe in rapport.get("identite", {}).get("prises", {}).values():
        out += [groupe[b] for b in sorted(groupe)]
    for tour in rapport.get("grandes", {}).get("tours", []):
        out += [tour[b] for b in BRAS if b in tour]
    for etape in ("ng", "k10"):
        for tours in rapport.get(etape, {}).get("trames", {}).values():
            out += [t[b] for t in tours for b in BRAS if b in t]
    return out


def reverifier(rapport, sortie, refus):
    """Le rejeu brut fait autorite : chaque journal re-hache et relu par le lecteur strict, resume recalcule."""
    racine, vus = os.path.realpath(sortie), set()
    for p in prises_du_rapport(rapport):
        chemin = os.path.realpath(os.path.join(racine, p["journal"]))
        if os.path.commonpath((racine, chemin)) != racine or chemin in vus or not os.path.isfile(chemin):
            refus.append("journal externe, reutilise ou absent : %s" % p["journal"])
            continue
        vus.add(chemin)
        if bf.sha256_file(chemin) != p["journal_sha256"]:
            refus.append("journal modifie : %s" % p["journal"])
            continue
        etat, _raison, resu = admettre(chemin, p["code"], p["attendu"])
        if etat != p["etat"] or json.dumps(resu, sort_keys=True) != json.dumps(p["resume"], sort_keys=True):
            refus.append("resume different du brut : %s" % p["journal"])


def juger_identite(rapport, refus):
    ident, identique, details = rapport.get("identite", {}).get("prises", {}), True, {}
    if not ident:
        refus.append("identite absente")
        return False, details
    for cle, groupe in sorted(ident.items()):
        if set(groupe) != {"avant", "apres"} or any(p["etat"] != "ok" for p in groupe.values()):
            refus.append("identite %s : prise absente ou non admise" % cle)
            continue
        if cle == "v12set_k5":
            a, b = groupe["avant"]["resume"]["empreintes"], groupe["apres"]["resume"]["empreintes"]
            ecarts = [i for i in range(len(a)) if a[i] != b[i]]
            details[cle] = {"trames": len(a), "ecarts": ecarts}
            identique = identique and not ecarts and len(a) == len(b)
        else:
            vues = {e for p in groupe.values() for e in p["resume"]["empreintes"]}
            details[cle] = sorted(e[:16] for e in vues)
            identique = identique and len(vues) == 1
    return identique, details


def logs_grandes(rapport, bras, refus):
    """Par tour : log de la moyenne geometrique sur les grandes trames du rapport bras / avant (second tour)."""
    out = []
    for t, tour in enumerate(rapport.get("grandes", {}).get("tours", [])):
        if any(tour.get(b, {}).get("etat") != "ok" for b in ("avant", bras)):
            refus.append("grandes : tour %d sans prise admise (%s ou avant)" % (t, bras))
            continue
        n = len(tour["ordre"])
        ma, mb = tour["avant"]["resume"]["murs_ns"][n:], tour[bras]["resume"]["murs_ns"][n:]
        out.append(sum(math.log(mb[i] / ma[i]) for i in range(n)) / n)
    return out


def logs_ng(rapport, trame, bras, refus):
    out = []
    for t, tour in enumerate(rapport.get("ng", {}).get("trames", {}).get(trame, [])):
        if any(tour.get(b, {}).get("etat") != "ok" for b in ("avant", bras)):
            refus.append("%s : tour %d sans prise admise (%s ou avant)" % (trame, t, bras))
            continue
        out.append(math.log(mediane(tour[bras]["resume"]["murs_ns"][1:]) /
                            mediane(tour["avant"]["resume"]["murs_ns"][1:])))
    return out


def logs_k10(rapport, trame, bras):
    """Information : tours dont les deux prises sont admises, sans refus pour les autres."""
    out = []
    for tour in rapport.get("k10", {}).get("trames", {}).get(trame, []):
        if all(tour.get(b, {}).get("etat") == "ok" for b in ("avant", bras)):
            out.append(math.log(mediane(tour[bras]["resume"]["murs_ns"][1:]) /
                                mediane(tour["avant"]["resume"]["murs_ns"][1:])))
    return out


def plan_demande(args):
    """Cohorte issue de la commande et du SEUL manifeste v12set ; jamais des prises a juger (fermeture de l'auditeur
    Codex, recu a6_pilote_admission, portee au pilote R1)."""
    if not args.archive_v12set:
        raise RuntimeError("rapport exige --archive-v12set (manifeste de la cohorte)")
    with tarfile.open(args.archive_v12set) as tar:
        membres = [m for m in tar.getmembers() if m.name == "bundle_manifest.json"]
        if len(membres) != 1 or not membres[0].isfile():
            raise RuntimeError("manifeste v12set absent ou multiple")
        brut = tar.extractfile(membres[0]).read()
    manifeste = json.loads(brut)
    return {"cas": [[c["name"], c["count"]] for c in manifeste["cases"]], "fils": args.fils, "passes": args.passes,
            "tours": args.tours, "tours_grandes": args.tours_grandes, "tours_k10": args.tours_k10,
            "passes_k10": args.passes_k10, "essai": args.essai, "base": args.base}


def _attendu_plan(essai, k, fils, passes, empreinte, trames):
    return dict(voie="cpu" if essai else "appareil", k=k, fils=fils, passes=passes, empreinte=empreinte,
                trames=trames, budget_appareil="partage", bits=21, schema="recouvert")


def _enc(x):
    return json.dumps(x, sort_keys=True, allow_nan=False)


def _prise_conforme(p, bras, attendu, admise=True):
    """Prise du bras attendu, configuration commandee ; admise (code 0, lecteur strict) sauf pour l'information K10."""
    if not isinstance(p, dict) or p.get("bras") != bras or _enc(p.get("attendu")) != _enc(attendu):
        return False
    return not admise or (type(p.get("code")) is int and p["code"] == 0 and p.get("etat") == "ok")


def _cohorte(rapport, plan, essai):
    """Erreur bloquante (texte) ou chaine vide ; leve KeyError, TypeError ou ValueError sur un rapport mal forme."""
    if not isinstance(plan, dict) or type(plan.get("essai")) is not bool or plan["essai"] != essai:
        return "plan de cohorte externe absent ou incoherent"
    if type(plan.get("base")) is not str or not plan["base"] or rapport["construction"].get("base") != plan["base"]:
        return "base du bras avant absente ou differente entre construction et jugement"
    if any(type(plan.get(k)) is not int or plan[k] <= 0
           for k in ("fils", "passes", "tours", "tours_grandes", "tours_k10", "passes_k10")):
        return "configuration de cohorte invalide"
    if not essai and (plan["fils"] != 48 or plan["passes"] < REGLE_R1["passes_min"] or
                      min(plan["tours"], plan["tours_grandes"]) < REGLE_R1["tours_min"]):
        return "configuration hors protocole R1"
    cas = plan["cas"]
    if type(cas) is not list or len(cas) != 37 or any(
            type(c) is not list or len(c) != 2 or type(c[0]) is not str or not c[0] or type(c[1]) is not int or
            c[1] <= 0 for c in cas):
        return "cohorte v12set invalide"
    if len({c[0] for c in cas}) != len(cas) or len({c[0][-23:] for c in cas}) != len(cas):
        return "trames ou labels dupliques"
    grandes = [c for c in cas if c[1] > SEUIL_GRANDES]
    if not grandes:
        return "aucune grande trame"
    if essai:
        cas, grandes = cas[:2], grandes[-2:]
    fils = plan["fils"]
    ident = {}
    for nom, sites in TRAMES.items():
        for k, passes in ((5, 2),) if essai else ((5, 2), (10, 1)):
            ident["%s_k%d" % (nom, k)] = _attendu_plan(essai, k, fils, passes, True, [[nom, sites]] * passes)
    ident["ng00_k5_1fil"] = _attendu_plan(essai, 5, 1, 1, True, [["ng00", TRAMES["ng00"]]])
    for n in UNIFORMES[:2] if essai else UNIFORMES:
        ident["u%d_k5" % n] = _attendu_plan(essai, 5, fils, 1, True, [["u%d" % n, n]])
    ident["v12set_k5"] = _attendu_plan(essai, 5, fils, len(cas), True, [[n[-23:], s] for n, s in cas])
    prises = rapport["identite"]["prises"]
    if set(prises) != set(ident) or any(set(prises[n]) != {"avant", "apres"} or any(
            not _prise_conforme(prises[n][b], b, att) for b in ("avant", "apres")) for n, att in ident.items()):
        return "identite hors cohorte commandee"
    camp = rapport["grandes"]
    if camp["trames"] != [n for n, _ in grandes] or len(camp["tours"]) != plan["tours_grandes"]:
        return "grandes : trames ou nombre de tours hors commande"
    for t, tour in enumerate(camp["tours"]):
        j = t % len(grandes)
        ordre = grandes[j:] + grandes[:j]
        att = _attendu_plan(essai, 5, fils, 2 * len(ordre), False, [[n[-23:], s] for n, s in ordre] * 2)
        if set(tour) != set(BRAS) | {"ordre"} or tour["ordre"] != [n for n, _ in ordre] or any(
                not _prise_conforme(tour[b], b, att) for b in BRAS):
            return "grandes : ordre, bras ou configuration hors commande"
    for etape, k, tours, passes, admise in (("ng", 5, plan["tours"], plan["passes"], True),
                                            ("k10", 10, plan["tours_k10"], plan["passes_k10"], False)):
        camp = rapport[etape]
        if camp["passes"] != passes or set(camp["trames"]) != set(TRAMES):
            return "%s : cohorte ou passes hors commande" % etape
        for nom, sites in TRAMES.items():
            if len(camp["trames"][nom]) != tours:
                return "%s : nombre de tours hors commande" % etape
            att = _attendu_plan(essai, k, fils, passes, False, [[nom, sites]] * passes)
            if any(set(tour) != set(BRAS) or any(not _prise_conforme(tour[b], b, att, admise) for b in BRAS)
                   for tour in camp["trames"][nom]):
                return "%s : bras ou configuration hors commande" % etape
    binaires = rapport["construction"]["binaires"]
    if set(binaires) != set(BRAS):
        return "construction incomplete"
    shas = {b: binaires[b]["sha256"] for b in BRAS}
    if any(type(h) is not str or len(h) != 64 or any(c not in "0123456789abcdef" for c in h) for h in shas.values()) \
            or shas["avant_bis"] != shas["avant"]:
        return "empreintes binaires ou identite A/A invalides"
    if rapport["ng"]["binaires_apres"] != shas or rapport["k10"]["binaires_apres"] != shas:
        return "binaires changes pendant les mesures (fermeture ng ou k10)"
    return ""


def verifier_cohorte(rapport, plan, essai):
    """Fermeture de la cohorte avant toute relecture et statistique : identite (cles, bras, configuration), grandes
    (trames du manifeste, ordre tourne, nombre de tours), ng et k10 (tours, passes, configuration), memes binaires
    avant et avant_bis, fermes apres les mesures."""
    try:
        return _cohorte(rapport, plan, essai)
    except (KeyError, TypeError, ValueError, IndexError, AttributeError):
        return "rapport ou plan mal forme"


def _juger(rapport, sortie, verifier, essai, plan=None):
    regle, refus, cas = REGLE_R1, [], {}
    if verifier:
        pourquoi = verifier_cohorte(rapport, plan, essai)
        if pourquoi:
            return {"verdict": "refuse", "refus": [pourquoi], "regle": regle, "cas": {}, "identite_ok": False}
    construction = rapport.get("construction", {}).get("binaires", {})
    if set(construction) != set(BRAS):
        refus.append("construction absente ou incomplete")
    if any(rapport.get("ng", {}).get("binaires_apres", {}).get(n) != construction.get(n, {}).get("sha256")
           for n in construction):
        refus.append("binaire change pendant les mesures")
    k10 = rapport.get("k10")
    if k10 is not None and any(k10.get("binaires_apres", {}).get(n) != construction.get(n, {}).get("sha256")
                               for n in construction):
        refus.append("binaire change pendant l'etape k10")
    env = rapport.get("environnement", {})
    if not essai and any(env.get(m, {}).get("gpu_apps") != "" for m in ("avant", "apres")):
        refus.append("GPU non vide avant ou apres")
    if verifier:
        reverifier(rapport, sortie, refus)
    identique, cas["identite"] = juger_identite(rapport, refus)
    tours_min = 1 if essai else regle["tours_min"]
    stats = {}
    for nom, logs in [("grandes", logs_grandes(rapport, "apres", refus)),
                      ("grandes_aa", logs_grandes(rapport, "avant_bis", refus))] + \
            [(t + s, logs_ng(rapport, t, b, refus)) for t in TRAMES for s, b in (("", "apres"), ("_aa", "avant_bis"))]:
        if len(logs) < tours_min:
            refus.append("%s : %d tours valides sur %d exiges" % (nom, len(logs), tours_min))
            continue
        moyenne, bas, haut = ic_bootstrap(logs, regle["graine"], regle["bootstrap"])
        stats[nom] = {"tours": len(logs), "moyenne_geometrique": moyenne, "ic95": [bas, haut]}
    for nom in ["grandes_aa"] + [t + "_aa" for t in TRAMES]:
        if nom in stats and abs(stats[nom]["moyenne_geometrique"] - 1.0) > regle["fenetre_aa"]:
            refus.append("veto A/A : %s = %.4f hors de 1 +/- %.3f" % (nom, stats[nom]["moyenne_geometrique"],
                                                                     regle["fenetre_aa"]))
    cas["statistiques"] = stats
    information = {}
    for trame in TRAMES:
        for suffixe, bras in (("", "apres"), ("_aa", "avant_bis")):
            logs = logs_k10(rapport, trame, bras)
            if logs:
                moyenne, bas, haut = ic_bootstrap(logs, regle["graine"], regle["bootstrap"])
                information["k10_" + trame + suffixe] = {"tours": len(logs), "moyenne_geometrique": moyenne,
                                                         "ic95": [bas, haut]}
    cas["information_k10"] = information
    if refus:
        verdict = "refuse"
    elif identique and stats["grandes"]["ic95"][1] < regle["borne_grandes"] and \
            all(stats[t]["ic95"][1] < regle["borne_ng"] for t in TRAMES):
        verdict = "adopte"
    else:
        verdict = "rejete"
    return {"verdict": verdict, "refus": refus, "regle": regle, "cas": cas, "identite_ok": identique}


def juger(rapport, sortie, verifier=True, essai=False, plan=None):
    """Copie JSON du rapport, jamais modifiee par le juge ; la voie sans rejeu est reservee a l'auto-test."""
    return _juger(json.loads(json.dumps(rapport, allow_nan=False)), sortie, verifier, essai, plan)


# ---- Auto-test du juge ------------------------------------------------------------------------------------------------
def synthetique(effet_grandes, effet_ng, aa=1.0, empreinte_differente=False, manque=False, graine=1):
    rng = random.Random(graine)
    bruit = lambda: 1 + 0.004 * (rng.random() - 0.5)  # noqa: E731
    rapport = {"construction": {"binaires": {b: {"sha256": "x"} for b in BRAS}},
               "ng": {"binaires_apres": {b: "x" for b in BRAS}, "trames": {}},
               "environnement": {"avant": {"gpu_apps": ""}, "apres": {"gpu_apps": ""}},
               "identite": {"prises": {}}, "grandes": {"tours": []}}
    for cle in ("ng00_k5", "ng01_k5", "u8000_k5"):
        rapport["identite"]["prises"][cle] = {b: {"etat": "ok", "resume": {"empreintes": ["ab" * 32] * 2}}
                                              for b in ("avant", "apres")}
    if empreinte_differente:
        rapport["identite"]["prises"]["ng01_k5"]["apres"]["resume"]["empreintes"][1] = "cd" * 32
    facteur = {"avant": 1.0, "avant_bis": aa}
    for t in range(5):
        tour = {"ordre": ["a", "b", "c"]}
        for b in BRAS:
            f = effet_grandes if b == "apres" else facteur[b]
            murs = [int(200e6 * (1 + 0.1 * i) * f * bruit()) for i in range(3)] * 2
            tour[b] = {"etat": "ok", "resume": {"murs_ns": murs}}
        if manque and t == 2:
            tour["apres"]["etat"] = "echec"
        rapport["grandes"]["tours"].append(tour)
    for trame in TRAMES:
        tours = []
        for t in range(5):
            tour = {}
            for b in BRAS:
                f = effet_ng.get(trame, 1.0) if b == "apres" else facteur[b]
                tour[b] = {"etat": "ok", "resume": {"murs_ns": [int(100e6 * f * bruit()) for _ in range(10)]}}
            tours.append(tour)
        rapport["ng"]["trames"][trame] = tours
    return rapport


def etape_auto_test():
    cas = [(dict(effet_grandes=0.97, effet_ng={}), "adopte"),
           (dict(effet_grandes=0.995, effet_ng={}), "rejete"),
           (dict(effet_grandes=0.97, effet_ng={"ng01": 1.05}), "rejete"),
           (dict(effet_grandes=0.97, effet_ng={}, empreinte_differente=True), "rejete"),
           (dict(effet_grandes=0.97, effet_ng={}, aa=1.03), "refuse"),
           (dict(effet_grandes=0.97, effet_ng={}, manque=True), "refuse")]
    ecarts = []
    for i, (params, attendu) in enumerate(cas):
        obtenu = juger(synthetique(graine=i + 1, **params), "", verifier=False)["verdict"]
        if obtenu != attendu:
            ecarts.append("cas %d : %s au lieu de %s" % (i, obtenu, attendu))
    for e in ecarts:
        print("pilote_r1_auto_test_ecart " + e)
    if ecarts:
        return 1
    print("pilote_r1_auto_test_ok cas=%d verdicts=adopte,rejete,refuse" % len(cas))
    return 0


# ---- Rapport ----------------------------------------------------------------------------------------------------------
def tableaux(rapport, jugement):
    st = jugement["cas"].get("statistiques", {})
    base = rapport.get("construction", {}).get("base")
    out = ["# R1 : raccourci du registre R, avant / apres (genere par pilote_r1.py)", "",
           "Base (bras avant) : %s ; verdict (REGLE_R1) : **%s** ; identite FUL1 : %s." % (
               base, jugement["verdict"], "oui" if jugement.get("identite_ok") else "non"), ""]
    if jugement["refus"]:
        out += ["Refus : " + " ; ".join(jugement["refus"]), ""]
    out += ["| cohorte | tours | moyenne geometrique apres / avant | IC 95 % | A/A |",
            "| --- | ---: | ---: | --- | ---: |"]
    for nom in ["grandes"] + list(TRAMES):
        s, a = st.get(nom), st.get(nom + "_aa")
        if s:
            out.append("| %s | %d | %.3f | %.3f - %.3f | %s |" % (nom, s["tours"], s["moyenne_geometrique"],
                                                                  s["ic95"][0], s["ic95"][1],
                                                                  "%.4f" % a["moyenne_geometrique"] if a else "-"))
    info = jugement["cas"].get("information_k10", {})
    if info:
        out += ["", "K10 (information, sans jugement) :", "", "| trame | tours | apres / avant | IC 95 % | A/A |",
                "| --- | ---: | ---: | --- | ---: |"]
        for trame in TRAMES:
            s, a = info.get("k10_" + trame), info.get("k10_" + trame + "_aa")
            if s:
                out.append("| %s | %d | %.3f | %.3f - %.3f | %s |" % (
                    trame, s["tours"], s["moyenne_geometrique"], s["ic95"][0], s["ic95"][1],
                    "%.4f" % a["moyenne_geometrique"] if a else "-"))
    tours = rapport.get("grandes", {}).get("tours", [])
    if tours:
        out += ["", "Grandes trames, par trame (medianes sur les tours, second tour du processus, ms) :", "",
                "| trame | mur avant | mur apres | queue avant | queue apres | R K - max(M K, V K) avant | apres |",
                "| --- | ---: | ---: | ---: | ---: | ---: | ---: |"]
        for nom in rapport["grandes"]["trames"]:
            ligne = {}
            for b in ("avant", "apres"):
                murs, queues, retards = [], [], []
                for tour in tours:
                    if tour.get(b, {}).get("etat") != "ok":
                        continue
                    n, i = len(tour["ordre"]), tour["ordre"].index(nom)
                    r = tour[b]["resume"]
                    murs.append(r["murs_ns"][n + i])
                    queues.append(r["queues_ns"][n + i])
                    fins = r["fins_k"][n + i]
                    retards.append(fins[4] - max(fins[2], fins[3]))
                ligne[b] = [mediane(x) / 1e6 if x else float("nan") for x in (murs, queues, retards)]
            out.append("| %s | %.1f | %.1f | %.1f | %.1f | %.1f | %.1f |" % (
                nom, ligne["avant"][0], ligne["apres"][0], ligne["avant"][1], ligne["apres"][1], ligne["avant"][2],
                ligne["apres"][2]))
    return "\n".join(out) + "\n"


def etape_rapport(args, rapport):
    plan = plan_demande(args)
    rapport["cohorte_demandee"] = plan
    jugement = juger(rapport, args.sortie, essai=args.essai, plan=plan)
    if args.essai:
        jugement["verdict"] = "essai"  # avant les tableaux, pas seulement avant le JSON final
    rapport["jugement"] = jugement
    with open(os.path.join(args.sortie, "tableaux_r1.md"), "w", encoding="utf-8") as f:
        f.write(tableaux(rapport, jugement))
    return jugement


def main(argv):
    p = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    p.add_argument("etapes", nargs="+", choices=("auto-test", "construire", "identite", "grandes", "ng", "k10",
                                                 "rapport", "tout"))
    for nom in ("--src", "--base", "--travail", "--donnees", "--sortie", "--avant-archive", "--avant-sha256",
                "--archive-v12set"):
        p.add_argument(nom)
    for nom, defaut in (("--fils", 48), ("--jobs", 44), ("--tours", 5), ("--tours-grandes", 6), ("--passes", 10),
                        ("--tours-k10", 3), ("--passes-k10", 5), ("--delai", 600)):
        p.add_argument(nom, type=int, default=defaut)
    p.add_argument("--essai", action="store_true")
    args = p.parse_args(argv[1:])
    etapes = ["auto-test", "construire", "identite", "grandes", "ng", "k10", "rapport"] if "tout" in args.etapes \
        else args.etapes
    if etapes == ["auto-test"]:
        return etape_auto_test()
    if not args.sortie or not args.travail or (not args.essai and (
            args.tours < REGLE_R1["tours_min"] or args.tours_grandes < REGLE_R1["tours_min"] or
            args.passes < REGLE_R1["passes_min"])):
        print("pilote_r1_refus usage : --sortie, --travail ; au moins %d tours et %d passes" % (
            REGLE_R1["tours_min"], REGLE_R1["passes_min"]))
        return 2
    os.makedirs(args.sortie, exist_ok=True)
    args.sortie = os.path.abspath(args.sortie)
    chemin = os.path.join(args.sortie, "rapport_r1.json")
    rapport = {"schema": "ehgp.v12.r1_pilote.v1", "regle": REGLE_R1}
    if os.path.isfile(chemin):
        with open(chemin, encoding="utf-8") as f:
            rapport = json.load(f)
    code = 0
    try:
        for etape in etapes:
            if etape == "auto-test":
                if etape_auto_test() != 0:
                    return 1
            elif etape == "construire":
                if not (args.src and args.avant_archive and args.avant_sha256 and args.base):
                    raise RuntimeError("construire exige --src --base --avant-archive --avant-sha256")
                rapport.setdefault("environnement", {})["avant"] = bf.environment("nvcc", not args.essai)
                etape_construire(args, rapport)
            elif etape == "rapport":
                rapport.setdefault("environnement", {})["apres"] = bf.environment("nvcc", not args.essai)
                if etape_auto_test() != 0:
                    return 1
                jugement = etape_rapport(args, rapport)
                if args.essai:
                    jugement["verdict"] = "essai"
                print("pilote_r1_verdict %s refus=%d" % (jugement["verdict"], len(jugement["refus"])))
                code = 3 if jugement["refus"] and not args.essai else 0
            else:
                args.binaires = {n: b["chemin"] for n, b in rapport["construction"]["binaires"].items()}
                if not args.donnees or not args.archive_v12set:
                    raise RuntimeError("%s exige --donnees et --archive-v12set" % etape)
                {"identite": etape_identite, "grandes": etape_grandes, "ng": etape_ng,
                 "k10": etape_k10}[etape](args, rapport)
            with open(chemin, "w", encoding="utf-8") as f:
                json.dump(rapport, f, indent=1, sort_keys=True)
    except (RuntimeError, KeyError, OSError, ValueError, tarfile.TarError) as e:
        print("pilote_r1_refus %s" % e)
        code = 3
    with open(chemin, "w", encoding="utf-8") as f:
        json.dump(rapport, f, indent=1, sort_keys=True)
    return code


if __name__ == "__main__":
    sys.exit(main(sys.argv))
