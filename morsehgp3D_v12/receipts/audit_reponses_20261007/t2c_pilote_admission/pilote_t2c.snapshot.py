#!/usr/bin/env python3
"""Pilote et juge de la tranche T2-c (cout de l'etage G du produit, voie CPU) : avant / apres en processus alternes.
Bibliotheque standard seule, jouable sous python3 -S -O (aucune garde par assert).

Bras (sondes mhgp12_tower_probe construites ici, memes options : Release, MHGP12_MODULES=tower) :
  avant      archive epinglee des sources de la base : HEAD de main + patch 1 de T2-c (empreinte de l'objet seul),
             sha256 verifie avant deballage ; meme definition de l'empreinte que les autres bras
  avant_bis  le MEME binaire que avant, joue comme un bras distinct (bras A/A : bruit de la session, publie)
  apres      les sources du paquet ({src}) : index des naissances parallele et canonique, file de sondes prechargees
             (G-L7, voie produit), table S* a fiches contigues, preparation parallele, frontieres des chronos
  sans_gl7   copie de {src} dont la file a une seule place (ablation de G-L7 : chaque sonde est jouee aussitot)
  gl5        copie de {src} dont les premieres sondes passent par la jointure triee (G-L5) au lieu de la file
  profil     {src} construit avec -DMHGP12_TOWER_PROFILE (profil par composante, publie, jamais juge)

Etapes (dans l'ordre si plusieurs) :
  auto-test     le juge sur des campagnes synthetiques (adopte, rejete, refuse) ; tout autre resultat : code 1
  construire    deballage, copies, substitutions (chaque motif present une seule fois), configuration, construction ;
                empreintes des binaires
  campagne      K5, ng00 ng01 ng02, --fils fils (48 sur G4) : --processus tours ; dans chaque tour, un processus neuf
                par bras, dans un ordre tournant (decale d'un bras par tour, a rebours un tour sur deux) ; chaque
                processus joue --passes passes de l'etage G (la premiere, a froid, est ecartee) et l'empreinte de
                l'objet
  informations  non jugees : K10 a --fils fils (avant, apres) ; un fil sur ng00 K5 (avant, sans_gl7, apres, gl5) ;
                uniformes de 8 000, 16 000 et 32 000 sites a K5 (empreintes des portes d'echelle) ; profil
  rapport       juge (REGLE_T2C), rapport JSON et tableaux Markdown
  tout          auto-test construire campagne informations rapport

REGLE_T2C (ecrite le 7 octobre 2026, avant toute mesure sur G4) : par processus, temps de l'etage G = mediane des
passes 2 a P du mur de resolve_tower (wall_ns de la ligne tour_g) ; par tour, rapport apres / avant de deux bras ;
moyenne geometrique des rapports des tours et IC 95 % par bootstrap sur les tours (10 000 tirages, graine 20261007),
par trame. Un levier est ADOPTE si l'empreinte de l'objet est identique pour tous les processus de tous les bras de
chaque trame (et egale, sur ng00, a celle de la porte mhgp12_tower_determinism_lidar_ng00_k5) ET si la borne haute de
l'IC est sous 1 sur CHACUNE des trames ng00, ng01, ng02 a K5 ; REJETE sinon ; REFUSE si une prise manque (processus en
echec, passes ou empreinte absentes, moins de --processus tours valides par trame), si un binaire change pendant la
campagne, si un journal manque ou a change, ou si l'auto-test du juge echoue. Leviers juges : « lot_t2c » (avant ->
apres : la tranche entiere, regle 8 de MESURE.md), « G-L7 » (sans_gl7 -> apres), « index_et_corrections » (avant ->
sans_gl7) et « G-L5_au_lieu_de_G-L7 » (apres -> gl5 : adopte seulement si la jointure triee bat la file). Le bras A/A
(avant -> avant_bis) est publie avec son IC (fenetre de +/- 1,5 % de MESURE.md, regle 6) et ne change aucun verdict.

Exemple (G4, commande d'un plan de session ; en local : --fils 3) :
  python3 pilote_t2c.py --src <racine du paquet> --travail <dossier> --donnees <donnees> --sortie <sortie> \\
      --avant-archive <donnees>/v12_src_9c5809919.tar.gz --avant-sha256 <hex> --fils 48 --jobs 44 tout

Codes : 0 campagne complete et jugee (le verdict est dans le rapport, il ne change pas le code) ; 1 auto-test en echec ;
2 usage ; 3 refus (preuve absente, binaire change, construction en echec, prise manquante).
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

ICI = os.path.dirname(os.path.abspath(__file__))
SONDE = "mhgp12_tower_probe"
TRAMES = ("ng00", "ng01", "ng02")
EMPREINTE_NG00_K5 = "231d826bb0d4fe57"  # ligne gravee de mhgp12_tower_determinism_lidar_ng00_k5
UNIFORMES = {8000: "5304d1c8fe7c25ff", 16000: "fdd42b4e0fba5d08", 32000: "9fd0fd9e98fa5305"}  # mhgp12_tower_scale*
BRAS_JUGES = ("avant", "avant_bis", "sans_gl7", "apres", "gl5")
LEVIERS = (("lot_t2c", "avant", "apres"), ("G-L7", "sans_gl7", "apres"), ("index_et_corrections", "avant", "sans_gl7"),
           ("G-L5_au_lieu_de_G-L7", "apres", "gl5"))
CONTROLE_AA = ("A/A", "avant", "avant_bis")
# Bras par substitution exacte (chaque motif present une seule fois) d'une copie de {src}.
ABLATIONS = {
    "sans_gl7": [{"fichier": "src/tower/passes.cpp",
                  "cherche": "  static constexpr u64 kLag = 16, kHalf = kLag / 2;\n",
                  "remplace": "  static constexpr u64 kLag = 1, kHalf = 0;  // ablation de G-L7 (pilote_t2c.py)\n"}],
    "gl5": [{"fichier": "src/tower/passes.cpp",
             "cherche": "  const std::span<const u32> candidates{};  // G-L7 ; G-L5 : first_probe_candidates (bras du pilote)\n",
             "remplace": ("  MHGP12_TRY(first_probe_candidates(d, order, table, buffers.join, budget, pool));  // bras G-L5\n"
                          "  const std::span<const u32> candidates = buffers.join.candidates.span().first(order.representatives());\n")}]}
REGLE_T2C = {"trames_decisives": TRAMES, "k": 5, "processus_min": 8, "passes_min": 6, "bootstrap": 10000,
             "graine": 20261007, "seuil_borne_haute": 1.0, "fenetre_aa": 0.015,
             "statistique": "par processus : mediane des passes 2..P du mur de l'etage G (wall_ns) ; par tour : "
                            "rapport des deux bras ; moyenne geometrique et IC 95 % par bootstrap sur les tours",
             "adoption": "empreinte identique dans tous les processus de tous les bras de chaque trame (ng00 : celle "
                         "de la porte) et borne haute de l'IC sous 1 sur chacune des trames ng00, ng01, ng02 a K5",
             "leviers": [list(l) for l in LEVIERS], "controle": list(CONTROLE_AA)}


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
    """Moyenne geometrique et IC 95 % par bootstrap (conventions de REGLE_M3 et REGLE_G1)."""
    gm = math.exp(sum(logs) / len(logs))
    boot = []
    for _ in range(tirages):
        echantillon = [logs[rng.randrange(len(logs))] for _ in logs]
        boot.append(sum(echantillon) / len(echantillon))
    boot.sort()
    return gm, math.exp(boot[int(0.025 * tirages)]), math.exp(boot[int(0.975 * tirages) - 1])


def jouer(commande, journal, cwd=None):
    """Execute une commande ; sortie brute dans journal ; rend (code, lignes JSON)."""
    try:
        proc = subprocess.run(commande, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=False)
        code, sortie, erreur = proc.returncode, proc.stdout, proc.stderr
    except OSError as e:
        code, sortie, erreur = 127, "", str(e)
    with open(journal, "w", encoding="utf-8") as f:
        f.write(sortie)
        if erreur:
            f.write("\n# stderr\n" + erreur[-4000:])
    lignes = []
    for ligne in sortie.splitlines():
        if ligne.startswith("{"):
            try:
                lignes.append(json.loads(ligne))
            except json.JSONDecodeError:
                lignes.append({"phase": "ligne_illisible"})
    return code, lignes


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


def substituer(racine, substitutions):
    for s in substitutions:
        chemin = os.path.join(racine, s["fichier"])
        with open(chemin, encoding="utf-8") as f:
            texte = f.read()
        if texte.count(s["cherche"]) != 1:
            raise RuntimeError("motif absent ou multiple dans %s (ablation)" % s["fichier"])
        with open(chemin, "w", encoding="utf-8") as f:
            f.write(texte.replace(s["cherche"], s["remplace"]))


def ignorer_dossiers(dossier, noms):
    """Copie de {src} : seuls les DOSSIERS build* et __pycache__ sont ecartes (src/index/build.cpp est une source)."""
    return [n for n in noms if os.path.isdir(os.path.join(dossier, n)) and (n.startswith("build") or n == "__pycache__")]


def construire_bras(nom, source, travail, jobs, journaux, drapeaux=()):
    build = os.path.join(travail, nom, "build")
    os.makedirs(build, exist_ok=True)
    configure = ["cmake", "-S", source, "-B", build, "-DCMAKE_BUILD_TYPE=Release", "-DMHGP12_MODULES=tower"]
    configure += list(drapeaux)
    code, _ = jouer(configure, os.path.join(journaux, "configure_%s.txt" % nom))
    if code == 0:
        code, _ = jouer(["cmake", "--build", build, "-j", str(jobs), "--target", SONDE],
                        os.path.join(journaux, "build_%s.txt" % nom))
    binaire = os.path.join(build, SONDE)
    if code != 0 or not os.path.isfile(binaire):
        raise RuntimeError("construction du bras %s en echec (journal build_%s.txt)" % (nom, nom))
    return binaire


def etape_construire(args, rapport):
    travail = os.path.abspath(args.travail)
    journaux = os.path.join(args.sortie, "construction")
    os.makedirs(journaux, exist_ok=True)
    src_v12 = os.path.join(args.src, "morsehgp3D_v12")
    sources = {}
    base = os.path.join(travail, "avant", "src")
    if os.path.isdir(base):
        shutil.rmtree(base)
    os.makedirs(base)
    sources["avant"] = deballer(args.avant_archive, args.avant_sha256, base)
    sources["apres"] = src_v12
    for nom, substitutions in ABLATIONS.items():
        copie = os.path.join(travail, nom, "src", "morsehgp3D_v12")
        if os.path.isdir(copie):
            shutil.rmtree(copie)
        shutil.copytree(src_v12, copie, ignore=ignorer_dossiers)
        substituer(copie, substitutions)
        sources[nom] = copie
    binaires = {}
    for nom in ("avant", "apres", "sans_gl7", "gl5"):
        binaires[nom] = construire_bras(nom, sources[nom], travail, args.jobs, journaux)
    binaires["profil"] = construire_bras("profil", src_v12, travail, args.jobs, journaux,
                                         ["-DCMAKE_CXX_FLAGS=-DMHGP12_TOWER_PROFILE"])
    binaires["avant_bis"] = binaires["avant"]
    rapport["construction"] = {"debut": maintenant(), "archive_avant_sha256": args.avant_sha256,
                               "binaires": {n: {"chemin": b, "sha256": sha256(b)} for n, b in binaires.items()},
                               "ablations": ABLATIONS}


# ---- Campagnes ----------------------------------------------------------------------------------------------------------
def entree(args, trame):
    if trame.startswith("u"):
        return ["--uniform=%s,20261007,18" % trame[1:]]
    stem = os.path.join(args.donnees, "lidar_%s" % trame)
    return [stem + ".u32le", stem + ".ids.u32le"]


def prise(args, binaire, trame, k, fils, passes, journal):
    """Un processus neuf de la sonde : passes de l'etage G, empreinte ; rend un dictionnaire de prise."""
    commande = [binaire] + entree(args, trame) + ["--k=%d" % k, "--threads=%d" % fils, "--passes=%d" % passes,
                                                  "--digest", "--frame=%s" % trame]
    code, lignes = jouer(commande, journal)
    tours = [l for l in lignes if l.get("phase") == "tour_g"]
    empreinte = [l.get("resolution_sha256") for l in lignes if l.get("phase") == "digest"]
    fin = [l for l in lignes if l.get("phase") == "exit"]
    murs = [t.get("wall_ns") for t in tours if t.get("status") == "ok" and isinstance(t.get("wall_ns"), int)]
    valide = code == 0 and len(tours) == passes and len(murs) == passes and len(empreinte) == 1 and \
        len(fin) == 1 and fin[0].get("status") == "ok" and passes >= 2
    out = {"journal": os.path.relpath(journal, args.sortie), "journal_sha256": sha256(journal), "code": code,
           "valide": valide, "murs_ns": murs, "empreinte": empreinte[0] if empreinte else None,
           "profil": [l for l in lignes if l.get("phase") == "profil_g"]}
    if valide:
        out["g_ns"] = mediane(murs[1:])
        out["diagnostics"] = tours[-1].get("diagnostics")
    return out


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
            ligne = [(t + i) % n for i in range(n)]
            if t % 2:
                ligne.reverse()
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
    info = {"debut": maintenant()}
    plans = [("k10_w%d" % args.fils, TRAMES, 10, args.fils, 3, ("avant", "apres"), 3),
             ("k5_w1_ng00", ("ng00",), 5, 1, 3, ("avant", "sans_gl7", "apres", "gl5"), 3),
             ("uniformes_k5_w%d" % args.fils, ("u8000", "u16000", "u32000"), 5, args.fils, 5, ("avant", "apres"), 1),
             ("profil_k5", ("ng00",), 5, 1, 3, ("profil",), 1),
             ("profil_k5_w%d" % args.fils, ("ng00",), 5, args.fils, 5, ("profil",), 1)]
    for nom, trames, k, fils, passes, bras, tours in plans:
        res = {}
        for trame in trames:
            dossier = os.path.join(args.sortie, "journaux", nom, trame)
            os.makedirs(dossier, exist_ok=True)
            res[trame] = []
            for t in range(tours):
                ordre = bras if t % 2 == 0 else tuple(reversed(bras))
                res[trame].append({b: prise(args, binaires[b], trame, k, fils, passes,
                                            os.path.join(dossier, "%s_t%02d.jsonl" % (b, t))) for b in ordre})
        info[nom] = {"k": k, "fils": fils, "passes": passes, "tours": res}
    info["fin"] = maintenant()
    rapport["informations"] = info


