#!/usr/bin/env python3
"""Pilote et juge du chantier T2-d-B2 (ouverture et cout interne de l'etage G dans la Session recouverte) : mur FULL du
produit, catalogue sur l'appareil, un bras par levier, en processus alternes. Bibliotheque standard seule, jouable sous
python3 -S -O (aucune garde par assert) ; lecture par le lecteur strict commun microbancs/outils/lecteur_full.py,
environnement et constructions par microbancs/outils/banc_full.py.

Bras (sondes mhgp12_full_probe construites ici avec CUDA, profil 21, voie par defaut = Session recouverte) :
  avant      archive epinglee des sources de main 72f622a55 (sha256 verifie avant deballage)
  avant_bis  le MEME binaire que avant, joue comme un bras distinct (A/A : bruit de la session ; veto)
  tables     avant + B2-T : index des naissances de tous les ordres construits ensemble a l'ouverture de build_tower
             (PopulationTable::build_all : une invocation du Pool par phase pour toutes les tables ; radix_sort_many)
  relecture  avant + B2-S : seconde recherche du support evitee quand le support certifie egale le support propose et
             que la table vient de repondre << absent >>
  census     avant + B2-C : census a plat (sur le modele de microbancs/mes_g_appareil/noyau_g.hpp) : garde evaluee en
             ligne (voies native et certifiee), issue rendue sans Result par noeud ni par site, site lu par ses
             coordonnees sans Point::make ; voies controlee et large hors ligne, memes compteurs et memes refus
  apres      avant + B2-T + B2-S + B2-C (le lot)
Chaque bras autre que avant est une copie de l'arbre de avant ou s'appliquent, dans l'ordre, les substitutions
exactes de bras_t2d_b2.json (dossier de ce pilote ; fichiers de src/ seulement ; SHA-256 de chaque fichier controle
avant et apres, chaque motif present une seule fois). Rien d'autre ne differe entre les bras.

Pourquoi le mur FULL et non G seul : B2-T n'agit que dans la Session recouverte (la voie sequentielle, resolve_tower,
construit l'index d'un ordre a la fois) et l'ouverture de build_tower est sur le chemin critique du mur ; le contrat
juge le mur FULL. B2-S et B2-C agissent dans les deux voies ; le mur FULL est leur juge pour la meme raison. Trames decisives : celles ou G est le chemin critique (session M) : ng00-02 et deux trames moyennes du
v12set ; sur les plus grandes, le noyau sequentiel de l'ordre 5 (chantier A6) ferme la tour et masque le gain de G :
la plus grande trame est publiee en information seulement. La fin du dernier calcul de G, l'ouverture et les tables
sont publiees par bras. Sonde au defaut du produit : Session recouverte avec le cache de blocs de 8 Gio.

Etapes (dans l'ordre si plusieurs) :
  auto-test     le juge sur des campagnes synthetiques (adopte, rejete, refuse, veto A/A, identite absente ou fausse)
  construire    environnement (cmake, nvcc, GPU vide) ; deballage, bras par substitution ; constructions CUDA des cinq
                sondes en parallele ; empreintes des binaires
  identite      par trame decisive et par bras, un processus de 2 passes avec --digest : une seule empreinte FUL1 par
                trame dans tous les bras et toutes les passes (ng00 : la reference gravee)
  campagne      trames decisives ng00, ng01, ng02 et deux trames moyennes du v12set (kitti_ng_02_001606, 64 740
                sites, la mediane ; kitti_ng_08_001176, 67 114), K5, --device, --fils fils, SANS --digest (l'empreinte,
                hors du mur, couterait plus que le mur a chaque passe) : --processus tours, un processus neuf par bras
                et par tour, ordre decale d'un bras par tour, sans inversion ; --passes passes (la premiere, a froid,
                ecartee)
  informations  non jugees : la plus grande trame du v12set (kitti_ng_08_002119, 99 099 sites ; fin de G, ouverture,
                tables et mur par bras, 3 tours) ; K10 sur ng00-02 (avant, apres), un tour ; G sequentiel (voie
                --sequentiel, resolve_tower : etage G, tables et resolution) sur ng00 et la trame mediane, bras avant,
                relecture, census et apres, 3 tours (ajout du 8 octobre a 11:44 UTC, sans verdict : B2-S et B2-C
                agissent aussi dans resolve_tower) ; environnement a la fin (l'environnement juge, GPU vide, est
                releve a la fin de la campagne, avant le verdict)
  rapport       auto-test puis juge (REGLE_T2D_B2), rapport JSON et tableaux Markdown
  tout          auto-test construire identite campagne rapport informations rapport (verdict avant les informations ;
                rapport_t2d_b2.json ecrit apres chaque etape)

REGLE_T2D_B2 (ecrite le 8 octobre 2026 a 09:44 UTC ; revisee a 09:48 UTC, avant toute mesure de ces bras sur G4, sur la
consigne du coordinateur apres la session M : base 72f622a55, trames decisives ng00-02 et deux trames moyennes, la plus
grande en information ; revisee a 11:22 UTC, toujours avant toute mesure G4 de ces bras, sur la consigne du
coordinateur apres la session G-APP : bras census (B2-C) ajoute et juge comme les autres leviers, le lot apres
comprend B2-C ; rien d'autre ne change) : par processus, mur FULL =
mediane des passes 2 a P de wall_ns (Session recouverte, catalogue sur l'appareil) ; par tour, rapport bras / avant ;
moyenne geometrique des rapports des tours et IC 95 % par bootstrap sur les tours (10 000 tirages, graine 20261008),
par trame. Un levier est ADOPTE si l'identite FUL1 est etablie (une seule empreinte par trame decisive dans tous les
bras et toutes les passes de l'etape identite, et sur ng00 la reference gravee) ET si la borne haute de l'IC de son
rapport est sous 1 sur CHACUNE des cinq trames decisives ; REJETE sinon, et en particulier si l'identite est fausse
(deux empreintes, ou ng00 differente de la reference) ; REFUSE si une prise de la campagne manque ou est illisible
(lecteur strict commun), si l'etape identite manque ou est illisible, si l'environnement est incomplet ou le GPU
occupe avant ou apres, si un bras ne se reconstruit pas a ses empreintes, si un binaire ou un journal change, si
l'auto-test du juge echoue, si moins de --processus tours valides, ou si la moyenne geometrique du controle A/A
(avant -> avant_bis) sort de la fenetre de +/- 1,5 % sur l'une des trames decisives (veto : appariement invalide).
Leviers juges, chacun contre avant : lot_b2 (apres), tables (tables), relecture (relecture), census (census). Rapports
publies sans verdict : relecture_apres_tables (tables -> apres), tables_apres_relecture (relecture -> apres).

Exemple (G4, commande d'un plan de session) :
  python3 pilote_t2d_b2.py tout --src <racine du paquet> --travail <dossier> --donnees <donnees> --sortie <sortie> \\
      --avant-archive <donnees>/v12_src_72f622a55.tar.gz --avant-sha256 <hex> --fils 48 --jobs 44

Codes : 0 campagne complete et jugee (le verdict est dans le rapport) ; 1 auto-test en echec ; 2 usage ; 3 refus
(preuve absente, bras non reconstruit, binaire change, construction en echec, prise manquante).
"""
import argparse
import concurrent.futures
import datetime
import hashlib
import json
import math
import os
import random
import shutil
import sys
import tarfile

