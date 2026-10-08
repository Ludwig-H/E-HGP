#!/usr/bin/env python3
"""MES-G-APP, etape 2 : pilote et juge de l'etude de faisabilite de l'etage G sur l'appareil de G4 (hors produit).
Bibliotheque standard seule, Python 3.10, jouable sous python3 -S -O (aucune garde par assert).

Objet : trois postes archetypes de l'etage G, joues sur l'hote par le PRODUIT et sur l'appareil par un noyau en source
unique __host__ __device__ (noyau_g.hpp), sur les MEMES entrees :
  census        census garde a temoins de chaque boule certifiee que G recense (requetes interceptees dans le produit
                a l'edition de liens, -Wl,--wrap) ; identite a l'octet ;
  sondes        premiere sonde de la table de populations (LEM-POP) de chaque representant ; identite a l'octet ;
  propositions  proposition de plus petite boule des traces dont la premiere sonde echoue, en trois MECANISMES :
                p64    DWelzl binaire64 du produit (session gapp du 8 octobre : 0,74 sur l'appareil, rejete) ;
                l4     voie entiere du bras L4 de T2-d-B (paire la plus eloignee et boule diametrale, triangle aigu) puis
                       DWelzl binaire64 amorce, sur une file compactee des parties difficiles ;
                l4f32  la meme voie entiere, puis DWelzl binaire32 amorce sur la file compactee.
                L'ISSUE de chaque proposition (boule de la table, ou sphere certifiee et support canonique pour un census)
                est calculee par les fonctions exactes du produit (lem_t1, certify_part, exact_support, find_support) et
                comparee a celle de p64 ; le MECANISME (t1 sans arithmetique, certificat, repli exact) est compte.
Mesure decisive : duree des noyaux de l'appareil, donnees residentes, contre le lot du produit sur l'hote a --fils fils.

Etapes (dans l'ordre si plusieurs) :
  auto-test   le juge sur des campagnes synthetiques ; tout autre resultat que celui attendu : code 1
  construire  noyaux CUDA (nvcc, sm_120, -fmad=false) et programme hote lie a <produit>/libmhgp12.a avec interception
              du census (symbole verifie par nm) ; deux mutants causaux (copies compilees a part) : << cote nul >>
              (census) et << L4 sans test diametral >> (voie entiere) ; empreintes ; --sans-appareil : hote seul
  campagne    ng00, mediane v12set (kitti_ng_02_001606), maximum v12set (kitti_ng_08_002119), K5 ; --processus
              processus neufs par trame, entrelaces ; une passe d'echauffement puis --passes passes, chaque mesure deux
              fois (A/A), ordre hote/appareil alterne ; GPU isole avant et apres chaque processus ; puis les mutants sur
              ng00
  rapport     auto-test, puis juge (REGLE_G_APPAREIL_2), rapport JSON et tableaux Markdown
  tout        auto-test construire campagne rapport

REGLE_G_APPAREIL_2 (ecrite le 8 octobre 2026 a 11:09 UTC, apres le verdict de REGLE_G_APPAREIL sur la session gapp et
avant toute mesure G4 des mecanismes l4 et l4f32). Par processus et par mesure : mediane des passes 1 a N de la premiere
prise ; C_h, C_a census (hote, appareil) ; S_h, S_a sondes ; P_h propositions p64 du produit sur l'hote ; L_h, L_a
mecanisme l4 (executeur de l'hote de la meme source, appareil) ; F_a mecanisme l4f32 sur l'appareil. A/A = mediane des
quotients seconde prise / premiere prise. Par trame : moyenne geometrique sur les processus, IC 95 % par bootstrap sur
les processus (10 000 tirages, graine 20261008). Deux conceptions sont jugees, la premiere preferee :
  D2 (premiere ; elle exige l'amendement de CONTRAT_TOUR.md § 8 : les compteurs du travail des plus petites boules se
  comptent par ISSUE, table ou census, et le mecanisme devient un compteur physique de l'executeur) : l'hote garde p64,
  l'appareil joue l4f32. ADOPTE si identite complete (census, sondes, bits de l4f32 egaux entre l'appareil et
  l'executeur de l'hote, issues de l4f32 egales a celles de p64 pour toute partie, aucune issue refusee), replis de
  l4f32 (sans proposition ou apres un certificat en echec) au plus 0,1 % des parties, et sur chacune des trois trames :
  borne haute de (C_a + S_a + F_a) / (C_h + S_h + P_h) <= 0,10 et de F_a / P_h <= 0,20. REJETE sinon.
  D1 (seconde ; aucun amendement : politique l4 partagee par les deux executeurs, a declarer) : ADOPTE si identite
  complete (dont bits de l4 egaux entre l'appareil et l'hote, issues de l4 egales a celles de p64), replis de l4 au plus
  0,1 %, et sur chaque trame : borne haute de (C_a + S_a + L_a) / (C_h + S_h + P_h) <= 0,15, de L_a / P_h <= 0,50, et
  neutralite de l'hote L_h / P_h <= 1,05. REJETE sinon.
  REFUSE (les deux verdicts) si une prise manque (processus en echec hors code 1, journal incomplet, moins de
  --processus processus valides par trame), la recolte est incoherente, l'appareil est absent ou non isole, plus de
  0,1 % des requetes de census sont non resolues, un mutant n'est pas tue (cote nul : code 1 exige ; L4 sans test
  diametral : replis de l4 et de l4f32 au-dessus de 0,1 % exiges sur ng00), un binaire ou une source change pendant la
  campagne, une moyenne geometrique A/A d'une mesure decisive (C_h, C_a, S_h, S_a, P_h, L_h, L_a, F_a) sort de
  [0,90 ; 1,10], ou l'auto-test du juge echoue.
  Decision ecrite d'avance : D2 adopte -> proposer l'amendement aux auditeurs et ouvrir la tranche << G sur l'appareil >>
  avec l4f32 sur l'appareil et p64 sur l'hote ; sinon D1 adopte -> l'ouvrir avec l4 partage (reintroduit dans le produit
  comme politique declaree des deux executeurs) ; sinon -> pas de tranche sur cette base.
  Seuils : 0,10 pour D2 se place entre l'effet attendu (environ 0,08 : propositions binaire32 sous 1 ms sur la trame
  maximale) et celui de D1, pour qu'un amendement du contrat ne s'achete que par un gain net ; 0,20 par poste est la
  barre du census et des sondes dans REGLE_G_APPAREIL. Pour D1, 0,15 est le milieu entre le total de la session gapp
  (0,18 a 0,19) et l'effet optimiste attendu (environ 0,12) ; 0,50 par poste est le seuil des propositions de
  REGLE_G_APPAREIL ; 1,05 borne le cout de la politique partagee sur l'hote (T2-d-B : L4 neutre en place, 0,996 a 0,999).
Informations publiees sans verdict : ng00 a K10 (un processus, deux passes : les propositions y pesent trois fois plus
qu'a K5), temps absolus, rapport de p64 sur l'appareil (rejeu de la session gapp), cout de
l4f32 sur l'hote (F_h / P_h : pourquoi D2 ne partage pas sa politique), parties conclues par la voie entiere,
mecanismes et issues par politique, census a plat sur l'hote, transferts, proprietes de l'appareil.

Exemple (G4, commande d'un plan de session ; en local : --sans-appareil --fils 8 --processus 1 --passes 2) :
  python3 pilote_g_appareil.py tout --src <racine du paquet> --produit <build par defaut> --travail <dossier> \\
      --donnees <donnees> --sortie <sortie> --fils 48 --processus 5 --passes 5

Codes : 0 campagne complete et jugee (les verdicts sont dans le rapport, ils ne changent pas le code) ; 1 auto-test en
echec ; 2 usage ; 3 refus (construction en echec, preuve absente, prise manquante).
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
MESURES = ("census", "sondes", "propositions", "propositions_l4", "propositions_l4f32")
# Mesures dont l'A/A est decisif, cote hote et cote appareil (F_h, l4f32 sur l'hote, est une information).
AA_HOTE = ("census", "sondes", "propositions", "propositions_l4")
AA_APPAREIL = ("census", "sondes", "propositions_l4", "propositions_l4f32")
SEUILS = {"D2": {"total": 0.10, "propositions": 0.20},
          "D1": {"total": 0.15, "propositions": 0.50, "neutralite_hote": 1.05}}
FENETRE_AA = (0.90, 1.10)
PART_REPLIS = 0.001
PART_NON_RESOLUES = 0.001
TIRAGES = 10000
GRAINE = 20261008
SYMBOLE = ("_ZN6mhgp1215CensusWorkspace5queryERKNS_11GlobalIndexERKNS_3num13CertifiedBallEjSt4spanIKNS_7SiteIdxELm"
           "18446744073709551615EEPvPFNS_7OutcomeESC_RKNS_14BorrowedCensusEE")
GPU_APPS = ["nvidia-smi", "--query-compute-apps=pid,process_name,used_memory", "--format=csv,noheader"]
PASSES_K10 = 2
MUTANTS = (("mes_g_app_mutant_cote_nul", "-DMESG_MUTANT_COTE_NUL"),
           ("mes_g_app_mutant_l4", "-DMESG_MUTANT_L4_SANS_DIAMETRE"))


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


def entier_naturel(x):
    return type(x) is int and x >= 0


def taux_replis(bloc, parties):
    m = bloc.get("mecanismes") if type(bloc) is dict else None
    if type(m) is not dict or not entier_naturel(m.get("repli_sans_proposition")) or \
            not entier_naturel(m.get("repli_certificat")) or not parties:
        raise Refus("mecanismes illisibles")
    return (m["repli_sans_proposition"] + m["repli_certificat"]) / parties


def resumer_processus(journal, n_passes, avec_appareil):
    """Mesures d'un processus : par mesure, t_hote, t_app, A/A ; identite, replis et comptes. Leve Refus."""
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
        entree = {"t_hote_ms": mediane(hote), "aa_hote": mediane(aa_h)}
        if avec_appareil:
            entree.update({"t_app_ms": mediane(app), "aa_app": mediane(aa_a)})
        sortie["mesures"][m] = entree
    mes = sortie["mesures"]
    if avec_appareil:
        base = mes["census"]["t_hote_ms"] + mes["sondes"]["t_hote_ms"] + mes["propositions"]["t_hote_ms"]
        appareil_commun = mes["census"]["t_app_ms"] + mes["sondes"]["t_app_ms"]
        p_h = mes["propositions"]["t_hote_ms"]
        sortie["rapports"] = {
            "D2": {"total": (appareil_commun + mes["propositions_l4f32"]["t_app_ms"]) / base,
                   "propositions": mes["propositions_l4f32"]["t_app_ms"] / p_h},
            "D1": {"total": (appareil_commun + mes["propositions_l4"]["t_app_ms"]) / base,
                   "propositions": mes["propositions_l4"]["t_app_ms"] / p_h,
                   "neutralite_hote": mes["propositions_l4"]["t_hote_ms"] / p_h},
            "information": {"p64_appareil": mes["propositions"]["t_app_ms"] / p_h,
                            "l4f32_hote": mes["propositions_l4f32"]["t_hote_ms"] / p_h,
                            "total_p64": (appareil_commun + mes["propositions"]["t_app_ms"]) / base}}
    hd = [p["census"].get("hote_hd_ms") for p in passes[1:]]
    if all(nombre_positif(x) for x in hd):
        sortie["census_hd_sur_produit"] = mediane(hd) / mes["census"]["t_hote_ms"]
    c, s, pr = identite.get("census", {}), identite.get("sondes", {}), identite.get("propositions", {})
    l4, l4f = identite.get("propositions_l4", {}), identite.get("propositions_l4f32", {})
    parties = pr.get("parties")
    if not entier_naturel(parties) or parties == 0:
        raise Refus("parties illisibles")
    ecarts_communs = {
        "census_hote_hd": c.get("ecarts_hote_hd"), "census_appareil": c.get("ecarts_appareil"),
        "census_lot_produit": c.get("ecarts_lot_produit"), "coquilles_larges": c.get("coquilles_larges"),
        "sondes_appareil": s.get("ecarts_appareil"), "propositions_hote_hd": pr.get("ecarts_hote_hd"),
        "propositions_appareil": pr.get("ecarts_appareil"),
        "issues_refusees_p64": (pr.get("issues") or {}).get("refus")}
    ecarts = {"D2": {"bits_l4f32": l4f.get("ecarts_hote_appareil"), "issues_l4f32": l4f.get("issues_differentes_p64"),
                     "refus_l4f32": (l4f.get("issues") or {}).get("refus")},
              "D1": {"bits_l4": l4.get("ecarts_hote_appareil"), "issues_l4": l4.get("issues_differentes_p64"),
                     "refus_l4": (l4.get("issues") or {}).get("refus")}}
    tous = list(ecarts_communs.values()) + list(ecarts["D2"].values()) + list(ecarts["D1"].values())
    if not all(entier_naturel(v) for v in tous):
        raise Refus("compteurs d'identite invalides")
    if avec_appareil:
        if identite.get("appareil") is not True or c.get("comparees_appareil", 0) + c.get("non_resolues", 0) != \
                c.get("requetes") or s.get("comparees_appareil") != recolte.get("representants"):
            raise Refus("identite de l'appareil incomplete")
    commun_ok = all(v == 0 for v in ecarts_communs.values()) and s.get("coherentes") is True
    sortie["identite"] = {"commune": commun_ok,
                          "D2": commun_ok and all(v == 0 for v in ecarts["D2"].values()),
                          "D1": commun_ok and all(v == 0 for v in ecarts["D1"].values())}
    sortie["ecarts"] = {"communs": ecarts_communs, "D2": ecarts["D2"], "D1": ecarts["D1"]}
    sortie["replis"] = {"D2": taux_replis(l4f, parties), "D1": taux_replis(l4, parties),
                        "p64": taux_replis(pr, parties)}
    sortie["mecanismes"] = {"p64": pr.get("mecanismes"), "l4": l4.get("mecanismes"), "l4f32": l4f.get("mecanismes")}
    sortie["issues"] = {"p64": pr.get("issues"), "l4": l4.get("issues"), "l4f32": l4f.get("issues")}
    sortie["entieres"] = l4.get("entieres")
    sortie["requetes"] = c.get("requetes")
    sortie["non_resolues"] = c.get("non_resolues")
    sortie["distinctes"] = recolte.get("distinctes")
    sortie["representants"] = recolte.get("representants")
    sortie["parties"] = parties
    sortie["sites"] = recolte.get("sites")
    sortie["appareil"] = appareil
    sortie["transferts"] = transferts
    return sortie


