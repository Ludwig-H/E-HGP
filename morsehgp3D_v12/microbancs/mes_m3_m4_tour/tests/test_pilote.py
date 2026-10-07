#!/usr/bin/env python3
"""Porte du pilote de MES-M3 et MES-M4 (pilote.py) : constats CST-0018, CST-0213 et CST-0214.

Partie A, sorties REELLES du recu G4 g4_t0b_20261007 (aucun vidage : inventaires, empreintes et journaux publies) :
  les validateurs du pilote acceptent toutes les sorties reelles (vidages, portes, MES-M3 cinq et trois processus,
  variante « mere puis Welzl », MES-M4, mutant), sans faux refus ; LEM-T6 y juge toutes les naissances des ordres
  2..K ; la prise unique de resolution de chaque cas K10 est relue (rapports 0,542 / 0,541 / 0,547) et le juge de
  MES-M3 rend « refuse » faute de cinq prises (CST-0213 sur donnees reelles). Rejouer cette prise cinq fois (rejeu,
  pas une mesure) donne « adopte » avec la moyenne publiee : seule la replication manquait.
Partie B, injections de l'auditeur (recu audit_socle_microbancs_20261007/preuves/probe_pilotes.py) et autres preuves
  manquantes, par des binaires SIMULES (scripts) sur les vidages synthetiques du carre (tests/carre.py) :
  - processus silencieux (code 0 ou 1 sans sortie) : MES-M3 et MES-M4 refuses (code 3), routes jamais declarees
    identiques, mutant jamais declare tue ; portes muettes : aucune conforme ;
  - resolution : --processus 5 lance cinq vidages par cas ; une seconde campagne s'ajoute sans effacer la premiere ;
    vidages reecrits differents, ordre manquant, graines differentes, temps non fini : prise refusee ;
  - binaire remplace apres « construire », vidage sans FLOWER, vidages modifies avant MES-M4 : refus ;
  - juge de MES-M3 sur rapports synthetiques : adopte, rejete (borne, identite), refuse (4 prises, campagne
    incomplete plus recente, binaire different, vidages differents, preuve d'identite absente).
Partie C (--binaires DIR, binaires reels construits) : mhgp12_mes_m4 et son mutant dans le pilote, sur le carre :
  conforme avec six naissances jugees par LEM-T6, mutant tue ; mhgp12_mes_m3 sur le carre (zero partie) : refus pour
  preuve vide.

Usage : python3 -S -O tests/test_pilote.py [--recu-g4 DOSSIER_g4_t0b] [--binaires DIR]
Bibliotheque standard ; aucune garde par assert. Codes : 0 conforme, 1 ecart (detail en JSON), 2 usage.
"""
import contextlib
import copy
import importlib.util
import io
import json
import os
import shutil
import stat
import sys
import tempfile
import types
from pathlib import Path

ICI = Path(__file__).resolve().parent
sys.dont_write_bytecode = True  # aucune trace dans l'arbre des sources (hachees par les pilotes)
sys.path.insert(0, str(ICI))
from carre import make  # noqa: E402  (generateur partage du carre de l'auditeur)

PILOTE = ICI.parent / "pilote.py"
RECU_G4 = ICI.parents[2] / "receipts" / "g4_t0b_20261007"
LOTS = (("002_m34_k5_publier/files/m34_k5", 5, ("ng00", "ng01", "ng02")),
        ("004_m34_k10_publier/files/m34_k10", 10, ("ng00", "ng01", "ng02")))

