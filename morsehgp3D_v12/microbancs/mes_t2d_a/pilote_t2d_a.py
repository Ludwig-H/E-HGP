#!/usr/bin/env python3
"""Pilote et juge de la tranche T2-d-A (recouvrement de G et de T, M, V, R, decision D-F2 : Session recouverte
build_tower) : avant / apres en processus alternes, mur FULL de bench/full_probe.cpp, voie appareil. Bibliotheque
standard seule, jouable sous python3 -S -O (aucune garde par assert).

Bras (sondes mhgp12_full_probe construites ici : Release, profil 21, MHGP12_ENABLE_CUDA=ON, sm_120) :
  avant  archive epinglee des sources de la base (main a 902041f66 : resolve_tower puis build_forests ; sonde avec
         cpu_ns et memoire par etage), sha256 verifie avant deballage ; schema sequentiel (sans etapes_schema)
  apres  les sources du paquet ({src}), sonde jouee avec --recouvert : Session recouverte (build_tower) ; lignes
         etapes_schema = "recouvert" (partition murale P, C, G, raccord, TMVR ; fenetres_ns : sommes de fenetres
         murales des taches ; recouvrement ; fins par ordre). Sans --recouvert, la meme sonde joue la voie sequentielle
         refondue (prises d'identite apres_sequentiel)

Etapes (dans l'ordre si plusieurs) :
  auto-test   le juge sur des campagnes synthetiques (adopte, rejete, refuse) ; tout autre resultat : code 1
  construire  deballage, configuration et construction des deux sondes ; journaux, empreintes des binaires et extrait
              du CMakeCache de chaque bras
  identite    processus --digest (empreinte FUL1 hors du mur), par bras : ng00, ng01, ng02 a K5 (2 passes) et a K10
              (1 passe) ; ng00 a K5 a UN fil (1 passe, determinisme 1 contre --fils fils : memes empreintes que
              ng00 a K5) ; uniformes de 8 000, 16 000 et 32 000 sites a K5 (--uniform=N,20261007,18, 1 passe) ; les 37
              trames v12set a K5 (un processus, une passe par trame) ; en plus, ng00, ng01, ng02 a K5 par la sonde
              apres SANS --recouvert (voie sequentielle refondue, 2 passes)
  campagne    K5, ng00 ng01 ng02, --fils fils, voie appareil, sans empreinte : --processus tours ; dans chaque tour,
              les trames en ordre tourne et, pour chacune, un processus neuf par bras, l'ordre des bras alterne d'un
              tour a l'autre ; --passes passes par processus (la premiere, a froid, ecartee)
  session     les 37 trames v12set en Session, K5, voie appareil, sans empreinte : --processus-session processus par
              bras, deux tours chacun (le second fait foi), ordre des trames tourne d'un processus a l'autre ; publie
              la mediane et le maximum (sur les trames, des maximums des medianes par processus), non juge
  rapport     auto-test du juge, puis juge (REGLE_T2D_A), rapport JSON et tableaux Markdown
  tout        auto-test construire identite campagne session rapport

REGLE_T2D_A (ecrite le 8 octobre 2026, avant toute mesure sur G4 ; bras avant reporte de 8dc5d6b16 a 902041f66 a la
reprise du meme jour, meme produit plus la memoire par etage de la sonde, regle inchangee) : par processus de la
campagne, mur FULL = mediane
des passes 2 a P de wall_ns ; par tour et par trame, rapport apres / avant ; moyenne geometrique des rapports des tours
et IC 95 % par bootstrap sur les tours (10 000 tirages, graine 20261008). ADOPTE si toutes les empreintes FUL1 des
processus d'identite sont identiques d'un bras a l'autre et d'une passe a l'autre (pour chaque trame et chaque K) ET si
la borne haute de l'IC est sous 1 sur CHACUNE des trames ng00, ng01, ng02 a K5 ; REJETE sinon ; REFUSE si une prise
manque (processus en echec, passe ou empreinte absente, moins de --processus tours valides par trame), si un binaire
change pendant les mesures, si un journal manque ou a change, si le GPU n'est pas isole, ou si l'auto-test echoue.

Mode --essai (local, jamais une decision) : voie CPU, construction sans CUDA, 2 tours, K10 et uniformes de 32 000 sites
ecartes, 2 trames v12set ; verdict force a « essai ».

Publies sans jugement, par bras : temps CPU du processus pendant le mur (cpu_ns de la sonde, tous les fils : la
Session recouverte fait patienter ses fils sans travail par pauses, cessions puis sommeils de 20 us, ce temps-la y
figure) et pic du budget de la Session (pic_octets ; apres : un index des naissances par ordre, admission unique de la
region).

Exemple (commande d'un plan de session G4) :
  python3 pilote_t2d_a.py tout --src <racine du paquet> --travail <dossier> --donnees <donnees> --sortie <sortie> \\
      --avant-archive <donnees>/v12_src_avant_t2d_a.tar.gz --avant-sha256 <hex> --fils 48 --jobs 44

Codes : 0 campagne complete et jugee (le verdict est dans le rapport) ; 1 auto-test en echec ; 2 usage ; 3 refus.
"""
import argparse
import datetime
import hashlib
import json
import math
import os
import random
import re
import shutil
import subprocess
import sys
import tarfile

SONDE = "mhgp12_full_probe"
TRAMES = ("ng00", "ng01", "ng02")
BRAS = ("avant", "apres")
UNIFORMES = (8000, 16000, 32000)
REGLE_T2D_A = {"trames_decisives": TRAMES, "k": 5, "processus_min": 5, "passes_min": 6, "bootstrap": 10000,
               "graine": 20261008, "seuil_borne_haute": 1.0,
               "statistique": "par processus : mediane des passes 2..P du mur FULL (wall_ns) ; par tour : rapport "
                              "apres / avant ; moyenne geometrique et IC 95 % par bootstrap sur les tours",
               "adoption": "empreintes FUL1 des processus d'identite identiques d'un bras et d'une passe a l'autre, "
                           "et borne haute de l'IC sous 1 sur chacune des trames ng00, ng01, ng02 a K5"}
