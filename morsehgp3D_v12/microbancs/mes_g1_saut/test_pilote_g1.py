#!/usr/bin/env python3
"""Porte du pilote et du juge de MES-G1 (pilote_g1.py), sur des sorties SYNTHETIQUES (aucun binaire, aucune donnee).

Partie A, validation d'une prise du vidage a quatre bras (valider_prise) : prise conforme ; ordre manquant ; temps
nul ; foret differente avec code 1 et sortie « ecart » (ecart, pas defaut de preuve) ; foret differente avec code 0
(refus) ; controle de foret non reproduit sur la v11 (refus).
Partie B, juge (REGLE_G1) : adopte ; rejete (borne haute >= 1 ; premiere moitie sous 50 % ; foret differente) ;
refuse (quatre prises ; binaire different ; journal modifie ; porte du mutant de foret absente ; comptes differents
d'une prise a l'autre ; construction absente).

Usage : python3 -S -O test_pilote_g1.py. Bibliotheque standard ; aucune garde par assert.
Codes : 0 conforme ; 1 ecart (detail en JSON).
"""
import copy
import hashlib
import importlib.util
import json
import os
import sys
import tempfile

ICI = os.path.dirname(os.path.abspath(__file__))
sys.dont_write_bytecode = True
_spec = importlib.util.spec_from_file_location("pilote_g1", os.path.join(ICI, "pilote_g1.py"))
pg = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(pg)


def ligne_saut(k, t12=1.0, tsa=0.9, sat12=100, satsa=20, foret=True, reproduit=True):
    return {"phase": "resolution_saut", "k": k,
            "secondes": {"v11": 2.0, "replique_v11": 1.5, "replique_v12": t12, "replique_v12_saut": tsa},
            "census": {"replique_v12": {"satures": sat12, "complets": 10},
                       "replique_v12_saut": {"satures": satsa, "complets": 10}},
            "sauts": {"tentatives": 90, "certifies": 80, "candidats_testes": 900, "tests_par_tentative": 10.0},
            "pas": {"replique_v12": 1000, "replique_v12_saut": 1001},
            "chaines": {"replique_v12": {"max": 5}, "replique_v12_saut": {"max": 5}},
            "graines_differentes_de_la_v11": 3,
            "foret": {"identique": foret, "controle_v11_reproduit": reproduit}}


def prise(k_max=5, code=0, foret=True, sans=None, temps_nul=False, reproduit=True):
    lignes = [{"phase": "voisins_etage_p", "K": k_max, "secondes": 0.1}]
    for k in range(2, k_max + 1):
        if k == sans:
            continue
        lignes.append({"phase": "resolution_un_fil", "k": k, "graines_identiques": True})
        lignes.append(ligne_saut(k, tsa=0.0 if temps_nul else 0.9, foret=foret or k != 3, reproduit=reproduit))
    lignes.append({"phase": "fin", "seconds": 1.0})
    lignes.append({"phase": "exit", "status": "ok" if code == 0 else ("ecart" if code == 1 else "invariant_violated")})
    return lignes, code


def partie_a():
    out = []
    cas = [("conforme", prise(), False, False),
           ("ordre_manquant", prise(sans=4), True, False),
           ("temps_nul", prise(temps_nul=True), True, False),
           ("foret_differente_code_1", prise(code=1, foret=False), False, True),
           ("foret_differente_code_0", prise(code=0, foret=False), True, True),
           ("controle_non_reproduit", prise(reproduit=False), True, False)]
    for nom, (lignes, code), refus_attendu, foret_attendue in cas:
        problemes, differente, _ = pg.valider_prise(lignes, code, 5)
        ok = bool(problemes) == refus_attendu and differente == foret_attendue
        out.append({"partie": "A", "cas": nom, "conforme": ok, "problemes": problemes[:3], "foret": differente})
    return out


def ecrire(sortie, chemin, texte):
    complet = os.path.join(sortie, chemin)
    os.makedirs(os.path.dirname(complet), exist_ok=True)
    with open(complet, "w", encoding="utf-8") as f:
        f.write(texte)
    return hashlib.sha256(texte.encode()).hexdigest()


