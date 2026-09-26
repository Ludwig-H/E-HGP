"""Portes du certificat de separation : correction d'abord, force ensuite.

Le certificat de tranche (hyperplan ou sphere) fournit la borne INFERIEURE
qui manquait au moteur. Deux exigences, dans cet ordre :

1. CORRECTION. Un certificat ne doit JAMAIS affirmer une separation qui n'a
   pas lieu. La porte confronte donc chaque certificat a l'ultrametrique
   exacte de l'oracle : un niveau certifie doit toujours minorer, et un
   niveau declare exact doit etre exactement celui de l'oracle. Une seule
   violation est un echec.
2. FORCE. Le certificat est mesure, pas declare : la porte publie combien de
   paires il certifie, sans exiger un minimum eleve, parce que sa faiblesse
   en ordre superieur est un resultat et non un defaut.

Aucune porte ne repose sur le mot-cle assert.
"""

import random
import sys
import unittest
from fractions import Fraction
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "src"))

from ehgp.engine.critical import critical_catalogue
from ehgp.engine.segment import rational_cloud
from ehgp.engine.separation import (
    candidate_spheres,
    certified_bracket,
    certified_exact_levels,
    covers_sphere,
    hyperplane_order_statistic,
    sphere_certificate,
)
from ehgp.engine.witness_tower import witness_ultrametric
from ehgp.exact.projection import projected_ultrametric
from ehgp.exact.tower import FullTower

PLANCHER_PAIRES = 60
PLANCHER_DIMENSIONS = 3


def nuage(generator, count, dimension, extent=200):
    while True:
        cloud = [
            tuple(generator.randint(0, extent) for _ in range(dimension)) for _ in range(count)
        ]
        if len(set(cloud)) == count:
            return cloud


class TestPrimitives(unittest.TestCase):
    """Les predicats exacts, sur des cas calcules a la main."""

    def test_statistique_d_ordre_sur_un_hyperplan(self):
        cloud = rational_cloud([(0, 0), (3, 0), (0, 4), (10, 10)])
        normal = (1, 0)
        niveaux = [
            hyperplane_order_statistic(cloud, normal, Fraction(1), order)
            for order in (1, 2, 3, 4)
        ]
        self.assertEqual(niveaux[0], Fraction(1))
        self.assertEqual(niveaux[1], Fraction(1))
        self.assertEqual(niveaux[2], Fraction(4))
        self.assertEqual(niveaux[3], Fraction(81))

    def test_couverture_de_sphere_sans_racine(self):
        """`covers_sphere` doit coincider avec le test a racines, au signe pres."""
        from math import sqrt

        generator = random.Random(4242)
        controles = 0
        for _essai in range(3000):
            s = Fraction(generator.randint(0, 400), generator.randint(1, 8))
            r = Fraction(generator.randint(1, 400), generator.randint(1, 8))
            a = Fraction(generator.randint(0, 800), generator.randint(1, 8))
            seuil = (sqrt(float(s)) - sqrt(float(r))) ** 2
            exact = covers_sphere(s, r, a)
            if abs(float(a) - seuil) > 1e-9 * max(1.0, seuil):
                self.assertEqual(exact, float(a) >= seuil)
                controles += 1
        self.assertGreater(controles, 2000)

    def test_certificat_faux_est_refuse(self):
        """Une sphere qui ne separe pas ne certifie rien."""
        cloud = rational_cloud([(0, 0), (10, 0), (5, 0)])
        centre = (Fraction(0), Fraction(0))
        self.assertFalse(
            sphere_certificate(cloud, centre, Fraction(1000), 1, Fraction(1), 0, 1)
        )


class TestCorrection(unittest.TestCase):
    """Le certificat ne ment jamais : confrontation a l'oracle exact."""

    def test_encadrement_par_hyperplan_est_coherent(self):
        generator = random.Random(515)
        paires = 0
        dimensions = (2, 3, 20)
        for dimension in dimensions:
            cloud = nuage(generator, 6, dimension)
            tower = FullTower(cloud, 2)
            for order in (1, 2):
                exact = projected_ultrametric(tower, order)
                superieur, _stats = witness_ultrametric(cloud, order, triples=True)
                bracket = certified_bracket(cloud, superieur, order)
                for pair, record in bracket.items():
                    reference = exact.get(pair)
                    if reference is None:
                        continue
                    self.assertLessEqual(record["lower"], reference)
                    self.assertLessEqual(reference, record["upper"])
                    paires += 1
        self.assertGreaterEqual(paires, PLANCHER_PAIRES)
        self.assertGreaterEqual(len(dimensions), PLANCHER_DIMENSIONS)

    def test_niveaux_certifies_sont_exacts(self):
        generator = random.Random(616)
        certifies = 0
        paires = 0
        for dimension in (2, 3, 20):
            cloud = nuage(generator, 6, dimension)
            points = rational_cloud(cloud)
            tower = FullTower(cloud, 2)
            for order in (1, 2):
                exact = projected_ultrametric(tower, order)
                superieur, _stats = witness_ultrametric(cloud, order, triples=True)
                centres = list(points)
                for mass in (order, order + 1):
                    if mass <= len(points):
                        catalogue, _statistics = critical_catalogue(points, mass)
                        centres.extend([centre for centre, _radius in catalogue])
                spheres = candidate_spheres(points, centres)
                certified = certified_exact_levels(cloud, superieur, order, spheres)
                for pair, record in certified.items():
                    reference = exact.get(pair)
                    if reference is None:
                        continue
                    self.assertEqual(record["level"], reference)
                    certifies += 1
                paires += len(exact)
        self.assertGreaterEqual(paires, PLANCHER_PAIRES)
        self.assertGreater(certifies, 0)


if __name__ == "__main__":
    unittest.main()
