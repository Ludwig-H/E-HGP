#!/usr/bin/env python3
"""Test du juge du pilote R1 : fermeture de la cohorte avant toute statistique (port du controle de l'auditeur Codex,
recu a6_pilote_admission, sur ce pilote). Journaux FULL synthetiques (noms et temps inventes, aucune trame reelle),
ecrits par les aides du test du lecteur partage, admis par le vrai lecteur strict ; le juge rejoue (verifier=True)
contre un plan externe tire de la commande et du seul manifeste.

  nominal                       11 cles d'identite, 21 grandes trames sur 6 tours, ng 5 tours x 10 passes, K10 3 tours
                                x 5 passes : adopte ;
  refus (cohorte)               identite reduite a une cle ; 5 tours de grandes au lieu de 6 ; ordre des grandes reduit
                                a une trame (memes passes brutes) ; binaire avant_bis distinct de avant ; chronos a un
                                fil au lieu de 48 ; K10 a 2 tours au lieu de 3 ;
  information                   une prise K10 en echec ne change pas le verdict ;
  essai                         plan reduit (deux trames d'identite, deux grandes) admis ; verdict et tableaux
                                « essai » ; rapport sans manifeste refuse.

Python 3.10 nu, aucun assert. Codes : 0 conforme (ligne test_pilote_r1_ok) ; 1 ecart.
"""
import copy
import importlib.util
import io
import json
import os
import sys
import tarfile
import tempfile
from types import SimpleNamespace

ICI = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(ICI, "..", "outils"))


def charger(nom, chemin):
    spec = importlib.util.spec_from_file_location(nom, chemin)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


pilote = charger("pilote_r1", os.path.join(ICI, "pilote_r1.py"))
aides = charger("test_lecteur_full", os.path.join(ICI, "..", "outils", "test_lecteur_full.py"))


class Campagne:
    """Journaux synthetiques d'une campagne, ecrits dans un dossier temporaire."""

    def __init__(self, racine):
        self.racine, self.n = racine, 0

    def prise(self, bras, trames, k, fils, empreinte, mur, cpu=False):
        chemin = os.path.join(self.racine, "journal_%03d.jsonl" % self.n)
        self.n += 1
        attendu = dict(voie="cpu" if cpu else "appareil", k=k, fils=fils, passes=len(trames), empreinte=empreinte,
                       trames=[list(t) for t in trames], budget_appareil="partage", bits=21, schema="recouvert")
        lignes = [] if cpu else [dict(phase="open", status="ok", reason="none", wall_ns=5, budget_appareil="partage")]
        for i, (nom, sites) in enumerate(trames):
            r = aides.overlapped_row(i, mur)
            r.update(trame=nom, sites=sites, kmax=k, threads=fils, pic_octets=1 << 30, rss_max_octets=2 << 30,
                     pic_appareil_octets=0, memoire_octets=dict(P=[1 << 27, 1 << 28], C=[1 << 28, 1 << 30],
                                                                tour=[1 << 29, 1 << 30]))
            if cpu:
                r.update(voie="cpu", appareil_octets=0, epinglee_octets=0)
            if k == 10:
                fins = r["fins_par_ordre_ns"]
                r["fins_par_ordre_ns"] = fins + copy.deepcopy(fins[1:]) + [fins[-1][:]]
            if empreinte:
                r["full_sha256"] = "%064x" % (hash((nom, k)) & ((1 << 256) - 1))
            else:
                r.pop("full_sha256")
            lignes += [r, dict(phase="liberation", pass_=i, liberation_ns=3)]
        lignes.append(dict(phase="exit", status="ok", reason="none"))
        with open(chemin, "w", encoding="utf-8") as f:
            f.write(aides.dump(lignes))
        etat, raison, resume = pilote.admettre(chemin, 0, attendu)
        if etat != "ok":
            raise RuntimeError("journal synthetique refuse par le lecteur : %s" % raison)
        return dict(journal=os.path.basename(chemin), journal_sha256=pilote.bf.sha256_file(chemin), code=0, bras=bras,
                    attendu=attendu, secondes=0, etat=etat, raison=raison, resume=resume)