def rapport_base(sortie, rapports, part=0.8, foret=False):
    binaires = {n: "h-" + n for n in pg.BINAIRES}
    r = {"construction": {"binaires": dict(binaires)}, "portes": {}, "campagnes": {}}
    for nom, exe in (("mes_g1", "mhgp12_mes_g1"), ("mes_g1_mutant_cote_nul", "mhgp12_mes_g1_mutant_cote_nul"),
                     ("bras_saut_ng00_k5", "mhgp12_vidage"),
                     ("bras_saut_mutant_cibles_decalees_ng00_k5", "mhgp12_vidage_mutant_cibles_decalees")):
        j = "portes/c1/%s.jsonl" % nom
        r["portes"][nom] = {"conforme": True, "binaire": {"binaire": exe, "sha256": binaires[exe]}, "journal": j,
                            "journal_sha256": ecrire(sortie, j, nom + "\n")}
    for cas in pg.REGLE_G1["cas_decisifs"]:
        prises = []
        for i, x in enumerate(rapports):
            j = "%s/campagnes/c1/p%d/vidage.jsonl" % (cas, i)
            prises.append({"valide": True, "binaire": {"binaire": "mhgp12_vidage", "sha256": binaires["mhgp12_vidage"]},
                           "journal": j, "journal_sha256": ecrire(sortie, j, "%s %d\n" % (cas, i)),
                           "vidages_sha256": {"cat.bin": "v"}, "foret_differente": foret,
                           "totaux": {"rapport_total": x, "part_satures_evites": part, "satures_v12": 1000,
                                      "satures_saut": int(1000 * (1 - part))}})
        r["campagnes"][cas] = {"c1": {"debut": "2026-10-07T18:00:00Z", "processus_demandes": len(rapports),
                                      "prises": prises, "refus": [], "vidages_reference": {"cat.bin": "v"},
                                      "passes_par_processus": 3}}
    return r


def partie_b():
    out = []
    bons = [0.90, 0.91, 0.89, 0.92, 0.90]
    with tempfile.TemporaryDirectory(prefix="pilote-g1-") as tmp:
        cas = []
        r = rapport_base(os.path.join(tmp, "adopte"), bons)
        cas.append(("adopte", r, os.path.join(tmp, "adopte"), "adopte"))
        r = rapport_base(os.path.join(tmp, "ic"), [1.02, 0.97, 1.05, 1.01, 0.99])
        cas.append(("rejete_borne_haute", r, os.path.join(tmp, "ic"), "rejete"))
        r = rapport_base(os.path.join(tmp, "moitie"), bons, part=0.4)
        cas.append(("rejete_premiere_moitie", r, os.path.join(tmp, "moitie"), "rejete"))
        r = rapport_base(os.path.join(tmp, "foret"), bons, foret=True)
        cas.append(("rejete_foret", r, os.path.join(tmp, "foret"), "rejete"))
        r = rapport_base(os.path.join(tmp, "quatre"), bons[:4])
        cas.append(("refuse_quatre_prises", r, os.path.join(tmp, "quatre"), "refuse"))
        d = os.path.join(tmp, "binaire")
        r = rapport_base(d, bons)
        r["campagnes"]["ng01_k5"]["c1"]["prises"][2]["binaire"]["sha256"] = "autre"
        cas.append(("refuse_binaire_different", r, d, "refuse"))
        d = os.path.join(tmp, "journal")
        r = rapport_base(d, bons)
        ecrire(d, r["campagnes"]["ng02_k5"]["c1"]["prises"][1]["journal"], "modifie\n")
        cas.append(("refuse_journal_modifie", r, d, "refuse"))
        d = os.path.join(tmp, "porte")
        r = rapport_base(d, bons)
        del r["portes"]["bras_saut_mutant_cibles_decalees_ng00_k5"]
        cas.append(("refuse_mutant_de_foret_absent", r, d, "refuse"))
        d = os.path.join(tmp, "comptes")
        r = rapport_base(d, bons)
        r["campagnes"]["ng00_k5"]["c1"]["prises"][3]["totaux"]["part_satures_evites"] = 0.79
        cas.append(("refuse_comptes_non_deterministes", r, d, "refuse"))
        d = os.path.join(tmp, "construction")
        r = rapport_base(d, bons)
        r["construction"] = {}
        cas.append(("refuse_construction_absente", r, d, "refuse"))
        for nom, rapport, sortie, attendu in cas:
            v = pg.juger(copy.deepcopy(rapport), sortie)
            ok = v["verdict"] == attendu
            out.append({"partie": "B", "cas": nom, "verdict": v["verdict"], "attendu": attendu, "conforme": ok,
                        "motifs": (v["refus"] or v["rejets"])[:2]})
            if nom == "adopte" and ok:
                g = v["cas"]["ng00_k5"]
                ok = g["ic95"][1] < 1.0 and abs(g["moyenne_geometrique"] - 0.9039) < 0.002
                out[-1]["conforme"] = ok
        md = pg.tableaux({"verdict": pg.juger(copy.deepcopy(cas[0][1]), cas[0][2]), "campagnes": {}})
        out.append({"partie": "B", "cas": "tableaux", "conforme": "adopte" in md})
    return out


def main():
    resultats = partie_a() + partie_b()
    ecarts = [r["cas"] for r in resultats if not r["conforme"]]
    for r in resultats:
        print(json.dumps(r, ensure_ascii=False))
    print(json.dumps({"porte": "pilote_g1", "cas": len(resultats), "ecarts": ecarts, "optimise": sys.flags.optimize}))
    return 0 if not ecarts else 1


if __name__ == "__main__":
    sys.exit(main())