ETAPES = ("P", "C", "G", "raccord", "TMVR", "T", "M", "V", "R")  # schema sequentiel : murs
MUR = ("P", "C", "G", "raccord", "TMVR")  # partition murale, les deux schemas
FENETRES = ("G", "foret", "foret_apres_g", "T", "M", "V", "R")  # recouvert : sommes de fenetres murales des taches
RECOUVREMENT = ("tour_ns", "ouverture_ns", "fin_g_ns", "fin_ns", "queue_ns", "noyau_reprises", "noyau_arrets",
                "admis_octets")
CHAMPS_RECOUVERTS = ("etapes_schema", "recouvrement", "fenetres_ns", "fins_par_ordre_ns")
RESUME = ("murs_ns", "empreintes", "etapes", "recouvrements", "sites", "cpu_ns", "pics_octets")
# Provenance de la construction (comme MES-B, reponse du developpeur du 8 octobre) : extrait du CMakeCache de chaque bras.
CMAKE_CLES = re.compile(r"^(CMAKE_BUILD_TYPE|CMAKE_CXX_COMPILER|CMAKE_CUDA_COMPILER|CMAKE_CUDA_ARCHITECTURES|"
                        r"CMAKE_CXX_FLAGS|CMAKE_CXX_FLAGS_RELEASE|CMAKE_CUDA_FLAGS|CMAKE_CUDA_FLAGS_RELEASE|"
                        r"MHGP12_[A-Z0-9_]+)(:[A-Z]+)?=")


def maintenant():
    return datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def sha256(chemin):
    h = hashlib.sha256()
    with open(chemin, "rb") as f:
        for bloc in iter(lambda: f.read(1 << 20), b""):
            h.update(bloc)
    return h.hexdigest()