def mutant_l4_tue(mutant):
    """Le mutant << L4 sans test diametral >> est tue si ses replis de l4 et de l4f32 depassent 0,1 % (ng00)."""
    try:
        recolte, appareil, passes, identite, transferts = lire_journal(mutant.get("lignes", []))
        parties = identite.get("propositions", {}).get("parties")
        return taux_replis(identite.get("propositions_l4", {}), parties) > PART_REPLIS and \
            taux_replis(identite.get("propositions_l4f32", {}), parties) > PART_REPLIS
    except Refus:
        return False


def juger(campagne):
    """campagne : {"processus", "passes", "appareil", "isolation_ok", "empreintes_stables",
    "mutants": {"cote_nul": {code, identite}, "l4": {code, lignes}}, "trames": {nom: [{code, lignes}]}} -> rapport."""
    refus = []
    rejets = {"D2": [], "D1": []}
    resultats = {}
    avec = campagne.get("appareil") is True
    if not avec:
        refus.append("appareil absent (campagne sans --appareil)")
    if campagne.get("isolation_ok") is not True:
        refus.append("appareil non isole ou isolation illisible")
    if campagne.get("empreintes_stables") is not True:
        refus.append("binaire ou source modifie pendant la campagne")
    mutants = campagne.get("mutants") or {}
    cote = mutants.get("cote_nul") or {}
    if cote.get("code") != 1 or cote.get("identite") is not False:
        refus.append("mutant cote_nul non tue (code %r, identite %r)" % (cote.get("code"), cote.get("identite")))
    if not mutant_l4_tue(mutants.get("l4") or {}):
        refus.append("mutant L4 sans test diametral non tue (replis sous 0,1 %)")
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
        trame = {"processus": len(valides), "mesures": {}, "rapports": {}}
        for v in valides:
            for d in ("D2", "D1"):
                if not v["identite"][d]:
                    rejets[d].append("%s : ecart d'identite %s" % (
                        nom, json.dumps({"communs": v["ecarts"]["communs"], d: v["ecarts"][d]}, sort_keys=True)))
                if v["replis"][d] > PART_REPLIS:
                    rejets[d].append("%s : replis %.5f au-dessus de 0,1 %%" % (nom, v["replis"][d]))
            if v["requetes"] and v["non_resolues"] > PART_NON_RESOLUES * v["requetes"]:
                refus.append("%s : %d requetes non resolues sur l'appareil" % (nom, v["non_resolues"]))
        for m in MESURES:
            entree = {"t_hote_ms": mediane([v["mesures"][m]["t_hote_ms"] for v in valides]),
                      "aa_hote": moyenne_geometrique([v["mesures"][m]["aa_hote"] for v in valides])}
            if m in AA_HOTE and not FENETRE_AA[0] <= entree["aa_hote"] <= FENETRE_AA[1]:
                refus.append("%s : A/A hote %s = %.4f hors fenetre" % (nom, m, entree["aa_hote"]))
            if avec:
                entree.update({"t_app_ms": mediane([v["mesures"][m]["t_app_ms"] for v in valides]),
                               "aa_app": moyenne_geometrique([v["mesures"][m]["aa_app"] for v in valides])})
                if m in AA_APPAREIL and not FENETRE_AA[0] <= entree["aa_app"] <= FENETRE_AA[1]:
                    refus.append("%s : A/A appareil %s = %.4f hors fenetre" % (nom, m, entree["aa_app"]))
            trame["mesures"][m] = entree
        if avec:
            for d, seuils in SEUILS.items():
                trame["rapports"][d] = {}
                for cle, seuil in seuils.items():
                    r = [v["rapports"][d][cle] for v in valides]
                    bas, haut = bootstrap(r)
                    trame["rapports"][d][cle] = {"rapport": moyenne_geometrique(r), "ic95": [bas, haut],
                                                 "seuil": seuil}
                    if haut > seuil:
                        rejets[d].append("%s : %s %s, borne haute %.4f au-dessus de %.2f" % (nom, d, cle, haut, seuil))
            trame["rapports"]["information"] = {
                cle: moyenne_geometrique([v["rapports"]["information"][cle] for v in valides])
                for cle in ("p64_appareil", "l4f32_hote", "total_p64")}
        infos = {}
        vals = [v["census_hd_sur_produit"] for v in valides if "census_hd_sur_produit" in v]
        if len(vals) == len(valides):
            infos["census_hd_sur_produit"] = moyenne_geometrique(vals)
        for cle in ("requetes", "distinctes", "representants", "parties", "sites", "non_resolues", "entieres",
                    "mecanismes", "issues", "replis", "appareil", "transferts"):
            infos[cle] = valides[0][cle]
        trame["informations"] = infos
        resultats[nom] = trame
    information_k10 = None
    if campagne.get("information_k10") is not None:
        try:
            v = resumer_processus(lire_journal(campagne["information_k10"].get("lignes", [])), PASSES_K10, avec)
            information_k10 = {"code": campagne["information_k10"].get("code"), "rapports": v.get("rapports"),
                               "mesures": v["mesures"], "identite": v["identite"], "replis": v["replis"],
                               "parties": v["parties"], "requetes": v["requetes"], "entieres": v["entieres"],
                               "mecanismes": v["mecanismes"]}
        except Refus as r:
            information_k10 = {"refus": str(r), "code": campagne["information_k10"].get("code")}
    verdicts = {}
    for d in ("D2", "D1"):
        verdicts[d] = "refuse" if refus else ("rejete" if rejets[d] else "adopte")
    if refus:
        decision = "aucune (campagne refusee)"
    elif verdicts["D2"] == "adopte":
        decision = "D2 : amendement de CONTRAT_TOUR § 8 a proposer, tranche G sur l'appareil avec l4f32 (appareil) et p64 (hote)"
    elif verdicts["D1"] == "adopte":
        decision = "D1 : tranche G sur l'appareil avec la politique l4 partagee (aucun amendement)"
    else:
        decision = "aucune tranche sur cette base"
    return {"regle": "REGLE_G_APPAREIL_2", "verdicts": verdicts, "decision": decision, "refus": refus,
            "rejets": rejets, "trames": resultats, "information_k10": information_k10, "seuils": SEUILS,
            "fenetre_aa": list(FENETRE_AA),
            "part_replis": PART_REPLIS, "tirages": TIRAGES, "graine": GRAINE}


