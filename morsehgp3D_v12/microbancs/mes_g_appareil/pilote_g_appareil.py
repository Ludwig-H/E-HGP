#!/usr/bin/env python3
"""MES-G-APP : pilote et juge de l'etude de faisabilite de l'etage G sur l'appareil de G4 (hors produit). Bibliotheque
standard seule, Python 3.10, jouable sous python3 -S -O (aucune garde par assert).

Objet : trois postes archetypes de l'etage G, joues sur l'hote par le PRODUIT et sur l'appareil par un noyau en source
unique __host__ __device__ (noyau_g.hpp), sur les MEMES entrees, avec identite exigee a l'octet :
  census        census garde a temoins de chaque boule certifiee que G recense (requetes interceptees dans le produit
                a l'edition de liens, -Wl,--wrap) : parcours d'arbre sans pile, arithmetique exacte i128 ;
  sondes        premiere sonde de la table de populations (LEM-POP) de chaque representant, voie G-L7 du produit :
                acces memoire aleatoires ;
  propositions  proposition flottante (DWelzl, binaire64) des traces dont la premiere sonde echoue : calcul flottant
                double precision et bits identiques entre l'hote et l'appareil (routes, donc compteurs du travail).
Mesure decisive : duree des noyaux de l'appareil, donnees residentes (dans une voie G sur l'appareil, elles seraient
produites sur place par la fin d'etage du catalogue), contre le lot du produit sur l'hote a --fils fils (48 sur G4).

Etapes (dans l'ordre si plusieurs) :
  auto-test   le juge sur des campagnes synthetiques (adopte, rejete, refuse) ; tout autre resultat : code 1
  construire  noyaux CUDA (nvcc, sm_120, -fmad=false) et programme hote lie a <produit>/libmhgp12.a avec interception
              du census (symbole verifie par nm : defini par census_workspace.cpp.o, appele par resolve.cpp.o) ;
              mutant causal << cote nul >> (copie compilee a part) ; empreintes des sources, des sources portees du
              produit et des binaires ; --sans-appareil : construction hote seule (essai local)
  campagne    trames ng00 (39 885 sites), mediane v12set (kitti_ng_02_001606, 64 740) et maximum v12set
              (kitti_ng_08_002119, 99 099) a K5 : --processus processus neufs par trame, entrelaces ; chacun recolte
              les requetes, joue une passe d'echauffement puis --passes passes (chaque mesure deux fois : A/A ; ordre
              hote/appareil alterne), puis compare a l'octet ; GPU isole (nvidia-smi : aucun autre processus de calcul)
              avant et apres chaque processus ; puis le mutant sur ng00, qui doit etre tue (code 1)
  rapport     auto-test, puis juge (REGLE_G_APPAREIL), rapport JSON et tableaux Markdown
  tout        auto-test construire campagne rapport

REGLE_G_APPAREIL (ecrite le 8 octobre 2026 a 10:04 UTC, avant toute mesure sur G4). Par processus et par mesure M de
{census, sondes, propositions} : t_hote = mediane des passes 1 a N de la premiere prise hote (lot du produit a --fils
fils) ; t_app = mediane des passes 1 a N de la premiere prise de l'appareil (noyau seul) ; rapport r = t_app / t_hote ;
A/A hote et appareil = mediane des passes des quotients seconde prise / premiere prise. Par trame et par mesure :
moyenne geometrique des r des processus, IC 95 % par bootstrap sur les processus (10 000 tirages, graine 20261008).
  REFUSE si : une prise manque (processus en echec hors code 1, journal incomplet, moins de --processus processus
  valides par trame), la recolte est incoherente (recolte_ok faux), l'appareil est absent ou non isole, plus de 0,1 %
  des requetes de census sont non resolues sur l'appareil (voies controlee ou large), le mutant n'est pas tue, un
  binaire ou une source change pendant la campagne, une moyenne geometrique A/A (hote ou appareil, mesure decisive)
  sort de [0,90 ; 1,10], ou l'auto-test du juge echoue.
  REJETE si : un ecart d'identite apparait dans un processus (census : genre, p, m, I, U, neuf compteurs du travail ;
  sondes : naissance par representant, et reussites egales a first_probe_hits de G ; propositions : bits de la boule
  et support), OU si la borne haute de l'IC depasse le seuil sur l'une des trois trames : census 0,20, sondes 0,20,
  propositions 0,50.
  ADOPTE sinon : la poursuite (tranche << G sur l'appareil >>) est fondee sur ces trois postes. Le verdict ne promeut
  rien dans le produit ; il dit seulement si l'etude justifie le portage.
Informations publiees sans verdict : temps absolus, debit par requete, transferts (copies non epinglees), rapport du
noyau en source unique joue sur l'hote au lot du produit (gain d'une voie CPU << a plat >>), requetes distinctes
(memo possible), proprietes de l'appareil (SM, L2, horloges).

Exemple (G4, commande d'un plan de session ; en local : --sans-appareil --fils 8 --processus 1 --passes 2) :
  python3 pilote_g_appareil.py tout --src <racine du paquet> --produit <build par defaut> --travail <dossier> \\
      --donnees <donnees> --sortie <sortie> --fils 48 --processus 5 --passes 5

Codes : 0 campagne complete et jugee (le verdict est dans le rapport, il ne change pas le code) ; 1 auto-test en echec ;
2 usage ; 3 refus (construction en echec, preuve absente, prise manquante).
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

ICI = os.path.dirname(os.path.abspath(__file__))
SOURCES = ("noyau_g.hpp", "appareil.hpp", "appareil.cu", "mes_g_app.cpp", "pilote_g_appareil.py")
# Sources du produit portees ou reproduites par le noyau (empreintes publiees : un ecart d'identite se lit avec elles).
PORTEES = ("src/tower/proposal.hpp", "src/tower/resolve.cpp", "src/tower/passes.cpp", "src/tower/populations.cpp",
           "src/index/census_workspace.cpp", "src/num/guard.cpp")
TRAMES = (("ng00", "lidar_ng00"), ("mediane", "kitti_ng_02_001606"), ("max", "kitti_ng_08_002119"))
MESURES = ("census", "sondes", "propositions")
SEUILS = {"census": 0.20, "sondes": 0.20, "propositions": 0.50}
FENETRE_AA = (0.90, 1.10)
PART_NON_RESOLUES = 0.001
TIRAGES = 10000
GRAINE = 20261008
SYMBOLE = ("_ZN6mhgp1215CensusWorkspace5queryERKNS_11GlobalIndexERKNS_3num13CertifiedBallEjSt4spanIKNS_7SiteIdxELm"
           "18446744073709551615EEPvPFNS_7OutcomeESC_RKNS_14BorrowedCensusEE")
GPU_APPS = ["nvidia-smi", "--query-compute-apps=pid,process_name,used_memory", "--format=csv,noheader"]


class Refus(Exception):
    pass


def maintenant():
    return datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def sha256(chemin):
    try:
        h = hashlib.sha256()
        with open(chemin, "rb") as f:
            for bloc in iter(lambda: f.read(1 << 20), b""):
                h.update(bloc)
        return h.hexdigest()
    except OSError:
        return None


def capturer(argv, delai, cwd=None):
    try:
        fait = subprocess.run(argv, capture_output=True, text=True, timeout=delai, check=False, cwd=cwd)
        return fait.returncode, fait.stdout, fait.stderr
    except (OSError, subprocess.TimeoutExpired) as erreur:
        return -1, "", str(erreur)


def trouver_nvcc(explicite):
    candidats = [explicite, shutil.which("nvcc")]
    if os.environ.get("CUDA_HOME"):
        candidats.append(os.path.join(os.environ["CUDA_HOME"], "bin", "nvcc"))
    candidats.append("/usr/local/cuda/bin/nvcc")
    for c in candidats:
        if c and os.path.isfile(c) and os.access(c, os.X_OK):
            return c
    return None


def ecrire_json(chemin, valeur):
    os.makedirs(os.path.dirname(chemin), exist_ok=True)
    temporaire = chemin + ".tmp"
    with open(temporaire, "w", encoding="utf-8") as f:
        json.dump(valeur, f, indent=1, sort_keys=True)
        f.write("\n")
    os.replace(temporaire, chemin)


def lire_json(chemin):
    with open(chemin, encoding="utf-8") as f:
        return json.load(f)


# ------------------------------------------------------------------------------------------------ statistique
def mediane(valeurs):
    v = sorted(valeurs)
    n = len(v)
    if n == 0:
        return None
    return v[n // 2] if n % 2 else 0.5 * (v[n // 2 - 1] + v[n // 2])


def moyenne_geometrique(valeurs):
    return math.exp(sum(math.log(x) for x in valeurs) / len(valeurs))


def bootstrap(valeurs, tirages=TIRAGES, graine=GRAINE):
    """IC 95 % de la moyenne geometrique par bootstrap sur les processus (percentiles 2,5 et 97,5)."""
    alea = random.Random(graine)
    n = len(valeurs)
    logs = [math.log(x) for x in valeurs]
    moyennes = []
    for _ in range(tirages):
        s = 0.0
        for _ in range(n):
            s += logs[alea.randrange(n)]
        moyennes.append(math.exp(s / n))
    moyennes.sort()
    return moyennes[int(0.025 * (tirages - 1))], moyennes[int(math.ceil(0.975 * (tirages - 1)))]


# ------------------------------------------------------------------------------------------------ lecture stricte
def lire_journal(lignes):
    """Rend (recolte, appareil, passes, identite, transferts) d'un journal de mes_g_app, ou leve Refus."""
    recolte = appareil = identite = transferts = None
    passes = []
    for ligne in lignes:
        ligne = ligne.strip()
        if not ligne:
            continue
        try:
            d = json.loads(ligne)
        except ValueError:
            raise Refus("ligne JSON illisible")
        if type(d) is not dict or type(d.get("phase")) is not str:
            raise Refus("ligne sans phase")
        phase = d["phase"]
        if phase == "recolte":
            recolte = d
        elif phase == "appareil":
            appareil = d
        elif phase == "passe":
            passes.append(d)
        elif phase == "identite":
            identite = d
        elif phase == "transferts":
            transferts = d
        elif phase == "refus":
            raise Refus("refus du programme : " + json.dumps(d, sort_keys=True))
    if recolte is None or identite is None:
        raise Refus("journal incomplet (recolte ou identite absente)")
    return recolte, appareil, passes, identite, transferts


