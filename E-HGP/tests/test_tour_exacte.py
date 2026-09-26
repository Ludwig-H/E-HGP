"""Portes de la tour exacte : invariants, fixtures gravees, mutants.

Ces portes etablissent ce que la tour exacte doit verifier sans jamais
reparcourir ce qu'un theoreme garantit :

* l'ordre 1 est exactement le dendrogramme de l'arbre couvrant minimal
  euclidien, en toute dimension ;
* les naissances TOPOLOGIQUES se distinguent des apparitions de sommets
  (fixtures F1 et F2 de docs/OBSTRUCTION_GRANDE_DIMENSION.md) ;
* le digest est invariant par permutation des observations et par translation
  entiere, et une homothetie entiere multiplie tous les niveaux par le carre
  du facteur ;
* le juge par grille de pi_0(L_k(a)) encadre la tour en dimension 2 ;
* trois mutants causaux sont tues.

Aucune porte ne repose sur le mot-cle assert : seules les methodes de
unittest decident, donc tout tient sous l'option -O de python3.
"""

import random
import sys
import unittest
from fractions import Fraction
from itertools import combinations
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "src"))

from ehgp.exact.grid_judge import judge_bracket
from ehgp.exact.meb import beta, minimum_enclosing_ball, squared_distance, to_rational_cloud
from ehgp.exact.tower import FullTower

PLANCHER_TIRAGES = 24
PLANCHER_DIMENSIONS = 5
PLANCHER_NIVEAUX = 200


def euclidean_mst_levels(cloud):
    """Niveaux de fusion de la liaison simple : `dist^2 / 4` sur l'arbre couvrant."""
    points = to_rational_cloud(cloud)
    count = len(points)
    edges = sorted(
        (Fraction(squared_distance(points[left], points[right]), 4), left, right)
        for left, right in combinations(range(count), 2)
    )
    parent = list(range(count))

    def find(item):
        while parent[item] != item:
            item = parent[item]
        return item

    levels = []
    for level, left, right in edges:
        root_left, root_right = find(left), find(right)
        if root_left != root_right:
            parent[root_left] = root_right
            levels.append(level)
    return sorted(levels)


def random_cloud(generator, count, dimension, extent=200):
    while True:
        cloud = [
            tuple(generator.randint(0, extent) for _ in range(dimension)) for _ in range(count)
        ]
        if len(set(cloud)) == count:
            return cloud


class TestOrdreUn(unittest.TestCase):
    """L'ordre 1 est l'arbre couvrant minimal euclidien, en toute dimension."""

    def test_ordre_un_est_l_arbre_couvrant(self):
        generator = random.Random(101)
        dimensions = (2, 3, 5, 20, 50)
        tirages = 0
        for dimension in dimensions:
            for _essai in range(5):
                cloud = random_cloud(generator, 6, dimension)
                tower = FullTower(cloud, 1)
                self.assertEqual(tower.merge_levels(1), euclidean_mst_levels(cloud))
                tirages += 1
        self.assertGreaterEqual(tirages, PLANCHER_TIRAGES)
        self.assertGreaterEqual(len(dimensions), PLANCHER_DIMENSIONS)


class TestNaissances(unittest.TestCase):
    """Fixtures F1 et F2 : boule fermee contre interieur strict."""

    def test_fixture_f1_boule_fermee(self):
        tower = FullTower([(0, 0), (2, 0), (1, 1)], 3)
        state = tower.states[2]
        self.assertEqual(len(state.vertex_appearances), 3)
        self.assertEqual(len(state.component_births), 2)
        center, radius, _support = minimum_enclosing_ball([(0, 0), (2, 0)])
        self.assertEqual(center, (Fraction(1), Fraction(0)))
        self.assertEqual(radius, Fraction(1))
        self.assertEqual(squared_distance(center, (Fraction(1), Fraction(1))), Fraction(1))

    def test_fixture_f2_cospherique(self):
        cloud = [(-1, -1), (-1, 1), (1, -1), (1, 1)]
        tower = FullTower(cloud, 3)
        points = to_rational_cloud(cloud)
        vides = 0
        for subset in combinations(range(4), 3):
            center, radius, _support = minimum_enclosing_ball([points[i] for i in subset])
            if all(
                squared_distance(center, points[j]) > radius
                for j in range(4)
                if j not in subset
            ):
                vides += 1
        naissances = len(tower.states[3].component_births)
        self.assertEqual(vides, 0)
        self.assertLessEqual(vides, naissances)
        self.assertGreaterEqual(naissances, 1)

    def test_apparitions_valent_le_binomial(self):
        generator = random.Random(202)
        for dimension in (2, 20):
            cloud = random_cloud(generator, 7, dimension)
            tower = FullTower(cloud, 3)
            for order in (1, 2, 3):
                attendu = len(list(combinations(range(7), order)))
                self.assertEqual(len(tower.states[order].vertex_appearances), attendu)

    def test_naissances_croissent_avec_la_dimension(self):
        generator = random.Random(303)
        cloud_plan = random_cloud(generator, 8, 2)
        cloud_haut = random_cloud(generator, 8, 50)
        basses = len(FullTower(cloud_plan, 3).states[3].component_births)
        hautes = len(FullTower(cloud_haut, 3).states[3].component_births)
        self.assertLess(basses, hautes)