# ---- Juge ---------------------------------------------------------------------------------------------------------------
def juger(rapport, sortie, verifier_journaux=True):
    """Verdicts de REGLE_T2C ; journaux re-haches (un journal modifie est refuse)."""
    refus, cas, verdicts = [], {}, {}
    construction = rapport.get("construction", {}).get("binaires", {})
    camp = rapport.get("campagne_k5")
    if not construction or any(b not in construction for b in BRAS_JUGES):
        refus.append("construction absente ou incomplete")
    if not camp:
        refus.append("campagne K5 absente")
        return {"verdicts": {l[0]: "refuse" for l in LEVIERS}, "refus": refus, "regle": REGLE_T2C, "cas": cas}
    if any(camp.get("binaires_apres", {}).get(n) != construction.get(n, {}).get("sha256") for n in construction):
        refus.append("binaire change pendant la campagne")
    if camp.get("passes", 0) < REGLE_T2C["passes_min"]:
        refus.append("moins de %d passes par processus" % REGLE_T2C["passes_min"])
    rng = random.Random(REGLE_T2C["graine"])
    empreintes_ok = True
    for trame in REGLE_T2C["trames_decisives"]:
        tours = camp.get("trames", {}).get(trame, [])
        valides = [t for t in tours if all(b in t and t[b].get("valide") for b in BRAS_JUGES)]
        exigees = max(REGLE_T2C["processus_min"], camp.get("tours_demandes", 0))
        if len(valides) != len(tours) or len(valides) < exigees:
            refus.append("%s : %d tours valides sur %d exiges" % (trame, len(valides), exigees))
            continue
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
        cas[trame] = {"tours": len(valides), "empreintes": sorted(e[:16] for e in empreintes if e),
                      "empreinte_identique": identique,
                      "g_ms_median": {b: mediane([t[b]["g_ns"] for t in valides]) / 1e6 for b in BRAS_JUGES}}
        for nom, avant, apres in LEVIERS + (CONTROLE_AA,):
            logs = [math.log(t[apres]["g_ns"] / t[avant]["g_ns"]) for t in valides]
            gm, bas, haut = bootstrap_gm(logs, rng, REGLE_T2C["bootstrap"])
            cas[trame][nom] = {"moyenne_geometrique": gm, "ic95": [bas, haut], "rapports": [math.exp(x) for x in logs]}
    for nom, _, _ in LEVIERS:
        if refus:
            verdicts[nom] = "refuse"
        elif not empreintes_ok:
            verdicts[nom] = "rejete"
        else:
            hautes = [cas[t][nom]["ic95"][1] for t in REGLE_T2C["trames_decisives"]]
            verdicts[nom] = "adopte" if all(h < REGLE_T2C["seuil_borne_haute"] for h in hautes) else "rejete"
    aa = [cas[t][CONTROLE_AA[0]]["moyenne_geometrique"] for t in cas if CONTROLE_AA[0] in cas[t]]
    controle = {"moyennes": aa, "dans_la_fenetre": all(abs(x - 1) <= REGLE_T2C["fenetre_aa"] for x in aa)}
    return {"verdicts": verdicts, "refus": refus, "regle": REGLE_T2C, "cas": cas, "controle_aa": controle}


