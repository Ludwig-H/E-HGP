#!/usr/bin/env python3
"""Pilote et juge du chantier T2-d-B (cout interne de l'etage G : census, LEM-T1, proposition), voie CPU : un bras
par levier, en processus alternes. Bibliotheque standard seule, jouable sous python3 -S -O (aucune garde par assert).

Bras (sondes mhgp12_tower_probe construites ici, memes options : Release, MHGP12_MODULES=tower) :
  avant        archive epinglee des sources de main 902041f66 (sha256 verifie avant deballage)
  avant_bis    le MEME binaire que avant, joue comme un bras distinct (A/A : bruit de la session, publie)
  garde        avant + L1 : garde de census resserree (pave (m-M, m+2M), preuve de l'auditeur Codex, NUM-GARDE)
  report       avant + L2 : compteurs de la garde reportes une fois par parcours (plus de registre par appel)
  temoins      avant + L3 : census a temoins sur la sphere (support certifie transmis par valeur jusqu'a la requete)
  census       avant + L1 + L2 + L3 (bras combine du census)
  proposition  avant + L4 : proposition par la voie entiere exacte (paire diametrale, triangle aigu), puis DWelzl
               amorce sur la paire la plus eloignee exacte
  apres        avant + L1 + L2 + L3 + L4 (le lot T2-d-B)
Chaque bras autre que avant est une copie de l'arbre de avant ou s'appliquent, dans l'ordre, les substitutions
exactes de bras_t2d_b.json (dossier de ce pilote ; fichiers de src/ seulement) : SHA-256 de chaque fichier controle
avant et apres, chaque motif present une seule fois. Rien d'autre ne differe entre les bras. Le paquet {src} n'est
pas un bras : la conformite de ses fichiers au lot est publiee (information), jamais supposee.

Etapes (dans l'ordre si plusieurs) :
  auto-test     le juge sur des campagnes synthetiques (adopte, rejete, refuse) ; tout autre resultat : code 1
  construire    deballage, bras par substitution, configuration, construction ; empreintes des binaires
  campagne      K5, ng00 ng01 ng02, --fils fils (48 sur G4) : --processus tours ; dans chaque tour, un processus neuf
                par bras, ordre decale d'un bras par tour, sans inversion (avec huit bras, l'inversion un tour sur deux
                figerait la parite de la position de chaque bras : preaudit de l'auditeur) ; chaque processus joue
                --passes passes de l'etage G (la premiere, a froid, est ecartee) et l'empreinte de l'objet
  informations  non jugees, construites apres le verdict : profil par composante (avant, apres) et, si nvcc est la,
                sonde FULL sur l'appareil (avant, apres ; MHGP12_ENABLE_CUDA=ON, profil 21 ; --nvcc aucun : non
                jouee) ; puis K10 a --fils fils (avant, apres) ; uniformes de 8 000, 16 000 et 32 000 sites a K5
                (avant, apres ; empreintes des portes d'echelle) ; profil par composante a un fil sur ng00 K5 ; mur
                FULL sur l'appareil (bench/full_probe.cpp --device --digest, ng00-02 K5, avant et apres alternes) avec
                l'identite des empreintes FUL1 entre bras, passes et processus, chaque journal lu par le lecteur
                strict partage microbancs/outils/lecteur_full.py (schema sequentiel de 902041f66) ; compteurs du
                TRAVAIL compares entre bras (les leviers ne changent ni les noeuds visites ni les sites testes ni les
                routes)
  rapport       auto-test du juge, puis juge (REGLE_T2D_B), rapport JSON et tableaux Markdown
  tout          auto-test construire campagne rapport informations rapport (le verdict avant les informations ;
                rapport_t2d_b.json ecrit apres chaque etape)

REGLE_T2D_B (ecrite le 8 octobre 2026 a 04:48 UTC, avant toute mesure de ces bras sur G4) : par processus, temps de
l'etage G = mediane des passes 2 a P du mur de resolve_tower (wall_ns de la ligne tour_g) ; par tour, rapport bras /
avant ; moyenne geometrique des rapports des tours et IC 95 % par bootstrap sur les tours (10 000 tirages, graine
20261008), par trame. Un levier est ADOPTE si l'empreinte de l'objet est identique pour tous les processus de tous les
bras de chaque trame (et egale, sur ng00, a celle de la porte mhgp12_tower_determinism_lidar_ng00_k5) ET si la borne
haute de l'IC de son rapport est sous 1 sur CHACUNE des trames ng00, ng01, ng02 a K5 ; REJETE sinon ; REFUSE si une
prise manque (processus en echec, passes ou empreinte absentes, moins de --processus tours valides par trame), si un
bras ne se reconstruit pas a ses empreintes, si un binaire change pendant la campagne, si un journal manque ou a
change, si l'auto-test du juge echoue, ou (amendement du 8 octobre a 06:04 UTC, avant toute mesure G4, sur le
preaudit de l'auditeur t2d_b_admission) si la moyenne geometrique du controle A/A sort de la fenetre de +/- 1,5 % sur
l'une des trames decisives : l'appariement est alors invalide. Le meme amendement exige a la lecture la partition du
mur de G (sommes, enveloppes, reste) et fixe l'ordre des bras sans inversion. Leviers juges, chacun contre avant :
lot_t2d_b (apres), garde_seule (garde), report_seul (report), temoins_seuls (temoins), census_combine (census),
proposition (proposition). Rapports
publies sans verdict : temoins_apres_garde (garde -> census), proposition_apres_census (census -> apres). Le bras A/A
(avant -> avant_bis) est publie avec son IC ; hors de la fenetre de +/- 1,5 % de MESURE.md (regle 6), il impose le
refus (veto).
Les evaluations de bornes evitees par les temoins (microbanc local) sont un diagnostic : seul ce juge dit un gain de G.

Exemple (G4, commande d'un plan de session ; en local : --fils 3 --essai) :
  python3 pilote_t2d_b.py tout --src <racine du paquet> --travail <dossier> --donnees <donnees> --sortie <sortie> \\
      --avant-archive <donnees>/v12_src_902041f66.tar.gz --avant-sha256 <hex> --fils 48 --jobs 44

Codes : 0 campagne complete et jugee (le verdict est dans le rapport, il ne change pas le code) ; 1 auto-test en echec ;
2 usage ; 3 refus (preuve absente, bras non reconstruit, binaire change, construction en echec, prise manquante).
"""
import argparse
import datetime
import hashlib
import json
import math
import os
import random
import shutil
import subprocess
import sys
import tarfile
import tempfile

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "outils"))
import lecteur_full as lf  # noqa: E402  lecteur strict partage (MES-FULL, MES-B) pour les informations FULL