# ------------------------------------------------------------------------------------------------ auto-test
def journal_synthetique(rapports, passes=3, aa=1.0, recolte_ok=True, ecart=None, replis=0, parties=1000):
    """rapports : appareil / hote par mesure ; ecart : (cle du bloc, champ) a mettre a 3."""
    lignes = [json.dumps({"phase": "recolte", "recolte_ok": recolte_ok, "representants": 100, "distinctes": 7,
                          "sites": 1000, "requetes": 10}),
              json.dumps({"phase": "appareil", "nom": "synthetique", "sm": 1})]
    hote = {"census": 20.0, "sondes": 16.0, "propositions": 8.0, "propositions_l4": 8.0 * rapports.get("neutre", 1.0),
            "propositions_l4f32": 15.0}
    for i in range(passes + 1):
        p = {"phase": "passe", "passe": i, "echauffement": i == 0, "ok": True}
        for m in MESURES:
            h = hote[m] * (1.0 + 0.001 * i)
            a = 8.0 * (1.0 + 0.001 * i) * rapports[m] if m.startswith("propositions") else h * rapports[m]
            p[m] = {"hote_ms": [h, h * aa], "appareil_ms": [a, a * aa], "hote_hd_ms": h * 0.6}
        lignes.append(json.dumps(p))
    mec = {"t1": parties - 100 - replis, "certificat": 100, "repli_sans_proposition": 0, "repli_certificat": replis}
    iss = {"table": parties - 100, "census": 100, "refus": 0}
    ident = {"phase": "identite", "appareil": True, "identite": True,
             "census": {"requetes": 10, "non_resolues": 0, "comparees_appareil": 10, "ecarts_hote_hd": 0,
                        "ecarts_appareil": 0, "ecarts_lot_produit": 0, "coquilles_larges": 0},
             "sondes": {"comparees_appareil": 100, "ecarts_appareil": 0, "coherentes": True},
             "propositions": {"parties": parties, "ecarts_hote_hd": 0, "ecarts_appareil": 0,
                              "mecanismes": dict(mec, repli_certificat=0, t1=parties - 100), "issues": dict(iss),
                              "issues_differentes_p64": 0},
             "propositions_l4": {"ecarts_hote_appareil": 0, "entieres": parties // 2, "mecanismes": dict(mec),
                                 "issues": dict(iss), "issues_differentes_p64": 0},
             "propositions_l4f32": {"ecarts_hote_appareil": 0, "mecanismes": dict(mec), "issues": dict(iss),
                                    "issues_differentes_p64": 0}}
    if ecart is not None:
        ident[ecart[0]][ecart[1]] = 3
        ident["identite"] = False
    lignes.append(json.dumps(ident))
    return lignes