def tableaux(rapport, jugement):
    out = ["# Tranche T2-c : etage G avant / apres (genere par pilote_t2c.py)", "",
           "Verdicts : " + ", ".join("%s **%s**" % (n, v) for n, v in jugement["verdicts"].items()), ""]
    if jugement["refus"]:
        out += ["Refus : " + " ; ".join(jugement["refus"]), ""]
    out += ["| trame | " + " | ".join("G %s (ms)" % b for b in BRAS_JUGES) + " | " +
            " | ".join("%s (IC 95 %%)" % l[0] for l in LEVIERS + (CONTROLE_AA,)) + " | empreinte |",
            "|" + " --- |" * (2 + len(BRAS_JUGES) + len(LEVIERS))]
    for trame, c in jugement["cas"].items():
        if "g_ms_median" not in c:
            continue
        cellules = ["%.2f" % c["g_ms_median"][b] for b in BRAS_JUGES]
        cellules += ["%.3f (%.3f-%.3f)" % (c[l[0]]["moyenne_geometrique"], c[l[0]]["ic95"][0], c[l[0]]["ic95"][1])
                     for l in LEVIERS + (CONTROLE_AA,)]
        out.append("| %s | %s | %s |" % (trame, " | ".join(cellules), ",".join(c["empreintes"])))
    return "\n".join(out) + "\n"