ICI = os.path.dirname(os.path.abspath(__file__))
SONDE = "mhgp12_tower_probe"
SONDE_FULL = "mhgp12_full_probe"
TRAMES = ("ng00", "ng01", "ng02")
EMPREINTE_NG00_K5 = "e5a81154fb1b15f1"  # ligne gravee de mhgp12_tower_determinism_lidar_ng00_k5
UNIFORMES = {8000: "a40f1b2ef8547269", 16000: "cf7c7745fcb4ae6e", 32000: "d1f08fd0dbdf48eb"}  # mhgp12_tower_scale*
BRAS_FICHIER = os.path.join(ICI, "bras_t2d_b.json")
BRAS_SUBSTITUES = ("garde", "report", "temoins", "census", "proposition", "apres")
BRAS_JUGES = ("avant", "avant_bis") + BRAS_SUBSTITUES
LEVIERS = (("lot_t2d_b", "avant", "apres"), ("garde_seule", "avant", "garde"), ("report_seul", "avant", "report"),
           ("temoins_seuls", "avant", "temoins"), ("census_combine", "avant", "census"),
           ("proposition", "avant", "proposition"))
INFORMATIFS = (("temoins_apres_garde", "garde", "census"), ("proposition_apres_census", "census", "apres"))
CONTROLE_AA = ("A/A", "avant", "avant_bis")
REGLE_T2D_B = {"trames_decisives": TRAMES, "k": 5, "processus_min": 10, "passes_min": 6, "bootstrap": 10000,
               "graine": 20261008, "seuil_borne_haute": 1.0, "fenetre_aa": 0.015,
               "statistique": "par processus : mediane des passes 2..P du mur de l'etage G (wall_ns) ; par tour : "
                              "rapport bras / avant ; moyenne geometrique et IC 95 % par bootstrap sur les tours",
               "adoption": "empreinte de l'objet identique dans tous les processus de tous les bras de chaque trame "
                           "(ng00 : celle de la porte) et borne haute de l'IC sous 1 sur chacune des trames ng00, "
                           "ng01, ng02 a K5",
               "leviers": [list(x) for x in LEVIERS], "informatifs": [list(x) for x in INFORMATIFS],
               "controle": list(CONTROLE_AA), "base": "902041f66",
               "veto_aa": "refus si la moyenne geometrique A/A sort de 1 +/- fenetre_aa sur une trame decisive",
               "amendement": "8 octobre 2026 a 06:04 UTC, avant toute mesure G4 : veto A/A, partition du mur de G "
                             "exigee a la lecture, ordre des bras sans inversion (preaudit t2d_b_admission)"}


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


def jouer(commande, journal, cwd=None):
    """Execute une commande ; sortie brute dans journal ; rend le code."""
    try:
        proc = subprocess.run(commande, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
                              encoding="utf-8", errors="surrogateescape", check=False)
        code, sortie, erreur = proc.returncode, proc.stdout, proc.stderr
    except OSError as e:
        code, sortie, erreur = 127, "", str(e)
    with open(journal, "w", encoding="utf-8", errors="surrogateescape") as f:
        f.write(sortie)
        if erreur:
            f.write("\n# stderr\n" + erreur[-4000:])
    return code


def exiger(condition, message):
    if not condition:
        raise ValueError(message)


# ---- Construction ----------------------------------------------------------------------------------------------------
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


def lire_bras():
    with open(BRAS_FICHIER, encoding="utf-8") as f:
        bras = json.load(f)
    if bras.get("schema") != "ehgp.v12.t2d_b_bras.v1" or set(bras.get("bras", {})) != set(BRAS_SUBSTITUES):
        raise RuntimeError("bras_t2d_b.json illisible ou incomplet")
    return bras


def substituer(racine, fichiers):
    """Substitutions d'un bras sur sa copie ; empreintes avant et apres controlees, motifs presents une seule fois."""
    for relatif, entree in sorted(fichiers.items()):
        if os.path.isabs(relatif) or ".." in relatif.split("/") or not relatif.startswith("src/"):
            raise RuntimeError("fichier de bras refuse : %r" % relatif)
        chemin = os.path.join(racine, relatif)
        with open(chemin, "rb") as f:
            octets = f.read()
        if hashlib.sha256(octets).hexdigest() != entree["sha256_avant"]:
            raise RuntimeError("%s : fichier de la base d'une autre empreinte" % relatif)
        texte = octets.decode("utf-8")
        for s in entree["substitutions"]:
            if texte.count(s["cherche"]) != 1:
                raise RuntimeError("%s : motif absent ou multiple" % relatif)
            texte = texte.replace(s["cherche"], s["remplace"])
        octets = texte.encode("utf-8")
        if hashlib.sha256(octets).hexdigest() != entree["sha256_apres"]:
            raise RuntimeError("%s : bras reconstruit d'une autre empreinte" % relatif)
        with open(chemin, "wb") as f:
            f.write(octets)


def construire(nom, source, travail, jobs, journaux, cible=SONDE, drapeaux=(), modules=True):
    build = os.path.join(travail, nom, "build")
    os.makedirs(build, exist_ok=True)
    configure = ["cmake", "-S", source, "-B", build, "-DCMAKE_BUILD_TYPE=Release"]
    configure += ["-DMHGP12_MODULES=tower"] if modules else []
    configure += list(drapeaux)
    code = jouer(configure, os.path.join(journaux, "configure_%s.txt" % nom))
    if code == 0:
        code = jouer(["cmake", "--build", build, "-j", str(jobs), "--target", cible],
                     os.path.join(journaux, "build_%s.txt" % nom))
    binaire = os.path.join(build, cible)
    if code != 0 or not os.path.isfile(binaire):
        return None
    return binaire


def conformite_src(args, bras):
    """Information : les fichiers du lot dans {src} sont-ils ceux du bras apres ?"""
    ecarts = []
    for relatif, entree in sorted(bras["bras"]["apres"]["fichiers"].items()):
        chemin = os.path.join(args.src, "morsehgp3D_v12", relatif)
        if not os.path.isfile(chemin) or sha256(chemin) != entree["sha256_apres"]:
            ecarts.append(relatif)
    return {"fichiers": len(bras["bras"]["apres"]["fichiers"]), "ecarts": ecarts}


def etape_construire(args, rapport):
    travail = os.path.abspath(args.travail)
    journaux = os.path.join(args.sortie, "construction")
    os.makedirs(journaux, exist_ok=True)
    bras = lire_bras()
    sources = {}
    base = os.path.join(travail, "avant", "src")
    if os.path.isdir(base):
        shutil.rmtree(base)
    os.makedirs(base)
    sources["avant"] = deballer(args.avant_archive, args.avant_sha256, base)
    for nom in BRAS_SUBSTITUES:
        copie = os.path.join(travail, nom, "src", "morsehgp3D_v12")
        if os.path.isdir(copie):
            shutil.rmtree(copie)
        shutil.copytree(sources["avant"], copie)
        substituer(copie, bras["bras"][nom]["fichiers"])
        sources[nom] = copie
    binaires = {}
    for nom in ("avant",) + BRAS_SUBSTITUES:
        binaires[nom] = construire(nom, sources[nom], travail, args.jobs, journaux)
        if binaires[nom] is None:
            raise RuntimeError("construction du bras %s en echec (journal build_%s.txt)" % (nom, nom))
    binaires["avant_bis"] = binaires["avant"]
    rapport["construction"] = {
        "debut": maintenant(), "archive_avant_sha256": args.avant_sha256, "bras_sha256": sha256(BRAS_FICHIER),
        "binaires": {n: {"chemin": b, "sha256": sha256(b)} for n, b in binaires.items()},
        "sources": {n: sources[n] for n in ("avant", "apres")}, "src_conforme_au_lot": conformite_src(args, bras)}


