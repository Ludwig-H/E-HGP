#!/usr/bin/env python3
"""Pilote et juge de la tranche T2-d-A6 (chaine de l'ordre K de la Session recouverte : numerotation par morceaux,
indices de racine pour le noyau, historique par morceaux), avant / avant_bis / apres en processus alternes, mur FULL
de la voie par defaut de bench/full_probe.cpp (Session recouverte), voie appareil. Bibliotheque standard seule,
jouable sous python3 -S -O (aucune garde par assert). Chaque journal est lu par le lecteur strict partage
microbancs/outils/lecteur_full.py (schema 'recouvert').

Bras (sondes mhgp12_full_probe, Release, profil 21, MHGP12_ENABLE_CUDA=ON) :
  avant      archive epinglee des sources de main bdfca8fb1 (sha256 verifie avant deballage)
  avant_bis  le MEME binaire que avant, joue comme un bras distinct (A/A : bruit de la session ; veto)
  apres      les sources du paquet ({src}) : bdfca8fb1 + lot T2-d-A6
Les deux sondes ouvrent leur Session avec le cache de blocs de 8 Gio, defaut de la sonde depuis 72f622a55 (le
pilote ne passe pas --cache) : les deux bras l'ont.
L'ordre des bras tourne d'un cran par tour, sans inversion.

Etapes (dans l'ordre si plusieurs) :
  auto-test   le juge sur des campagnes synthetiques ; tout autre resultat que l'attendu : code 1
  construire  deballage, configuration et construction des deux sondes ; empreintes, extraits du CMakeCache
  identite    processus --digest, bras avant et apres : ng00-02 a K5 (2 passes) et K10 (1 passe) ; ng00 a K5 a un fil ;
              uniformes de 8 000, 16 000 et 32 000 sites a K5 ({donnees}/uniform_u18_n*.u32le) ; les 37 trames v12set
              en Session (une passe par trame)
  grandes     DECISIF : les trames v12set de plus de 60 000 sites, en Session, K5 : --tours-grandes tours ; dans chaque
              tour, un processus neuf par bras, qui joue les grandes trames deux fois (ordre tourne d'un tour a
              l'autre) ; le second tour du processus fait foi
  ng          GARDE : ng00, ng01, ng02 a K5 : --tours tours ; dans chaque tour, trames en ordre tourne et, pour chacune,
              un processus neuf par bras ; --passes passes (la premiere, a froid, ecartee)
  rapport     auto-test du juge, puis juge (REGLE_T2D_A6), rapport JSON et tableaux Markdown
  tout        auto-test construire identite grandes ng rapport

REGLE_T2D_A6 (ecrite le 8 octobre 2026 a 09:11 UTC, avant toute mesure de ces bras sur G4) :
  identite  toutes les empreintes FUL1 des processus d'identite identiques d'un bras et d'une passe a l'autre (par cle ;
            v12set trame par trame) ; sinon REJETE ;
  grandes   par tour, par grande trame : rapport des murs (second tour du processus) apres / avant ; par tour : moyenne
            geometrique sur les trames ; IC 95 % par bootstrap sur les tours (10 000 tirages, graine 20261008) ;
  ng        par tour et par trame : rapport des medianes des passes 2..P du mur, apres / avant ; meme IC ;
  A/A       memes statistiques pour avant_bis / avant ;
  ADOPTE    si l'identite tient, si la borne haute de l'IC des grandes trames est sous 0,97 ET si la borne haute de
            l'IC de chacune de ng00, ng01, ng02 est sous 1,02 ; REJETE sinon ;
  REFUSE    si une prise manque (processus en echec, journal illisible pour le lecteur strict, moins de tours valides
            que demandes), si un binaire change, si un journal manque ou a change, si le GPU n'est pas vide avant ou
            apres, si l'auto-test echoue, ou si la moyenne geometrique A/A sort de [0,985 ; 1,015] sur les grandes
            trames ou sur l'une de ng00, ng01, ng02 (veto : appariement invalide).
Publies sans jugement : queue de la Session (tour sans G), fins de l'ordre K (G, noyau, M, V, R), CPU par passe, pic du
budget, par bras.
Amendements du 8 octobre 2026, avant toute mesure G4 de ces bras (seuils, statistiques et veto inchanges) : 09:55 UTC,
base 72f622a55 au lieu de 86d7e39d8 (rebase demande par le coordinateur), les deux bras ont le cache de blocs de
8 Gio par defaut de la sonde ; 10:30 UTC, base bdfca8fb1 (porte native de terminaison integree, lecteur partage
qui exige l'ordre des fins par ordre : produit identique a celui de 72f622a55).

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
REGLE_T2D_A6 = {"base": "bdfca8fb1", "k": 5, "seuil_grandes_sites": SEUIL_GRANDES, "bootstrap": 10000,
                "graine": 20261008, "borne_grandes": 0.97, "borne_ng": 1.02, "fenetre_aa": 0.015,
                "tours_min": 5, "passes_min": 6,
                "statistique": "grandes : par tour, moyenne geometrique sur les grandes trames du rapport des murs "
                               "(second tour du processus) apres / avant ; ng : par tour, rapport des medianes des "
                               "passes 2..P ; IC 95 % par bootstrap sur les tours",
                "adoption": "identite FUL1 ; borne haute grandes < 0,97 ; borne haute < 1,02 sur chacune de ng00-02",
                "veto_aa": "refus si la moyenne geometrique avant_bis / avant sort de 1 +/- 0,015 (grandes, ng00-02)",
                "cache_blocs": "8 Gio, defaut de la sonde depuis 72f622a55, pour les deux bras",
                "ecrite": "8 octobre 2026 a 09:11 UTC, avant toute mesure G4 de ces bras ; amendee a 09:55 UTC "
                          "(base 72f622a55, cache de blocs des deux bras) et a 10:30 UTC (base bdfca8fb1), seuils "
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
    rapport["construction"] = {"debut": maintenant(), "archive_avant_sha256": args.avant_sha256, "nvcc": nvcc,
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


# ---- Juge -------------------------------------------------------------------------------------------------------------
def prises_du_rapport(rapport):
    out = []
    for groupe in rapport.get("identite", {}).get("prises", {}).values():
        out += [groupe[b] for b in sorted(groupe)]
    for tour in rapport.get("grandes", {}).get("tours", []):
        out += [tour[b] for b in BRAS if b in tour]
    for tours in rapport.get("ng", {}).get("trames", {}).values():
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


def _juger(rapport, sortie, verifier, essai):
    regle, refus, cas = REGLE_T2D_A6, [], {}
    construction = rapport.get("construction", {}).get("binaires", {})
    if set(construction) != set(BRAS):
        refus.append("construction absente ou incomplete")
    if any(rapport.get("ng", {}).get("binaires_apres", {}).get(n) != construction.get(n, {}).get("sha256")
           for n in construction):
        refus.append("binaire change pendant les mesures")
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
    if refus:
        verdict = "refuse"
    elif identique and stats["grandes"]["ic95"][1] < regle["borne_grandes"] and \
            all(stats[t]["ic95"][1] < regle["borne_ng"] for t in TRAMES):
        verdict = "adopte"
    else:
        verdict = "rejete"
    return {"verdict": verdict, "refus": refus, "regle": regle, "cas": cas, "identite_ok": identique}


def juger(rapport, sortie, verifier=True, essai=False):
    """Copie JSON du rapport, jamais modifiee par le juge ; la voie sans rejeu est reservee a l'auto-test."""
    return _juger(json.loads(json.dumps(rapport, allow_nan=False)), sortie, verifier, essai)


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
    cas = [(dict(effet_grandes=0.8, effet_ng={}), "adopte"),
           (dict(effet_grandes=0.99, effet_ng={}), "rejete"),
           (dict(effet_grandes=0.8, effet_ng={"ng01": 1.05}), "rejete"),
           (dict(effet_grandes=0.8, effet_ng={}, empreinte_differente=True), "rejete"),
           (dict(effet_grandes=0.8, effet_ng={}, aa=1.03), "refuse"),
           (dict(effet_grandes=0.8, effet_ng={}, manque=True), "refuse")]
    ecarts = []
    for i, (params, attendu) in enumerate(cas):
        obtenu = juger(synthetique(graine=i + 1, **params), "", verifier=False)["verdict"]
        if obtenu != attendu:
            ecarts.append("cas %d : %s au lieu de %s" % (i, obtenu, attendu))
    for e in ecarts:
        print("pilote_t2d_a6_auto_test_ecart " + e)
    if ecarts:
        return 1
    print("pilote_t2d_a6_auto_test_ok cas=%d verdicts=adopte,rejete,refuse" % len(cas))
    return 0