FAUX = r'''#!/usr/bin/env python3
# Binaire SIMULE de la porte du pilote (jamais un binaire de mesure). Comportement : FAUX_SCENARIO (JSON) par nom.
import json, os, shutil, sys
nom = os.path.basename(sys.argv[0])
sc = json.loads(os.environ.get("FAUX_SCENARIO", "{}"))
mode = sc.get(nom, "conforme")
with open(os.environ["FAUX_COMPTEUR"], "a") as f:
    f.write(nom + " " + " ".join(sys.argv[1:]) + "\n")
if mode.startswith("silencieux_"):
    sys.exit(int(mode.split("_")[1]))
if nom == "mhgp12_mes_m4" and mode == "t6_vide":  # ancien comportement sans FLOWER : succes, zero naissance jugee
    comptes = {1: (4, 5, 12, 5), 2: (4, 1, 4, 5), 3: (1, 0, 0, 1), 4: (1, 0, 0, 1)}
    print(json.dumps(dict(phase="entree", trame="audit_square", K=4, repetitions=1, mutant_sans_contraction=False)))
    for k, (nb, nc, ns, nn) in comptes.items():
        print(json.dumps(dict(phase="ordre", k=k, naissances=nb, cellules=nc, representants=ns, noeuds=nn,
                              noeuds_v11=nn, fusions=nn - nb, racine_unique=True,
                              identite=dict(identiques=True), lem_t6=dict(naissances_jugees=0, ecarts=0),
                              temps_un_fil_s=dict(naissances=1e-6, noyau=1e-6, contraction=1e-6),
                              contraction_parallele=dict(fils=2, secondes=1e-6, identique=True))))
    print(json.dumps(dict(phase="fin", code=0)))
    sys.exit(0)
if nom != "mhgp12_vidage":
    sys.exit(0)
a = sys.argv[1:]
trame, K, dossier, opts = a[2], int(a[3]), a[6], a[7:]
val = lambda o, d=None: opts[opts.index(o) + 1] if o in opts else d
ful1, journal, R = val("--ful1"), val("--journal", "tous"), int(val("--chrono-resolution", "0"))
modele = os.environ["FAUX_MODELE"]
for f in sorted(os.listdir(modele)):
    shutil.copyfile(os.path.join(modele, f), os.path.join(dossier, f))
if journal == "aucun" and mode == "vidage_altere":
    with open(os.path.join(dossier, "cat.bin"), "ab") as h:
        h.write(b"\0" * 8)
if ful1:
    open(ful1, "wb").write(b"FUL1-SIMULE")
out = [dict(phase="domaine", trame=trame, K=K, coord_bits=21, sites=4), dict(phase="forets_publiees")]
if ful1:
    out.append(dict(phase="ful1"))
out += [dict(phase="catalogue_vide"), dict(phase="journaux_v11", ordres=0 if journal == "aucun" else K,
                                           forets_serie_identiques=True)]
for k in range(1, K + 1):
    out.append(dict(phase="ordre", k=k, traces=10 * k))
    out.append(dict(phase="graines", k=k, journal_v11_compare=journal != "aucun",
                    identiques=True if journal != "aucun" else None))
if R > 0:
    rapport = sc.get("rapport", 0.5)
    for k in range(2, K + 1):
        if mode == "ordre_manquant" and k == K:
            continue
        r12 = float("nan") if mode == "temps_non_fini" else rapport
        out.append(dict(phase="resolution_un_fil", k=k, traces=10 * k, graines_identiques=mode != "graines_fausses",
                        secondes=dict(v11=1.25, replique_v11=1.0, replique_v12=r12), rapport_v12_sur_replique_v11=r12))
    out.append(dict(phase="resolution_fin"))
out += [dict(phase="fin"), dict(phase="exit", status="ok", reason="none", order=0)]
print("\n".join(json.dumps(x) for x in out))
'''


class Ecart(Exception):
    pass


def exiger(condition, message):
    if not condition:
        raise Ecart(message)


def charger(nom="pilote_sous_test"):
    spec = importlib.util.spec_from_file_location(nom, PILOTE)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def lire_jsonl(chemin):
    out = []
    for ligne in Path(chemin).read_text().splitlines():
        ligne = ligne.strip()
        if ligne.startswith("{"):
            out.append(json.loads(ligne))
    return out