def constructions_information(args, rapport):
    """Sondes d'information (jamais jugees), construites apres le verdict : profil par composante et sonde FULL."""
    travail = os.path.abspath(args.travail)
    journaux = os.path.join(args.sortie, "construction")
    sources = rapport["construction"]["sources"]
    informations = {}
    for nom in ("avant", "apres"):  # profil par composante
        informations["profil_" + nom] = construire("profil_" + nom, sources[nom], travail, args.jobs, journaux,
                                                   drapeaux=["-DCMAKE_CXX_FLAGS=-DMHGP12_TOWER_PROFILE"])
    if args.nvcc == "aucun":
        nvcc = None
    elif args.nvcc:
        nvcc = args.nvcc if os.path.isfile(args.nvcc) else None
    else:
        nvcc = shutil.which("nvcc") or ("/usr/local/cuda/bin/nvcc" if os.path.isfile("/usr/local/cuda/bin/nvcc") else
                                        None)
    for nom in ("avant", "apres"):  # mur FULL sur l'appareil
        informations["full_" + nom] = None if nvcc is None else construire(
            "full_" + nom, sources[nom], travail, args.jobs, journaux, cible=SONDE_FULL, modules=False,
            drapeaux=["-DMHGP12_COORD_BITS=21", "-DMHGP12_ENABLE_CUDA=ON", "-DCMAKE_CUDA_COMPILER=" + nvcc])
    return {"nvcc": nvcc, "binaires": {n: (None if b is None else {"chemin": b, "sha256": sha256(b)})
                                       for n, b in informations.items()}}


# ---- Prises ----------------------------------------------------------------------------------------------------------
def entree(args, trame):
    if trame.startswith("u"):
        return ["--uniform=%s,20261007,18" % trame[1:]]
    stem = os.path.join(args.donnees, "lidar_%s" % trame)
    return [stem + ".u32le", stem + ".ids.u32le"]


OBJET = frozenset("births cells inert_cells extended_cells representatives".split())
TRAVAIL = frozenset(("probes first_probe_hits probe_hits_after_steps route_t1 route_cert_table route_cert_census "
                     "route_fallback_table route_fallback_census fallback_no_proposal fallback_not_in_part "
                     "fallback_certificate census_saturated census_complete census_sites census_sites_max "
                     "census_nodes jumps_catalogue jumps_census inert_steps cell_stops birth_stops controls "
                     "max_chain").split())
DIAG = frozenset(("count_ns fill_ns tables_ns resolve_ns workspace_bytes table_bytes peak_bytes prepare_ns setup_ns "
                  "workspace_ns joins_ns orders_ns reste_ns").split())
SECTIONS_PROFIL = frozenset(("trace sonde proposition t1 certificat repli census_sature census_complet pas arret "
                             "total").split())


def naturel(value, maximum=(1 << 64) - 1, minimum=0):
    exiger(type(value) is int and minimum <= value <= maximum, "entier hors domaine")
    return value


def champs(value, keys):
    exiger(type(value) is dict and set(value) == set(keys), "champs absents ou inconnus")


def hexadecimal(value):
    exiger(type(value) is str and len(value) == 64 and all(c in "0123456789abcdef" for c in value),
           "empreinte SHA-256 invalide")
    return value


def objet_unique(pairs):
    out = {}
    for key, value in pairs:
        exiger(key not in out, "cle JSON repetee")
        out[key] = value
    return out


def constante_interdite(value):
    raise ValueError("constante JSON non finie : " + value)


def lignes_json(journal):
    with open(journal, encoding="utf-8", errors="strict") as f:
        lines = [json.loads(line, object_pairs_hook=objet_unique, parse_constant=constante_interdite)
                 for line in f.read().splitlines()]
    exiger(all(type(row) is dict for row in lines), "ligne JSON non objet")
    return lines


def diagnostics_valides(diag, k):
    arrays = {"order_ns": k, "table_ns": k - 1, "join_ns": k - 1, "pass_ns": k}
    champs(diag, DIAG | arrays.keys())
    for key in DIAG:
        naturel(diag[key])
    for key, size in arrays.items():
        exiger(type(diag[key]) is list and len(diag[key]) == size, "taille diagnostic invalide")
        for value in diag[key]:
            naturel(value)


def bornes_temporelles(row, k):
    """Partition du mur de G (tower.hpp, ResolutionDiagnostics ; tower_probe.cpp, stage_json) : totaux egaux aux sommes
    des tableaux par ordre (passes.cpp), index, jointure et passe de chaque ordre dans son enveloppe, ordre un egal a sa
    passe ; prepare + count + setup + fill + workspace + orders <= mur et reste = mur - cette somme (preaudit de
    l'auditeur, t2d_b_admission : un mur forge a 1 ns etait admis)."""
    d = row["diagnostics"]
    exiger(d["tables_ns"] == sum(d["table_ns"]) and d["joins_ns"] == sum(d["join_ns"]) and
           d["resolve_ns"] == sum(d["pass_ns"]) and d["orders_ns"] == sum(d["order_ns"]), "totaux des ordres faux")
    exiger(d["pass_ns"][0] == d["order_ns"][0], "ordre un : passe differente de son enveloppe")
    for i in range(1, k):
        exiger(d["table_ns"][i - 1] + d["join_ns"][i - 1] + d["pass_ns"][i] <= d["order_ns"][i],
               "ordre %d : index, jointure et passe hors de son enveloppe" % (i + 1))
    dedans = d["prepare_ns"] + d["count_ns"] + d["setup_ns"] + d["fill_ns"] + d["workspace_ns"] + d["orders_ns"]
    exiger(dedans <= row["wall_ns"] and d["reste_ns"] == row["wall_ns"] - dedans, "partition du mur de G fausse")


def profil_valide(row, passe, k):
    champs(row, ("phase", "pass", "k", "ghz_tsc", "biais_cycles", "table_ns", "join_ns", "sections", "reste"))
    exiger(row["phase"] == "profil_g" and naturel(row["pass"]) == passe and naturel(row["k"]) == k,
           "ordre du profil invalide")

    def reel(value):
        exiger(type(value) in (int, float) and math.isfinite(value) and value >= 0, "reel de profil invalide")
    for key in ("ghz_tsc", "biais_cycles"):
        reel(row[key])
    naturel(row["table_ns"])
    naturel(row["join_ns"])
    champs(row["sections"], SECTIONS_PROFIL)
    for section in row["sections"].values():
        champs(section, ("cycles", "n", "part", "ns"))
        naturel(section["cycles"])
        naturel(section["n"])
        reel(section["part"])
        reel(section["ns"])
    champs(row["reste"], ("cycles", "part"))
    naturel(row["reste"]["cycles"])
    reel(row["reste"]["part"])