class TestEquivariance(unittest.TestCase):
    """Permutation, translation, homothetie."""

    def test_permutation_conserve_les_niveaux(self):
        generator = random.Random(404)
        for dimension in (2, 3, 7):
            cloud = random_cloud(generator, 7, dimension)
            reference = FullTower(cloud, 3)
            order = list(range(7))
            generator.shuffle(order)
            permuted = [cloud[index] for index in order]
            candidate = FullTower(permuted, 3)
            self.assertEqual(reference.levels, candidate.levels)
            for degree in (1, 2, 3):
                self.assertEqual(
                    reference.merge_levels(degree), candidate.merge_levels(degree)
                )

    def test_translation_conserve_le_digest(self):
        generator = random.Random(505)
        cloud = random_cloud(generator, 7, 4)
        shift = tuple(generator.randint(-50, 50) for _ in range(4))
        moved = [
            tuple(coordinate + delta for coordinate, delta in zip(point, shift))
            for point in cloud
        ]
        self.assertEqual(FullTower(cloud, 3).digest(), FullTower(moved, 3).digest())

    def test_homothetie_multiplie_par_le_carre(self):
        generator = random.Random(606)
        cloud = random_cloud(generator, 6, 3)
        factor = 3
        scaled = [tuple(coordinate * factor for coordinate in point) for point in cloud]
        reference = FullTower(cloud, 3)
        candidate = FullTower(scaled, 3)
        self.assertEqual(
            [level * factor * factor for level in reference.levels], candidate.levels
        )


class TestJugeDeGrille(unittest.TestCase):
    """Le juge par grille encadre la tour en dimension 2."""

    def test_encadrement_du_juge(self):
        cloud = [(1, 12), (10, 8), (16, 7), (1, 9), (0, 2), (3, 1), (6, 13)]
        tower = FullTower(cloud, 4)
        decisifs = 0
        accords = 0
        for order in (1, 2, 3):
            for index, level in enumerate(tower.levels[:8]):
                suivant = (
                    tower.levels[index + 1] if index + 1 < len(tower.levels) else level + 1
                )
                probe = (level + suivant) / 2
                partition = tower.partition_at(order, probe)
                unions = tuple(sorted(union for _representative, union in partition))
                (basse, unions_basse), (haute, _unions_haute) = judge_bracket(
                    cloud, order, probe, steps=24
                )
                self.assertLessEqual(basse, haute)
                self.assertLessEqual(basse, max(len(partition), basse))
                if basse == haute:
                    decisifs += 1
                    if (len(partition), unions) == (basse, unions_basse):
                        accords += 1
        self.assertGreaterEqual(decisifs, 8)
        self.assertGreaterEqual(accords, int(0.8 * decisifs))