ICI = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(ICI, '..', 'outils'))
import banc_full as bf  # noqa: E402
import lecteur_full as lf  # noqa: E402

BRAS_FICHIER = os.path.join(ICI, "bras_t2d_b2.json")
BRAS_SUBSTITUES = ("tables", "relecture", "census", "apres")
BRAS_CONSTRUITS = ("avant",) + BRAS_SUBSTITUES
BRAS_JUGES = ("avant", "avant_bis") + BRAS_SUBSTITUES
LEVIERS = (("lot_b2", "avant", "apres"), ("tables", "avant", "tables"), ("relecture", "avant", "relecture"),
           ("census", "avant", "census"))
INFORMATIFS = (("relecture_apres_tables", "tables", "apres"), ("tables_apres_relecture", "relecture", "apres"))
CONTROLE_AA = ("A/A", "avant", "avant_bis")
LIDAR = {"ng00": 39885, "ng01": 35551, "ng02": 45845}  # sites des trames ng00-02 (tests/tower/mes_m0.py)
MOYENNES = ("kitti_ng_02_001606", "kitti_ng_08_001176")  # trames moyennes du v12set (64 740 et 67 114 sites)
GRANDE = "kitti_ng_08_002119"  # la plus grande du v12set (99 099 sites) : information seulement
SEQ_TRAMES = ("ng00", MOYENNES[0])  # G sequentiel (resolve_tower) : information seulement
SEQ_BRAS = ("avant", "relecture", "census", "apres")
TRAMES = ("ng00", "ng01", "ng02") + MOYENNES
FUL1_NG00_K5 = "3a2bfb4f9f48b4b0cc5b0318d9fcf4887906638e034b2c97dda1918e3170a6fe"  # MESURE.md, paragraphe 4
REGLE_T2D_B2 = {"trames_decisives": TRAMES, "k": 5, "processus_min": 10, "passes_min": 6, "bootstrap": 10000,
                "graine": 20261008, "seuil_borne_haute": 1.0, "fenetre_aa": 0.015, "base": "72f622a55",
                "information": GRANDE,
                "revision": "09:48 UTC : base 72f622a55, trames moyennes au lieu des grandes ; 11:22 UTC : bras census "
                            "(B2-C, census a plat) ajoute et juge, le lot apres comprend B2-C",
                "mesure": "mur FULL de la Session recouverte (wall_ns), catalogue sur l'appareil, sans --digest",
                "statistique": "par processus : mediane des passes 2..P du mur FULL ; par tour : rapport bras / "
                               "avant ; moyenne geometrique et IC 95 % par bootstrap sur les tours",
                "adoption": "identite FUL1 etablie (une empreinte par trame decisive, ng00 = reference) et borne haute "
                            "de l'IC sous 1 sur chacune des cinq trames decisives",
                "veto_aa": "refus si la moyenne geometrique A/A sort de 1 +/- fenetre_aa sur une trame decisive",
                "leviers": [list(x) for x in LEVIERS], "informatifs": [list(x) for x in INFORMATIFS],
                "controle": list(CONTROLE_AA), "ful1_ng00_k5": FUL1_NG00_K5}