def campagne_synthetique(rapports, aa=1.0, manque=False, recolte_ok=True, ecart=None, replis=0, mutant_cote=1,
                         mutant_l4_replis=500):
    trames = {}
    for nom, _ in TRAMES:
        prises = []
        for j in range(3):
            r = {m: rapports[m] * (1.0 + 0.01 * j) for m in MESURES}
            r["neutre"] = rapports.get("neutre", 1.0)
            prises.append({"code": 0 if ecart is None else 1,
                           "lignes": journal_synthetique(r, aa=aa, recolte_ok=recolte_ok, ecart=ecart, replis=replis)})
        if manque and nom == "max":
            prises = prises[:2]
        trames[nom] = prises
    mutant_l4 = {"code": 0, "lignes": journal_synthetique({m: 0.1 for m in MESURES}, replis=mutant_l4_replis)}
    return {"processus": 3, "passes": 3, "appareil": True, "isolation_ok": True, "empreintes_stables": True,
            "mutants": {"cote_nul": {"code": mutant_cote, "identite": mutant_cote != 1}, "l4": mutant_l4},
            "trames": trames}


def auto_test():
    # Rapports par mesure : census et sondes appareil/hote ; propositions : appareil en multiple de 8 ms (hote p64).
    d2_bon = {"census": 0.09, "sondes": 0.05, "propositions": 0.70, "propositions_l4": 0.40, "propositions_l4f32": 0.08,
              "neutre": 1.0}
    d1_seul = dict(d2_bon, propositions_l4f32=0.6)                 # D2 rejete (F_a/P_h), D1 adopte
    aucun = dict(d2_bon, propositions_l4f32=0.6, propositions_l4=0.9)  # les deux rejetes
    d1_hote_lent = dict(d1_seul, neutre=1.2)                       # D1 rejete par la neutralite de l'hote
    cas = [({"D2": "adopte", "D1": "adopte"}, campagne_synthetique(d2_bon)),
           ({"D2": "rejete", "D1": "adopte"}, campagne_synthetique(d1_seul)),
           ({"D2": "rejete", "D1": "rejete"}, campagne_synthetique(aucun)),
           ({"D2": "rejete", "D1": "rejete"}, campagne_synthetique(d1_hote_lent)),
           ({"D2": "rejete", "D1": "adopte"}, campagne_synthetique(d2_bon, ecart=("propositions_l4f32",
                                                                                 "issues_differentes_p64"))),
           ({"D2": "rejete", "D1": "rejete"}, campagne_synthetique(d2_bon, ecart=("census", "ecarts_appareil"))),
           ({"D2": "rejete", "D1": "rejete"}, campagne_synthetique(d2_bon, replis=5)),
           ({"D2": "refuse", "D1": "refuse"}, campagne_synthetique(d2_bon, mutant_cote=0)),
           ({"D2": "refuse", "D1": "refuse"}, campagne_synthetique(d2_bon, mutant_l4_replis=0)),
           ({"D2": "refuse", "D1": "refuse"}, campagne_synthetique(d2_bon, aa=1.2)),
           ({"D2": "refuse", "D1": "refuse"}, campagne_synthetique(d2_bon, manque=True)),
           ({"D2": "refuse", "D1": "refuse"}, campagne_synthetique(d2_bon, recolte_ok=False))]
    echecs = []
    for i, (attendu, camp) in enumerate(cas):
        rendu = juger(camp)["verdicts"]
        if rendu != attendu:
            echecs.append("cas %d : attendu %s, rendu %s" % (i, attendu, rendu))
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

    variantes = (("mes_g_app", ""),) + MUTANTS
    if args.sans_appareil:
        for nom, defn in variantes:
            sortie = os.path.join(args.travail, nom)
            defs = [defn] if defn else []
            jouer(commun[:1] + defs + commun[1:] + lien + ["-o", sortie], nom)
            binaires[nom] = sortie
    else:
        nvcc = trouver_nvcc(args.nvcc)
        if not nvcc:
            raise Refus("nvcc introuvable")
        cuda = os.path.dirname(os.path.dirname(os.path.realpath(nvcc)))
        for nom, defn in variantes:
            defs = [defn] if defn else []
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


