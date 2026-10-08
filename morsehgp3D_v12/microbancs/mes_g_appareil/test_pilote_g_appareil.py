#!/usr/bin/env python3
"""Porte locale du pilote MES-G-APP (juge seul, sans construction ni appareil) : auto-test du juge, lecture stricte des
journaux, refus et rejets attendus. Bibliotheque standard seule ; jouable sous python3 -S -O (aucun assert nu).

  python3 -S -O test_pilote_g_appareil.py
"""
import json
import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pilote_g_appareil as pg  # noqa: E402


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
        lignes = pg.journal_synthetique(1000, {m: 0.1 for m in pg.MESURES})[:-1]
        with self.assertRaises(pg.Refus):
            pg.lire_journal(lignes)

    def test_passes_manquantes(self):
        lignes = pg.journal_synthetique(1000, {m: 0.1 for m in pg.MESURES}, passes=3)
        journal = pg.lire_journal(lignes)
        with self.assertRaises(pg.Refus):
            pg.resumer_processus(journal, 4, True)

    def test_seuil_propositions(self):
        camp = pg.campagne_synthetique({"census": 0.05, "sondes": 0.05, "propositions": 0.6})
        rapport = pg.juger(camp)
        self.assertEqual(rapport["verdict"], "rejete")
        self.assertTrue(any("propositions" in r for r in rapport["rejets"]))

    def test_sans_appareil_refuse(self):
        camp = pg.campagne_synthetique({m: 0.05 for m in pg.MESURES})
        camp["appareil"] = False
        self.assertEqual(pg.juger(camp)["verdict"], "refuse")

    def test_isolation_refusee(self):
        camp = pg.campagne_synthetique({m: 0.05 for m in pg.MESURES})
        camp["isolation_ok"] = False
        self.assertEqual(pg.juger(camp)["verdict"], "refuse")


if __name__ == "__main__":
    unittest.main()