def nombre_positif(x):
    return type(x) in (int, float) and math.isfinite(x) and x > 0


def resumer_processus(journal, n_passes, avec_appareil):
    """Mesures d'un processus : par mesure, t_hote, t_app, r, A/A ; plus identite et comptes. Leve Refus."""
    recolte, appareil, passes, identite, transferts = journal
    if recolte.get("recolte_ok") is not True:
        raise Refus("recolte incoherente")
    if avec_appareil and appareil is None:
        raise Refus("appareil absent du journal")
    numeros = [p.get("passe") for p in passes]
    if numeros != list(range(n_passes + 1)):
        raise Refus("passes manquantes ou desordonnees : %r" % (numeros,))
    if passes[0].get("echauffement") is not True or any(p.get("echauffement") is not False for p in passes[1:]):
        raise Refus("echauffement mal marque")
    if any(p.get("ok") is not True for p in passes):
        raise Refus("passe en echec")
    sortie = {"mesures": {}}
    for m in MESURES:
        hote, app, aa_h, aa_a = [], [], [], []
        for p in passes[1:]:
            bloc = p.get(m)
            if type(bloc) is not dict:
                raise Refus("mesure %s absente" % m)
            h, a = bloc.get("hote_ms"), bloc.get("appareil_ms")
            if type(h) is not list or len(h) != 2 or not all(nombre_positif(x) for x in h):
                raise Refus("prises hote invalides (%s)" % m)
            if type(a) is not list or len(a) != 2:
                raise Refus("prises appareil invalides (%s)" % m)
            hote.append(h[0])
            aa_h.append(h[1] / h[0])
            if avec_appareil:
                if not all(nombre_positif(x) for x in a):
                    raise Refus("prises appareil nulles (%s)" % m)
                app.append(a[0])
                aa_a.append(a[1] / a[0])
        t_h = mediane(hote)
        entree = {"t_hote_ms": t_h, "aa_hote": mediane(aa_h)}
        if avec_appareil:
            t_a = mediane(app)
            entree.update({"t_app_ms": t_a, "rapport": t_a / t_h, "aa_app": mediane(aa_a)})
        sortie["mesures"][m] = entree
    hd = [p["census"].get("hote_hd_ms") for p in passes[1:]]
    if all(nombre_positif(x) for x in hd):
        sortie["census_hd_sur_produit"] = mediane(hd) / sortie["mesures"]["census"]["t_hote_ms"]
    phd = [p["propositions"].get("hote_hd_ms") for p in passes[1:]]
    if all(nombre_positif(x) for x in phd):
        sortie["propositions_hd_sur_produit"] = mediane(phd) / sortie["mesures"]["propositions"]["t_hote_ms"]
    c, s, pr = identite.get("census", {}), identite.get("sondes", {}), identite.get("propositions", {})
    ecarts = {
        "census_hote_hd": c.get("ecarts_hote_hd"), "census_appareil": c.get("ecarts_appareil"),
        "census_lot_produit": c.get("ecarts_lot_produit"), "coquilles_larges": c.get("coquilles_larges"),
        "sondes_appareil": s.get("ecarts_appareil"), "propositions_hote_hd": pr.get("ecarts_hote_hd"),
        "propositions_appareil": pr.get("ecarts_appareil")}
    if not all(type(v) is int and v >= 0 for v in ecarts.values()):
        raise Refus("compteurs d'identite invalides")
    if avec_appareil:
        if identite.get("appareil") is not True or c.get("comparees_appareil", 0) + c.get("non_resolues", 0) != \
                c.get("requetes") or s.get("comparees_appareil") != recolte.get("representants"):
            raise Refus("identite de l'appareil incomplete")
    sortie["ecarts"] = ecarts
    sortie["sondes_coherentes"] = s.get("coherentes") is True
    sortie["identite"] = identite.get("identite") is True and all(v == 0 for v in ecarts.values()) and \
        sortie["sondes_coherentes"]
    sortie["requetes"] = c.get("requetes")
    sortie["non_resolues"] = c.get("non_resolues")
    sortie["distinctes"] = recolte.get("distinctes")
    sortie["representants"] = recolte.get("representants")
    sortie["parties"] = pr.get("parties")
    sortie["sites"] = recolte.get("sites")
    sortie["appareil"] = appareil
    sortie["transferts"] = transferts
    return sortie