# ---- Rapport ----------------------------------------------------------------------------------------------------------
def tableaux(rapport, jugement):
    st = jugement["cas"].get("statistiques", {})
    out = ["# T2-d-A6 : chaine de l'ordre K, avant / apres (genere par pilote_t2d_a6.py)", "",
           "Verdict (REGLE_T2D_A6) : **%s** ; identite FUL1 : %s." % (
               jugement["verdict"], "oui" if jugement.get("identite_ok") else "non"), ""]
    if jugement["refus"]:
        out += ["Refus : " + " ; ".join(jugement["refus"]), ""]
    out += ["| cohorte | tours | moyenne geometrique apres / avant | IC 95 % | A/A |", "| --- | ---: | ---: | --- | ---: |"]
    for nom in ["grandes"] + list(TRAMES):
        s, a = st.get(nom), st.get(nom + "_aa")
        if s:
            out.append("| %s | %d | %.3f | %.3f - %.3f | %s |" % (nom, s["tours"], s["moyenne_geometrique"],
                                                                  s["ic95"][0], s["ic95"][1],
                                                                  "%.4f" % a["moyenne_geometrique"] if a else "-"))
    tours = rapport.get("grandes", {}).get("tours", [])
    if tours:
        out += ["", "Grandes trames, par trame (medianes sur les tours, second tour du processus, ms) :", "",
                "| trame | mur avant | mur apres | queue avant | queue apres | noyau K - G K avant | apres |",
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
                    retards.append(r["fins_k"][n + i][1] - r["fins_k"][n + i][0])
                ligne[b] = [mediane(x) / 1e6 if x else float("nan") for x in (murs, queues, retards)]
            out.append("| %s | %.1f | %.1f | %.1f | %.1f | %.1f | %.1f |" % (
                nom, ligne["avant"][0], ligne["apres"][0], ligne["avant"][1], ligne["apres"][1], ligne["avant"][2],
                ligne["apres"][2]))
    return "\n".join(out) + "\n"


def etape_rapport(args, rapport):
    jugement = juger(rapport, args.sortie, essai=args.essai)
    rapport["jugement"] = jugement
    with open(os.path.join(args.sortie, "tableaux_t2d_a6.md"), "w", encoding="utf-8") as f:
        f.write(tableaux(rapport, jugement))
    return jugement


def main(argv):
    p = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    p.add_argument("etapes", nargs="+", choices=("auto-test", "construire", "identite", "grandes", "ng", "rapport",
                                                 "tout"))
    for nom in ("--src", "--travail", "--donnees", "--sortie", "--avant-archive", "--avant-sha256",
                "--archive-v12set"):
        p.add_argument(nom)
    for nom, defaut in (("--fils", 48), ("--jobs", 44), ("--tours", 5), ("--tours-grandes", 6), ("--passes", 10),
                        ("--delai", 600)):
        p.add_argument(nom, type=int, default=defaut)
    p.add_argument("--essai", action="store_true")
    args = p.parse_args(argv[1:])
    etapes = ["auto-test", "construire", "identite", "grandes", "ng", "rapport"] if "tout" in args.etapes \
        else args.etapes
    if etapes == ["auto-test"]:
        return etape_auto_test()
    if not args.sortie or not args.travail or (not args.essai and (
            args.tours < REGLE_T2D_A6["tours_min"] or args.tours_grandes < REGLE_T2D_A6["tours_min"] or
            args.passes < REGLE_T2D_A6["passes_min"])):
        print("pilote_t2d_a6_refus usage : --sortie, --travail ; au moins %d tours et %d passes" % (
            REGLE_T2D_A6["tours_min"], REGLE_T2D_A6["passes_min"]))
        return 2
    os.makedirs(args.sortie, exist_ok=True)
    args.sortie = os.path.abspath(args.sortie)
    chemin = os.path.join(args.sortie, "rapport_t2d_a6.json")
    rapport = {"schema": "ehgp.v12.t2d_a6_pilote.v1", "regle": REGLE_T2D_A6}
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
                if not (args.src and args.avant_archive and args.avant_sha256):
                    raise RuntimeError("construire exige --src --avant-archive --avant-sha256")
                rapport.setdefault("environnement", {})["avant"] = bf.environment("nvcc", not args.essai)
                etape_construire(args, rapport)
            elif etape == "rapport":
                rapport.setdefault("environnement", {})["apres"] = bf.environment("nvcc", not args.essai)
                if etape_auto_test() != 0:
                    return 1
                jugement = etape_rapport(args, rapport)
                if args.essai:
                    jugement["verdict"] = "essai"
                print("pilote_t2d_a6_verdict %s refus=%d" % (jugement["verdict"], len(jugement["refus"])))
                code = 3 if jugement["refus"] and not args.essai else 0
            else:
                args.binaires = {n: b["chemin"] for n, b in rapport["construction"]["binaires"].items()}
                if not args.donnees or not args.archive_v12set:
                    raise RuntimeError("%s exige --donnees et --archive-v12set" % etape)
                {"identite": etape_identite, "grandes": etape_grandes, "ng": etape_ng}[etape](args, rapport)
            with open(chemin, "w", encoding="utf-8") as f:
                json.dump(rapport, f, indent=1, sort_keys=True)
    except (RuntimeError, KeyError, OSError, ValueError, tarfile.TarError) as e:
        print("pilote_t2d_a6_refus %s" % e)
        code = 3
    with open(chemin, "w", encoding="utf-8") as f:
        json.dump(rapport, f, indent=1, sort_keys=True)
    return code


if __name__ == "__main__":
    sys.exit(main(sys.argv))