# ---- Partie A : sorties reelles du recu G4 ---------------------------------------------------------------------------
def partie_a(m, recu):
    resultats = []
    base = recu / "resultats" / "cmd"
    rapports = {}
    for lot, k_max, trames in LOTS:
        dossier = base / lot
        rapport = json.loads((dossier / "rapport_mes_m3_m4.json").read_text())
        rapports[k_max] = (dossier, rapport)
        for trame in trames:
            cas = "%s_k%d" % (trame, k_max)
            fichiers = rapport["vidages"][cas]["fichiers"]
            for nom, f in fichiers.items():  # inventaires publies : sections exigees (FLOWER a k >= 2) presentes
                inv = f["inventaire"]
                attendu = m.SECTIONS[inv["genre"]].keys() - ({"FLOWER"} if inv["genre"] == 3 and inv["ordre"] < 2
                                                             else set())
                exiger(set(inv["sections"]) == set(attendu), "%s/%s : sections %s" % (cas, nom, sorted(inv["sections"])))
            lignes = lire_jsonl(dossier / cas / "vidage.jsonl")
            chrono = 3 if k_max <= 5 else 1
            p = m.valider_vidage(lignes, 0, trame, k_max, True, chrono, True)
            exiger(not p, "%s : vidage reel refuse : %s" % (cas, p))
            processus = rapport["mes_m3"][cas]["processus"]
            for i in range(len(processus)):
                p, ident = m.valider_m3(lire_jsonl(dossier / cas / ("mes_m3_%d.jsonl" % i)), processus[i]["code"],
                                        trame, k_max, fichiers)
                exiger(not p and ident, "%s : MES-M3 processus %d refuse : %s" % (cas, i, p))
            processus = rapport["mes_m4"][cas]["processus"]
            jugees = 0
            for i in range(len(processus)):
                lignes_m4 = lire_jsonl(dossier / cas / ("mes_m4_%d.jsonl" % i))
                p, ident = m.valider_m4(lignes_m4, processus[i]["code"], trame, k_max, fichiers, 48)
                exiger(not p and ident, "%s : MES-M4 processus %d refuse : %s" % (cas, i, p))
                jugees = sum(l["lem_t6"]["naissances_jugees"] for l in lignes_m4 if l.get("phase") == "ordre")
            p, ordres, totaux = m.valider_resolution(lire_jsonl(dossier / cas / "vidage.jsonl"), k_max)
            publie = rapport["synthese"]["resolution"][cas]["rapport_total_v12_sur_replique_v11"]
            exiger(not p and round(totaux["replique_v12"] / totaux["replique_v11"], 3) == publie,
                   "%s : resolution relue %s" % (cas, p))
            resultats.append({"partie": "A", "cas": cas, "processus_m3": len(rapport["mes_m3"][cas]["processus"]),
                              "processus_m4": len(processus), "naissances_jugees_lem_t6_par_processus": jugees,
                              "rapport_resolution_prise_unique": publie, "sorties_reelles_admises": True})
    dossier5, rapport5 = rapports[5]
    for nom, exe, options, attendu in m.PORTES:
        lignes = lire_jsonl(dossier5 / ("porte_%s.jsonl" % nom))
        p = m.valider_porte(nom, rapport5["portes"][nom]["code"], lignes)
        exiger(not p, "porte reelle %s refusee : %s" % (nom, p))
    fichiers = rapport5["vidages"]["ng00_k5"]["fichiers"]
    for nom, code in (("normal", 0), ("mutant_sans_s_dans_f", 1)):
        p, reponse, comptes, _ = m.valider_variante(nom, code, lire_jsonl(dossier5 / "ng00_k5" / (
            "mes_m3_mere_%s.jsonl" % nom)), 5, fichiers)
        exiger(not p and reponse, "variante reelle %s : %s" % (nom, p))
    tue, geo, complete = m.valider_mutant_m4(1, lire_jsonl(dossier5 / "ng00_k5" / "mes_m4_mutant.jsonl"))
    exiger(tue and geo and complete, "mutant reel de MES-M4 non reconnu tue")
    resultats.append({"partie": "A", "cas": "portes_variante_mutant_reels", "portes": 4, "variante": "conforme",
                      "mutant_m4_tue_par_ecart": True})
    # Juge de MES-M3 sur les sorties reelles K10 : prise unique (refus), puis rejeu x5 de cette prise (arithmetique).
    dossier10, rapport10 = rapports[10]
    synth = rapport_synthetique(m, rapport10, dossier10, dossier5)
    verdict = m.juger_m3(synth)
    exiger(verdict["verdict"] == "refuse" and all("1 prises valides sur 5" in r for r in verdict["refus"]),
           "prise unique : %s" % verdict["refus"])
    rejeu = rapport_synthetique(m, rapport10, dossier10, dossier5, repetition=5)
    v5 = m.juger_m3(rejeu)
    exiger(v5["verdict"] == "adopte", "rejeu x5 : %s %s" % (v5["refus"], v5["rejets"]))
    resultats.append({"partie": "A", "cas": "juge_m3_sorties_reelles_k10", "prise_unique": verdict["verdict"],
                      "rejeu_x5_de_la_meme_prise": v5["verdict"],
                      "moyennes": {c: round(v["moyenne_geometrique"], 3) for c, v in v5["cas"].items()},
                      "note": "rejeu d'une prise reelle, pas une mesure : seule la replication manquait"})
    return resultats