def nominal(c, cas, grandes):
    a, b = "a" * 64, "b" * 64
    rapport = {"schema": "ehgp.v12.r1_pilote.v1", "regle": pilote.REGLE_R1,
               "construction": {"base": "basetest", "binaires": {x: {"sha256": b if x == "apres" else a}
                                                                 for x in pilote.BRAS}},
               "environnement": {x: {"gpu_apps": ""} for x in ("avant", "apres")}, "identite": {"prises": {}},
               "grandes": {"trames": [n for n, _ in grandes], "tours": []},
               "ng": {"passes": 10, "trames": {n: [] for n in pilote.TRAMES},
                      "binaires_apres": {x: b if x == "apres" else a for x in pilote.BRAS}},
               "k10": {"passes": 5, "trames": {n: [] for n in pilote.TRAMES},
                       "binaires_apres": {x: b if x == "apres" else a for x in pilote.BRAS}}}
    for bras in ("avant", "apres"):
        p = rapport["identite"]["prises"]
        for nom, sites in pilote.TRAMES.items():
            for k, passes in ((5, 2), (10, 1)):
                p.setdefault("%s_k%d" % (nom, k), {})[bras] = c.prise(bras, [(nom, sites)] * passes, k, 48, True,
                                                                      10 ** 8)
        p.setdefault("ng00_k5_1fil", {})[bras] = c.prise(bras, [("ng00", 39885)], 5, 1, True, 10 ** 8)
        for n in pilote.UNIFORMES:
            p.setdefault("u%d_k5" % n, {})[bras] = c.prise(bras, [("u%d" % n, n)], 5, 48, True, 10 ** 8)
        p.setdefault("v12set_k5", {})[bras] = c.prise(bras, cas, 5, 48, True, 10 ** 8)
    for t in range(6):
        ordre = grandes[t:] + grandes[:t]
        tour = {"ordre": [n for n, _ in ordre]}
        for bras in pilote.BRAS:
            tour[bras] = c.prise(bras, ordre * 2, 5, 48, False, 160_000_000 if bras == "apres" else 200_000_000)
        rapport["grandes"]["tours"].append(tour)
    for etape, k, tours, passes in (("ng", 5, 5, 10), ("k10", 10, 3, 5)):
        for nom, sites in pilote.TRAMES.items():
            for _ in range(tours):
                rapport[etape]["trames"][nom].append(
                    {bras: c.prise(bras, [(nom, sites)] * passes, k, 48, False, 10 ** 8) for bras in pilote.BRAS})
    return rapport


def verdict(rapport, racine, plan):
    return pilote.juger(rapport, racine, plan=plan)["verdict"]