def juger(campagne):
    """campagne : {"processus": n, "passes": n, "appareil": bool, "mutant": {...}, "isolation_ok": bool,
    "empreintes_stables": bool, "trames": {nom: [journal brut (liste de lignes) et code]}} -> rapport."""
    refus, rejets = [], []
    resultats = {}
    avec = campagne.get("appareil") is True
    if not avec:
        refus.append("appareil absent (campagne sans --appareil)")
    if campagne.get("isolation_ok") is not True:
        refus.append("appareil non isole ou isolation illisible")
    if campagne.get("empreintes_stables") is not True:
        refus.append("binaire ou source modifie pendant la campagne")
    mutant = campagne.get("mutant") or {}
    if mutant.get("code") != 1 or mutant.get("identite") is not False:
        refus.append("mutant cote_nul non tue (code %r, identite %r)" % (mutant.get("code"), mutant.get("identite")))
    for nom, _ in TRAMES:
        prises = campagne.get("trames", {}).get(nom, [])
        valides = []
        for prise in prises:
            if prise.get("code") not in (0, 1):
                refus.append("%s : processus en echec (code %r)" % (nom, prise.get("code")))
                continue
            try:
                valides.append(resumer_processus(lire_journal(prise.get("lignes", [])), campagne["passes"], avec))
            except Refus as r:
                refus.append("%s : %s" % (nom, r))
        if len(valides) < campagne["processus"]:
            refus.append("%s : %d processus valides sur %d" % (nom, len(valides), campagne["processus"]))
        if not valides:
            continue
        trame = {"processus": len(valides), "mesures": {}}
        for v in valides:
            if not v["identite"]:
                rejets.append("%s : ecart d'identite %s" % (nom, json.dumps(v["ecarts"], sort_keys=True)))
            if v["requetes"] and v["non_resolues"] > PART_NON_RESOLUES * v["requetes"]:
                refus.append("%s : %d requetes non resolues sur l'appareil" % (nom, v["non_resolues"]))
        for m in MESURES:
            t_h = [v["mesures"][m]["t_hote_ms"] for v in valides]
            entree = {"t_hote_ms": mediane(t_h),
                      "aa_hote": moyenne_geometrique([v["mesures"][m]["aa_hote"] for v in valides])}
            if avec:
                r = [v["mesures"][m]["rapport"] for v in valides]
                bas, haut = bootstrap(r)
                entree.update({"t_app_ms": mediane([v["mesures"][m]["t_app_ms"] for v in valides]),
                               "rapport": moyenne_geometrique(r), "ic95": [bas, haut], "seuil": SEUILS[m],
                               "aa_app": moyenne_geometrique([v["mesures"][m]["aa_app"] for v in valides])})
                if haut > SEUILS[m]:
                    rejets.append("%s : %s, borne haute %.4f au-dessus du seuil %.2f" % (nom, m, haut, SEUILS[m]))
                if not FENETRE_AA[0] <= entree["aa_app"] <= FENETRE_AA[1]:
                    refus.append("%s : A/A appareil %s = %.4f hors fenetre" % (nom, m, entree["aa_app"]))
            if not FENETRE_AA[0] <= entree["aa_hote"] <= FENETRE_AA[1]:
                refus.append("%s : A/A hote %s = %.4f hors fenetre" % (nom, m, entree["aa_hote"]))
            trame["mesures"][m] = entree
        infos = {}
        for cle in ("census_hd_sur_produit", "propositions_hd_sur_produit"):
            vals = [v[cle] for v in valides if cle in v]
            if len(vals) == len(valides):
                infos[cle] = moyenne_geometrique(vals)
        for cle in ("requetes", "distinctes", "representants", "parties", "sites", "non_resolues"):
            infos[cle] = valides[0][cle]
        infos["appareil"] = valides[0]["appareil"]
        infos["transferts"] = valides[0]["transferts"]
        trame["informations"] = infos
        resultats[nom] = trame
    if refus:
        verdict = "refuse"
    elif rejets:
        verdict = "rejete"
    else:
        verdict = "adopte"
    return {"regle": "REGLE_G_APPAREIL", "verdict": verdict, "refus": refus, "rejets": rejets, "trames": resultats,
            "seuils": SEUILS, "fenetre_aa": list(FENETRE_AA), "tirages": TIRAGES, "graine": GRAINE}