def rapport_synthetique(m, rapport10, dossier10, dossier5, repetition=1):
    """Rapport du nouveau format construit depuis les sorties reelles du recu (binaires de la session non haches par
    l'ancien pilote : empreintes de substitution coherentes, marquees comme telles)."""
    binaires = {n: "empreinte-de-substitution-" + n for n in m.BINAIRES}
    prov = lambda n: {"binaire": n, "sha256": binaires[n], "inchange": True, "construit": True}
    r = {"construction": {"binaires": binaires}, "vidages": {}, "mes_m3": {}, "resolution": {}, "portes": {}}
    for nom, exe, _, attendu in m.PORTES:
        r["portes"][nom] = {"conforme": True, "binaire": prov(exe), "journal": "porte_%s.jsonl" % nom,
                            "journal_sha256": m.sha256(str(Path(dossier5) / ("porte_%s.jsonl" % nom)))}
    for cas in m.REGLE_M3["cas_decisifs"]:
        fichiers = rapport10["vidages"][cas]["fichiers"]
        r["vidages"][cas] = {"conforme": True, "fichiers": fichiers}
        ref = m.empreintes(fichiers)
        processus = [{"binaire": prov("mhgp12_mes_m3"), "journal": "%s/mes_m3_%d.jsonl" % (cas, i),
                      "journal_sha256": m.sha256(str(Path(dossier10) / cas / ("mes_m3_%d.jsonl" % i)))}
                     for i in range(3)]
        r["mes_m3"][cas] = {"complet": True, "identique": True, "vidages_sha256": ref, "processus": processus}
        _, ordres, totaux = m.valider_resolution(lire_jsonl(Path(dossier10) / cas / "vidage.jsonl"), 10)
        prise = {"valide": True, "binaire": prov("mhgp12_vidage"), "journal": "%s/vidage.jsonl" % cas,
                 "journal_sha256": m.sha256(str(Path(dossier10) / cas / "vidage.jsonl")),
                 "rapport_total": totaux["replique_v12"] / totaux["replique_v11"], "ordres": ordres}
        r["resolution"][cas] = {"campagnes": {"g4_t0b": {
            "debut": "2026-10-07T11:39:00Z", "processus_demandes": repetition, "passes_par_processus": 1,
            "statistique_intra_processus": "minimum de 1 passe(s)", "vidages_reference": ref,
            "prises": [dict(prise, processus=i) for i in range(repetition)], "refus": []}}}
    return r


# ---- Partie B : injections par binaires simules ----------------------------------------------------------------------
class Banc:
    """Dossiers temporaires, binaires simules et arguments du pilote pour un scenario."""

    REELS = ("mhgp12_mes_m4", "mhgp12_mes_m4_mutant_sans_contraction", "mhgp12_mes_m3",
             "mhgp12_mes_m3_mutant_sans_s_dans_f")

    def __init__(self, m, tmp, scenario=None, flower="valid", reels=None):
        self.m = m
        self.tmp = Path(tmp)
        self.modele = self.tmp / "modele"
        self.modele.mkdir()
        make(self.modele, flower)
        self.construction = self.tmp / "construction"
        self.construction.mkdir()
        for nom in m.BINAIRES:
            chemin = self.construction / nom
            if reels and nom in self.REELS and (Path(reels) / nom).is_file():
                shutil.copyfile(Path(reels) / nom, chemin)
            else:
                chemin.write_text(FAUX)
            chemin.chmod(chemin.stat().st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)
        self.v11 = self.tmp / "v11build"
        self.v11.mkdir()
        (self.v11 / "libmhgp11.a").write_bytes(b"BIBLIOTHEQUE_SIMULEE")
        self.compteur = self.tmp / "appels.txt"
        self.compteur.write_text("")
        self.scenario = scenario or {}
        os.environ["FAUX_SCENARIO"] = json.dumps(self.scenario)
        os.environ["FAUX_MODELE"] = str(self.modele)
        os.environ["FAUX_COMPTEUR"] = str(self.compteur)
        self.rapport = {}
        self.args = None
        self.numero = 0
        self.nouvelle_campagne()
        self.rapport["construction"] = m.enregistrer_binaires(self.args, {"code": 0, "etapes": []})

    def nouvelle_campagne(self, **options):
        valeurs = dict(sortie=str(self.tmp / "out"), construction=str(self.construction), cas="audit_square:4",
                       processus=5, repetitions_k5=1, repetitions_k10=1, repetitions_m4=1, fils_contraction=2,
                       donnees=str(self.tmp / "donnees"), fils=1, chrono_k5=1, chrono_k10=1, chrono_vidage="non",
                       journal="tous", garder_ful1=False, v11_build=str(self.v11), v11_source=str(self.tmp),
                       campagne="c%02d_test" % (self.numero + 1))
        valeurs.update(options)
        self.numero += 1
        self.args = types.SimpleNamespace(**valeurs)
        os.makedirs(self.args.sortie, exist_ok=True)
        return self.args

    def scenario_courant(self, scenario):
        self.scenario = scenario
        os.environ["FAUX_SCENARIO"] = json.dumps(scenario)

    def etape(self, nom):
        fonction = {"portes": self.m.portes, "vider": self.m.vider, "resolution": self.m.resolution,
                    "m3": self.m.m3, "m3var": self.m.m3_variante, "m4": self.m.m4}[nom]
        with contextlib.redirect_stdout(io.StringIO()):
            return fonction(self.args, self.rapport)

    def appels(self, binaire):
        return [l for l in self.compteur.read_text().splitlines() if l.split(" ", 1)[0] == binaire]