def maintenant():
    return datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def mediane(valeurs):
    v = sorted(valeurs)
    n = len(v)
    return v[n // 2] if n % 2 else 0.5 * (v[n // 2 - 1] + v[n // 2])


def bootstrap_gm(logs, rng, tirages):
    """Moyenne geometrique et IC 95 % par bootstrap (conventions de REGLE_T2D_B)."""
    gm = math.exp(sum(logs) / len(logs))
    boot = []
    for _ in range(tirages):
        echantillon = [logs[rng.randrange(len(logs))] for _ in logs]
        boot.append(sum(echantillon) / len(echantillon))
    boot.sort()
    return gm, math.exp(boot[int(0.025 * tirages)]), math.exp(boot[int(0.975 * tirages) - 1])


def exiger(condition, message):
    if not condition:
        raise ValueError(message)


# ---- Construction ----------------------------------------------------------------------------------------------------
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
    return os.path.join(dest, "morsehgp3D_v12")


def lire_bras():
    with open(BRAS_FICHIER, encoding="utf-8") as f:
        bras = json.load(f)
    if bras.get("schema") != "ehgp.v12.t2d_b2_bras.v1" or set(bras.get("bras", {})) != set(BRAS_SUBSTITUES):
        raise RuntimeError("bras_t2d_b2.json illisible ou incomplet")
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


def nvcc_de(args):
    if args.nvcc:
        return args.nvcc if os.path.isfile(args.nvcc) else None
    return shutil.which("nvcc") or ("/usr/local/cuda/bin/nvcc" if os.path.isfile("/usr/local/cuda/bin/nvcc") else None)


def etape_construire(args, rapport):
    travail = os.path.abspath(args.travail)
    journaux = os.path.join(args.sortie, "construction")
    os.makedirs(journaux, exist_ok=True)
    nvcc = nvcc_de(args)
    rapport["environnement"] = {"avant": bf.environment(nvcc, True), "nvcc": nvcc}
    if nvcc is None:
        raise RuntimeError("nvcc absent : la sonde FULL sur l'appareil ne peut pas etre construite")
    bras = lire_bras()
    racines = {}
    base = os.path.join(travail, "avant", "src")
    if os.path.isdir(base):
        shutil.rmtree(base)
    os.makedirs(base)
    sources_avant = deballer(args.avant_archive, args.avant_sha256, base)
    racines["avant"] = base
    for nom in BRAS_SUBSTITUES:
        racine = os.path.join(travail, nom, "src")
        if os.path.isdir(racine):
            shutil.rmtree(racine)
        shutil.copytree(sources_avant, os.path.join(racine, "morsehgp3D_v12"))
        substituer(os.path.join(racine, "morsehgp3D_v12"), bras["bras"][nom]["fichiers"])
        racines[nom] = racine
    jobs = max(1, args.jobs // len(BRAS_CONSTRUITS))
    with concurrent.futures.ThreadPoolExecutor(max_workers=len(BRAS_CONSTRUITS)) as pool:
        futurs = {nom: pool.submit(bf.build, racines[nom], os.path.join(travail, nom, "build"), nvcc, jobs,
                                   os.path.join(journaux, "build_%s.txt" % nom)) for nom in BRAS_CONSTRUITS}
        binaires = {nom: f.result() for nom, f in futurs.items()}
    for nom, b in binaires.items():
        if b is None:
            raise RuntimeError("construction du bras %s en echec (journal build_%s.txt)" % (nom, nom))
    binaires["avant_bis"] = binaires["avant"]
    rapport["construction"] = {
        "debut": maintenant(), "archive_avant_sha256": args.avant_sha256, "bras_sha256": bf.sha256_file(BRAS_FICHIER),
        "binaires": {n: {"chemin": b, "sha256": bf.sha256_file(b)} for n, b in binaires.items()},
        "cmake": {n: bf.cmake_extract(os.path.join(travail, n, "build")) for n in BRAS_CONSTRUITS}}


# ---- Trames et prises ------------------------------------------------------------------------------------------------
def trames(args, rapport):
    """Fichiers et sites des trames : ng00-02 dans les donnees, trames du v12set depuis son archive."""
    if "trames" in rapport:
        return rapport["trames"]
    out = {}
    for t, sites in LIDAR.items():
        stem = os.path.join(args.donnees, "lidar_%s" % t)
        out[t] = {"xyz": stem + ".u32le", "ids": stem + ".ids.u32le", "sites": sites}
    dossier = os.path.join(os.path.abspath(args.travail), "v12set")
    manifeste = bf.unpack(os.path.join(args.donnees, "g4_kitti_v12set_xyz.tar"), dossier)
    if manifeste is None:
        raise RuntimeError("archive v12set absente ou refusee")
    cas = {c["name"]: c for c in manifeste.get("cases", []) if isinstance(c, dict)}
    for t in MOYENNES + (GRANDE,):
        if t not in cas or cas[t].get("duplicate_sites") != 0:
            raise RuntimeError("trame %s absente du v12set ou avec doublons" % t)
        out[t] = {"xyz": os.path.join(dossier, t + ".u32le"), "ids": os.path.join(dossier, t + ".ids.u32le"),
                  "sites": int(cas[t]["count"])}
    rapport["trames"] = out
    return out


def attendu(trame, sites, k, fils, passes, digest, schema="recouvert"):
    return dict(voie="appareil", k=k, fils=fils, passes=passes, empreinte=digest, trames=[(trame, sites)],
                budget_appareil="partage", bits=21, schema=schema)


def resume(passes):
    """Champs d'une prise lue : murs, fin du dernier calcul de G, ouverture, tables, empreintes FUL1."""
    return {"murs_ns": [p["wall_ns"] for p in passes],
            "fin_g_ns": [p["recouvrement"]["fin_g_ns"] for p in passes],
            "ouverture_ns": [p["g_ns"]["ouverture"] for p in passes],
            "tables_ns": [p["g_ns"]["tables"] for p in passes],
            "ful1": sorted({p["full_sha256"] for p in passes if "full_sha256" in p})}


def prise(args, binaire, trame, info, k, passes, journal, digest, schema="recouvert"):
    """Un processus neuf de la sonde FULL sur l'appareil ; lecture stricte de toute la sortie. schema="sequentiel" :
    voie --sequentiel (resolve_tower puis build_forests), etage G, tables et resolution par passe."""
    argv = [binaire, "--trame=%s,%s,%s" % (info["xyz"], info["ids"], trame), "--k=%d" % k,
            "--threads=%d" % args.fils, "--passes=%d" % passes, "--device"] + (["--digest"] if digest else []) + \
        (["--sequentiel"] if schema == "sequentiel" else [])
    code, sortie, _err, _s = bf.run(argv, args.delai, raw_out=journal)
    lu = lf.parse_output(code, sortie, attendu(trame, info["sites"], k, args.fils, passes, digest, schema))
    out = {"journal": os.path.relpath(journal, args.sortie), "journal_sha256": bf.sha256_file(journal),
           "code": code if type(code) is int else str(code), "etat": lu["etat"], "raison": lu["raison"],
           "valide": lu["etat"] == "ok"}
    if out["valide"] and schema == "sequentiel":
        out.update({"murs_ns": [p["wall_ns"] for p in lu["passes"]],
                    "g_ns": [p["etapes_ns"]["G"] for p in lu["passes"]],
                    "tables_ns": [p["g_ns"]["tables"] for p in lu["passes"]],
                    "resolution_ns": [p["g_ns"]["resolution"] for p in lu["passes"]]})
        out["mur_ns"] = mediane(out["murs_ns"][1:])
        out["g_median_ns"] = mediane(out["g_ns"][1:])
    elif out["valide"]:
        out.update(resume(lu["passes"]))
        out["mur_ns"] = mediane(out["murs_ns"][1:])
    return out


def etape_identite(args, rapport):
    binaires = {n: b["chemin"] for n, b in rapport["construction"]["binaires"].items()}
    infos = trames(args, rapport)
    ident = {"debut": maintenant(), "trames": {}}
    for t in TRAMES:
        dossier = os.path.join(args.sortie, "journaux", "identite", t)
        os.makedirs(dossier, exist_ok=True)
        ident["trames"][t] = {b: prise(args, binaires[b], t, infos[t], 5, 2, os.path.join(dossier, "%s.jsonl" % b),
                                       True) for b in BRAS_CONSTRUITS}
    ident["fin"] = maintenant()
    rapport["identite"] = ident


def etape_campagne(args, rapport):
    binaires = {n: b["chemin"] for n, b in rapport["construction"]["binaires"].items()}
    infos = trames(args, rapport)
    camp = {"debut": maintenant(), "fils": args.fils, "passes": args.passes, "tours_demandes": args.processus,
            "trames": {}}
    for t in TRAMES:
        dossier = os.path.join(args.sortie, "journaux", "k5", t)
        os.makedirs(dossier, exist_ok=True)
        tours = []
        for tour in range(args.processus):
            n = len(BRAS_JUGES)
            ligne = [(tour + i) % n for i in range(n)]  # decale d'un bras par tour, sans inversion
            res = {}
            for i in ligne:
                b = BRAS_JUGES[i]
                res[b] = prise(args, binaires[b], t, infos[t], 5, args.passes,
                               os.path.join(dossier, "%s_t%02d.jsonl" % (b, tour)), False)
            tours.append(res)
        camp["trames"][t] = tours
    camp["fin"] = maintenant()
    camp["binaires_apres"] = {n: bf.sha256_file(b) for n, b in binaires.items()}
    rapport["campagne_k5"] = camp
    # GPU vide apres la campagne : releve avant le verdict (la regle l'exige), pas apres les informations.
    rapport.setdefault("environnement", {})["apres"] = bf.environment(rapport["environnement"].get("nvcc"), True)


def etape_informations(args, rapport):
    binaires = {n: b["chemin"] for n, b in rapport["construction"]["binaires"].items()}
    infos = trames(args, rapport)
    info = {"debut": maintenant(), "k10": {}, "grande": []}
    dossier = os.path.join(args.sortie, "journaux", "grande", GRANDE)
    os.makedirs(dossier, exist_ok=True)
    for tour in range(3):  # la plus grande trame : information (queue du noyau de l'ordre 5, chantier A6)
        n = len(BRAS_JUGES)
        res = {}
        for i in [(tour + j) % n for j in range(n)]:
            b = BRAS_JUGES[i]
            res[b] = prise(args, binaires[b], GRANDE, infos[GRANDE], 5, args.passes,
                           os.path.join(dossier, "%s_t%02d.jsonl" % (b, tour)), False)
        info["grande"].append(res)
    for t in ("ng00", "ng01", "ng02"):
        dossier = os.path.join(args.sortie, "journaux", "k10", t)
        os.makedirs(dossier, exist_ok=True)
        info["k10"][t] = {b: prise(args, binaires[b], t, infos[t], 10, 5, os.path.join(dossier, "%s.jsonl" % b), False)
                          for b in ("avant", "apres")}
    info["sequentiel"] = etape_sequentiel(args, binaires, infos)
    info["environnement_fin"] = bf.environment(rapport.get("environnement", {}).get("nvcc"), True)
    info["fin"] = maintenant()
    rapport["informations"] = info


def etape_sequentiel(args, binaires, infos):
    """Information : G sequentiel (voie --sequentiel) par bras de SEQ_BRAS, 3 tours, ordre tournant sans inversion."""
    out = {}
    for t in SEQ_TRAMES:
        dossier = os.path.join(args.sortie, "journaux", "sequentiel", t)
        os.makedirs(dossier, exist_ok=True)
        tours = []
        for tour in range(3):
            n = len(SEQ_BRAS)
            res = {}
            for i in [(tour + j) % n for j in range(n)]:
                b = SEQ_BRAS[i]
                res[b] = prise(args, binaires[b], t, infos[t], 5, args.passes,
                               os.path.join(dossier, "%s_t%02d.jsonl" % (b, tour)), False, "sequentiel")
            tours.append(res)
        out[t] = tours
    return out


# ---- Juge ------------------------------------------------------------------------------------------------------------
def juger_identite(ident):
    """'absente' (refus), 'fausse' (rejet) ou 'etablie', avec les empreintes par trame."""
    if not isinstance(ident, dict) or set(ident.get("trames", {})) != set(TRAMES):
        return "absente", {}
    empreintes, etat = {}, "etablie"
    for t in TRAMES:
        prises = ident["trames"][t]
        if set(prises) != set(BRAS_CONSTRUITS) or not all(p.get("valide") for p in prises.values()):
            return "absente", empreintes
        tous = sorted({e for p in prises.values() for e in p.get("ful1", [])})
        empreintes[t] = [e[:16] for e in tous]
        if len(tous) != 1 or (t == "ng00" and tous[0] != FUL1_NG00_K5):
            etat = "fausse"
    return etat, empreintes


def _juger_admis(rapport, sortie, verifier_journaux=True):
    refus, cas, verdicts = [], {}, {}
    construction = rapport.get("construction", {}).get("binaires", {})
    camp = rapport.get("campagne_k5")
    if not construction or any(b not in construction for b in BRAS_JUGES):
        refus.append("construction absente ou incomplete")
    env = rapport.get("environnement", {})
    for quand in ("avant", "apres"):
        if not bf.environment_ok(env.get(quand, {})):
            refus.append("environnement %s incomplet ou GPU occupe" % quand)
    identite, empreintes = juger_identite(rapport.get("identite"))
    if identite == "absente":
        refus.append("identite FUL1 absente ou illisible")
    if not camp:
        refus.append("campagne K5 absente")
        return {"verdicts": {x[0]: "refuse" for x in LEVIERS}, "refus": refus, "regle": REGLE_T2D_B2, "cas": cas,
                "identite": {"etat": identite, "empreintes": empreintes}}
    if any(camp.get("binaires_apres", {}).get(n) != construction.get(n, {}).get("sha256") for n in construction):
        refus.append("binaire change pendant la campagne")
    if camp.get("passes", 0) < REGLE_T2D_B2["passes_min"]:
        refus.append("moins de %d passes par processus" % REGLE_T2D_B2["passes_min"])
    rng = random.Random(REGLE_T2D_B2["graine"])
    for t in REGLE_T2D_B2["trames_decisives"]:
        tours = camp.get("trames", {}).get(t, [])
        valides = [x for x in tours if all(b in x and x[b].get("valide") for b in BRAS_JUGES)]
        exigees = max(REGLE_T2D_B2["processus_min"], camp.get("tours_demandes", 0))
        if len(valides) != len(tours) or len(valides) < exigees:
            refus.append("%s : %d tours valides sur %d exiges" % (t, len(valides), exigees))
            if len(valides) < 2:
                continue
        if verifier_journaux:
            for x in valides:
                for b in BRAS_JUGES:
                    chemin = os.path.join(sortie, x[b]["journal"])
                    if not os.path.isfile(chemin) or bf.sha256_file(chemin) != x[b]["journal_sha256"]:
                        refus.append("%s : journal absent ou modifie (%s)" % (t, x[b]["journal"]))
        cas[t] = {"tours": len(valides), "mur_ms_median": {b: mediane([x[b]["mur_ns"] for x in valides]) / 1e6
                                                           for b in BRAS_JUGES}}
        for nom, avant, apres in LEVIERS + INFORMATIFS + (CONTROLE_AA,):
            logs = [math.log(x[apres]["mur_ns"] / x[avant]["mur_ns"]) for x in valides]
            gm, bas, haut = bootstrap_gm(logs, rng, REGLE_T2D_B2["bootstrap"])
            cas[t][nom] = {"moyenne_geometrique": gm, "ic95": [bas, haut], "rapports": [math.exp(v) for v in logs]}
    aa = [cas[t][CONTROLE_AA[0]]["moyenne_geometrique"] for t in cas if CONTROLE_AA[0] in cas[t]]
    controle = {"moyennes": aa, "dans_la_fenetre": all(abs(x - 1) <= REGLE_T2D_B2["fenetre_aa"] for x in aa)}
    if aa and not controle["dans_la_fenetre"]:
        refus.append("controle A/A hors de la fenetre de +/- %.1f %%" % (100 * REGLE_T2D_B2["fenetre_aa"]))
    for nom, _, _ in LEVIERS:
        if refus:
            verdicts[nom] = "refuse"
        elif identite != "etablie":
            verdicts[nom] = "rejete"
        else:
            hautes = [cas[t][nom]["ic95"][1] for t in REGLE_T2D_B2["trames_decisives"]]
            verdicts[nom] = "adopte" if all(h < REGLE_T2D_B2["seuil_borne_haute"] for h in hautes) else "rejete"
    return {"verdicts": verdicts, "refus": refus, "regle": REGLE_T2D_B2, "cas": cas, "controle_aa": controle,
            "identite": {"etat": identite, "empreintes": empreintes}}


def juger(rapport, sortie, verifier_journaux=True):
    """Rejeu strict : chaque journal re-hache puis relu par lecteur_full avec l'attendu de sa place."""
    if not verifier_journaux:
        return _juger_admis(rapport, sortie, verifier_journaux=False)
    try:
        admis = json.loads(json.dumps(rapport, allow_nan=False))
        camp = admis["campagne_k5"]
        fils, passes = camp["fils"], camp["passes"]
        exiger(type(fils) is int and type(passes) is int and passes >= 2, "configuration invalide")
        exiger(set(camp["trames"]) == set(TRAMES), "cohorte de trames incomplete ou inconnue")
        exiger(admis["construction"]["bras_sha256"] == bf.sha256_file(BRAS_FICHIER), "bras_t2d_b2.json change")
        racine, vus = os.path.realpath(sortie), set()
        for t in TRAMES:
            sites = admis["trames"][t]["sites"]
            for tour in camp["trames"][t]:
                exiger(set(tour) == set(BRAS_JUGES), "tour incomplet")
                for b in BRAS_JUGES:
                    p = tour[b]
                    if not p.get("valide"):
                        continue
                    chemin = os.path.realpath(os.path.join(racine, p["journal"]))
                    exiger(os.path.commonpath((racine, chemin)) == racine and chemin not in vus,
                           "journal externe ou reutilise")
                    vus.add(chemin)
                    exiger(bf.sha256_file(chemin) == p["journal_sha256"], "journal modifie")
                    with open(chemin, encoding="utf-8", errors="replace") as f:
                        texte = f.read()
                    code = int(p["code"]) if str(p["code"]).lstrip("-").isdigit() else p["code"]
                    lu = lf.parse_output(code, texte, attendu(t, sites, 5, fils, passes, False))
                    exiger(lu["etat"] == "ok", "prise relue illisible : " + lu["raison"])
                    r = resume(lu["passes"])
                    exiger(r["murs_ns"] == p["murs_ns"] and mediane(r["murs_ns"][1:]) == p["mur_ns"],
                           "resume different du brut")
        return _juger_admis(admis, sortie, verifier_journaux=True)
    except (ValueError, TypeError, KeyError, OSError, UnicodeError, OverflowError) as erreur:
        return {"verdicts": {nom: "refuse" for nom, _, _ in LEVIERS},
                "refus": ["admission des journaux : " + str(erreur)], "regle": REGLE_T2D_B2, "cas": {}}


def tableaux(rapport, jugement):
    out = ["# Chantier T2-d-B2 : mur FULL par levier (genere par pilote_t2d_b2.py)", "",
           "Verdicts : " + ", ".join("%s **%s**" % (n, v) for n, v in jugement["verdicts"].items()), "",
           "Identite FUL1 : %s" % json.dumps(jugement.get("identite", {}), sort_keys=True), ""]
    if jugement["refus"]:
        out += ["Refus : " + " ; ".join(jugement["refus"]), ""]
    out += ["| trame | " + " | ".join("mur %s (ms)" % b for b in BRAS_JUGES) + " |",
            "|" + " --- |" * (1 + len(BRAS_JUGES))]
    for t, c in jugement["cas"].items():
        out.append("| %s | %s |" % (t, " | ".join("%.2f" % c["mur_ms_median"][b] for b in BRAS_JUGES)))
    paires = LEVIERS + INFORMATIFS + (CONTROLE_AA,)
    out += ["", "| trame | " + " | ".join("%s (IC 95 %%)" % x[0] for x in paires) + " |",
            "|" + " --- |" * (1 + len(paires))]
    for t, c in jugement["cas"].items():
        out.append("| %s | %s |" % (t, " | ".join("%.3f (%.3f-%.3f)" % (
            c[x[0]]["moyenne_geometrique"], c[x[0]]["ic95"][0], c[x[0]]["ic95"][1]) for x in paires)))
    out += ["", "Informations (jamais jugees) :", "", "```json",
            json.dumps(rapport.get("resume_informations", {}), indent=1, sort_keys=True), "```"]
    return "\n".join(out) + "\n"


def resume_informations(rapport):
    """Medianes par bras et trame de la fin de G, de l'ouverture, des tables et du mur (campagne et plus grande trame),
    K10 (avant, apres), G sequentiel (medianes et moyenne geometrique des rapports par tour, bras sur avant)."""
    try:
        out = {"campagne": {}, "k10": {}}
        trames_lues = dict(rapport.get("campagne_k5", {}).get("trames", {}))
        trames_lues[GRANDE] = rapport.get("informations", {}).get("grande", [])
        for t, tours in trames_lues.items():
            out["campagne"][t] = {}
            for b in BRAS_JUGES:
                ok = [x[b] for x in tours if x.get(b, {}).get("valide")]
                if ok:
                    out["campagne"][t][b] = {c: mediane([mediane(p[c + "_ns"][1:]) for p in ok]) / 1e6
                                             for c in ("fin_g", "ouverture", "tables", "murs")}
        for t, prises in rapport.get("informations", {}).get("k10", {}).items():
            out["k10"][t] = {b: (p["mur_ns"] / 1e6 if p.get("valide") else p.get("raison")) for b, p in prises.items()}
        out["sequentiel"] = {}
        for t, tours in rapport.get("informations", {}).get("sequentiel", {}).items():
            par_bras = {}
            for b in SEQ_BRAS:
                ok = [x[b] for x in tours if x.get(b, {}).get("valide")]
                if ok:
                    par_bras[b] = {"g_ms": mediane([p["g_median_ns"] for p in ok]) / 1e6,
                                   "mur_ms": mediane([p["mur_ns"] for p in ok]) / 1e6}
            rapports = {}
            for b in SEQ_BRAS[1:]:
                logs = [math.log(x[b]["g_median_ns"] / x["avant"]["g_median_ns"]) for x in tours
                        if x.get(b, {}).get("valide") and x.get("avant", {}).get("valide")]
                if logs:
                    rapports[b] = math.exp(sum(logs) / len(logs))
            out["sequentiel"][t] = {"medianes": par_bras, "g_bras_sur_avant_moyenne_geometrique": rapports}
        return out
    except (ValueError, TypeError, KeyError, AttributeError, ZeroDivisionError) as erreur:
        return {"erreur_du_resume": str(erreur)}


def etape_rapport(args, rapport):
    jugement = juger(rapport, args.sortie)
    rapport["jugement"] = jugement
    rapport["resume_informations"] = resume_informations(rapport)
    with open(os.path.join(args.sortie, "tableaux_t2d_b2.md"), "w", encoding="utf-8") as f:
        f.write(tableaux(rapport, jugement))
    return jugement


# ---- Auto-test du juge -----------------------------------------------------------------------------------------------
def campagne_synthetique(effets, identite="etablie", manque=False, graine=1):
    rng = random.Random(graine)
    env = {"cmake": "x", "nvcc": "x", "gpu": "x", "gpu_apps": ""}
    rapport = {"construction": {"binaires": {b: {"sha256": "x"} for b in BRAS_JUGES}},
               "environnement": {"avant": env, "apres": env},
               "campagne_k5": {"passes": 10, "tours_demandes": 10, "binaires_apres": {b: "x" for b in BRAS_JUGES},
                               "trames": {}}}
    if identite != "absente":
        ful = {t: ["ab" * 32] for t in TRAMES}
        ful["ng00"] = [FUL1_NG00_K5]
        if identite == "fausse":
            ful["ng01"] = ["ab" * 32, "cd" * 32]
        rapport["identite"] = {"trames": {t: {b: {"valide": True, "ful1": ful[t]} for b in BRAS_CONSTRUITS}
                                          for t in TRAMES}}
    for t in TRAMES:
        tours = []
        for x in range(10):
            base = 150e6 * (1 + 0.02 * rng.random())
            tour = {}
            for b in BRAS_JUGES:
                tour[b] = {"valide": not (manque and b == "tables" and x == 5), "journal": "", "journal_sha256": "",
                           "mur_ns": base * effets.get(b, 1.0) * (1 + 0.004 * (rng.random() - 0.5))}
            tours.append(tour)
        rapport["campagne_k5"]["trames"][t] = tours
    return rapport


def etape_auto_test():
    tous = lambda v: {x[0]: v for x in LEVIERS}  # noqa: E731
    tous_bras = lambda r: {"apres": r, "tables": r, "relecture": r, "census": r}  # noqa: E731
    attendus = [({"apres": 0.95, "tables": 0.98, "relecture": 0.97, "census": 0.96}, "etablie", False, tous("adopte")),
                ({"apres": 0.97, "tables": 1.0, "relecture": 0.97, "census": 1.003}, "etablie", False,
                 {"lot_b2": "adopte", "tables": "rejete", "relecture": "adopte", "census": "rejete"}),
                ({"apres": 1.02, "tables": 1.0, "relecture": 1.01, "census": 1.0}, "etablie", False, tous("rejete")),
                (tous_bras(0.9), "fausse", False, tous("rejete")),
                (tous_bras(0.9), "absente", False, tous("refuse")),
                (tous_bras(0.9), "etablie", True, tous("refuse")),
                (dict(tous_bras(0.9), avant_bis=1.2), "etablie", False, tous("refuse"))]
    ecarts = []
    for i, (effets, identite, manque, voulu) in enumerate(attendus):
        jugement = juger(campagne_synthetique(effets, identite, manque, graine=i + 1), "", verifier_journaux=False)
        if jugement["verdicts"] != voulu:
            ecarts.append("cas %d : %s au lieu de %s" % (i, jugement["verdicts"], voulu))
    occupe = campagne_synthetique(tous_bras(0.9), graine=9)
    occupe["environnement"]["apres"] = {"cmake": "x", "nvcc": "x", "gpu": "x", "gpu_apps": "1234"}
    if juger(occupe, "", verifier_journaux=False)["verdicts"] != tous("refuse"):
        ecarts.append("GPU occupe apres la campagne non refuse")
    for e in ecarts:
        print("pilote_t2d_b2_auto_test_ecart " + e)
    if ecarts:
        return 1
    print("pilote_t2d_b2_auto_test_ok cas=%d verdicts=adopte,rejete,refuse" % (len(attendus) + 1))
    return 0


def main(argv):
    p = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    p.add_argument("etapes", nargs="+", choices=("auto-test", "construire", "identite", "campagne", "informations",
                                                 "rapport", "tout"))
    p.add_argument("--src", help="racine du paquet (contient morsehgp3D_v12/)")
    p.add_argument("--travail", help="dossier des constructions des bras")
    p.add_argument("--donnees", help="dossier des trames et des archives")
    p.add_argument("--sortie", help="dossier des journaux et du rapport")
    p.add_argument("--avant-archive")
    p.add_argument("--avant-sha256")
    p.add_argument("--nvcc", default="")
    p.add_argument("--fils", type=int, default=48)
    p.add_argument("--jobs", type=int, default=8)
    p.add_argument("--processus", type=int, default=10)
    p.add_argument("--passes", type=int, default=10)
    p.add_argument("--delai", type=int, default=600, help="delai d'un processus de la sonde, en secondes")
    p.add_argument("--essai", action="store_true", help="essai : moins de tours ou de passes admis (jamais decisif)")
    args = p.parse_args(argv[1:])
    etapes = ["auto-test", "construire", "identite", "campagne", "rapport", "informations", "rapport"] \
        if "tout" in args.etapes else args.etapes
    if etapes == ["auto-test"]:
        return etape_auto_test()
    if not args.sortie or not args.travail or not args.donnees or (
            "construire" in etapes and not (args.avant_archive and args.avant_sha256)):
        print("pilote_t2d_b2_refus usage : --sortie --travail --donnees, et pour construire --avant-archive "
              "--avant-sha256")
        return 2
    if not args.essai and (args.processus < REGLE_T2D_B2["processus_min"] or
                           args.passes < REGLE_T2D_B2["passes_min"]):
        print("pilote_t2d_b2_refus usage : au moins %d processus et %d passes" % (REGLE_T2D_B2["processus_min"],
                                                                                REGLE_T2D_B2["passes_min"]))
        return 2
    os.makedirs(args.sortie, exist_ok=True)
    chemin = os.path.join(args.sortie, "rapport_t2d_b2.json")
    rapport = {"schema": "ehgp.v12.t2d_b2_pilote.v1", "regle": REGLE_T2D_B2}
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
            elif etape == "identite":
                etape_identite(args, rapport)
            elif etape == "campagne":
                etape_campagne(args, rapport)
            elif etape == "informations":
                etape_informations(args, rapport)
            elif etape == "rapport":
                if etape_auto_test() != 0:
                    return 1
                jugement = etape_rapport(args, rapport)
                print("pilote_t2d_b2_verdicts " + " ".join("%s=%s" % kv for kv in jugement["verdicts"].items()))
                code = 3 if jugement["refus"] else 0
            sauver()  # un delai de la commande garde les etapes deja jouees
    except (RuntimeError, KeyError, OSError, ValueError) as e:
        print("pilote_t2d_b2_refus %s" % e)
        code = 3
    sauver()
    return code


if __name__ == "__main__":
    sys.exit(main(sys.argv))