# ------------------------------------------------------------------------------------------------ auto-test
def journal_synthetique(sites, rapports, identite=True, passes=3, aa=1.0, recolte_ok=True):
    lignes = [json.dumps({"phase": "recolte", "recolte_ok": recolte_ok, "representants": 100, "distinctes": 7,
                          "sites": sites, "requetes": 10}),
              json.dumps({"phase": "appareil", "nom": "synthetique", "sm": 1})]
    for i in range(passes + 1):
        p = {"phase": "passe", "passe": i, "echauffement": i == 0, "ok": True}
        for m in MESURES:
            h = 10.0 + i * 0.01
            a = h * rapports[m]
            p[m] = {"hote_ms": [h, h * aa], "appareil_ms": [a, a * aa], "hote_hd_ms": h * 0.5}
        lignes.append(json.dumps(p))
    ec = 0 if identite else 3
    lignes.append(json.dumps({"phase": "identite", "appareil": True, "identite": identite,
                              "census": {"requetes": 10, "non_resolues": 0, "comparees_appareil": 10,
                                         "ecarts_hote_hd": 0, "ecarts_appareil": ec, "ecarts_lot_produit": 0,
                                         "coquilles_larges": 0},
                              "sondes": {"comparees_appareil": 100, "ecarts_appareil": 0, "coherentes": True},
                              "propositions": {"parties": 5, "ecarts_hote_hd": 0, "ecarts_appareil": 0}}))
    return lignes


