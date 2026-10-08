#!/usr/bin/env python3
"""Porte locale du pilote MES-G-APP, etape 2 (juge seul, sans construction ni appareil) : auto-test du juge, lecture
stricte des journaux, refus et rejets attendus des deux conceptions de REGLE_G_APPAREIL_2. Bibliotheque standard seule ;
jouable sous python3 -S -O (aucun assert nu).

  python3 -S -O test_pilote_g_appareil.py
"""
import json
import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pilote_g_appareil as pg  # noqa: E402

BON = {"census": 0.09, "sondes": 0.05, "propositions": 0.70, "propositions_l4": 0.40, "propositions_l4f32": 0.08,
       "neutre": 1.0}


class JugeTest(unittest.TestCase):
    def test_auto_test(self):
        self.assertEqual(pg.auto_test(), [])

    def test_bootstrap_ordonne(self):
        bas, haut = pg.bootstrap([0.05, 0.07, 0.06, 0.08, 0.05])
        self.assertLessEqual(bas, haut)
        self.assertGreater(bas, 0.0)

    def test_ligne_illisible_refusee(self):
        with self.assertRaises(pg.Refus):
            pg.lire_journal(["{pas du json"])

    def test_refus_du_programme(self):
        with self.assertRaises(pg.Refus):
            pg.lire_journal([json.dumps({"phase": "refus", "raison": "appareil"})])

    def test_journal_sans_identite(self):
        lignes = pg.journal_synthetique({m: 0.1 for m in pg.MESURES})[:-1]
        with self.assertRaises(pg.Refus):
            pg.lire_journal(lignes)

    def test_passes_manquantes(self):
        journal = pg.lire_journal(pg.journal_synthetique({m: 0.1 for m in pg.MESURES}, passes=3))
        with self.assertRaises(pg.Refus):
            pg.resumer_processus(journal, 4, True)

    def test_d2_et_d1_adoptes(self):
        r = pg.juger(pg.campagne_synthetique(BON))
        self.assertEqual(r["verdicts"], {"D2": "adopte", "D1": "adopte"})
        self.assertTrue(r["decision"].startswith("D2"))

    def test_d2_seuil_par_poste(self):
        r = pg.juger(pg.campagne_synthetique(dict(BON, propositions_l4f32=0.25)))
        self.assertEqual(r["verdicts"]["D2"], "rejete")
        self.assertTrue(any("D2 propositions" in x for x in r["rejets"]["D2"]))
        self.assertTrue(r["decision"].startswith("D1"))

    def test_d1_neutralite_hote(self):
        r = pg.juger(pg.campagne_synthetique(dict(BON, neutre=1.10)))
        self.assertEqual(r["verdicts"]["D1"], "rejete")
        self.assertTrue(any("neutralite_hote" in x for x in r["rejets"]["D1"]))

    def test_issues_differentes(self):
        r = pg.juger(pg.campagne_synthetique(BON, ecart=("propositions_l4", "issues_differentes_p64")))
        self.assertEqual(r["verdicts"], {"D2": "adopte", "D1": "rejete"})

    def test_replis(self):
        r = pg.juger(pg.campagne_synthetique(BON, replis=2))
        self.assertEqual(r["verdicts"], {"D2": "rejete", "D1": "rejete"})

    def test_mutant_l4_vivant(self):
        r = pg.juger(pg.campagne_synthetique(BON, mutant_l4_replis=1))
        self.assertEqual(r["verdicts"], {"D2": "refuse", "D1": "refuse"})

    def test_sans_appareil_refuse(self):
        camp = pg.campagne_synthetique(BON)
        camp["appareil"] = False
        self.assertEqual(pg.juger(camp)["verdicts"], {"D2": "refuse", "D1": "refuse"})

    def test_isolation_refusee(self):
        camp = pg.campagne_synthetique(BON)
        camp["isolation_ok"] = False
        self.assertEqual(pg.juger(camp)["verdicts"], {"D2": "refuse", "D1": "refuse"})


if __name__ == "__main__":
    unittest.main()