def main():
    ecarts = []
    with tempfile.TemporaryDirectory(prefix="test-pilote-r1-") as racine:
        c = Campagne(racine)
        cas = [("synthetic_%02d" % i, 70000 if i < 21 else 40000) for i in range(37)]
        grandes = cas[:21]
        plan = dict(cas=[list(x) for x in cas], fils=48, passes=10, tours=5, tours_grandes=6, tours_k10=3,
                    passes_k10=5, essai=False, base="basetest")
        rapport = nominal(c, cas, grandes)
        if verdict(rapport, racine, plan) != "adopte":
            ecarts.append("nominal non adopte : %s" % pilote.juger(rapport, racine, plan=plan)["refus"])
        variantes = {}
        r = copy.deepcopy(rapport)
        r["identite"]["prises"] = {"ng00_k5": r["identite"]["prises"]["ng00_k5"]}
        variantes["identite_une_cle"] = r
        r = copy.deepcopy(rapport)
        r["grandes"]["tours"].pop()
        variantes["cinq_tours_grandes"] = r
        r = copy.deepcopy(rapport)
        for tour in r["grandes"]["tours"]:
            tour["ordre"] = tour["ordre"][:1]
        variantes["ordre_une_trame"] = r
        r = copy.deepcopy(rapport)
        r["construction"]["binaires"]["avant_bis"]["sha256"] = "c" * 64
        for etape in ("ng", "k10"):
            r[etape]["binaires_apres"]["avant_bis"] = "c" * 64
        variantes["aa_binaire_distinct"] = r
        r = copy.deepcopy(rapport)
        for nom in pilote.TRAMES:
            r["k10"]["trames"][nom].pop()
        variantes["k10_deux_tours"] = r
        r = copy.deepcopy(rapport)
        r["construction"]["base"] = "autrebase"
        variantes["base_differente"] = r
        r = copy.deepcopy(rapport)
        for p in pilote.prises_du_rapport(r):
            if p["attendu"]["empreinte"]:
                continue
            chemin = os.path.join(racine, p["journal"])
            with open(chemin, encoding="utf-8") as f:
                lignes = [json.loads(x) for x in f.read().splitlines()]
            for ligne in lignes:
                if ligne["phase"] == "full":
                    ligne["threads"] = 1
            nouveau = chemin + ".w1"
            with open(nouveau, "w", encoding="utf-8") as f:
                f.write("".join(json.dumps(x) + "\n" for x in lignes))
            p["journal"] = os.path.basename(nouveau)
            p["attendu"]["fils"] = 1
            p["journal_sha256"] = pilote.bf.sha256_file(nouveau)
            etat, _raison, p["resume"] = pilote.admettre(nouveau, 0, p["attendu"])
            if etat != "ok":
                ecarts.append("variante W1 : journal refuse par le lecteur")
        variantes["chronos_un_fil"] = r
        for nom, variante in variantes.items():
            if verdict(variante, racine, plan) != "refuse":
                ecarts.append("%s : %s au lieu de refuse" % (nom, verdict(variante, racine, plan)))
        r = copy.deepcopy(rapport)
        p = r["k10"]["trames"]["ng01"][0]["apres"]
        chemin = os.path.join(racine, "journal_k10_echec.jsonl")
        with open(chemin, "w", encoding="utf-8") as f:  # processus interrompu : ouverture seule, code 3
            f.write(aides.dump([dict(phase="open", status="ok", reason="none", wall_ns=5, budget_appareil="partage")]))
        etat, raison, resume = pilote.admettre(chemin, 3, p["attendu"])
        p.update(journal=os.path.basename(chemin), journal_sha256=pilote.bf.sha256_file(chemin), code=3, etat=etat,
                 raison=raison, resume=resume)
        if etat == "ok":
            ecarts.append("journal K10 interrompu admis par le lecteur")
        if verdict(r, racine, plan) != "adopte":
            ecarts.append("une prise K10 en echec change le verdict : %s" % pilote.juger(r, racine, plan=plan)["refus"])
        if pilote.juger(rapport, racine, plan=None)["verdict"] != "refuse":
            ecarts.append("rejeu sans plan externe admis")
        # Mode essai : plan reduit, verdict et tableaux « essai ».
        e = copy.deepcopy(rapport)
        e["identite"] = {"prises": {}}
        for bras in ("avant", "apres"):
            p = e["identite"]["prises"]
            for nom, sites in pilote.TRAMES.items():
                p.setdefault(nom + "_k5", {})[bras] = c.prise(bras, [(nom, sites)] * 2, 5, 1, True, 10 ** 8, cpu=True)
            p.setdefault("ng00_k5_1fil", {})[bras] = c.prise(bras, [("ng00", 39885)], 5, 1, True, 10 ** 8, cpu=True)
            for n in pilote.UNIFORMES[:2]:
                p.setdefault("u%d_k5" % n, {})[bras] = c.prise(bras, [("u%d" % n, n)], 5, 1, True, 10 ** 8, cpu=True)
            p.setdefault("v12set_k5", {})[bras] = c.prise(bras, cas[:2], 5, 1, True, 10 ** 8, cpu=True)
        ordre = grandes[-2:]
        tour = {"ordre": [n for n, _ in ordre]}
        for bras in pilote.BRAS:
            tour[bras] = c.prise(bras, ordre * 2, 5, 1, False, 160_000_000 if bras == "apres" else 200_000_000,
                                 cpu=True)
        e["grandes"] = {"trames": [n for n, _ in ordre], "tours": [tour]}
        for etape, k in (("ng", 5), ("k10", 10)):
            e[etape]["passes"] = 2
            for nom, sites in pilote.TRAMES.items():
                e[etape]["trames"][nom] = [{bras: c.prise(bras, [(nom, sites)] * 2, k, 1, False, 10 ** 8, cpu=True)
                                            for bras in pilote.BRAS}]
        archive = os.path.join(racine, "manifeste_seul.tar")
        contenu = json.dumps({"cases": [{"name": n, "count": s} for n, s in cas]}).encode()
        with tarfile.open(archive, "w") as tar:
            info = tarfile.TarInfo("bundle_manifest.json")
            info.size = len(contenu)
            tar.addfile(info, io.BytesIO(contenu))
        args = SimpleNamespace(archive_v12set=archive, fils=1, passes=2, tours=1, tours_grandes=1, tours_k10=1,
                               passes_k10=2, essai=True, sortie=racine, base="basetest")
        plan_essai = pilote.plan_demande(args)
        if pilote.verifier_cohorte(e, plan_essai, True) != "":
            ecarts.append("cohorte essai refusee : %s" % pilote.verifier_cohorte(e, plan_essai, True))
        if pilote.etape_rapport(args, copy.deepcopy(e))["verdict"] != "essai":
            ecarts.append("verdict essai non force")
        with open(os.path.join(racine, "tableaux_r1.md"), encoding="utf-8") as f:
            if "**essai**" not in f.read():
                ecarts.append("tableaux sans verdict essai")
        args.archive_v12set = None
        try:
            pilote.plan_demande(args)
            ecarts.append("rapport sans manifeste admis")
        except RuntimeError:
            pass
        journaux = c.n
    for ecart in ecarts:
        print("test_pilote_r1_ecart " + ecart)
    if ecarts:
        return 1
    print("test_pilote_r1_ok journaux=%d variantes_refusees=%d information_k10=ok essai=ok" % (journaux, len(variantes)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