def campagne_synthetique(rapports, identite=True, mutant_code=1, aa=1.0, manque=False, recolte_ok=True):
    trames = {}
    for nom, _ in TRAMES:
        prises = []
        for j in range(3):
            r = {m: rapports[m] * (1.0 + 0.01 * j) for m in MESURES}
            prises.append({"code": 0 if identite else 1,
                           "lignes": journal_synthetique(1000, r, identite, aa=aa, recolte_ok=recolte_ok)})
        if manque and nom == "max":
            prises = prises[:2]
        trames[nom] = prises
    return {"processus": 3, "passes": 3, "appareil": True, "isolation_ok": True, "empreintes_stables": True,
            "mutant": {"code": mutant_code, "identite": mutant_code != 1}, "trames": trames}


def auto_test():
    bon = {"census": 0.05, "sondes": 0.08, "propositions": 0.2}
    lent = {"census": 0.05, "sondes": 0.30, "propositions": 0.2}
    cas = [("adopte", campagne_synthetique(bon)),
           ("rejete", campagne_synthetique(lent)),
           ("rejete", campagne_synthetique(bon, identite=False)),
           ("refuse", campagne_synthetique(bon, mutant_code=0)),
           ("refuse", campagne_synthetique(bon, aa=1.2)),
           ("refuse", campagne_synthetique(bon, manque=True)),
           ("refuse", campagne_synthetique(bon, recolte_ok=False))]
    echecs = []
    for attendu, camp in cas:
        rendu = juger(camp)["verdict"]
        if rendu != attendu:
            echecs.append("attendu %s, rendu %s" % (attendu, rendu))
    a = bootstrap([0.1, 0.1, 0.1])
    if not (abs(a[0] - 0.1) < 1e-12 and abs(a[1] - 0.1) < 1e-12):
        echecs.append("bootstrap degenere")
    if bootstrap([0.1, 0.2, 0.3]) != bootstrap([0.1, 0.2, 0.3]):
        echecs.append("bootstrap non deterministe")
    return echecs