def lire_prise(journal, code, k, fils, passes, profil=False):
    """Valide tout le journal de succes de la sonde de G et recalcule les champs qui portent le verdict."""
    exiger(type(code) is int and code == 0, "processus en echec")
    naturel(k, 12, 1)
    naturel(fils, 256, 1)
    naturel(passes, 1000, 2)
    lines = lignes_json(journal)
    cursor, murs, profiles, sites, last = 0, [], [], None, None

    def take():
        nonlocal cursor
        exiger(cursor < len(lines), "journal tronque")
        value = lines[cursor]
        cursor += 1
        return value
    for p in range(passes):
        row = take()
        champs(row, ("phase", "pass", "status", "reason", "order", "coord_bits", "kmax", "threads", "sites",
                     "wall_ns", "diagnostics"))
        exiger(row["phase"] == "tour_g" and naturel(row["pass"]) == p, "passes absentes, repetees ou desordonnees")
        exiger(row["status"] == "ok" and row["reason"] == "none" and naturel(row["order"]) == 0,
               "statut de passe invalide")
        exiger(naturel(row["coord_bits"]) == 21 and naturel(row["kmax"]) == k and naturel(row["threads"]) == fils,
               "configuration differente de la commande")
        current_sites = naturel(row["sites"], (1 << 32) - 1, 1)
        exiger(sites is None or sites == current_sites, "sites variables entre passes")
        sites = current_sites
        murs.append(naturel(row["wall_ns"], minimum=1))
        diagnostics_valides(row["diagnostics"], k)
        bornes_temporelles(row, k)
        last = row["diagnostics"]
        if profil:
            for order in range(2, k + 1):
                item = take()
                profil_valide(item, p, order)
                profiles.append(item)
    travail = []
    for order in range(1, min(k, sites) + 1):
        row = take()
        champs(row, ("phase", "k", "objet", "travail"))
        exiger(row["phase"] == "ordre" and naturel(row["k"]) == order, "ordres absents ou desordonnes")
        champs(row["objet"], OBJET)
        champs(row["travail"], TRAVAIL | {"chaines"})
        for key in OBJET:
            naturel(row["objet"][key])
        for key in TRAVAIL:
            naturel(row["travail"][key])
        histogram = row["travail"]["chaines"]
        exiger(type(histogram) is list and len(histogram) == 16, "histogramme incomplet")
        for value in histogram:
            naturel(value)
        travail.append(row["travail"])
    digest, end = take(), take()
    champs(digest, ("phase", "resolution_sha256"))
    exiger(digest["phase"] == "digest", "digest final absent")
    empreinte = hexadecimal(digest["resolution_sha256"])
    champs(end, ("phase", "status", "reason", "order"))
    exiger(end["phase"] == "exit" and end["status"] == "ok" and end["reason"] == "none" and
           naturel(end["order"]) == 0 and cursor == len(lines), "fin de journal invalide")
    return {"valide": True, "murs_ns": murs, "empreinte": empreinte, "profil": profiles,
            "g_ns": mediane(murs[1:]), "diagnostics": last, "sites": sites,
            "travail_sha256": hashlib.sha256(json.dumps(travail, sort_keys=True).encode()).hexdigest()}


def prise(args, binaire, trame, k, fils, passes, journal, profil=False):
    """Un processus neuf de la sonde de G ; admission du journal complet."""
    commande = [binaire] + entree(args, trame) + ["--k=%d" % k, "--threads=%d" % fils, "--passes=%d" % passes,
                                                  "--digest", "--frame=%s" % trame]
    code = jouer(commande, journal)
    out = {"journal": os.path.relpath(journal, args.sortie), "journal_sha256": sha256(journal), "code": code,
           "valide": False, "murs_ns": [], "empreinte": None, "profil": []}
    try:
        out.update(lire_prise(journal, code, k, fils, passes, profil))
    except (ValueError, TypeError, KeyError, OSError, UnicodeError, OverflowError) as error:
        out["admission"] = str(error)
    return out


def lire_full(journal, code, passes, trame, fils, sites, k=5):
    """Journal de la sonde FULL sur l'appareil (schema sequentiel de bench/full_probe.cpp, celui de 902041f66,
    --device --digest), lu par le lecteur strict partage microbancs/outils/lecteur_full.py (residu de l'auditeur,
    t2d_b_admission_reprise : blocs c_ns, g_ns et hors_mur_ns, types, usage au plus le pic, T + M + V + R dans TMVR,
    tables + resolution dans G) : open (budget de l'appareil partage), puis par passe une ligne full (voie "device",
    configuration de la commande, trame et sites attendus) et sa liberation, puis exit ; toute autre issue leve
    ValueError avec la raison du lecteur."""
    with open(journal, encoding="utf-8") as f:
        texte = f.read()
    attendu = dict(voie="appareil", k=k, fils=fils, passes=passes, empreinte=True, trames=[(trame, sites)],
                   budget_appareil="partage", bits=21, schema="sequentiel")
    etat = lf.parse_output(code, texte, attendu)
    exiger(etat["etat"] == "ok", "%s : %s" % (etat["etat"], etat["raison"]))
    murs = [p["wall_ns"] for p in etat["passes"]]
    g = [p["etapes_ns"]["G"] for p in etat["passes"]]
    return {"valide": True, "mur_ns": mediane(murs[1:]), "g_ns": mediane(g[1:]),
            "ful1": sorted({p["full_sha256"] for p in etat["passes"]})}