def partie_b(m):
    resultats = []
    cas = "audit_square_k4"
    with tempfile.TemporaryDirectory(prefix="pilote-") as tmp:  # processus silencieux (injection de l'auditeur)
        b = Banc(m, tmp)
        exiger(b.etape("vider") == 0 and b.rapport["vidages"][cas]["conforme"], "vidage simule refuse : %s" %
               b.rapport["vidages"][cas]["refus"])
        b.scenario_courant({"mhgp12_mes_m3": "silencieux_0", "mhgp12_mes_m4": "silencieux_0",
                            "mhgp12_mes_m4_mutant_sans_contraction": "silencieux_1"})
        c3, c4 = b.etape("m3"), b.etape("m4")
        bloc3, bloc4 = b.rapport["mes_m3"][cas], b.rapport["mes_m4"][cas]
        mutant = b.rapport["mes_m4"]["mutant_sans_contraction_" + cas]
        exiger(c3 == 3 and not bloc3["conforme"] and bloc3["routes_identiques_entre_processus"] is False and
               len(bloc3["processus"]) == 5, "M3 silencieux : code %d, %s" % (c3, bloc3["refus"][:2]))
        exiger(c4 == 3 and not bloc4["conforme"] and not mutant["tue"], "M4 silencieux : code %d, mutant %s" % (
            c4, mutant))
        synth = m.synthese(b.rapport)
        exiger(synth["mes_m3"][cas]["conforme"] is False and synth["mes_m4"][cas]["conforme"] is False,
               "synthese : conformite sans preuve")
        resultats.append({"partie": "B", "cas": "processus_silencieux", "code_m3": c3, "code_m4": c4,
                          "routes_declarees_identiques": bloc3["routes_identiques_entre_processus"],
                          "mutant_declare_tue": mutant["tue"], "avant_correction": "codes 0, routes vraies, mutant tue"})
        b.scenario_courant({n: ("silencieux_1" if "mutant" in n else "silencieux_0") for n in m.BINAIRES})
        cp = b.etape("portes")
        exiger(cp == 1 and not any(v["conforme"] for v in b.rapport["portes"].values()),
               "portes muettes : %s" % {k: v["verdict"] for k, v in b.rapport["portes"].items()})
        resultats.append({"partie": "B", "cas": "portes_muettes", "code": cp,
                          "verdicts": {k: v["verdict"] for k, v in b.rapport["portes"].items()}})

    with tempfile.TemporaryDirectory(prefix="pilote-") as tmp:  # succes sans verticales jugees (CST-0214)
        b = Banc(m, tmp)
        b.etape("vider")
        b.scenario_courant({"mhgp12_mes_m4": "t6_vide", "mhgp12_mes_m4_mutant_sans_contraction": "silencieux_1"})
        code = b.etape("m4")
        refus = b.rapport["mes_m4"][cas]["refus"]
        exiger(code == 3 and not b.rapport["mes_m4"][cas]["conforme"] and
               sum("LEM-T6" in r for r in refus) == 3 * 5, "M4 sans naissance jugee admis : %s" % refus[:3])
        resultats.append({"partie": "B", "cas": "m4_succes_sans_verticales_jugees", "code": code,
                          "ordres_refuses_par_processus": 3, "avant_correction": "code 0, zero naissance jugee"})

    with tempfile.TemporaryDirectory(prefix="pilote-") as tmp:  # replication et non-ecrasement (CST-0213)
        b = Banc(m, tmp, {"rapport": 0.1})
        b.etape("vider")
        avant = len(b.appels("mhgp12_vidage"))
        c1 = b.etape("resolution")
        premiere = b.args.campagne
        apres1 = len(b.appels("mhgp12_vidage"))
        b.nouvelle_campagne()
        b.scenario_courant({"rapport": 0.2})
        c2 = b.etape("resolution")
        campagnes = b.rapport["resolution"][cas]["campagnes"]
        exiger(c1 == 0 and c2 == 0 and apres1 - avant == 5 and len(b.appels("mhgp12_vidage")) - apres1 == 5,
               "replication : codes %d %d, appels %d puis %d" % (c1, c2, apres1 - avant,
                                                                 len(b.appels("mhgp12_vidage")) - apres1))
        exiger(len(campagnes) == 2 and all(c["prises_valides"] == 5 for c in campagnes.values()) and
               all(abs(p["rapport_total"] - 0.1) < 1e-12 for p in campagnes[premiere]["prises"]) and
               all(abs(p["rapport_total"] - 0.2) < 1e-12 for p in campagnes[b.args.campagne]["prises"]),
               "seconde campagne : premiere effacee ou modifiee")
        journaux = {p["journal"] for c in campagnes.values() for p in c["prises"]}
        exiger(len(journaux) == 10 and all((Path(b.args.sortie) / j).is_file() for j in journaux),
               "journaux de prises ecrases")
        restes = list(Path(b.args.sortie, cas, "resolution").rglob("*.bin"))
        exiger(not restes, "vidages des prises non effaces : %d" % len(restes))
        resultats.append({"partie": "B", "cas": "resolution_repliquee_sans_ecrasement", "processus_demandes": 5,
                          "vidages_lances_par_campagne": [apres1 - avant, len(b.appels("mhgp12_vidage")) - apres1],
                          "campagnes_conservees": len(campagnes), "journaux_distincts": len(journaux),
                          "avant_correction": "une commande pour cinq processus, seconde prise ecrasant la premiere"})
        for mode, raison in (("vidage_altere", "vidages reecrits differents"), ("ordre_manquant", "ordres"),
                             ("graines_fausses", "graines identiques"), ("temps_non_fini", "temps finis")):
            b.nouvelle_campagne(processus=2)
            b.scenario_courant({"mhgp12_vidage": mode})
            code = b.etape("resolution")
            camp = b.rapport["resolution"][cas]["campagnes"][b.args.campagne]
            exiger(code == 3 and camp["prises_valides"] == 0 and camp["refus"] and
                   all(any(raison in r for r in p["raisons"]) for p in camp["prises"]),
                   "%s : code %d, %s" % (mode, code, [p["raisons"] for p in camp["prises"]]))
            resultats.append({"partie": "B", "cas": "prise_" + mode, "code": code, "prises_valides": 0})

    with tempfile.TemporaryDirectory(prefix="pilote-") as tmp:  # provenance : binaire remplace apres construire
        b = Banc(m, tmp)
        chemin = Path(b.construction) / "mhgp12_vidage"
        chemin.write_text(FAUX + "# remplace\n")
        code = b.etape("vider")
        exiger(code == 3 and not b.rapport["vidages"][cas]["conforme"] and
               any("provenance" in r for r in b.rapport["vidages"][cas]["refus"]), "binaire remplace admis")
        resultats.append({"partie": "B", "cas": "binaire_remplace_apres_construire", "code": code})

    with tempfile.TemporaryDirectory(prefix="pilote-") as tmp:  # vidage sans verticales (CST-0214)
        b = Banc(m, tmp, flower="absent")
        code = b.etape("vider")
        exiger(code == 3 and any("FLOWER" in r for r in b.rapport["vidages"][cas]["refus"]),
               "vidage sans FLOWER admis : %s" % b.rapport["vidages"][cas]["refus"])
        code4 = b.etape("m4")
        exiger(code4 == 3 and not b.appels("mhgp12_mes_m4"), "M4 joue sur un vidage refuse")
        resultats.append({"partie": "B", "cas": "vidage_sans_flower", "code_vider": code, "code_m4": code4,
                          "processus_m4_lances": 0})

    with tempfile.TemporaryDirectory(prefix="pilote-") as tmp:  # vidages modifies entre vider et MES-M4
        b = Banc(m, tmp)
        b.etape("vider")
        with open(Path(b.args.sortie, cas, "ordre_2.bin"), "r+b") as f:
            f.seek(80)
            f.write(b"\x01")
        code = b.etape("m4")
        exiger(code == 3 and not b.appels("mhgp12_mes_m4") and
               any("empreinte differente" in r for r in b.rapport["mes_m4"][cas]["refus"]),
               "vidage modifie admis par MES-M4")
        resultats.append({"partie": "B", "cas": "vidages_modifies_avant_m4", "code": code})

    resultats += juge_m3_synthetique(m)
    return resultats