# ------------------------------------------------------------------------------------------------ construction
def bits_du_produit(produit):
    try:
        with open(os.path.join(produit, "CMakeCache.txt"), encoding="utf-8", errors="replace") as f:
            for ligne in f:
                if ligne.startswith("MHGP12_COORD_BITS:"):
                    return ligne.strip().split("=", 1)[1]
    except OSError:
        pass
    return None


def verifier_symbole(bibliotheque):
    """Le symbole intercepte doit etre DEFINI par census_workspace.cpp.o et APPELE par resolve.cpp.o."""
    code, sortie, err = capturer(["nm", "-A", bibliotheque], 120)
    if code != 0:
        raise Refus("nm illisible : " + err.strip()[:200])
    defini = appele = False
    for ligne in sortie.splitlines():
        if not ligne.endswith(" " + SYMBOLE):
            continue
        if "census_workspace.cpp.o:" in ligne and " T " in ligne:
            defini = True
        if "resolve.cpp.o:" in ligne and " U " in ligne:
            appele = True
    if not (defini and appele):
        raise Refus("symbole du census introuvable (defini %s, appele %s) : la signature a change" % (defini, appele))


def construire(args):
    os.makedirs(args.travail, exist_ok=True)
    bibliotheque = os.path.join(args.produit, "libmhgp12.a")
    if not os.path.isfile(bibliotheque):
        raise Refus("libmhgp12.a absent de " + args.produit)
    verifier_symbole(bibliotheque)
    bits = bits_du_produit(args.produit) or "21"
    src = os.path.join(args.src, "morsehgp3D_v12", "src")
    gxx = args.gxx or shutil.which("g++")
    if not gxx:
        raise Refus("g++ introuvable")
    commun = [gxx, "-std=c++20", "-O3", "-DNDEBUG", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
              "-DMHGP12_COORD_BITS=" + bits, "-I" + src, "-I" + ICI, os.path.join(ICI, "mes_g_app.cpp")]
    lien = [bibliotheque, "-lpthread", "-Wl,--wrap=" + SYMBOLE]
    journal = {"date_utc": maintenant(), "bits": bits, "commandes": []}
    binaires = {}

    def jouer(argv, nom):
        code, sortie, err = capturer(argv, 900, cwd=args.travail)
        journal["commandes"].append({"nom": nom, "argv": argv, "code": code, "stderr": err[-4000:]})
        if code != 0:
            raise Refus("construction %s en echec : %s" % (nom, err.strip()[-600:]))

    if args.sans_appareil:
        for nom, defs in (("mes_g_app", []), ("mes_g_app_mutant", ["-DMESG_MUTANT_COTE_NUL"])):
            sortie = os.path.join(args.travail, nom)
            jouer(commun[:1] + defs + commun[1:] + lien + ["-o", sortie], nom)
            binaires[nom] = sortie
    else:
        nvcc = trouver_nvcc(args.nvcc)
        if not nvcc:
            raise Refus("nvcc introuvable")
        cuda = os.path.dirname(os.path.dirname(os.path.realpath(nvcc)))
        for nom, defs in (("mes_g_app", []), ("mes_g_app_mutant", ["-DMESG_MUTANT_COTE_NUL"])):
            objet = os.path.join(args.travail, nom + "_appareil.o")
            jouer([nvcc, "-std=c++20", "-O3", "-arch=sm_120", "-fmad=false", "-Werror=all-warnings",
                   "-Xcompiler=-Wall,-Wextra,-Werror", "-Xptxas=-v", "-I" + ICI] + defs +
                  ["-c", os.path.join(ICI, "appareil.cu"), "-o", objet], nom + "_nvcc")
            sortie = os.path.join(args.travail, nom)
            jouer(commun[:1] + defs + ["-DMESG_APPAREIL", "-I" + os.path.join(cuda, "include")] + commun[1:] +
                  [objet] + lien + ["-L" + os.path.join(cuda, "lib64"), "-lcudart",
                                    "-Wl,-rpath," + os.path.join(cuda, "lib64"), "-o", sortie], nom + "_lien")
            binaires[nom] = sortie
        journal["nvcc"] = capturer([nvcc, "--version"], 60)[1].strip().splitlines()[-2:]
    journal["gxx"] = capturer([gxx, "--version"], 60)[1].strip().splitlines()[:1]
    journal["binaires"] = {nom: sha256(chemin) for nom, chemin in binaires.items()}
    journal["sources"] = {nom: sha256(os.path.join(ICI, nom)) for nom in SOURCES}
    journal["portees"] = {nom: sha256(os.path.join(args.src, "morsehgp3D_v12", nom)) for nom in PORTEES}
    journal["bibliotheque"] = sha256(bibliotheque)
    journal["appareil"] = not args.sans_appareil
    ecrire_json(os.path.join(args.sortie, "construction.json"), journal)
    return journal