def etape_rapport(args, rapport):
    jugement = juger(rapport, args.sortie)
    rapport["jugement"] = jugement
    with open(os.path.join(args.sortie, "tableaux_t2c.md"), "w", encoding="utf-8") as f:
        f.write(tableaux(rapport, jugement))
    return jugement


# ---- Auto-test du juge --------------------------------------------------------------------------------------------------
def campagne_synthetique(effets, empreinte_differente=False, manque=False, graine=1):
    rng = random.Random(graine)
    rapport = {"construction": {"binaires": {b: {"sha256": "x"} for b in BRAS_JUGES}},
               "campagne_k5": {"passes": 10, "tours_demandes": 8, "binaires_apres": {b: "x" for b in BRAS_JUGES},
                               "trames": {}}}
    for trame in TRAMES:
        tours = []
        for t in range(8):
            base = 60e6 * (1 + 0.02 * rng.random())
            tour = {}
            for b in BRAS_JUGES:
                g = base * effets.get(b, 1.0) * (1 + 0.004 * (rng.random() - 0.5))
                e = EMPREINTE_NG00_K5 + "0" * 48 if trame == "ng00" else "ab" * 32
                if empreinte_differente and b == "apres" and t == 3:
                    e = "cd" * 32
                tour[b] = {"valide": not (manque and b == "sans_gl7" and t == 5), "g_ns": g, "empreinte": e,
                           "journal": "", "journal_sha256": ""}
            tours.append(tour)
        rapport["campagne_k5"]["trames"][trame] = tours
    return rapport