def prise_full(args, binaire, trame, passes, journal):
    stem = os.path.join(args.donnees, "lidar_%s" % trame)
    commande = [binaire, "--trame=%s.u32le,%s.ids.u32le,%s" % (stem, stem, trame), "--k=5", "--device", "--digest",
                "--threads=%d" % args.fils, "--passes=%d" % passes]
    code = jouer(commande, journal)
    out = {"journal": os.path.relpath(journal, args.sortie), "code": code, "valide": False}
    try:
        out.update(lire_full(journal, code, passes, trame, args.fils, os.path.getsize(stem + ".u32le") // 12))
    except (ValueError, TypeError, KeyError, OSError, UnicodeError, OverflowError) as error:
        out["admission"] = str(error)
    return out


# ---- Campagnes -------------------------------------------------------------------------------------------------------
def etape_campagne(args, rapport):
    binaires = {n: b["chemin"] for n, b in rapport["construction"]["binaires"].items()}
    camp = {"debut": maintenant(), "fils": args.fils, "passes": args.passes, "tours_demandes": args.processus,
            "trames": {}}
    for trame in TRAMES:
        dossier = os.path.join(args.sortie, "journaux", "k5", trame)
        os.makedirs(dossier, exist_ok=True)
        tours = []
        for t in range(args.processus):
            n = len(BRAS_JUGES)
            ligne = [(t + i) % n for i in range(n)]  # sans inversion : parite des positions variee (8 bras)
            tour = {}
            for i in ligne:
                bras = BRAS_JUGES[i]
                journal = os.path.join(dossier, "%s_t%02d.jsonl" % (bras, t))
                tour[bras] = prise(args, binaires[bras], trame, 5, args.fils, args.passes, journal)
            tours.append(tour)
        camp["trames"][trame] = tours
    camp["fin"] = maintenant()
    camp["binaires_apres"] = {n: sha256(b) for n, b in binaires.items()}
    rapport["campagne_k5"] = camp


def etape_informations(args, rapport):
    binaires = {n: b["chemin"] for n, b in rapport["construction"]["binaires"].items()}
    info = {"debut": maintenant(), "constructions": constructions_information(args, rapport)}
    autres = info["constructions"]["binaires"]
    plans = [("k10_w%d" % args.fils, TRAMES, 10, args.fils, 5, ("avant", "apres"), 2, False),
             ("uniformes_k5_w%d" % args.fils, ("u8000", "u16000", "u32000"), 5, args.fils, 5, ("avant", "apres"), 1,
              False)]
    if args.essai:  # essai local (jamais decisif) : informations reduites a une trame par plan
        plans = [("k10_w%d" % args.fils, ("ng00",), 10, args.fils, 4, ("avant", "apres"), 1, False),
                 ("uniformes_k5_w%d" % args.fils, ("u8000",), 5, args.fils, 5, ("avant", "apres"), 1, False)]
    if autres.get("profil_avant") and autres.get("profil_apres"):
        binaires["profil_avant"] = autres["profil_avant"]["chemin"]
        binaires["profil_apres"] = autres["profil_apres"]["chemin"]
        plans.append(("profil_k5_w1", ("ng00",), 5, 1, 3, ("profil_avant", "profil_apres"), 1, True))
    for nom, trames, k, fils, passes, bras, tours, profil in plans:
        res = {}
        for trame in trames:
            dossier = os.path.join(args.sortie, "journaux", nom, trame)
            os.makedirs(dossier, exist_ok=True)
            res[trame] = []
            for t in range(tours):
                ordre = bras if t % 2 == 0 else tuple(reversed(bras))
                res[trame].append({b: prise(args, binaires[b], trame, k, fils, passes,
                                            os.path.join(dossier, "%s_t%02d.jsonl" % (b, t)), profil) for b in ordre})
        info[nom] = {"k": k, "fils": fils, "passes": passes, "tours": res}
    if autres.get("full_avant") and autres.get("full_apres"):
        res = {}
        for trame in TRAMES:
            dossier = os.path.join(args.sortie, "journaux", "full_k5", trame)
            os.makedirs(dossier, exist_ok=True)
            res[trame] = []
            for t in range(2):
                ordre = ("full_avant", "full_apres") if t % 2 == 0 else ("full_apres", "full_avant")
                res[trame].append({b: prise_full(args, autres[b]["chemin"], trame, 10,
                                                 os.path.join(dossier, "%s_t%02d.jsonl" % (b, t))) for b in ordre})
        info["full_k5_appareil"] = {"k": 5, "fils": args.fils, "passes": 10, "tours": res}
    else:
        info["full_k5_appareil"] = "non joue (nvcc absent ou construction CUDA en echec)"
    info["fin"] = maintenant()
    rapport["informations"] = info


# ---- Juge ------------------------------------------------------------------------------------------------------------
def _juger_admis(rapport, sortie, verifier_journaux=True):
    """Verdicts de REGLE_T2D_B ; journaux re-haches (un journal modifie est refuse)."""
    refus, cas, verdicts = [], {}, {}
    construction = rapport.get("construction", {}).get("binaires", {})
    camp = rapport.get("campagne_k5")
    if not construction or any(b not in construction for b in BRAS_JUGES):
        refus.append("construction absente ou incomplete")
    if not camp:
        refus.append("campagne K5 absente")
        return {"verdicts": {x[0]: "refuse" for x in LEVIERS}, "refus": refus, "regle": REGLE_T2D_B, "cas": cas}
    if any(camp.get("binaires_apres", {}).get(n) != construction.get(n, {}).get("sha256") for n in construction):
        refus.append("binaire change pendant la campagne")
    if camp.get("passes", 0) < REGLE_T2D_B["passes_min"]:
        refus.append("moins de %d passes par processus" % REGLE_T2D_B["passes_min"])
    rng = random.Random(REGLE_T2D_B["graine"])
    empreintes_ok = True
    for trame in REGLE_T2D_B["trames_decisives"]:
        tours = camp.get("trames", {}).get(trame, [])
        valides = [t for t in tours if all(b in t and t[b].get("valide") for b in BRAS_JUGES)]
        exigees = max(REGLE_T2D_B["processus_min"], camp.get("tours_demandes", 0))
        if len(valides) != len(tours) or len(valides) < exigees:
            refus.append("%s : %d tours valides sur %d exiges" % (trame, len(valides), exigees))
            if len(valides) < 2:
                continue  # rien a publier ; sinon les chiffres sont publies, le refus demeure
        if verifier_journaux:
            for t in valides:
                for b in BRAS_JUGES:
                    chemin = os.path.join(sortie, t[b]["journal"])
                    if not os.path.isfile(chemin) or sha256(chemin) != t[b]["journal_sha256"]:
                        refus.append("%s : journal absent ou modifie (%s)" % (trame, t[b]["journal"]))
        empreintes = {t[b]["empreinte"] for t in valides for b in BRAS_JUGES}
        identique = len(empreintes) == 1 and (trame != "ng00" or
                                              next(iter(empreintes)).startswith(EMPREINTE_NG00_K5))
        empreintes_ok = empreintes_ok and identique
        travaux = {t[b].get("travail_sha256") for t in valides for b in BRAS_JUGES}
        cas[trame] = {"tours": len(valides), "empreintes": sorted(e[:16] for e in empreintes if e),
                      "empreinte_identique": identique, "travail_identique_entre_bras": len(travaux) == 1,
                      "g_ms_median": {b: mediane([t[b]["g_ns"] for t in valides]) / 1e6 for b in BRAS_JUGES}}
        for nom, avant, apres in LEVIERS + INFORMATIFS + (CONTROLE_AA,):
            logs = [math.log(t[apres]["g_ns"] / t[avant]["g_ns"]) for t in valides]
            gm, bas, haut = bootstrap_gm(logs, rng, REGLE_T2D_B["bootstrap"])
            cas[trame][nom] = {"moyenne_geometrique": gm, "ic95": [bas, haut], "rapports": [math.exp(x) for x in logs]}
    for nom, _, _ in LEVIERS:
        if refus:
            verdicts[nom] = "refuse"
        elif not empreintes_ok:
            verdicts[nom] = "rejete"
        else:
            hautes = [cas[t][nom]["ic95"][1] for t in REGLE_T2D_B["trames_decisives"]]
            verdicts[nom] = "adopte" if all(h < REGLE_T2D_B["seuil_borne_haute"] for h in hautes) else "rejete"
    aa = [cas[t][CONTROLE_AA[0]]["moyenne_geometrique"] for t in cas if CONTROLE_AA[0] in cas[t]]
    controle = {"moyennes": aa, "dans_la_fenetre": all(abs(x - 1) <= REGLE_T2D_B["fenetre_aa"] for x in aa)}
    if aa and not controle["dans_la_fenetre"]:  # veto A/A (amendement du 8 octobre, 06:04 UTC)
        refus.append("controle A/A hors de la fenetre de +/- %.1f %%" % (100 * REGLE_T2D_B["fenetre_aa"]))
        verdicts = {nom: "refuse" for nom, _, _ in LEVIERS}
    return {"verdicts": verdicts, "refus": refus, "regle": REGLE_T2D_B, "cas": cas, "controle_aa": controle}


def juger(rapport, sortie, verifier_journaux=True):
    """Le rejeu brut fait autorite ; la voie sans journaux est reservee a l'auto-test synthetique."""
    if not verifier_journaux:
        return _juger_admis(rapport, sortie, verifier_journaux=False)
    try:
        admitted = json.loads(json.dumps(rapport, allow_nan=False))
        camp = admitted["campagne_k5"]
        fils = naturel(camp["fils"], 256, 1)
        passes = naturel(camp["passes"], 1000, 2)
        requested = naturel(camp["tours_demandes"], minimum=1)
        exiger(set(camp["trames"]) == set(TRAMES), "cohorte de trames incomplete ou inconnue")
        construction = admitted["construction"]["binaires"]
        exiger(type(construction) is dict, "construction invalide")
        exiger(set(BRAS_JUGES) <= set(construction), "bras absent")
        exiger(admitted["construction"]["bras_sha256"] == sha256(BRAS_FICHIER), "bras_t2d_b.json change")
        for name, binary in construction.items():
            exiger(hexadecimal(binary["sha256"]) == hexadecimal(camp["binaires_apres"][name]),
                   "empreinte de binaire modifiee")
        root, journals = os.path.realpath(sortie), set()
        for frame in TRAMES:
            turns = camp["trames"][frame]
            exiger(type(turns) is list and len(turns) == requested, "nombre de tours different de la demande")
            sizes = set()
            for turn in turns:
                champs(turn, BRAS_JUGES)
                for arm in BRAS_JUGES:
                    take = turn[arm]
                    exiger(type(take) is dict and take.get("valide") is True, "prise declaree invalide")
                    name = take["journal"]
                    exiger(type(name) is str and name and not os.path.isabs(name), "chemin de journal invalide")
                    path = os.path.realpath(os.path.join(root, name))
                    exiger(os.path.commonpath((root, path)) == root and path not in journals,
                           "journal externe ou reutilise")
                    journals.add(path)
                    before = sha256(path)
                    exiger(before == hexadecimal(take["journal_sha256"]), "journal modifie")
                    checked = lire_prise(path, take["code"], REGLE_T2D_B["k"], fils, passes)
                    exiger(sha256(path) == before, "journal modifie pendant la lecture")
                    for key in ("murs_ns", "empreinte", "g_ns", "diagnostics", "profil", "travail_sha256"):
                        exiger(json.dumps(take[key], sort_keys=True, allow_nan=False) ==
                               json.dumps(checked[key], sort_keys=True, allow_nan=False),
                               "resume different du brut : " + key)
                    sizes.add(checked["sites"])
                    take.update(checked)
            exiger(len(sizes) == 1, "nombre de sites different entre bras ou tours")
        return _juger_admis(admitted, sortie, verifier_journaux=False)
    except (ValueError, TypeError, KeyError, OSError, UnicodeError, OverflowError) as error:
        return {"verdicts": {name: "refuse" for name, _, _ in LEVIERS},
                "refus": ["admission des journaux : " + str(error)], "regle": REGLE_T2D_B, "cas": {}}


def resume_informations(rapport):
    """Informations (jamais jugees) ; un echec du resume n'atteint jamais le verdict."""
    try:
        return _resume_informations(rapport)
    except (ValueError, TypeError, KeyError, AttributeError, ZeroDivisionError) as error:
        return {"erreur_du_resume": str(error)}


def _resume_informations(rapport):
    """Mediane de G par bras et trame, empreintes (uniformes : lignes gravees des portes d'echelle), sections du profil,
    mur FULL et identite FUL1 entre bras, passes et processus."""
    info = rapport.get("informations", {})
    out = {}
    for nom, bloc in info.items():
        if not isinstance(bloc, dict) or "tours" not in bloc:
            continue
        res = {}
        for trame, tours in bloc["tours"].items():
            bras = sorted({b for t in tours for b in t})
            ligne = {}
            for b in bras:
                prises = [t[b] for t in tours if b in t]
                valides = [p for p in prises if p.get("valide")]
                entree_b = {"prises": len(prises), "valides": len(valides)}
                if nom.startswith("full"):
                    if valides:
                        entree_b["full_ms"] = mediane([p["mur_ns"] for p in valides]) / 1e6
                        entree_b["g_ms"] = mediane([p["g_ns"] for p in valides]) / 1e6
                    entree_b["ful1"] = sorted({e for p in valides for e in p["ful1"]})
                elif valides:
                    entree_b["g_ms"] = mediane([p["g_ns"] for p in valides]) / 1e6
                    entree_b["empreintes"] = sorted({p["empreinte"][:16] for p in valides})
                    if trame.startswith("u") and int(trame[1:]) in UNIFORMES:
                        entree_b["empreinte_gravee"] = entree_b["empreintes"] == [UNIFORMES[int(trame[1:])]]
                    if valides[0].get("profil"):
                        sections = {}
                        for p in valides:
                            for item in p["profil"]:
                                for s, v in item["sections"].items():
                                    sections[s] = sections.get(s, 0.0) + v["ns"] * v["n"] / 1e6
                        entree_b["profil_ms_cumules"] = {s: round(v, 1) for s, v in sorted(sections.items())}
                ligne[b] = entree_b
            if nom.startswith("full"):
                tous = {e for b in ligne.values() for e in b.get("ful1", [])}
                ligne["ful1_identique_entre_bras"] = len(tous) == 1 and all(b["valides"] > 0 for b in ligne.values())
            res[trame] = ligne
        out[nom] = res
    return out


def tableaux(rapport, jugement):
    out = ["# Chantier T2-d-B : etage G par levier (genere par pilote_t2d_b.py)", "",
           "Verdicts : " + ", ".join("%s **%s**" % (n, v) for n, v in jugement["verdicts"].items()), ""]
    if jugement["refus"]:
        out += ["Refus : " + " ; ".join(jugement["refus"]), ""]
    out += ["| trame | " + " | ".join("G %s (ms)" % b for b in BRAS_JUGES) + " | empreinte | travail identique |",
            "|" + " --- |" * (3 + len(BRAS_JUGES))]
    for trame, c in jugement["cas"].items():
        if "g_ms_median" in c:
            out.append("| %s | %s | %s | %s |" % (trame, " | ".join("%.2f" % c["g_ms_median"][b] for b in BRAS_JUGES),
                                                  ",".join(c["empreintes"]), c["travail_identique_entre_bras"]))
    paires = LEVIERS + INFORMATIFS + (CONTROLE_AA,)
    out += ["", "| trame | " + " | ".join("%s (IC 95 %%)" % x[0] for x in paires) + " |",
            "|" + " --- |" * (1 + len(paires))]
    for trame, c in jugement["cas"].items():
        if "g_ms_median" in c:
            out.append("| %s | %s |" % (trame, " | ".join("%.3f (%.3f-%.3f)" % (
                c[x[0]]["moyenne_geometrique"], c[x[0]]["ic95"][0], c[x[0]]["ic95"][1]) for x in paires)))
    resume = resume_informations(rapport)
    if resume:
        out += ["", "Informations (jamais jugees) :", "", "```json", json.dumps(resume, indent=1, sort_keys=True),
                "```"]
    full = rapport.get("informations", {}).get("full_k5_appareil")
    if isinstance(full, dict):
        out += ["", "Mur FULL sur l'appareil (information, mediane des passes 2..10 par processus) :", "",
                "| trame | bras | FULL (ms) | G dans FULL (ms) | FUL1 |", "| --- | --- | --- | --- | --- |"]
        for trame, tours in full["tours"].items():
            for t in tours:
                for b, p in t.items():
                    if p.get("valide"):
                        out.append("| %s | %s | %.2f | %.2f | %s |" % (trame, b, p["mur_ns"] / 1e6, p["g_ns"] / 1e6,
                                                                      ",".join(e[:16] for e in p["ful1"])))
                    else:
                        out.append("| %s | %s | refus : %s | | |" % (trame, b, p.get("admission", "")))
    return "\n".join(out) + "\n"


def etape_rapport(args, rapport):
    jugement = juger(rapport, args.sortie)
    rapport["jugement"] = jugement
    rapport["resume_informations"] = resume_informations(rapport)
    with open(os.path.join(args.sortie, "tableaux_t2d_b.md"), "w", encoding="utf-8") as f:
        f.write(tableaux(rapport, jugement))
    return jugement


# ---- Auto-test du juge -----------------------------------------------------------------------------------------------
def campagne_synthetique(effets, empreinte_differente=False, manque=False, graine=1):
    rng = random.Random(graine)
    rapport = {"construction": {"binaires": {b: {"sha256": "x"} for b in BRAS_JUGES}},
               "campagne_k5": {"passes": 10, "tours_demandes": 10, "binaires_apres": {b: "x" for b in BRAS_JUGES},
                               "trames": {}}}
    for trame in TRAMES:
        tours = []
        for t in range(10):
            base = 60e6 * (1 + 0.02 * rng.random())
            tour = {}
            for b in BRAS_JUGES:
                g = base * effets.get(b, 1.0) * (1 + 0.004 * (rng.random() - 0.5))
                e = EMPREINTE_NG00_K5 + "0" * 48 if trame == "ng00" else "ab" * 32
                if empreinte_differente and b == "apres" and t == 3:
                    e = "cd" * 32
                tour[b] = {"valide": not (manque and b == "report" and t == 5), "g_ns": g, "empreinte": e,
                           "journal": "", "journal_sha256": "", "travail_sha256": "w"}
            tours.append(tour)
        rapport["campagne_k5"]["trames"][trame] = tours
    return rapport


def etape_auto_test():
    tous = lambda v: {x[0]: v for x in LEVIERS}  # noqa: E731
    attendus = [({"apres": 0.8, "garde": 0.97, "report": 0.99, "temoins": 0.9, "census": 0.88, "proposition": 0.97},
                 False, False, tous("adopte")),
                ({"apres": 0.8, "garde": 1.0, "report": 1.02, "temoins": 0.9, "census": 0.9, "proposition": 1.05},
                 False, False, {"lot_t2d_b": "adopte", "garde_seule": "rejete", "report_seul": "rejete",
                                "temoins_seuls": "adopte", "census_combine": "adopte", "proposition": "rejete"}),
                ({"apres": 1.05, "garde": 1.0, "report": 1.0, "temoins": 1.1, "census": 1.0, "proposition": 1.0},
                 False, False, tous("rejete")),
                ({"apres": 0.6, "garde": 0.9, "report": 0.9, "temoins": 0.8, "census": 0.7, "proposition": 0.9},
                 True, False, tous("rejete")),
                ({"apres": 0.6, "garde": 0.9, "report": 0.9, "temoins": 0.8, "census": 0.7, "proposition": 0.9},
                 False, True, tous("refuse")),
                ({"apres": 0.8, "garde": 0.9, "report": 0.9, "temoins": 0.8, "census": 0.8, "proposition": 0.9,
                  "avant_bis": 1.2}, False, False, tous("refuse"))]  # veto A/A
    ecarts = []
    for i, (effets, diff, manque, attendu) in enumerate(attendus):
        jugement = juger(campagne_synthetique(effets, diff, manque, graine=i + 1), "", verifier_journaux=False)
        if jugement["verdicts"] != attendu:
            ecarts.append("cas %d : %s au lieu de %s" % (i, jugement["verdicts"], attendu))
    lectures = auto_test_lecture(ecarts)
    for e in ecarts:
        print("pilote_t2d_b_auto_test_ecart " + e)
    if ecarts:
        return 1
    print("pilote_t2d_b_auto_test_ok cas=%d lectures=%d verdicts=adopte,rejete,refuse" % (len(attendus), lectures))
    return 0


def journal_g_synthetique(k=2, passes=2, fils=3, mur=1000):
    """Journal de sonde de G coherent (partition exacte du mur), pour l'auto-test de lecture."""
    lignes = []
    for p in range(passes):
        diag = {"prepare_ns": 10, "count_ns": 10, "setup_ns": 10, "fill_ns": 10, "workspace_ns": 10,
                "order_ns": [100] + [300] * (k - 1), "table_ns": [50] * (k - 1), "join_ns": [0] * (k - 1),
                "pass_ns": [100] + [200] * (k - 1), "workspace_bytes": 1, "table_bytes": 1, "peak_bytes": 1}
        diag.update(tables_ns=sum(diag["table_ns"]), joins_ns=0, resolve_ns=sum(diag["pass_ns"]),
                    orders_ns=sum(diag["order_ns"]))
        dedans = 50 + diag["orders_ns"]
        diag["reste_ns"] = mur - dedans
        lignes.append({"phase": "tour_g", "pass": p, "status": "ok", "reason": "none", "order": 0, "coord_bits": 21,
                       "kmax": k, "threads": fils, "sites": 8, "wall_ns": mur, "diagnostics": diag})
    for order in range(1, k + 1):
        travail = {cle: 0 for cle in TRAVAIL}
        travail["chaines"] = [0] * 16
        lignes.append({"phase": "ordre", "k": order, "objet": {cle: 0 for cle in OBJET}, "travail": travail})
    lignes.append({"phase": "digest", "resolution_sha256": "ab" * 32})
    lignes.append({"phase": "exit", "status": "ok", "reason": "none", "order": 0})
    return lignes


def journal_full_synthetique(passes=2, fils=3, trame="ng00"):
    """Journal de sonde FULL sur l'appareil coherent (schema sequentiel de 902041f66), pour l'auto-test de lecture."""
    lignes = [{"phase": "open", "status": "ok", "reason": "none", "wall_ns": 5, "budget_appareil": "partage"}]
    for p in range(passes):
        etapes = {"P": 10, "C": 10, "G": 10, "raccord": 10, "TMVR": 40, "T": 10, "M": 10, "V": 10, "R": 10}
        lignes.append({"phase": "full", "pass": p, "trame": trame, "voie": "device", "status": "ok", "coord_bits": 21,
                       "kmax": 5, "threads": fils, "sites": 8, "wall_ns": 100, "etapes_ns": etapes,
                       "c_ns": {k: 1 for k in lf.C_KEYS}, "g_ns": {"tables": 4, "resolution": 5},
                       "hors_mur_ns": {"validation": 1, "empreinte": 1}, "pic_octets": 9, "cpu_ns": 300,
                       "rss_max_octets": 1 << 20, "appareil_octets": 0, "epinglee_octets": 0, "pic_appareil_octets": 0,
                       "memoire_octets": {e: [1, 9 if e == "C" else 2] for e in lf.MEM_STAGES},
                       "full_sha256": "cd" * 32})
        lignes.append({"phase": "liberation", "pass": p, "liberation_ns": 3})
    lignes.append({"phase": "exit", "status": "ok", "reason": "none"})
    return lignes


def auto_test_lecture(ecarts):
    """Lectures strictes : un journal coherent est admis ; chaque falsification est refusee (ValueError)."""
    def ecrire(dossier, nom, lignes):
        chemin = os.path.join(dossier, nom)
        with open(chemin, "w", encoding="utf-8") as f:
            f.write("".join(json.dumps(x) + "\n" for x in lignes))
        return chemin

    def g_falsifie(modif):
        lignes = journal_g_synthetique()
        modif(lignes)
        return lignes

    def full_falsifie(modif):
        lignes = journal_full_synthetique()
        modif(lignes)
        return lignes
    cas_g = [("mur_1ns", lambda x: [r.update(wall_ns=1) for r in x if r["phase"] == "tour_g"]),
             ("reste_faux", lambda x: x[0]["diagnostics"].update(reste_ns=x[0]["diagnostics"]["reste_ns"] + 1)),
             ("total_faux", lambda x: x[1]["diagnostics"].update(tables_ns=x[1]["diagnostics"]["tables_ns"] + 1)),
             ("enveloppe_debordee", lambda x: x[0]["diagnostics"].update(pass_ns=[100, 400], resolve_ns=500)),
             ("passe_booleenne", lambda x: x[1].update({"pass": True}))]
    cas_full = [("voie_appareil", lambda x: x[1].update(voie="appareil")),
                ("sans_liberation", lambda x: x.pop(2)),
                ("pic_faux", lambda x: x[1].update(pic_octets=10)),
                ("passe_booleenne", lambda x: x[3].update({"pass": True})),
                ("etages_hors_du_mur", lambda x: x[1].update(wall_ns=20)),
                ("trame_autre", lambda x: x[3].update(trame="ng01")),
                # Residu de l'auditeur (t2d_b_admission_reprise) : quatre corruptions que l'ancien lecteur admettait.
                ("blocs_ignores", lambda x: x[1].update(c_ns=None)),
                ("usage_sup_pic", lambda x: x[1]["memoire_octets"].update(C=[10, 9])),
                ("sous_etages_hors_tmvr", lambda x: x[1]["etapes_ns"].update(T=41)),
                ("tables_hors_g", lambda x: x[1]["g_ns"].update(tables=11))]
    lectures = 0
    with tempfile.TemporaryDirectory() as dossier:
        try:
            lire_prise(ecrire(dossier, "g.jsonl", journal_g_synthetique()), 0, 2, 3, 2)
            lire_full(ecrire(dossier, "f.jsonl", journal_full_synthetique()), 0, 2, "ng00", 3, 8)
            lectures += 2
        except (ValueError, TypeError, KeyError) as error:
            ecarts.append("lecture d'un journal coherent refusee : %s" % error)
        for nom, modif in cas_g:
            try:
                lire_prise(ecrire(dossier, "g_%s.jsonl" % nom, g_falsifie(modif)), 0, 2, 3, 2)
                ecarts.append("journal de G falsifie admis : %s" % nom)
            except (ValueError, TypeError, KeyError):
                lectures += 1
        for nom, modif in cas_full:
            try:
                lire_full(ecrire(dossier, "f_%s.jsonl" % nom, full_falsifie(modif)), 0, 2, "ng00", 3, 8)
                ecarts.append("journal FULL falsifie admis : %s" % nom)
            except (ValueError, TypeError, KeyError):
                lectures += 1
    return lectures


def main(argv):
    p = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    p.add_argument("etapes", nargs="+", choices=("auto-test", "construire", "campagne", "informations", "rapport",
                                                 "tout"))
    p.add_argument("--src", help="racine du paquet (contient morsehgp3D_v12/)")
    p.add_argument("--travail", help="dossier des constructions des bras")
    p.add_argument("--donnees", help="dossier des trames et de l'archive de la base")
    p.add_argument("--sortie", help="dossier des journaux et du rapport")
    p.add_argument("--avant-archive")
    p.add_argument("--avant-sha256")
    p.add_argument("--fils", type=int, default=48)
    p.add_argument("--jobs", type=int, default=8)
    p.add_argument("--processus", type=int, default=10)
    p.add_argument("--passes", type=int, default=10)
    p.add_argument("--nvcc", default="", help="compilateur CUDA de la sonde FULL (defaut : PATH puis /usr/local/cuda) ;"
                                             " aucun : mur FULL non joue (essai local sans appareil)")
    p.add_argument("--essai", action="store_true",
                   help="essai local : moins de tours ou de passes admis ; le juge refuse alors (jamais decisif)")
    args = p.parse_args(argv[1:])
    # Le verdict ne depend que de la campagne : il est rendu avant les informations, puis le rapport est complete.
    etapes = ["auto-test", "construire", "campagne", "rapport", "informations", "rapport"] if "tout" in args.etapes \
        else args.etapes
    if etapes == ["auto-test"]:
        return etape_auto_test()
    if not args.sortie or not args.travail or (("construire" in etapes) and not (args.src and args.avant_archive and
                                                                                 args.avant_sha256)):
        print("pilote_t2d_b_refus usage : --sortie, --travail, et pour construire --src --avant-archive --avant-sha256")
        return 2
    if not args.essai and (args.processus < REGLE_T2D_B["processus_min"] or args.passes < REGLE_T2D_B["passes_min"]):
        print("pilote_t2d_b_refus usage : au moins %d processus et %d passes" % (REGLE_T2D_B["processus_min"],
                                                                               REGLE_T2D_B["passes_min"]))
        return 2
    os.makedirs(args.sortie, exist_ok=True)
    chemin = os.path.join(args.sortie, "rapport_t2d_b.json")
    rapport = {"schema": "ehgp.v12.t2d_b_pilote.v1", "regle": REGLE_T2D_B}
    if os.path.isfile(chemin):
        with open(chemin, encoding="utf-8") as f:
            rapport = json.load(f)
    code = 0

    def sauver():
        with open(chemin, "w", encoding="utf-8") as f:
            json.dump(rapport, f, indent=1, sort_keys=True)
    try:
        for etape in etapes:
            if etape == "auto-test":
                if etape_auto_test() != 0:
                    return 1
            elif etape == "construire":
                etape_construire(args, rapport)
            elif etape == "campagne":
                etape_campagne(args, rapport)
            elif etape == "informations":
                etape_informations(args, rapport)
            elif etape == "rapport":
                if etape_auto_test() != 0:  # le juge d'abord juge sur ses cas synthetiques
                    return 1
                jugement = etape_rapport(args, rapport)
                print("pilote_t2d_b_verdicts " + " ".join("%s=%s" % kv for kv in jugement["verdicts"].items()))
                code = 3 if jugement["refus"] else 0
            sauver()  # un delai de la commande garde les etapes deja jouees
    except (RuntimeError, KeyError, OSError, ValueError) as e:
        print("pilote_t2d_b_refus %s" % e)
        code = 3
    sauver()
    return code


if __name__ == "__main__":
    sys.exit(main(sys.argv))