# ------------------------------------------------------------------------------------------------ campagne
def gpu_isole(avec_appareil):
    if not avec_appareil:
        return True, "sans appareil"
    code, sortie, err = capturer(GPU_APPS, 60)
    if code != 0:
        return False, "nvidia-smi illisible : " + err.strip()[:200]
    return sortie.strip() == "", sortie.strip()


def campagne(args, construction):
    avec = construction.get("appareil") is True
    binaire = os.path.join(args.travail, "mes_g_app")
    mutant = os.path.join(args.travail, "mes_g_app_mutant")
    empreintes = {"mes_g_app": sha256(binaire), "mes_g_app_mutant": sha256(mutant)}
    if empreintes != construction["binaires"]:
        raise Refus("binaires differents de la construction")
    dossier = os.path.join(args.sortie, "journaux")
    os.makedirs(dossier, exist_ok=True)
    camp = {"processus": args.processus, "passes": args.passes, "appareil": avec, "isolation_ok": True,
            "isolation": [], "trames": {nom: [] for nom, _ in TRAMES}, "debut_utc": maintenant()}

    def jouer(executable, nom, fichier, etiquette):
        avant = gpu_isole(avec)
        argv = [executable, "--trame=%s,%s,%s" % (os.path.join(args.donnees, fichier + ".u32le"),
                                                  os.path.join(args.donnees, fichier + ".ids.u32le"), nom),
                "--k=5", "--fils=%d" % args.fils, "--passes=%d" % args.passes]
        if avec:
            argv.append("--appareil")
        code, sortie, err = capturer(argv, args.delai)
        apres = gpu_isole(avec)
        camp["isolation"].append({"prise": etiquette, "avant": avant[1], "apres": apres[1]})
        if not (avant[0] and apres[0]):
            camp["isolation_ok"] = False
        with open(os.path.join(dossier, etiquette + ".jsonl"), "w", encoding="utf-8") as f:
            f.write(sortie)
        with open(os.path.join(dossier, etiquette + ".stderr"), "w", encoding="utf-8") as f:
            f.write(err[-20000:])
        return {"code": code, "lignes": sortie.splitlines(), "journal": etiquette + ".jsonl",
                "journal_sha256": sha256(os.path.join(dossier, etiquette + ".jsonl"))}

    for i in range(args.processus):
        for nom, fichier in TRAMES:
            camp["trames"][nom].append(jouer(binaire, nom, fichier, "%s_p%d" % (nom, i)))
    m = jouer(mutant, "ng00", "lidar_ng00", "mutant_cote_nul")
    identite = None
    for ligne in m["lignes"]:
        try:
            d = json.loads(ligne)
        except ValueError:
            continue
        if type(d) is dict and d.get("phase") == "identite":
            identite = d.get("identite")
    camp["mutant"] = {"code": m["code"], "identite": identite, "journal": m["journal"]}
    camp["empreintes_stables"] = {"mes_g_app": sha256(binaire), "mes_g_app_mutant": sha256(mutant)} == empreintes and \
        all(sha256(os.path.join(ICI, nom)) == construction["sources"][nom] for nom in SOURCES)
    camp["fin_utc"] = maintenant()
    ecrire_json(os.path.join(args.sortie, "campagne.json"), camp)
    return camp