def identite_du_journal(lignes):
    for ligne in lignes:
        try:
            d = json.loads(ligne)
        except ValueError:
            continue
        if type(d) is dict and d.get("phase") == "identite":
            return d.get("identite")
    return None


def campagne(args, construction):
    avec = construction.get("appareil") is True
    chemins = {nom: os.path.join(args.travail, nom) for nom in construction["binaires"]}
    empreintes = {nom: sha256(chemin) for nom, chemin in chemins.items()}
    if empreintes != construction["binaires"] or "mes_g_app" not in chemins:
        raise Refus("binaires differents de la construction")
    dossier = os.path.join(args.sortie, "journaux")
    os.makedirs(dossier, exist_ok=True)
    camp = {"processus": args.processus, "passes": args.passes, "appareil": avec, "isolation_ok": True,
            "isolation": [], "trames": {nom: [] for nom, _ in TRAMES}, "debut_utc": maintenant()}

    def jouer(executable, nom, fichier, etiquette, passes, k=5):
        avant = gpu_isole(avec)
        argv = [executable, "--trame=%s,%s,%s" % (os.path.join(args.donnees, fichier + ".u32le"),
                                                  os.path.join(args.donnees, fichier + ".ids.u32le"), nom),
                "--k=%d" % k, "--fils=%d" % args.fils, "--passes=%d" % passes]
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
            camp["trames"][nom].append(jouer(chemins["mes_g_app"], nom, fichier, "%s_p%d" % (nom, i), args.passes))
    camp["mutants"] = {}
    for nom_binaire, _ in MUTANTS:
        cle = "cote_nul" if nom_binaire.endswith("cote_nul") else "l4"
        m = jouer(chemins[nom_binaire], "ng00", "lidar_ng00", nom_binaire, 1)
        camp["mutants"][cle] = {"code": m["code"], "identite": identite_du_journal(m["lignes"]),
                                "journal": m["journal"], "lignes": m["lignes"]}
    # Information (sans verdict) : ng00 a K10, un processus, deux passes ; les propositions y pesent trois fois plus.
    camp["information_k10"] = jouer(chemins["mes_g_app"], "ng00_k10", "lidar_ng00", "information_k10", PASSES_K10, 10)
    camp["empreintes_stables"] = {nom: sha256(chemin) for nom, chemin in chemins.items()} == empreintes and \
        all(sha256(os.path.join(ICI, nom)) == construction["sources"][nom] for nom in SOURCES)
    camp["fin_utc"] = maintenant()
    ecrire_json(os.path.join(args.sortie, "campagne.json"), camp)
    return camp