def etape_auto_test():
    tous = lambda v: {l[0]: v for l in LEVIERS}  # noqa: E731
    attendus = [({"apres": 0.6, "sans_gl7": 0.8, "gl5": 0.7}, False, False,
                 {"lot_t2c": "adopte", "G-L7": "adopte", "index_et_corrections": "adopte",
                  "G-L5_au_lieu_de_G-L7": "rejete"}),
                ({"apres": 0.8, "sans_gl7": 0.8, "gl5": 0.6}, False, False,
                 {"lot_t2c": "adopte", "G-L7": "rejete", "index_et_corrections": "adopte",
                  "G-L5_au_lieu_de_G-L7": "adopte"}),
                ({"apres": 1.05, "sans_gl7": 1.0, "gl5": 1.1}, False, False, tous("rejete")),
                ({"apres": 0.6, "sans_gl7": 0.8, "gl5": 0.5}, True, False, tous("rejete")),
                ({"apres": 0.6, "sans_gl7": 0.8, "gl5": 0.5}, False, True, tous("refuse"))]
    ecarts = []
    for i, (effets, diff, manque, attendu) in enumerate(attendus):
        jugement = juger(campagne_synthetique(effets, diff, manque, graine=i + 1), "", verifier_journaux=False)
        if jugement["verdicts"] != attendu:
            ecarts.append("cas %d : %s au lieu de %s" % (i, jugement["verdicts"], attendu))
    for e in ecarts:
        print("pilote_t2c_auto_test_ecart " + e)
    if ecarts:
        return 1
    print("pilote_t2c_auto_test_ok cas=%d verdicts=adopte,rejete,refuse" % len(attendus))
    return 0


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
    p.add_argument("--processus", type=int, default=8)
    p.add_argument("--passes", type=int, default=10)
    p.add_argument("--essai", action="store_true",
                   help="essai local : moins de tours ou de passes admis ; le juge refuse alors (jamais decisif)")
    args = p.parse_args(argv[1:])
    etapes = ["auto-test", "construire", "campagne", "informations", "rapport"] if "tout" in args.etapes else \
        args.etapes
    if etapes == ["auto-test"]:
        return etape_auto_test()
    if not args.sortie or not args.travail or (("construire" in etapes) and not (args.src and args.avant_archive and
                                                                                 args.avant_sha256)):
        print("pilote_t2c_refus usage : --sortie, --travail, et pour construire --src --avant-archive --avant-sha256")
        return 2
    if not args.essai and (args.processus < REGLE_T2C["processus_min"] or args.passes < REGLE_T2C["passes_min"]):
        print("pilote_t2c_refus usage : au moins %d processus et %d passes" % (REGLE_T2C["processus_min"],
                                                                             REGLE_T2C["passes_min"]))
        return 2
    os.makedirs(args.sortie, exist_ok=True)
    chemin = os.path.join(args.sortie, "rapport_t2c.json")
    rapport = {"schema": "ehgp.v12.t2c_pilote.v1", "regle": REGLE_T2C}
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
                etape_construire(args, rapport)
            elif etape == "campagne":
                etape_campagne(args, rapport)
            elif etape == "informations":
                etape_informations(args, rapport)
            elif etape == "rapport":
                jugement = etape_rapport(args, rapport)
                print("pilote_t2c_verdicts " + " ".join("%s=%s" % kv for kv in jugement["verdicts"].items()))
                code = 3 if jugement["refus"] else 0
    except (RuntimeError, KeyError, OSError) as e:
        print("pilote_t2c_refus %s" % e)
        code = 3
    with open(chemin, "w", encoding="utf-8") as f:
        json.dump(rapport, f, indent=1, sort_keys=True)
    return code


if __name__ == "__main__":
    sys.exit(main(sys.argv))