def juge_m3_synthetique(m):
    """Juge de MES-M3 sur des rapports synthetiques : chaque preuve manquante refuse ; la mesure rejette."""
    resultats = []
    binaires = {n: "h-" + n for n in m.BINAIRES}
    prov = lambda n: {"binaire": n, "sha256": binaires[n], "inchange": True, "construit": True}

    def rapport(rapports=(0.55, 0.54, 0.53, 0.55, 0.54)):
        r = {"construction": {"binaires": dict(binaires)}, "vidages": {}, "mes_m3": {}, "resolution": {}, "portes": {}}
        for nom, exe, _, _ in m.PORTES:
            r["portes"][nom] = {"conforme": True, "binaire": prov(exe), "journal": "p_" + nom, "journal_sha256": "x"}
        for cas in m.REGLE_M3["cas_decisifs"]:
            ref = {"cat.bin": "a-" + cas}
            r["vidages"][cas] = {"conforme": True, "fichiers": {"cat.bin": {"sha256": "a-" + cas}}}
            r["mes_m3"][cas] = {"complet": True, "identique": True, "vidages_sha256": ref,
                                "processus": [{"binaire": prov("mhgp12_mes_m3"), "journal": cas + "/m3",
                                               "journal_sha256": "y"}]}
            r["resolution"][cas] = {"campagnes": {"c1": {
                "debut": "2026-10-07T12:00:00Z", "processus_demandes": len(rapports), "vidages_reference": ref,
                "prises": [{"valide": True, "binaire": prov("mhgp12_vidage"), "journal": "%s/p%d" % (cas, i),
                            "journal_sha256": "z", "rapport_total": x} for i, x in enumerate(rapports)],
                "refus": []}}}
        return r

    cas_tests = []
    cas_tests.append(("complet", rapport(), "adopte"))
    cas_tests.append(("borne_haute_au_dela_de_0_60", rapport((0.58, 0.62, 0.66, 0.59, 0.61)), "rejete"))
    r = rapport()
    r["mes_m3"]["ng01_k10"]["identique"] = False
    cas_tests.append(("identite_mes_m3_en_defaut", r, "rejete"))
    cas_tests.append(("quatre_prises", rapport((0.5, 0.5, 0.5, 0.5)), "refuse"))
    r = rapport()
    r["resolution"]["ng02_k10"]["campagnes"]["c2"] = dict(copy.deepcopy(r["resolution"]["ng02_k10"]["campagnes"]["c1"]),
                                                          debut="2026-10-07T13:00:00Z", refus=["2 prises valides sur 5"])
    cas_tests.append(("campagne_plus_recente_incomplete", r, "refuse"))
    r = rapport()
    r["resolution"]["ng00_k10"]["campagnes"]["c1"]["prises"][3]["binaire"] = dict(prov("mhgp12_vidage"), sha256="autre")
    cas_tests.append(("prise_d_un_autre_binaire", r, "refuse"))
    r = rapport()
    r["resolution"]["ng00_k10"]["campagnes"]["c1"]["vidages_reference"] = {"cat.bin": "autre"}
    cas_tests.append(("vidages_de_reference_differents", r, "refuse"))
    r = rapport()
    del r["mes_m3"]["ng02_k10"]
    cas_tests.append(("identite_mes_m3_absente", r, "refuse"))
    r = rapport()
    r["portes"]["mes_m3_mutant_sans_s_dans_f"]["conforme"] = False
    cas_tests.append(("mutant_de_porte_non_tue", r, "refuse"))
    r = rapport()
    del r["resolution"]["ng01_k10"]
    cas_tests.append(("cas_qui_decide_sans_resolution", r, "refuse"))
    r = rapport()
    r["resolution"]["ng00_k10"]["campagnes"]["c1"]["prises"][0]["valide"] = False
    cas_tests.append(("prise_invalide_dans_la_campagne", r, "refuse"))
    for nom, r, attendu in cas_tests:
        v = m.juger_m3(r)
        exiger(v["verdict"] == attendu, "juge M3 %s : %s (refus %s, rejets %s)" % (nom, v["verdict"], v["refus"],
                                                                                v["rejets"]))
        if attendu == "adopte":
            exiger(all(c["prises"] == 5 and c["ic95"][1] <= 0.60 for c in v["cas"].values()) and v["preuves"],
                   "adoption sans preuves citees")
        resultats.append({"partie": "B", "cas": "juge_m3_" + nom, "verdict": v["verdict"]})
    return resultats