# ------------------------------------------------------------------------------------------------ rapport
def tableaux(rapport):
    lignes = ["# MES-G-APP, etape 2 : propositions sur l'appareil (genere par pilote_g_appareil.py)", "",
              "REGLE_G_APPAREIL_2 : D2 **%s**, D1 **%s** ; decision : %s" % (
                  rapport["verdicts"]["D2"], rapport["verdicts"]["D1"], rapport["decision"]), ""]
    if rapport["refus"]:
        lignes.append("Refus :")
        lignes += ["- " + x for x in rapport["refus"]]
        lignes.append("")
    for d in ("D2", "D1"):
        if rapport["rejets"][d]:
            lignes.append("Rejets %s :" % d)
            lignes += ["- " + x for x in rapport["rejets"][d]]
            lignes.append("")
    lignes += ["| trame | mesure | hote (ms) | appareil (ms) | A/A hote | A/A appareil |",
               "| --- | --- | ---: | ---: | ---: | ---: |"]
    for nom, trame in rapport["trames"].items():
        for m in MESURES:
            e = trame["mesures"][m]
            lignes.append("| %s | %s | %.3f | %s | %.3f | %s |" % (
                nom, m, e["t_hote_ms"], "%.3f" % e["t_app_ms"] if "t_app_ms" in e else "-", e["aa_hote"],
                "%.3f" % e["aa_app"] if "aa_app" in e else "-"))
    lignes += ["", "| trame | conception | rapport | valeur | IC 95 % | seuil |", "| --- | --- | --- | ---: | --- | ---: |"]
    for nom, trame in rapport["trames"].items():
        for d in ("D2", "D1"):
            for cle, e in trame.get("rapports", {}).get(d, {}).items():
                lignes.append("| %s | %s | %s | %.4f | %.4f-%.4f | %.2f |" % (
                    nom, d, cle, e["rapport"], e["ic95"][0], e["ic95"][1], e["seuil"]))
    lignes += ["", "Informations (non jugees) :", "", "```json",
               json.dumps({n: {"rapports": t.get("rapports", {}).get("information"), "informations": t.get(
                   "informations")} for n, t in rapport["trames"].items()}, indent=1, sort_keys=True), "```", "",
               "Information K10 (ng00, un processus, sans verdict) :", "", "```json",
               json.dumps(rapport.get("information_k10"), indent=1, sort_keys=True), "```", ""]
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
    p = argparse.ArgumentParser(description="MES-G-APP, etape 2 : pilote et juge (REGLE_G_APPAREIL_2)")
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
            print(json.dumps({"verdicts": r["verdicts"], "decision": r["decision"], "refus": r["refus"][:10],
                              "rejets": {d: v[:5] for d, v in r["rejets"].items()}}, ensure_ascii=False))
        return 0
    except Refus as r:
        print(json.dumps({"refus": str(r)}, ensure_ascii=False))
        return 3


if __name__ == "__main__":
    sys.exit(principal(sys.argv[1:]))