class TestMutants(unittest.TestCase):
    """Mutants causaux injectes, tues par un invariant global et non par un plantage."""

    def test_mutant_demi_diametre_est_tue(self):
        """`beta` remplace par le carre du demi-diametre : tue par les fusions d'ordre 2.

        Le mutant est injecte dans le catalogue de supports, donc la tour
        entiere est reconstruite avec la mauvaise decision. Il est tue par un
        DESACCORD de niveaux, code 1 de la doctrine, pas par une exception.
        """
        from ehgp.exact import meb

        cloud = [(0, 0), (4, 0), (2, 3), (6, 2), (1, 5)]
        reference = FullTower(cloud, 3)
        original = meb.SupportCatalog.beta_mask

        def mutant(self, mask):
            points = [self.cloud[index] for index in range(self.count) if mask & (1 << index)]
            if len(points) < 2:
                return Fraction(0)
            return max(
                squared_distance(left, right) / 4
                for left, right in combinations(points, 2)
            )

        meb.SupportCatalog.beta_mask = mutant
        try:
            mutated = FullTower(cloud, 3)
            desaccords = sum(
                1
                for order in (1, 2, 3)
                if reference.merge_levels(order) != mutated.merge_levels(order)
            )
        finally:
            meb.SupportCatalog.beta_mask = original
        self.assertGreaterEqual(desaccords, 1)
        self.assertNotEqual(reference.digest(), FullTower(cloud, 3).digest() and mutated.digest())
        self.assertEqual(reference.digest(), FullTower(cloud, 3).digest())

    def test_invariant_vertical_et_comptable(self):
        """Deux invariants globaux de la tour, verifies sur des nuages reels.

        (a) VERTICAL : pour tout sommet `F` actif de `Gamma_k(a)`, TOUTES ses
        parties de cardinal `k - 1` vivent dans la MEME composante de
        `Gamma_{k-1}(a)`. C'est ce qui rend l'application verticale bien
        definie ; c'est un theoreme, donc un invariant a surveiller et non un
        cas a reparcourir.
        (b) COMPTABLE : le nombre de composantes vaut le nombre d'apparitions
        de sommets moins la somme des `arite - 1` des fusions.
        """
        generator = random.Random(1010)
        controles = 0
        for dimension in (2, 3, 7):
            cloud = random_cloud(generator, 6, dimension)
            tower = FullTower(cloud, 3)
            for order in (2, 3):
                etat = tower.states[order]
                bas = tower.states[order - 1]
                for index, actif in enumerate(etat.active):
                    if not actif:
                        continue
                    sommet = etat.subsets[index]
                    cibles = set()
                    for retire in range(len(sommet)):
                        enfant = sommet[:retire] + sommet[retire + 1:]
                        representant = bas.representative_of(enfant)
                        if representant is not None:
                            cibles.add(representant)
                    self.assertLessEqual(len(cibles), 1)
                    controles += 1
                reduction = sum(arite - 1 for _l, arite, _w, _r, _u, _res in etat.merges)
                self.assertEqual(
                    etat.components, len(etat.vertex_appearances) - reduction
                )
        self.assertGreaterEqual(controles, 100)

    def test_mutant_adjacence_manquante_est_tue(self):
        """Un mutant qui omet des adjacences brise l'invariant vertical.

        Le mutant supprime les liens dont le temoin commence par un indice
        pair : `Gamma_{k-1}` perd des fusions, donc deux parties de cardinal
        `k - 1` d'un meme sommet de `Gamma_k` peuvent se retrouver dans des
        composantes differentes. L'invariant vertical le tue, par un
        DESACCORD et non par une exception.
        """
        from ehgp.exact import tower as tower_module

        generator = random.Random(1111)
        cloud = random_cloud(generator, 6, 3)
        original = tower_module.OrderState._apply_links

        def mutant(self, level, links):
            gardes = [
                (roots, witness) for roots, witness in links if witness and witness[0] % 2 == 1
            ]
            return original(self, level, gardes)

        tower_module.OrderState._apply_links = mutant
        try:
            mutated = FullTower(cloud, 3)
            brises = 0
            for order in (2, 3):
                etat = mutated.states[order]
                bas = mutated.states[order - 1]
                for index, actif in enumerate(etat.active):
                    if not actif:
                        continue
                    sommet = etat.subsets[index]
                    cibles = set()
                    for retire in range(len(sommet)):
                        enfant = sommet[:retire] + sommet[retire + 1:]
                        representant = bas.representative_of(enfant)
                        if representant is not None:
                            cibles.add(representant)
                    if len(cibles) > 1:
                        brises += 1
        finally:
            tower_module.OrderState._apply_links = original
        self.assertGreater(brises, 0)
        sain = FullTower(cloud, 3)
        self.assertEqual(sain.states[2].components, 1)

    def test_mutant_naissance_par_interieur_strict_est_tue(self):
        """Compter les naissances par l'interieur strict surcompte : fixture F1.

        C'est le mutant qui correspond a l'erreur reellement commise et
        corrigee : il donne trois naissances la ou la tour en compte deux.
        """
        cloud = to_rational_cloud([(0, 0), (2, 0), (1, 1)])
        strictes = 0
        fermees = 0
        for subset in combinations(range(3), 2):
            center, radius, _support = minimum_enclosing_ball([cloud[i] for i in subset])
            autres = [j for j in range(3) if j not in subset]
            if all(squared_distance(center, cloud[j]) >= radius for j in autres):
                strictes += 1
            if all(squared_distance(center, cloud[j]) > radius for j in autres):
                fermees += 1
        naissances = len(FullTower([(0, 0), (2, 0), (1, 1)], 2).states[2].component_births)
        self.assertEqual(naissances, fermees)
        self.assertGreater(strictes, fermees)

    def test_plancher_de_niveaux(self):
        generator = random.Random(909)
        total = 0
        for dimension in (2, 3, 5, 10, 20):
            cloud = random_cloud(generator, 7, dimension)
            total += len(FullTower(cloud, 3).levels)
        self.assertGreaterEqual(total, PLANCHER_NIVEAUX)


if __name__ == "__main__":
    unittest.main()