# ---- Partie C : binaires reels dans le pilote ------------------------------------------------------------------------
def partie_c(m, binaires):
    resultats = []
    cas = "audit_square_k4"
    with tempfile.TemporaryDirectory(prefix="pilote-") as tmp:
        b = Banc(m, tmp, reels=binaires)
        b.nouvelle_campagne(processus=2)
        exiger(b.etape("vider") == 0, "vidage simule refuse")
        c4 = b.etape("m4")
        bloc = b.rapport["mes_m4"][cas]
        mutant = b.rapport["mes_m4"]["mutant_sans_contraction_" + cas]
        jugees = [l["lem_t6"]["naissances_jugees"] for l in bloc["processus"][0]["lignes"] if l.get("phase") == "ordre"]
        exiger(c4 == 0 and bloc["conforme"] and mutant["tue"] and sum(jugees) == 6,
               "M4 reel sur le carre : code %d, %s, mutant %s, jugees %s" % (c4, bloc["refus"][:2], mutant.get("tue"),
                                                                           jugees))
        v = m.juger_m4(dict(b.rapport, portes={}))
        exiger(v["verdict"] == "refuse" and any("porte" in r for r in v["refus"]), "M4 conforme sans porte")
        resultats.append({"partie": "C", "cas": "mes_m4_reel_carre", "code": c4, "naissances_jugees": jugees,
                          "mutant_tue": mutant["tue"], "verdict_sans_portes": v["verdict"]})
        if (Path(binaires) / "mhgp12_mes_m3").is_file():
            c3 = b.etape("m3")
            refus = b.rapport["mes_m3"][cas]["refus"]
            exiger(c3 == 3 and any("aucune partie" in r for r in refus), "M3 sur zero partie : code %d %s" % (c3, refus))
            resultats.append({"partie": "C", "cas": "mes_m3_reel_carre_zero_partie", "code": c3,
                              "refus": "preuve vide"})
    return resultats