def mediane(valeurs):
    v = sorted(valeurs)
    n = len(v)
    return v[n // 2] if n % 2 else 0.5 * (v[n // 2 - 1] + v[n // 2])


def bootstrap_gm(logs, rng, tirages):
    """Moyenne geometrique et IC 95 % par bootstrap (conventions de REGLE_T2C)."""
    gm = math.exp(sum(logs) / len(logs))
    boot = []
    for _ in range(tirages):
        echantillon = [logs[rng.randrange(len(logs))] for _ in logs]
        boot.append(sum(echantillon) / len(echantillon))
    boot.sort()
    return gm, math.exp(boot[int(0.025 * tirages)]), math.exp(boot[int(0.975 * tirages) - 1])


def jouer(commande, journal, delai=3600):
    """Execute une commande ; sortie brute dans journal ; rend le code (ou 'expire')."""
    try:
        proc = subprocess.run(commande, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=delai, check=False)
        code, sortie, erreur = proc.returncode, proc.stdout, proc.stderr
    except subprocess.TimeoutExpired as e:
        code, sortie, erreur = "expire", e.stdout or b"", e.stderr or b""
    except OSError as e:
        code, sortie, erreur = 127, b"", str(e).encode()
    with open(journal, "wb") as f:
        f.write(sortie)
    if erreur:
        with open(journal + ".stderr", "wb") as f:
            f.write(erreur[-8000:])
    return code


def environnement():
    env = {}
    for nom, argv in (("gpu", ["nvidia-smi", "--query-gpu=name,driver_version,memory.total", "--format=csv,noheader"]),
                      ("gpu_apps", ["nvidia-smi", "--query-compute-apps=pid", "--format=csv,noheader"])):
        try:
            proc = subprocess.run(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=60, check=False)
            env[nom] = proc.stdout.decode("utf-8", "replace").strip() if proc.returncode == 0 else None
        except (OSError, subprocess.TimeoutExpired):
            env[nom] = None
    return env


# ---- Construction -----------------------------------------------------------------------------------------------------
def deballer(archive, empreinte, dest):
    if not os.path.isfile(archive) or sha256(archive) != empreinte:
        raise RuntimeError("archive de la base absente ou d'une autre empreinte : %s" % archive)
    with tarfile.open(archive, "r:gz") as tar:
        membres = tar.getmembers()
        for m in membres:
            nom = m.name
            if os.path.isabs(nom) or ".." in nom.split("/") or not (m.isfile() or m.isdir()) or \
                    not nom.startswith("morsehgp3D_v12"):
                raise RuntimeError("membre refuse dans l'archive : %r" % nom)
        tar.extractall(dest, members=membres)
    return os.path.join(dest, "morsehgp3D_v12")


def construire_bras(nom, source, travail, jobs, journaux, essai=False):
    build = os.path.join(travail, nom, "build")
    os.makedirs(build, exist_ok=True)
    configure = ["cmake", "-S", source, "-B", build, "-DCMAKE_BUILD_TYPE=Release", "-DMHGP12_COORD_BITS=21"]
    if not essai:
        nvcc = shutil.which("nvcc") or \
            ("/usr/local/cuda/bin/nvcc" if os.path.isfile("/usr/local/cuda/bin/nvcc") else None)
        if nvcc is None:
            raise RuntimeError("nvcc introuvable")
        configure += ["-DMHGP12_ENABLE_CUDA=ON", "-DCMAKE_CUDA_COMPILER=" + nvcc]
    code = jouer(configure, os.path.join(journaux, "configure_%s.txt" % nom))
    if code == 0:
        code = jouer(["cmake", "--build", build, "-j", str(jobs), "--target", SONDE],
                     os.path.join(journaux, "build_%s.txt" % nom))
    binaire = os.path.join(build, SONDE)
    if code != 0 or not os.path.isfile(binaire):
        raise RuntimeError("construction du bras %s en echec (journal build_%s.txt)" % (nom, nom))
    return binaire


def extrait_cmake(binaire):
    """Lignes retenues du CMakeCache du dossier de construction de la sonde (None s'il manque)."""
    try:
        with open(os.path.join(os.path.dirname(binaire), "CMakeCache.txt"), encoding="utf-8", errors="replace") as f:
            return [l.rstrip("\n") for l in f if CMAKE_CLES.match(l)]
    except OSError:
        return None


def etape_construire(args, rapport):
    travail = os.path.abspath(args.travail)
    journaux = os.path.join(args.sortie, "construction")
    os.makedirs(journaux, exist_ok=True)
    base = os.path.join(travail, "avant", "src")
    if os.path.isdir(base):
        shutil.rmtree(base)
    os.makedirs(base)
    sources = {"avant": deballer(args.avant_archive, args.avant_sha256, base),
               "apres": os.path.join(args.src, "morsehgp3D_v12")}
    binaires = {n: construire_bras(n, sources[n], travail, args.jobs, journaux, args.essai) for n in BRAS}
    rapport["construction"] = {"debut": maintenant(), "archive_avant_sha256": args.avant_sha256,
                               "binaires": {n: {"chemin": b, "sha256": sha256(b), "cmake": extrait_cmake(b)}
                                            for n, b in binaires.items()}}


# ---- Admission d'un journal de la sonde ---------------------------------------------------------------------------------
def exiger(condition, message):
    if not condition:
        raise ValueError(message)


def naturel(value, maximum=(1 << 64) - 1, minimum=0):
    exiger(type(value) is int and minimum <= value <= maximum, "entier hors domaine")
    return value


def objet_unique(pairs):
    out = {}
    for key, value in pairs:
        exiger(key not in out, "cle JSON repetee")
        out[key] = value
    return out


def constante_interdite(value):
    raise ValueError("constante JSON non finie : " + value)


def hexadecimal(value):
    exiger(type(value) is str and len(value) == 64 and all(c in "0123456789abcdef" for c in value),
           "empreinte SHA-256 invalide")
    return value


def lire_recouvrement(row, etapes, k):
    """Champs propres au schema recouvert d'une ligne ; rend l'entree de recouvrements de la passe."""
    exiger(row.get("etapes_schema") == "recouvert", "schema recouvert absent")
    rec, fen, fins = row.get("recouvrement"), row.get("fenetres_ns"), row.get("fins_par_ordre_ns")
    exiger(type(rec) is dict and set(rec) == set(RECOUVREMENT), "recouvrement absent ou inconnu")
    exiger(type(fen) is dict and set(fen) == set(FENETRES), "fenetres murales absentes ou inconnues")
    exiger(type(fins) is list and len(fins) == k and all(type(f) is list and len(f) == 5 for f in fins),
           "fins par ordre invalides")
    entree = {c: naturel(rec[c]) for c in RECOUVREMENT}
    exiger(entree["fin_g_ns"] == etapes["G"] and entree["queue_ns"] == etapes["TMVR"] and
           entree["fin_ns"] <= entree["tour_ns"], "recouvrement incoherent avec la partition murale")
    entree["fenetres_ns"] = {c: naturel(fen[c]) for c in FENETRES}
    entree["fins_par_ordre_ns"] = [[naturel(x) for x in f] for f in fins]
    return entree


def lire_prise(journal, code, attendu):
    """Journal complet d'un processus de la sonde ; attendu : k, fils, passes, bras, schema ("sequentiel" ou
    "recouvert"), empreinte (bool), trames (liste des noms attendus passe par passe). Rend murs, empreintes, etapes
    (partition murale, et murs T, M, V, R au schema sequentiel), recouvrements (schema recouvert), CPU et pics."""
    exiger(type(code) is int and code == 0, "processus en echec (code %s)" % code)
    with open(journal, encoding="utf-8", errors="strict") as f:
        lignes = [json.loads(l, object_pairs_hook=objet_unique, parse_constant=constante_interdite)
                  for l in f.read().splitlines()]
    exiger(all(type(l) is dict for l in lignes), "ligne JSON non objet")
    appareil = attendu.get("appareil", True)
    if appareil:
        exiger(lignes and lignes[0].get("phase") == "open" and lignes[0].get("status") == "ok", "ouverture absente")
    exiger(lignes and lignes[-1] == {"phase": "exit", "status": "ok", "reason": "none"}, "fin de journal invalide")
    corps = lignes[1:-1] if appareil else lignes[:-1]
    exiger(len(corps) == 2 * attendu["passes"], "passes absentes ou en trop")
    out = {c: [] for c in RESUME}
    for p in range(attendu["passes"]):
        row, libre = corps[2 * p], corps[2 * p + 1]
        exiger(row.get("phase") == "full" and naturel(row.get("pass")) == p and row.get("status") == "ok",
               "passe %d absente ou desordonnee" % p)
        exiger(libre.get("phase") == "liberation" and libre.get("pass") == p, "liberation absente")
        exiger(row.get("voie") == ("device" if appareil else "cpu") and row.get("coord_bits") == 21 and
               row.get("kmax") == attendu["k"] and
               row.get("threads") == attendu["fils"] and row.get("trame") == attendu["trames"][p],
               "configuration differente de la commande")
        out["sites"].append(naturel(row.get("sites"), (1 << 32) - 1, 1))
        out["murs_ns"].append(naturel(row.get("wall_ns"), minimum=1))
        etapes, cles = row.get("etapes_ns"), (MUR if attendu["schema"] == "recouvert" else ETAPES)
        exiger(type(etapes) is dict and set(etapes) == set(cles), "etapes absentes ou inconnues")
        out["etapes"].append({e: naturel(etapes[e]) for e in cles})
        exiger(sum(etapes[e] for e in MUR) <= out["murs_ns"][-1], "partition murale au-dela du mur")
        cpu = row.get("cpu_ns")
        out["cpu_ns"].append(None if cpu is None else naturel(cpu))
        out["pics_octets"].append(naturel(row.get("pic_octets")))
        if attendu["empreinte"]:
            out["empreintes"].append(hexadecimal(row.get("full_sha256")))
        else:
            exiger("full_sha256" not in row, "empreinte non demandee")
        if attendu["schema"] == "recouvert":
            out["recouvrements"].append(lire_recouvrement(row, etapes, attendu["k"]))
        else:
            exiger(all(c not in row for c in CHAMPS_RECOUVERTS), "champ du schema recouvert dans le schema sequentiel")
    return out


def commande(binaire, entrees, attendu):
    argv = [binaire] + entrees + ["--k=%d" % attendu["k"], "--threads=%d" % attendu["fils"],
                                  "--passes=%d" % attendu["passes"]]
    return argv + (["--device"] if attendu.get("appareil", True) else []) + \
        (["--digest"] if attendu["empreinte"] else []) + (["--recouvert"] if attendu["schema"] == "recouvert" else [])


def trame_args(args, nom):
    return ["--trame=%s,%s,%s" % (os.path.join(args.donnees, "lidar_%s.u32le" % nom),
                                  os.path.join(args.donnees, "lidar_%s.ids.u32le" % nom), nom)]


def prise(args, bras, entrees, attendu, journal):
    """Un processus neuf de la sonde du bras ; admission du journal complet. Schema : recouvert pour le bras apres,
    sequentiel pour le bras avant, sauf s'il est deja fixe (prises apres_sequentiel)."""
    attendu["appareil"] = not args.essai
    attendu.setdefault("schema", "recouvert" if bras == "apres" else "sequentiel")
    code = jouer(commande(args.binaires[bras], entrees, attendu), journal, args.delai)
    out = {"journal": os.path.relpath(journal, args.sortie), "journal_sha256": sha256(journal), "code": code,
           "bras": bras, "attendu": attendu, "valide": False}
    try:
        out.update(lire_prise(journal, code, attendu))
        out["valide"] = True
    except (ValueError, TypeError, KeyError, OSError, UnicodeError) as error:
        out["admission"] = str(error)
    return out


# ---- Campagnes --------------------------------------------------------------------------------------------------------
def v12set(args):
    dossier = os.path.join(os.path.abspath(args.travail), "v12set")
    os.makedirs(dossier, exist_ok=True)
    with tarfile.open(args.archive_v12set) as tar:
        membres = tar.getmembers()
        for m in membres:
            if not m.isfile() or "/" in m.name or m.name.startswith("."):
                raise RuntimeError("membre refuse dans l'archive v12set : %r" % m.name)
        for m in membres:
            with open(os.path.join(dossier, m.name), "wb") as out:
                out.write(tar.extractfile(m).read())
    with open(os.path.join(dossier, "bundle_manifest.json"), encoding="utf-8") as f:
        noms = [cas["name"] for cas in json.load(f)["cases"]]
    for n in noms:
        if not all(os.path.isfile(os.path.join(dossier, n + s)) for s in (".u32le", ".ids.u32le")):
            raise RuntimeError("trame v12set absente : %s" % n)
    return dossier, (noms[:2] if args.essai else noms)


def session_args(dossier, ordre):
    return ["--trame=%s,%s,%s" % (os.path.join(dossier, n + ".u32le"), os.path.join(dossier, n + ".ids.u32le"),
                                  n[-23:]) for n in ordre]


def etape_identite(args, rapport):
    dossier = os.path.join(args.sortie, "journaux", "identite")
    os.makedirs(dossier, exist_ok=True)
    v_dossier, noms = v12set(args)
    ident = {"debut": maintenant(), "prises": {}}
    for bras in BRAS:
        for trame in TRAMES:
            for k, passes in ((5, 2),) if args.essai else ((5, 2), (10, 1)):
                cle = "%s_k%d" % (trame, k)
                attendu = {"k": k, "fils": args.fils, "passes": passes, "bras": bras, "empreinte": True,
                           "trames": [trame] * passes}
                ident["prises"].setdefault(cle, {})[bras] = prise(args, bras, trame_args(args, trame), attendu,
                                                                  os.path.join(dossier, "%s_%s.jsonl" % (cle, bras)))
        # determinisme : ng00 a K5 a un fil, memes empreintes attendues qu'a --fils fils
        attendu = {"k": 5, "fils": 1, "passes": 1, "bras": bras, "empreinte": True, "trames": ["ng00"]}
        ident["prises"].setdefault("ng00_k5", {})[bras + "_1fil"] = prise(
            args, bras, trame_args(args, "ng00"), attendu, os.path.join(dossier, "ng00_k5_%s_1fil.jsonl" % bras))
        for n in UNIFORMES[:2] if args.essai else UNIFORMES:
            cle = "u%d_k5" % n
            attendu = {"k": 5, "fils": args.fils, "passes": 1, "bras": bras, "empreinte": True, "trames": ["uniforme"]}
            ident["prises"].setdefault(cle, {})[bras] = prise(args, bras, ["--uniform=%d,20261007,18" % n], attendu,
                                                              os.path.join(dossier, "%s_%s.jsonl" % (cle, bras)))
        attendu = {"k": 5, "fils": args.fils, "passes": len(noms), "bras": bras, "empreinte": True,
                   "trames": [n[-23:] for n in noms]}
        ident["prises"].setdefault("v12set_k5", {})[bras] = prise(args, bras, session_args(v_dossier, noms), attendu,
                                                                  os.path.join(dossier, "v12set_k5_%s.jsonl" % bras))
    # voie sequentielle refondue de la sonde apres (sans --recouvert) : memes empreintes attendues
    for trame in TRAMES:
        cle = "%s_k5" % trame
        attendu = {"k": 5, "fils": args.fils, "passes": 2, "bras": "apres", "schema": "sequentiel", "empreinte": True,
                   "trames": [trame] * 2}
        ident["prises"][cle]["apres_sequentiel"] = prise(args, "apres", trame_args(args, trame), attendu,
                                                         os.path.join(dossier, "%s_apres_sequentiel.jsonl" % cle))
    ident["fin"] = maintenant()
    rapport["identite"] = ident


def etape_campagne(args, rapport):
    dossier = os.path.join(args.sortie, "journaux", "k5")
    os.makedirs(dossier, exist_ok=True)
    camp = {"debut": maintenant(), "fils": args.fils, "passes": args.passes, "tours_demandes": args.processus,
            "trames": {t: [] for t in TRAMES}}
    for t in range(args.processus):
        for trame in TRAMES[t % 3:] + TRAMES[:t % 3]:
            tour = {}
            for bras in (BRAS if t % 2 == 0 else tuple(reversed(BRAS))):
                attendu = {"k": 5, "fils": args.fils, "passes": args.passes, "bras": bras, "empreinte": False,
                           "trames": [trame] * args.passes}
                tour[bras] = prise(args, bras, trame_args(args, trame), attendu,
                                   os.path.join(dossier, "%s_%s_t%02d.jsonl" % (trame, bras, t)))
            camp["trames"][trame].append(tour)
    camp["fin"] = maintenant()
    camp["binaires_apres"] = {n: sha256(b) for n, b in args.binaires.items()}
    rapport["campagne_k5"] = camp


def etape_session(args, rapport):
    dossier = os.path.join(args.sortie, "journaux", "v12set")
    os.makedirs(dossier, exist_ok=True)
    v_dossier, noms = v12set(args)
    ses = {"debut": maintenant(), "processus": args.processus_session, "prises": {b: [] for b in BRAS}}
    for rep in range(args.processus_session):
        ordre = noms[rep % len(noms):] + noms[:rep % len(noms)]
        for bras in (BRAS if rep % 2 == 0 else tuple(reversed(BRAS))):
            attendu = {"k": 5, "fils": args.fils, "passes": 2 * len(ordre), "bras": bras, "empreinte": False,
                       "trames": [n[-23:] for n in ordre] * 2}
            p = prise(args, bras, session_args(v_dossier, ordre), attendu,
                      os.path.join(dossier, "%s_r%d.jsonl" % (bras, rep)))
            p["ordre"] = [n[-23:] for n in ordre]
            ses["prises"][bras].append(p)
    ses["fin"] = maintenant()
    rapport["session_v12set"] = ses


# ---- Juge ---------------------------------------------------------------------------------------------------------------
def murs_chauds(take):
    return take["murs_ns"][1:]


def reverifier(rapport, sortie):
    """Le rejeu brut fait autorite : chaque journal re-hache, re-admis, et son resume recalcule doit etre identique."""
    racine, vus = os.path.realpath(sortie), set()
    prises = []
    for cle, groupe in rapport.get("identite", {}).get("prises", {}).items():
        prises += [groupe[n] for n in sorted(groupe)]
    for tours in rapport.get("campagne_k5", {}).get("trames", {}).values():
        prises += [t[b] for t in tours for b in BRAS if b in t]
    for liste in rapport.get("session_v12set", {}).get("prises", {}).values():
        prises += liste
    for take in prises:
        nom = take["journal"]
        exiger(type(nom) is str and nom and not os.path.isabs(nom), "chemin de journal invalide")
        chemin = os.path.realpath(os.path.join(racine, nom))
        exiger(os.path.commonpath((racine, chemin)) == racine and chemin not in vus, "journal externe ou reutilise")
        vus.add(chemin)
        exiger(sha256(chemin) == hexadecimal(take["journal_sha256"]), "journal modifie : " + nom)
        if not take.get("valide"):
            continue
        relu = lire_prise(chemin, take["code"], take["attendu"])
        for cle in RESUME:
            exiger(json.dumps(relu[cle], sort_keys=True) == json.dumps(take[cle], sort_keys=True),
                   "resume different du brut : %s (%s)" % (cle, nom))


def juger_identite(rapport, refus):
    """Empreintes des processus d'identite : par cle (trame et K), identiques d'une prise et d'une passe a l'autre
    (les deux bras, et a ng00 K5 les prises a un fil) ; pour v12set, trame par trame."""
    ident, resultats, identique = rapport.get("identite", {}).get("prises", {}), {}, True
    if not ident:
        refus.append("identite absente")
        return False, resultats
    for cle, groupe in sorted(ident.items()):
        if any(b not in groupe for b in BRAS) or not all(t.get("valide") for t in groupe.values()):
            refus.append("identite %s : prise absente ou invalide" % cle)
            continue
        if cle.startswith("v12set"):
            noms = groupe["avant"]["attendu"]["trames"]
            ecarts = [n for i, n in enumerate(noms)
                      if groupe["avant"]["empreintes"][i] != groupe["apres"]["empreintes"][i]]
            resultats[cle] = {"trames": len(noms), "ecarts": ecarts}
            identique = identique and not ecarts
        else:
            vues = {e for t in groupe.values() for e in t["empreintes"]}
            resultats[cle] = {"prises": sorted(groupe), "empreintes": sorted(e[:16] for e in vues)}
            identique = identique and len(vues) == 1
    return identique, resultats


def verifier_campagne(rapport):
    """Admission de la campagne decisive depuis sa position, avant de relire les resumes declares."""
    env = rapport.get("environnement", {})
    for moment in ("avant", "apres"):
        observation = env.get(moment, {})
        exiger(type(observation.get("gpu")) is str and observation["gpu"].strip() and
               type(observation.get("gpu_apps")) is str and observation["gpu_apps"] == "",
               "observation GPU absente, inconnue ou occupee")
    camp = rapport.get("campagne_k5", {})
    fils = naturel(camp.get("fils"), minimum=1)
    passes = naturel(camp.get("passes"), minimum=REGLE_T2D_A["passes_min"])
    tours = naturel(camp.get("tours_demandes"), minimum=REGLE_T2D_A["processus_min"])
    trames = camp.get("trames")
    exiger(type(trames) is dict and set(trames) == set(TRAMES), "cohorte de campagne differente")
    for trame in TRAMES:
        exiger(type(trames[trame]) is list and len(trames[trame]) == tours, "nombre de tours different")
        for tour in trames[trame]:
            exiger(type(tour) is dict and set(tour) == set(BRAS), "bras de campagne differents")
            for bras in BRAS:
                take = tour[bras]
                attendu = {"k": 5, "fils": fils, "passes": passes, "bras": bras, "empreinte": False,
                           "trames": [trame] * passes, "appareil": True,
                           "schema": "recouvert" if bras == "apres" else "sequentiel"}
                exiger(type(take) is dict and take.get("bras") == bras and
                       json.dumps(take.get("attendu"), sort_keys=True) == json.dumps(attendu, sort_keys=True),
                       "commande attendue incompatible avec sa place dans la campagne")


def _juger(rapport, sortie, verifier):
    refus, cas, verdict = [], {}, None
    construction = rapport.get("construction", {}).get("binaires", {})
    camp = rapport.get("campagne_k5")
    if set(construction) != set(BRAS):
        refus.append("construction absente ou incomplete")
    if not camp:
        refus.append("campagne K5 absente")
        return {"verdict": "refuse", "refus": refus, "regle": REGLE_T2D_A, "cas": cas}
    if any(camp.get("binaires_apres", {}).get(n) != construction.get(n, {}).get("sha256") for n in construction):
        refus.append("binaire change pendant les mesures")
    if camp.get("passes", 0) < REGLE_T2D_A["passes_min"]:
        refus.append("moins de %d passes par processus" % REGLE_T2D_A["passes_min"])
    env = rapport.get("environnement", {})
    if env.get("avant", {}).get("gpu_apps") or env.get("apres", {}).get("gpu_apps"):
        refus.append("GPU non isole avant ou apres")
    if verifier:
        try:
            verifier_campagne(rapport)
            reverifier(rapport, sortie)
        except (ValueError, TypeError, KeyError, OSError, UnicodeError) as error:
            refus.append("admission des journaux : " + str(error))
    identique, cas["identite"] = juger_identite(rapport, refus)
    rng = random.Random(REGLE_T2D_A["graine"])
    hautes = []
    for trame in REGLE_T2D_A["trames_decisives"]:
        tours = camp.get("trames", {}).get(trame, [])
        valides = [t for t in tours if all(b in t and t[b].get("valide") for b in BRAS)]
        exigees = max(REGLE_T2D_A["processus_min"], camp.get("tours_demandes", 0))
        if len(valides) != len(tours) or len(valides) < exigees:
            refus.append("%s : %d tours valides sur %d exiges" % (trame, len(valides), exigees))
        if len(valides) < 2:
            continue
        logs = [math.log(mediane(murs_chauds(t["apres"])) / mediane(murs_chauds(t["avant"]))) for t in valides]
        gm, bas, haut = bootstrap_gm(logs, rng, REGLE_T2D_A["bootstrap"])
        hautes.append(haut)
        cas[trame] = {"tours": len(valides), "moyenne_geometrique": gm, "ic95": [bas, haut],
                      "rapports": [math.exp(x) for x in logs],
                      "mur_ms": {b: mediane([mediane(murs_chauds(t[b])) for t in valides]) / 1e6 for b in BRAS}}
    if refus:
        verdict = "refuse"
    elif identique and len(hautes) == len(TRAMES) and all(h < REGLE_T2D_A["seuil_borne_haute"] for h in hautes):
        verdict = "adopte"
    else:
        verdict = "rejete"
    return {"verdict": verdict, "refus": refus, "regle": REGLE_T2D_A, "cas": cas, "identite_ok": identique}


def juger(rapport, sortie, verifier=True):
    """Copie JSON du rapport, jamais modifiee par le juge ; la voie sans rejeu est reservee a l'auto-test."""
    return _juger(json.loads(json.dumps(rapport, allow_nan=False)), sortie, verifier)


# ---- Auto-test du juge --------------------------------------------------------------------------------------------------
def campagne_synthetique(effet, empreinte_differente=False, manque=False, bruit=0.004, graine=1):
    rng = random.Random(graine)
    rapport = {"construction": {"binaires": {b: {"sha256": "x"} for b in BRAS}},
               "campagne_k5": {"passes": 10, "tours_demandes": 5, "binaires_apres": {b: "x" for b in BRAS},
                               "trames": {}},
               "identite": {"prises": {}}}
    for cle in ("ng00_k5", "ng01_k5", "ng02_k5", "u8000_k5"):
        rapport["identite"]["prises"][cle] = {b: {"valide": True, "empreintes": ["ab" * 32] * 2} for b in BRAS}
    if empreinte_differente:
        rapport["identite"]["prises"]["ng01_k5"]["apres"]["empreintes"][1] = "cd" * 32
    for trame in TRAMES:
        tours = []
        for t in range(5):
            base = 150e6 * (1 + 0.02 * rng.random())
            tour = {}
            for b in BRAS:
                facteur = effet.get(trame, 1.0) if b == "apres" else 1.0
                murs = [int(base * facteur * (1 + bruit * (rng.random() - 0.5))) for _ in range(10)]
                tour[b] = {"valide": not (manque and b == "apres" and t == 2), "murs_ns": murs}
            tours.append(tour)
        rapport["campagne_k5"]["trames"][trame] = tours
    return rapport


def etape_auto_test():
    cas = [({t: 0.65 for t in TRAMES}, False, False, 0.004, "adopte"),
           ({"ng00": 0.65, "ng01": 1.03, "ng02": 0.65}, False, False, 0.004, "rejete"),
           ({t: 0.65 for t in TRAMES}, True, False, 0.004, "rejete"),
           ({t: 0.65 for t in TRAMES}, False, True, 0.004, "refuse"),
           ({t: 0.995 for t in TRAMES}, False, False, 0.2, "rejete")]
    ecarts = []
    for i, (effet, diff, manque, bruit, attendu) in enumerate(cas):
        obtenu = juger(campagne_synthetique(effet, diff, manque, bruit, graine=i + 1), "", verifier=False)["verdict"]
        if obtenu != attendu:
            ecarts.append("cas %d : %s au lieu de %s" % (i, obtenu, attendu))
    for e in ecarts:
        print("pilote_t2d_a_auto_test_ecart " + e)
    if ecarts:
        return 1
    print("pilote_t2d_a_auto_test_ok cas=%d verdicts=adopte,rejete,refuse" % len(cas))
    return 0


# ---- Rapport ------------------------------------------------------------------------------------------------------------
def medianes_etapes(takes, chaud=True):
    """Mediane, sur les passes (chaudes), de chaque etape publiee par le schema des prises (ms)."""
    passes = [p for t in takes for p in (t["etapes"][1:] if chaud else t["etapes"])]
    return {e: mediane([p[e] for p in passes]) / 1e6 for e in passes[0]} if passes else {}


def medianes_recouvrement(takes, chaud=True):
    passes = [r for t in takes for r in (t["recouvrements"][1:] if chaud else t["recouvrements"])]
    if not passes:
        return {}
    out = {c: mediane([r[c] for r in passes]) / (1e6 if c.endswith("_ns") else 1) for c in RECOUVREMENT}
    out["fenetres_ms"] = {c: mediane([r["fenetres_ns"][c] for r in passes]) / 1e6 for c in FENETRES}
    k = len(passes[0]["fins_par_ordre_ns"])
    out["fins_par_ordre_ms"] = [[mediane([r["fins_par_ordre_ns"][i][j] for r in passes]) / 1e6 for j in range(5)]
                                for i in range(k)]
    return out


def ressources(takes):
    """CPU par passe chaude (mediane, ms ; None si getrusage a manque) et pic du budget (maximum, Mio)."""
    cpu = [c for t in takes for c in t["cpu_ns"][1:] if c is not None]
    pics = [p for t in takes for p in t["pics_octets"][1:]]
    return {"cpu_ms": mediane(cpu) / 1e6 if cpu else None, "pic_mio": max(pics) / 2 ** 20 if pics else None}


def statistiques_session(rapport):
    """Second tour de chaque processus : par trame, mediane et maximum sur les processus ; puis sur les trames."""
    ses = rapport.get("session_v12set")
    if not ses:
        return {}
    out = {}
    for bras in BRAS:
        par_trame = {}
        for take in ses["prises"][bras]:
            if not take.get("valide"):
                continue
            n = len(take["ordre"])
            for i, nom in enumerate(take["ordre"]):
                par_trame.setdefault(nom, []).append((take["murs_ns"][n + i], take["etapes"][n + i],
                                                      take["recouvrements"][n + i] if bras == "apres" else None,
                                                      take["sites"][n + i]))
        trames = {}
        for nom, vals in par_trame.items():
            trames[nom] = {"sites": vals[0][3], "mediane_ms": mediane([v[0] for v in vals]) / 1e6,
                           "max_ms": max(v[0] for v in vals) / 1e6, "processus": len(vals),
                           "etapes_ms": {e: mediane([v[1][e] for v in vals]) / 1e6 for e in vals[0][1]}}
            if bras == "apres":
                trames[nom]["queue_ms"] = mediane([v[2]["queue_ns"] for v in vals]) / 1e6
        if trames:
            out[bras] = {"trames": trames, "mediane_ms": mediane([t["mediane_ms"] for t in trames.values()]),
                         "maximum_ms": max(t["max_ms"] for t in trames.values())}
    return out


def tableaux(rapport, jugement):
    out = ["# T2-d-A : Session recouverte (D-F2), avant / apres (genere par pilote_t2d_a.py)", "",
           "Verdict (REGLE_T2D_A) : **%s** ; identite FUL1 : %s." % (jugement["verdict"],
                                                                       "oui" if jugement.get("identite_ok") else "non"), ""]
    if jugement["refus"]:
        out += ["Refus : " + " ; ".join(jugement["refus"]), ""]
    out += ["| trame | mur avant (ms) | mur apres (ms) | rapport (IC 95 %) |", "| --- | ---: | ---: | --- |"]
    for trame in TRAMES:
        c = jugement["cas"].get(trame)
        if c:
            out.append("| %s | %.1f | %.1f | %.3f (%.3f-%.3f) |" % (trame, c["mur_ms"]["avant"], c["mur_ms"]["apres"],
                                                                   c["moyenne_geometrique"], c["ic95"][0], c["ic95"][1]))
    camp = rapport.get("campagne_k5", {}).get("trames", {})
    out += ["", "Partition murale (mediane des passes chaudes, ms ; apres : G jusqu'a la fin du dernier calcul de G, "
                "TMVR = queue, foret non recouverte) :", "",
            "| trame | bras | " + " | ".join(MUR) + " |", "|" + " --- |" * (2 + len(MUR))]
    tmvr_lignes, rec_lignes = [], []
    for trame in TRAMES:
        par_bras = {}
        for bras in BRAS:
            takes = [t[bras] for t in camp.get(trame, []) if t.get(bras, {}).get("valide")]
            par_bras[bras] = (medianes_etapes(takes), medianes_recouvrement(takes) if bras == "apres" else {})
            if par_bras[bras][0]:
                out.append("| %s | %s | %s |" % (trame, bras, " | ".join("%.1f" % par_bras[bras][0][e] for e in MUR)))
        m, r = par_bras["avant"][0], par_bras["apres"][1]
        if m and r:
            f = r["fenetres_ms"]
            tmvr_lignes.append("| %s | %s | %s | %.1f / %.1f / %.1f |" % (
                trame, " / ".join("%.1f" % m[e] for e in "TMVR"), " / ".join("%.1f" % f[e] for e in "TMVR"),
                f["G"], f["foret"], f["foret_apres_g"]))
            rec_lignes.append("| %s | %.1f | %.1f | %.1f | %d | %d | %s |" % (
                trame, r["ouverture_ns"], r["fin_g_ns"], r["queue_ns"], r["noyau_reprises"], r["noyau_arrets"],
                " ; ".join("/".join("%.1f" % x for x in e) for e in r["fins_par_ordre_ms"])))
    out += ["", "T, M, V, R (ms) : avant, murs des etages sequentiels ; apres, SOMMES DE FENETRES MURALES des taches "
                "(temps-fils, ni murs ni temps CPU) :", "",
            "| trame | avant : murs T / M / V / R | apres : fenetres T / M / V / R | apres : fenetres G / foret / "
            "foret apres G |", "| --- | --- | --- | --- |"] + tmvr_lignes
    res_lignes = []
    for trame in TRAMES:
        for bras in BRAS:
            r = ressources([t[bras] for t in camp.get(trame, []) if t.get(bras, {}).get("valide")])
            if r["pic_mio"] is not None:
                res_lignes.append("| %s | %s | %s | %.1f |" % (
                    trame, bras, "-" if r["cpu_ms"] is None else "%.1f" % r["cpu_ms"], r["pic_mio"]))
    out += ["", "Ressources (publiees, non jugees) :", "",
            "| trame | bras | CPU par passe (ms, mediane) | pic du budget (Mio, max) |", "| --- | --- | ---: | ---: |"]
    out += res_lignes
    out += ["", "Recouvrement (apres, ms ; instants depuis le debut de build_tower) :", "",
            "| trame | ouverture | fin du dernier calcul de G | queue | reprises du noyau | arrets du noyau |"
            " fins par ordre G/noyau/M/V/R |", "| --- |" + " ---: |" * 5 + " --- |"] + rec_lignes
    ses = statistiques_session(rapport)
    if ses:
        out += ["", "Session sur les 37 trames v12set (second tour, publie, non juge) :", ""]
        for bras in BRAS:
            if bras in ses:
                out.append("- %s : mediane %.1f ms, maximum %.1f ms sur %d trames." % (
                    bras, ses[bras]["mediane_ms"], ses[bras]["maximum_ms"], len(ses[bras]["trames"])))
    return "\n".join(out) + "\n"


def etape_rapport(args, rapport):
    jugement = juger(rapport, args.sortie)
    rapport["jugement"] = jugement
    rapport["statistiques"] = {"session_v12set": statistiques_session(rapport)}
    camp = rapport.get("campagne_k5", {}).get("trames", {})
    rapport["statistiques"]["k5"] = {
        trame: {b: {"etapes_ms": medianes_etapes([t[b] for t in camp.get(trame, []) if t.get(b, {}).get("valide")]),
                    "ressources": ressources([t[b] for t in camp.get(trame, []) if t.get(b, {}).get("valide")]),
                    "recouvrement": medianes_recouvrement([t[b] for t in camp.get(trame, [])
                                                           if t.get(b, {}).get("valide")]) if b == "apres" else {}}
                for b in BRAS} for trame in TRAMES}
    with open(os.path.join(args.sortie, "tableaux_t2d_a.md"), "w", encoding="utf-8") as f:
        f.write(tableaux(rapport, jugement))
    return jugement


def main(argv):
    p = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    p.add_argument("etapes", nargs="+", choices=("auto-test", "construire", "identite", "campagne", "session",
                                                 "rapport", "tout"))
    for nom in ("--src", "--travail", "--donnees", "--sortie", "--avant-archive", "--avant-sha256",
                "--archive-v12set"):
        p.add_argument(nom)
    for nom, defaut in (("--fils", 48), ("--jobs", 44), ("--processus", 5), ("--passes", 10),
                        ("--processus-session", 3), ("--delai", 900)):
        p.add_argument(nom, type=int, default=defaut)
    p.add_argument("--essai", action="store_true")
    args = p.parse_args(argv[1:])
    etapes = ["auto-test", "construire", "identite", "campagne", "session", "rapport"] if "tout" in args.etapes \
        else args.etapes
    if etapes == ["auto-test"]:
        return etape_auto_test()
    if not args.sortie or not args.travail or (not args.essai and (args.processus < REGLE_T2D_A["processus_min"] or
                                                                   args.passes < REGLE_T2D_A["passes_min"])):
        print("pilote_t2d_a_refus usage : --sortie, --travail ; au moins %d processus et %d passes" % (
            REGLE_T2D_A["processus_min"], REGLE_T2D_A["passes_min"]))
        return 2
    os.makedirs(args.sortie, exist_ok=True)
    args.sortie = os.path.abspath(args.sortie)
    chemin = os.path.join(args.sortie, "rapport_t2d_a.json")
    rapport = {"schema": "ehgp.v12.t2d_a_pilote.v1", "regle": REGLE_T2D_A}
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
                rapport.setdefault("environnement", {})["avant"] = environnement()
                etape_construire(args, rapport)
            elif etape == "rapport":
                rapport.setdefault("environnement", {})["apres"] = environnement()
                if etape_auto_test() != 0:
                    return 1
                jugement = etape_rapport(args, rapport)
                if args.essai:
                    jugement["verdict"] = "essai"
                print("pilote_t2d_a_verdict %s refus=%d" % (jugement["verdict"], len(jugement["refus"])))
                code = 3 if jugement["refus"] and not args.essai else 0
            else:
                args.binaires = {n: b["chemin"] for n, b in rapport["construction"]["binaires"].items()}
                if etape == "identite" or etape == "session":
                    if not args.archive_v12set or not args.donnees:
                        raise RuntimeError("%s exige --donnees et --archive-v12set" % etape)
                (etape_identite if etape == "identite" else etape_campagne if etape == "campagne"
                 else etape_session)(args, rapport)
    except (RuntimeError, KeyError, OSError, ValueError, tarfile.TarError) as e:
        print("pilote_t2d_a_refus %s" % e)
        code = 3
    with open(chemin, "w", encoding="utf-8") as f:
        json.dump(rapport, f, indent=1, sort_keys=True)
    return code


if __name__ == "__main__":
    sys.exit(main(sys.argv))