# ------------------------------------------------------------------------------------------------ rapport
def tableaux(rapport):
    lignes = ["# MES-G-APP : etage G sur l'appareil, trois postes (genere par pilote_g_appareil.py)", "",
              "Verdict REGLE_G_APPAREIL : **%s**" % rapport["verdict"], ""]
    for titre, liste in (("Refus", rapport["refus"]), ("Rejets", rapport["rejets"])):
        if liste:
            lignes.append("%s :" % titre)
            lignes += ["- " + x for x in liste]
            lignes.append("")
    lignes += ["| trame | mesure | hote (ms) | appareil (ms) | rapport | IC 95 % | seuil | A/A hote | A/A appareil |",
               "| --- | --- | ---: | ---: | ---: | --- | ---: | ---: | ---: |"]
    for nom, trame in rapport["trames"].items():
        for m in MESURES:
            e = trame["mesures"][m]
            ic = e.get("ic95")
            lignes.append("| %s | %s | %.3f | %s | %s | %s | %.2f | %.3f | %s |" % (
                nom, m, e["t_hote_ms"], "%.3f" % e["t_app_ms"] if "t_app_ms" in e else "-",
                "%.4f" % e["rapport"] if "rapport" in e else "-",
                "%.4f-%.4f" % (ic[0], ic[1]) if ic else "-", SEUILS[m], e["aa_hote"],
                "%.3f" % e["aa_app"] if "aa_app" in e else "-"))
    lignes += ["", "Informations (non jugees) :", "", "```json",
               json.dumps({n: t.get("informations") for n, t in rapport["trames"].items()}, indent=1,
                          sort_keys=True), "```", ""]
    return "\n".join(lignes)


def rapport(args, camp, construction):
    echecs = auto_test()
    if echecs:
        raise Refus("auto-test du juge en echec : " + "; ".join(echecs))
    r = juger(camp)
    r["construction"] = {k: construction.get(k) for k in ("bits", "nvcc", "gxx", "binaires", "sources", "portees",
                                                           "bibliotheque", "appareil")}
    r["date_utc"] = maintenant()
    r["fils"] = args.fils
    ecrire_json(os.path.join(args.sortie, "rapport_g_appareil.json"), r)
    with open(os.path.join(args.sortie, "tableaux_g_appareil.md"), "w", encoding="utf-8") as f:
        f.write(tableaux(r))
    return r


def principal(argv):
    p = argparse.ArgumentParser(description="MES-G-APP : pilote et juge (REGLE_G_APPAREIL)")
    p.add_argument("etape", choices=("auto-test", "construire", "campagne", "rapport", "tout"))
    p.add_argument("--src", default=os.path.normpath(os.path.join(ICI, "..", "..", "..")))
    p.add_argument("--produit", help="construction par defaut du produit (libmhgp12.a, CMakeCache.txt)")
    p.add_argument("--travail")
    p.add_argument("--donnees")
    p.add_argument("--sortie")
    p.add_argument("--fils", type=int, default=48)
    p.add_argument("--processus", type=int, default=5)
    p.add_argument("--passes", type=int, default=5)
    p.add_argument("--delai", type=int, default=600)
    p.add_argument("--nvcc")
    p.add_argument("--gxx")
    p.add_argument("--sans-appareil", action="store_true", help="essai local : construction hote seule")
    args = p.parse_args(argv)
    if args.etape == "auto-test":
        echecs = auto_test()
        print(json.dumps({"auto_test": "ok" if not echecs else "echec", "echecs": echecs}))
        return 0 if not echecs else 1
    for nom in ("travail", "sortie"):
        if not getattr(args, nom):
            p.error("--%s requis" % nom)
    if args.fils < 1 or args.processus < 1 or args.passes < 1:
        p.error("--fils, --processus et --passes positifs")
    try:
        echecs = auto_test()
        if echecs:
            print(json.dumps({"auto_test": "echec", "echecs": echecs}))
            return 1
        construction = None
        if args.etape in ("construire", "tout"):
            if not args.produit:
                p.error("--produit requis")
            construction = construire(args)
        if args.etape in ("campagne", "tout"):
            if not args.donnees:
                p.error("--donnees requis")
            if construction is None:
                construction = lire_json(os.path.join(args.sortie, "construction.json"))
            camp = campagne(args, construction)
        if args.etape in ("rapport", "tout"):
            if construction is None:
                construction = lire_json(os.path.join(args.sortie, "construction.json"))
            if args.etape == "rapport":
                camp = lire_json(os.path.join(args.sortie, "campagne.json"))
            r = rapport(args, camp, construction)
            print(json.dumps({"verdict": r["verdict"], "refus": r["refus"][:10], "rejets": r["rejets"][:10]},
                             ensure_ascii=False))
        return 0
    except Refus as r:
        print(json.dumps({"refus": str(r)}, ensure_ascii=False))
        return 3


if __name__ == "__main__":
    sys.exit(principal(sys.argv[1:]))