def main(argv):
    recu, binaires = RECU_G4, None
    i = 1
    while i < len(argv):
        if argv[i] == "--recu-g4" and i + 1 < len(argv):
            recu = Path(argv[i + 1])
        elif argv[i] == "--binaires" and i + 1 < len(argv):
            binaires = Path(argv[i + 1])
        else:
            print(__doc__)
            return 2
        i += 2
    m = charger()
    resultats, ecarts = [], []
    if (recu / "resultats").is_dir():
        try:
            resultats += partie_a(m, recu)
        except Ecart as e:
            ecarts.append(str(e))
    else:
        ecarts.append("recu G4 absent : %s (preuve positive non jouee)" % recu)
    try:
        resultats += partie_b(m)
    except Ecart as e:
        ecarts.append(str(e))
    if binaires is not None:
        try:
            resultats += partie_c(m, binaires)
        except Ecart as e:
            ecarts.append(str(e))
    for r in resultats:
        print(json.dumps(r, ensure_ascii=False, sort_keys=True))
    print(json.dumps({"porte": "pilote_m3_m4", "cas": len(resultats), "ecarts": ecarts, "binaires_reels":
                      binaires is not None, "optimise": sys.flags.optimize, "python": sys.version.split()[0]},
                     ensure_ascii=False))
    return 0 if not ecarts else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
