"""Portes de la correspondance f-divergence / noyau et de l'encadrement rampe.

Trois blocs de verification, tous executables sous `python3 -O` (aucune
porte ne repose sur le mot-cle `assert` : `self.assertX` est une methode) :

1. la forme fermee de la proposition 2 de `ehgp.soft.families` est
   confrontee a un solveur generique `scipy` qui ne connait que l'objectif
   et son gradient, et la valeur du programme lineaire de la proposition 1
   est confrontee a `scipy.optimize.linprog` ;
2. la correspondance de Legendre de la proposition 3 et la proposition 4
   (famille `f_rho` de Bach) sont verifiees noyau par noyau ;
3. l'encadrement du theoreme 1 de `ehgp.soft.ramp_exact` est verifie en
   RATIONNELS EXACTS en dimensions 2, 3 et 20, avec planchers de
   couverture, cas critiques graves, monotonie en `epsilon`, temoins de
   tension du decalage, et mutants qui doivent etre tues.

Les nuages sont entiers et les points de test rationnels : aucune decision
de ce fichier ne passe par un flottant, sauf dans les blocs 1 et 2 qui
portent explicitement sur le proposeur numerique.
"""

import math
import os
import random
import sys
import unittest
from fractions import Fraction

import numpy as np
from scipy import integrate

sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "src"))

from ehgp.soft import families, fermi, ramp_exact  # noqa: E402

EPSILONS = (Fraction(1, 4), Fraction(1, 2), Fraction(2), Fraction(7, 2))
ORDERS = (1, 2, 3, 4)
GEOMETRIES = ((2, 8, 11), (3, 10, 23), (20, 12, 37))

# Planchers d'anti-vacuite. Chacun vaut environ le TIERS de la valeur
# mesuree sur le corpus deterministe (3360 / 1907 / 2004 / 592 / 764 / 2050,
# reproduits par le script de mesure) : assez haut pour qu'un corpus
# appauvri ou un predicat qui ne compare rien soit refuse, assez bas pour
# ne pas etre fragile.
FLOORS = {
    "cases": 1100,
    "inner": 600,
    "soft": 650,
    "outer_false": 190,
    "strict": 250,
    "critical": 680,
}

# Minima mesures par geometrie : 1120 cas, inner de 604 a 660, soft de 660
# a 672, outer_false de 192 a 204, strict de 252 a 256, critical de 673 a
# 694. Les planchers ci-dessous valent le tiers du plus petit.
PER_GEOMETRY_FLOORS = {
    "cases": 350,
    "inner": 200,
    "soft": 220,
    "outer_false": 64,
    "strict": 84,
    "critical": 224,
}

# Dimensions exigees, en dur : une derivation depuis `GEOMETRIES` serait
# auto-referente et survivrait a la suppression d'une geometrie.
REQUIRED_DIMENSIONS = frozenset((2, 3, 20))

_CORPUS = {}


def _cloud(count, dimension, seed, span=12):
    """Nuage entier deterministe, positions eventuellement dupliquees."""
    source = random.Random(seed)
    return [tuple(source.randrange(0, span) for _ in range(dimension)) for _ in range(count)]


def _probes(cloud, dimension, seed, number=5, span=12):
    """Points de test rationnels : tirages, une observation, le barycentre."""
    source = random.Random(seed + 101)
    points = []
    for _ in range(number):
        points.append(
            tuple(Fraction(source.randrange(0, 2 * span + 1), 2) for _ in range(dimension))
        )
    points.append(tuple(Fraction(value) for value in cloud[0]))
    count = len(cloud)
    points.append(
        tuple(Fraction(sum(point[axis] for point in cloud), count) for axis in range(dimension))
    )
    return points


def _levels(energies, order, epsilon):
    """Niveaux de test, avec le drapeau `critique` (niveau exactement noeud).

    Les noeuds sont les `e_i`, les `e_i + epsilon`, `a_order` et
    `A^order_epsilon` : ce sont les seuls endroits ou une implementation
    peut se tromper d'inegalite large ou stricte.
    """
    ordered = sorted(energies)
    hard = ordered[order - 1]
    soft = ramp_exact.soft_level_exact(energies, order, epsilon)
    knots = set(ordered) | set(value + epsilon for value in ordered) | {hard, soft}
    levels = [hard, hard + epsilon, soft, soft - epsilon / 3, soft + epsilon / 5]
    levels.extend([ordered[0], ordered[order - 1] + epsilon, ordered[-1]])
    levels.append((ordered[0] + ordered[-1]) / 2)
    levels.append(hard - epsilon / 2)
    return [(level, level in knots) for level in levels]


def corpus(dimension, count, seed):
    """Corpus exact partage par les portes d'encadrement, mis en cache."""
    key = (dimension, count, seed)
    if key in _CORPUS:
        return _CORPUS[key]
    cloud = _cloud(count, dimension, seed)
    cases = []
    for probe in _probes(cloud, dimension, seed):
        energies = ramp_exact.rational_energies(cloud, probe)
        for order in ORDERS:
            for epsilon in EPSILONS:
                for level, critical in _levels(energies, order, epsilon):
                    cases.append((energies, order, level, epsilon, critical))
    _CORPUS[key] = cases
    return cases


class TestProgrammeDeMasse(unittest.TestCase):
    """Proposition 1 : valeur du programme lineaire et multiplicateur."""

    def test_valeur_egale_somme_des_k_plus_petites(self):
        source = random.Random(5)
        checked = 0
        for _trial in range(30):
            count = source.randrange(4, 12)
            energies = np.array([source.uniform(0.0, 10.0) for _ in range(count)])
            for order in range(1, count + 1):
                value, weights, success = families.linear_program_value(energies, order)
                self.assertTrue(success)
                self.assertAlmostEqual(value, families.smallest_sum(energies, order), places=9)
                self.assertAlmostEqual(float(weights.sum()), float(order), places=9)
                checked += 1
        self.assertGreaterEqual(checked, 150)

    def test_multiplicateur_gauche_est_a_k(self):
        """L'argmax de la fonction duale commence exactement en `a_k`."""
        source = random.Random(7)
        checked = 0
        for _trial in range(20):
            count = source.randrange(5, 10)
            energies = np.sort(np.array([source.uniform(0.0, 10.0) for _ in range(count)]))
            for order in range(1, count):
                def dual(level):
                    return order * level - float(np.sum(np.clip(level - energies, 0.0, None)))

                best = dual(energies[order - 1])
                grid = np.linspace(energies[0] - 1.0, energies[-1] + 1.0, 401)
                self.assertGreaterEqual(best + 1e-9, float(max(dual(value) for value in grid)))
                self.assertLess(dual(energies[order - 1] - 1e-3), best - 1e-9)
                checked += 1
        self.assertGreaterEqual(checked, 60)


class TestFormeFermee(unittest.TestCase):
    """Proposition 2 : forme fermee contre solveur generique `scipy`."""

    def test_forme_fermee_contre_scipy(self):
        source = random.Random(13)
        checked = 0
        cases = (
            ("logistic", None),
            ("ramp", None),
            ("tsallis", 1.5),
            ("tsallis", 3.0),
            ("bach", 0.0),
            ("bach", 0.5),
        )
        for name, parameter in cases:
            for _trial in range(4):
                count = source.randrange(5, 10)
                energies = np.array([source.uniform(0.0, 6.0) for _ in range(count)])
                for mass in (1, 2, 3):
                    for epsilon in (0.5, 1.0, 2.0):
                        closed, level = families.regularised_weights(
                            energies, mass, epsilon, name, parameter
                        )
                        judged, judged_value, success = families.reference_weights(
                            energies, mass, epsilon, name, parameter
                        )
                        self.assertTrue(success, msg=name)
                        closed_value = families.objective_value(
                            closed, energies, epsilon, name, parameter
                        )
                        self.assertAlmostEqual(float(closed.sum()), float(mass), places=7)
                        self.assertLessEqual(closed_value, judged_value + 1e-7)
                        self.assertLess(float(np.max(np.abs(closed - judged))), 2e-4)
                        self.assertTrue(np.isfinite(level))
                        checked += 1
        self.assertGreaterEqual(checked, 200)

    def test_kkt_sur_les_coordonnees_interieures(self):
        """`phi'(w_i) = (mu - e_i) / epsilon` des que `0 < w_i < 1`."""
        source = random.Random(17)
        interior_seen = 0
        for name, parameter in (("logistic", None), ("ramp", None), ("bach", 0.5)):
            handle = families.family(name, parameter)
            for _trial in range(6):
                count = source.randrange(6, 11)
                energies = np.array([source.uniform(0.0, 6.0) for _ in range(count)])
                weights, level = families.regularised_weights(energies, 3, 0.8, name, parameter)
                interior = (weights > 1e-6) & (weights < 1.0 - 1e-6)
                if not np.any(interior):
                    continue
                expected = (level - energies[interior]) / 0.8
                self.assertLess(
                    float(np.max(np.abs(handle.potential_slope(weights[interior]) - expected))),
                    1e-6,
                )
                interior_seen += int(np.sum(interior))
        self.assertGreaterEqual(interior_seen, 20)


class TestLegendre(unittest.TestCase):
    """Proposition 3 et proposition 4 : forme des noyaux."""

    def test_inverse_de_la_derivee(self):
        grid = np.linspace(-6.0, 6.0, 241)
        checked = 0
        for name, parameter in (
            ("logistic", None),
            ("ramp", None),
            ("tsallis", 1.5),
            ("tsallis", 2.0),
            ("tsallis", 3.0),
            ("bach", 0.0),
            ("bach", 0.25),
            ("bach", 0.5),
            ("bach", 0.75),
        ):
            handle = families.family(name, parameter)
            low, high = handle.slope_bounds()
            values = handle.occupancy(grid)
            self.assertTrue(np.all(np.diff(values) >= -1e-12), msg=name)
            self.assertTrue(np.all((values >= 0.0) & (values <= 1.0)), msg=name)
            self.assertTrue(np.all(values[grid <= low] == 0.0), msg=name)
            self.assertTrue(np.all(values[grid >= high] == 1.0), msg=name)
            inside = (grid > low + 1e-6) & (grid < high - 1e-6)
            if np.any(inside):
                recovered = handle.potential_slope(values[inside])
                self.assertLess(float(np.max(np.abs(recovered - grid[inside]))), 1e-8, msg=name)
                checked += int(np.sum(inside))
        self.assertGreaterEqual(checked, 500)

    def test_formes_annoncees(self):
        grid = np.linspace(-4.0, 4.0, 161)
        sigmoid = 1.0 / (1.0 + np.exp(-grid))
        logistic = families.occupancy(grid, "logistic")
        self.assertLess(float(np.max(np.abs(logistic - sigmoid))), 1e-12)
        self.assertLess(
            float(np.max(np.abs(families.occupancy(grid, "ramp") - np.clip(grid, 0.0, 1.0)))), 1e-15
        )
        self.assertLess(
            float(
                np.max(
                    np.abs(
                        families.occupancy(grid, "tsallis", 2.0)
                        - np.clip((grid + 1.0) / 2.0, 0.0, 1.0)
                    )
                )
            ),
            1e-12,
        )
        self.assertLess(
            float(
                np.max(
                    np.abs(families.occupancy(grid, "bach", 0.0) - np.clip(grid + 1.0, 0.0, 1.0))
                )
            ),
            1e-15,
        )

    def test_largeurs_de_fenetre(self):
        """`c(rho) = (2 - rho) / (2 (1 - rho)^2)` et `alpha / (alpha - 1)`."""
        for rho in (0.0, 0.25, 0.5, 0.75, 0.9):
            expected = (2.0 - rho) / (2.0 * (1.0 - rho) ** 2)
            self.assertAlmostEqual(families.support_width("bach", rho), expected, places=12)
        self.assertAlmostEqual(families.support_width("bach", 0.0), 1.0, places=15)
        self.assertAlmostEqual(families.support_width("bach", 0.5), 3.0, places=12)
        for alpha in (1.5, 2.0, 3.0):
            self.assertAlmostEqual(
                families.support_width("tsallis", alpha), alpha / (alpha - 1.0), places=12
            )
        self.assertEqual(families.support_width("logistic"), float("inf"))
        self.assertAlmostEqual(families.support_width("ramp"), 1.0, places=15)

    def test_courbure_de_bach(self):
        """`f_rho''(w) = 1 / (rho w + 1 - rho)^3`, verifiee par differences finies."""
        grid = np.linspace(0.05, 0.95, 19)
        step = 1e-6
        for rho in (0.0, 0.25, 0.5, 0.75):
            handle = families.family("bach", rho)
            finite = (handle.potential_slope(grid + step) - handle.potential_slope(grid - step)) / (
                2.0 * step
            )
            closed = handle.potential_curvature(grid)
            self.assertLess(float(np.max(np.abs(finite - closed) / closed)), 1e-5)
            self.assertTrue(np.all(closed > 0.0))

    def test_limites_de_tsallis(self):
        """`alpha` grand degenere vers l'indicatrice dure, `alpha -> 1` vers Shannon."""
        grid = np.linspace(-3.0, 3.0, 25)
        far = families.occupancy(grid, "tsallis", 80.0)
        self.assertTrue(np.all(far[grid < -0.05] == 0.0))
        self.assertTrue(np.all(far[grid > 0.05] > 0.9))
        self.assertAlmostEqual(families.support_width("tsallis", 80.0), 80.0 / 79.0, places=12)
        self.assertGreater(families.support_width("tsallis", 1.01), 100.0)
        weights = np.linspace(0.05, 0.95, 9)
        shannon = weights * np.log(weights)
        self.assertLess(
            float(np.max(np.abs(families.family("tsallis", 1.001).potential(weights) - shannon))),
            2e-3,
        )

    def test_representation_integrale_de_bach(self):
        """`t log t - t + 1 = int_0^1 2 f_rho(t) (1 - rho) drho`, citee dans la doctrine.

        Deux quadratures independantes : un trapeze dense (temoin naif) et
        `scipy.integrate.quad` (temoin adaptatif, precision annoncee dans la
        doctrine). La seconde est celle qui autorise la citation a `1e-12`.
        """
        checked = 0
        for value in (0.05, 0.2, 0.5, 2.0, 5.0, 50.0):
            left = value * math.log(value) - value + 1.0
            grid = np.linspace(1e-9, 1.0 - 1e-9, 200001)
            integrand = (value - 1.0) ** 2 / (grid * value + 1.0 - grid) * (1.0 - grid)
            right = float(np.trapezoid(integrand, grid))
            self.assertLess(abs(left - right), 1e-4 * max(1.0, abs(left)))
            adaptive, error = integrate.quad(
                lambda rho, t=value: (t - 1.0) ** 2 / (rho * t + 1.0 - rho) * (1.0 - rho),
                0.0,
                1.0,
                limit=400,
            )
            self.assertLess(error, 1e-6)
            self.assertLess(abs(left - float(adaptive)), 1e-12 * max(1.0, abs(left)))
            checked += 1
        self.assertGreaterEqual(checked, 6)

    def test_limite_de_neyman(self):
        """Quand `rho` tend vers 1, `g_rho(u)` tend vers `(1 - 2 u)^{-1/2}`."""
        grid = np.linspace(-3.0, -0.1, 30)
        expected = np.power(1.0 - 2.0 * grid, -0.5)
        obtained = families.occupancy(grid, "bach", 0.999)
        self.assertLess(float(np.max(np.abs(obtained - expected))), 2e-3)

    def test_rampe_est_bach_rho_zero_a_translation_pres(self):
        """Fixture gravee : `w^2/2` et `f_0` donnent les memes poids.

        Le multiplicateur de `f_0` vaut celui de la rampe MOINS `epsilon`,
        conformement a l'invariance affine (`f_0 = w^2/2 - w + 1/2`, donc
        une pente affine `alpha = -1` et un decalage `epsilon * alpha`).
        """
        energies = np.array([0.0, 1.0, 1.5, 3.0, 4.0, 9.0])
        for mass in (1, 2, 3):
            for epsilon in (0.5, 1.0, 2.0):
                ramp_weights, ramp_level = families.regularised_weights(
                    energies, mass, epsilon, "ramp"
                )
                bach_weights, bach_level = families.regularised_weights(
                    energies, mass, epsilon, "bach", 0.0
                )
                self.assertLess(float(np.max(np.abs(ramp_weights - bach_weights))), 1e-12)
                self.assertAlmostEqual(bach_level - ramp_level, -epsilon, places=9)

    def test_tsallis_alpha_deux_est_la_rampe_de_largeur_double(self):
        energies = np.array([0.0, 0.7, 1.3, 2.9, 5.0, 8.0, 11.0])
        for mass in (1, 2, 3):
            for epsilon in (0.5, 1.0, 2.0):
                left, _level = families.regularised_weights(energies, mass, epsilon, "tsallis", 2.0)
                right, _other = families.regularised_weights(energies, mass, 2.0 * epsilon, "ramp")
                self.assertLess(float(np.max(np.abs(left - right))), 1e-9)


class TestEncadrementExact(unittest.TestCase):
    """Theoreme 1 : encadrement en rationnels exacts, en d = 2, 3 et 20."""

    def test_encadrement_et_planchers(self):
        totals = {"cases": 0, "inner": 0, "soft": 0, "outer_false": 0, "strict": 0, "critical": 0}
        visited = set()
        for dimension, count, seed in GEOMETRIES:
            local = dict((key, 0) for key in totals)
            for energies, order, level, epsilon, critical in corpus(dimension, count, seed):
                verdict = ramp_exact.check_bracket(energies, order, level, epsilon)
                self.assertTrue(verdict["inner_ok"], msg=(dimension, order, level, epsilon))
                self.assertTrue(verdict["outer_ok"], msg=(dimension, order, level, epsilon))
                local["cases"] += 1
                local["inner"] += int(verdict["inner"])
                local["soft"] += int(verdict["soft"])
                local["outer_false"] += int(not verdict["outer"])
                local["strict"] += int(verdict["strict"])
                local["critical"] += int(critical)
            # Planchers PAR GEOMETRIE : sans eux, les planchers globaux
            # (cales au tiers) seraient atteints par une seule dimension,
            # donc la perte silencieuse de `d = 20` passerait la porte.
            for key, floor in PER_GEOMETRY_FLOORS.items():
                self.assertGreaterEqual(
                    local[key], floor, msg=str(dimension) + " " + key + " " + str(local)
                )
            visited.add(dimension)
            for key in totals:
                totals[key] += local[key]
        self.assertEqual(visited, REQUIRED_DIMENSIONS)
        for key, floor in FLOORS.items():
            self.assertGreaterEqual(totals[key], floor, msg=key + " " + str(totals))

    def test_forme_fonctionnelle(self):
        """Theoreme 2 : `a_k < A^k_epsilon <= a_k + epsilon`, borne atteinte."""
        attained = 0
        checked = 0
        for dimension, count, seed in GEOMETRIES:
            seen = set()
            for energies, order, _level, epsilon, _critical in corpus(dimension, count, seed):
                key = (id(energies), order, epsilon)
                if key in seen:
                    continue
                seen.add(key)
                hard = ramp_exact.hard_level(energies, order)
                soft = ramp_exact.soft_level_exact(energies, order, epsilon)
                self.assertGreater(soft, hard)
                self.assertLessEqual(soft, hard + epsilon)
                attained += int(soft == hard + epsilon)
                checked += 1
        self.assertGreaterEqual(checked, 100)
        self.assertGreaterEqual(attained, 1)

    def test_borne_superieure_atteinte_fixture(self):
        """Fixture gravee : `k` points a egale distance et le reste au loin."""
        cloud = [(1, 0, 0), (0, 1, 0), (0, 0, 1), (100, 0, 0)]
        energies = ramp_exact.rational_energies(cloud, (0, 0, 0))
        self.assertEqual(ramp_exact.hard_level(energies, 3), Fraction(1))
        self.assertEqual(ramp_exact.soft_level_exact(energies, 3, Fraction(2)), Fraction(3))
        self.assertEqual(
            ramp_exact.soft_count_exact(energies, Fraction(2), Fraction(2)), Fraction(3, 2)
        )

    def test_accord_avec_le_proposeur_flottant(self):
        """Le niveau exact et le niveau de Fermi flottant coincident."""
        checked = 0
        for dimension, count, seed in GEOMETRIES[:2]:
            cloud = _cloud(count, dimension, seed)
            for probe in _probes(cloud, dimension, seed)[:3]:
                energies = ramp_exact.rational_energies(cloud, probe)
                numeric = np.array([float(value) for value in energies])
                for order in (1, 3):
                    for epsilon in (Fraction(1, 2), Fraction(2)):
                        exact = float(ramp_exact.soft_level_exact(energies, order, epsilon))
                        by_families = families.fermi_level(numeric, order, float(epsilon), "ramp")
                        by_fermi = fermi.fermi_level(numeric, order, float(epsilon), "ramp")
                        self.assertLess(abs(exact - by_families), 1e-8)
                        self.assertLess(abs(exact - by_fermi), 1e-6)
                        checked += 1
        self.assertGreaterEqual(checked, 20)


class TestMonotonie(unittest.TestCase):
    """Theoreme 3 : monotonie en `epsilon` et limite."""

    def test_comptage_decroissant_et_niveau_croissant(self):
        ladder = (
            Fraction(1, 8),
            Fraction(1, 4),
            Fraction(1, 2),
            Fraction(1),
            Fraction(3),
            Fraction(7),
        )
        checked = 0
        nested = 0
        for dimension, count, seed in GEOMETRIES:
            cloud = _cloud(count, dimension, seed)
            for probe in _probes(cloud, dimension, seed)[:5]:
                energies = ramp_exact.rational_energies(cloud, probe)
                for order in ORDERS:
                    levels = [ramp_exact.soft_level_exact(energies, order, eps) for eps in ladder]
                    for index in range(len(ladder) - 1):
                        self.assertLessEqual(levels[index], levels[index + 1])
                    # Les niveaux `a_order + delta` sont les seuls ou la
                    # transition d'appartenance tombe DANS l'echelle de
                    # `epsilon` : sans eux le compteur `nested` descend a 11
                    # sur 180 cas et le plancher deviendrait fragile.
                    hard_here = ramp_exact.hard_level(energies, order)
                    probes = (
                        hard_here,
                        Fraction(5),
                        Fraction(40),
                        hard_here + Fraction(1, 2),
                        hard_here + 3,
                    )
                    for level in probes:
                        counts = [
                            ramp_exact.soft_count_exact(energies, level, eps) for eps in ladder
                        ]
                        for index in range(len(ladder) - 1):
                            self.assertGreaterEqual(counts[index], counts[index + 1])
                        memberships = [value >= order for value in counts]
                        for index in range(len(ladder) - 1):
                            self.assertTrue(memberships[index] or not memberships[index + 1])
                            nested += int(memberships[index] and not memberships[index + 1])
                        checked += 1
        self.assertGreaterEqual(checked, 250)
        self.assertGreaterEqual(nested, 40)

    def test_limite_epsilon_nul_manque_le_bord(self):
        """Le bord `a_k(y) = a` n'est atteint par aucun `epsilon > 0`."""
        cloud = [(0, 0), (3, 4), (6, 8)]
        energies = ramp_exact.rational_energies(cloud, (0, 0))
        level = ramp_exact.hard_level(energies, 2)
        self.assertTrue(ramp_exact.belongs_hard(energies, 2, level))
        for epsilon in (Fraction(1, 1000), Fraction(1, 10), Fraction(1), Fraction(10)):
            self.assertFalse(ramp_exact.belongs_soft(energies, 2, level, epsilon))
        for epsilon in (Fraction(1, 1000), Fraction(1, 100)):
            self.assertTrue(ramp_exact.belongs_soft(energies, 2, level + epsilon, epsilon))


class TestTension(unittest.TestCase):
    """Theoreme 4 : le decalage `epsilon` ne peut pas etre reduit."""

    def test_temoin_grave_theta_un_demi(self):
        cloud = [(0, 0), (100, 0)]
        energies = ramp_exact.rational_energies(cloud, (1, 0))
        level = Fraction(2)
        epsilon = Fraction(2)
        self.assertTrue(ramp_exact.belongs_hard(energies, 1, level - epsilon / 2))
        self.assertFalse(ramp_exact.belongs_soft(energies, 1, level, epsilon))
        self.assertEqual(ramp_exact.soft_count_exact(energies, level, epsilon), Fraction(1, 2))
        self.assertTrue(ramp_exact.check_bracket(energies, 1, level, epsilon)["inner_ok"])

    def test_temoins_pour_tout_theta(self):
        """Pour `theta` dans (0, 1) et `k` varie, un temoin exact a chaque fois."""
        checked = 0
        for theta, epsilon, level in (
            (Fraction(1, 4), Fraction(4), Fraction(2)),
            (Fraction(1, 2), Fraction(2), Fraction(2)),
            (Fraction(3, 4), Fraction(4), Fraction(4)),
        ):
            radius_squared = level - theta * epsilon
            self.assertEqual(radius_squared, Fraction(1))
            for order in (1, 2, 3):
                cloud = []
                for index in range(order):
                    point = [0] * 4
                    point[index] = 1
                    cloud.append(tuple(point))
                cloud.append((50, 50, 50, 50))
                energies = ramp_exact.rational_energies(cloud, (0, 0, 0, 0))
                self.assertTrue(ramp_exact.belongs_hard(energies, order, level - theta * epsilon))
                self.assertFalse(ramp_exact.belongs_soft(energies, order, level, epsilon))
                self.assertEqual(
                    ramp_exact.soft_count_exact(energies, level, epsilon), order * theta
                )
                checked += 1
        self.assertGreaterEqual(checked, 9)


class TestVarianteLogistique(unittest.TestCase):
    """Theoreme 5 : constante exacte `epsilon log(2 n - 1)` et seuil `k - 1/2`."""

    @staticmethod
    def _soft(energies, level, epsilon):
        ratio = np.clip((level - energies) / epsilon, -700.0, 700.0)
        return float(np.sum(1.0 / (1.0 + np.exp(-ratio))))

    def test_encadrement_logistique(self):
        """Encadrement logistique, y compris dans le regime ou `s` travaille.

        Le corpus aleatoire du premier bloc ne fait JAMAIS travailler le
        decalage `s` a droite : sur les 608 cas ou `S >= k - 1/2`, le
        comptage dur SANS decalage suffit deja les 608 fois (mesure). Une
        implementation qui oublierait le `+ s` a droite passerait donc ce
        bloc, alors que l'enonce `{S >= k - 1/2} inclus dans L_k(a)` est
        faux en general. Le second bloc place les deux inclusions dans leur
        regime critique (`k = n` a gauche, `k = 1` a droite) et porte les
        planchers correspondants.
        """
        source = random.Random(29)
        checked = 0
        inner_seen = 0
        outer_seen = 0
        shift_needed = 0
        tight_left = 0
        for dimension, count, seed in ((3, 10, 23), (20, 12, 37)):
            cloud = np.array(_cloud(count, dimension, seed), dtype=float)
            shift_factor = math.log(2.0 * count - 1.0)
            for _trial in range(12):
                probe = np.array([source.uniform(0.0, 12.0) for _ in range(dimension)])
                energies = np.sum((cloud - probe) ** 2, axis=1)
                for order in (1, 3, count):
                    for epsilon in (0.25, 1.0, 4.0):
                        shift = epsilon * shift_factor
                        for level in (
                            float(np.min(energies)),
                            float(np.median(energies)),
                            float(np.max(energies)),
                            float(np.sort(energies)[order - 1]) + shift,
                        ):
                            soft = self._soft(energies, level, epsilon)
                            hard_inner = int(np.sum(energies <= level - shift)) >= order
                            hard_outer = int(np.sum(energies <= level + shift)) >= order
                            if hard_inner:
                                self.assertGreaterEqual(soft, order - 0.5 - 1e-12)
                                inner_seen += 1
                            if soft >= order - 0.5:
                                self.assertTrue(hard_outer)
                                outer_seen += 1
                                shift_needed += int(
                                    int(np.sum(energies <= level)) < order
                                )
                            checked += 1
        for count in (4, 8, 12, 30):
            shift_factor = math.log(2.0 * count - 1.0)
            for epsilon in (0.25, 1.0, 4.0):
                shift = epsilon * shift_factor
                level = 100.0
                # Droite, cas critique `k = 1` : les `n` observations sont
                # juste au-dela de `a + factor * s`, donc `L_1(a)` ne
                # contient pas `y` alors que `S >= 1/2`. Seule l'inclusion
                # DECALEE peut etre vraie ici.
                for factor in (0.25, 0.5, 0.9):
                    energies = np.full(count, level + factor * shift + 1e-6)
                    soft = self._soft(energies, level, epsilon)
                    self.assertGreaterEqual(soft, 0.5)
                    self.assertLess(int(np.sum(energies <= level)), 1)
                    self.assertGreaterEqual(int(np.sum(energies <= level + shift)), 1)
                    outer_seen += 1
                    shift_needed += 1
                    checked += 1
                # Gauche, cas critique `k = n` : les `n` observations sont
                # exactement a `a - s`, donc `S = n sigma(s / epsilon)` vaut
                # `n - 1/2` SANS marge. C'est la configuration qui rend la
                # constante `epsilon log(2 n - 1)` inameliorable a gauche.
                energies = np.full(count, level - shift)
                soft = self._soft(energies, level, epsilon)
                self.assertGreaterEqual(int(np.sum(energies <= level - shift)), count)
                self.assertGreaterEqual(soft, count - 0.5 - 1e-9)
                self.assertLess(soft - (count - 0.5), 1e-9)
                inner_seen += 1
                tight_left += 1
                checked += 1
        self.assertGreaterEqual(checked, 400)
        self.assertGreaterEqual(inner_seen, 150)
        self.assertGreaterEqual(outer_seen, 200)
        self.assertGreaterEqual(shift_needed, 12)
        self.assertGreaterEqual(tight_left, 4)

    def test_seuil_entier_impossible(self):
        """Avec le seuil `k` et `n = k`, le sous-niveau doux est vide.

        L'enonce est mathematique : `sigma < 1` strictement, donc
        `S_epsilon < n = k` pour tous `y` et `a`. En flottant, `sigma(u)`
        vaut exactement `1.0` des que `u` depasse environ `37`, si bien que
        la porte se tient dans le regime REPRESENTABLE `u <= 25` ; la
        saturation flottante au-dela est une limite de la representation,
        pas un contre-exemple.
        """
        count = 6
        cloud = np.eye(count) * 3.0
        probe = np.zeros(count)
        energies = np.sum((cloud - probe) ** 2, axis=1)
        checked = 0
        for epsilon, level in ((1.0, 9.0), (1.0, 20.0), (0.5, 12.0), (2.0, 40.0)):
            self.assertLessEqual((level - float(np.min(energies))) / epsilon, 25.0)
            self.assertLess(self._soft(energies, level, epsilon), float(count))
            checked += 1
        self.assertGreaterEqual(checked, 4)
        self.assertGreaterEqual(int(np.sum(energies <= 9.0)), count)

    def test_constante_optimale(self):
        """Un decalage plus petit que `epsilon log(2 n - 1)` echoue.

        Les DEUX cotes sont temoignes, avec des configurations critiques
        differentes : `k = 1` a droite (les `n` observations juste au-dela
        du niveau decale), `k = n` a gauche (les `n` observations
        exactement au niveau decale). Sans le second temoin, l'optimalite
        annoncee ne serait etablie qu'a droite.
        """
        epsilon = 1.0
        level = 10.0
        right_witnesses = 0
        left_witnesses = 0
        for count in (4, 12, 30):
            exact_shift = epsilon * math.log(2.0 * count - 1.0)
            for factor in (0.25, 0.5, 0.9):
                shift = factor * exact_shift
                # Cote droit : `S >= 1/2` alors que `L_1(a + s')` est vide.
                radius_squared = level + shift + 1e-9
                cloud = np.eye(count) * math.sqrt(radius_squared)
                energies = np.sum(cloud ** 2, axis=1)
                soft = self._soft(energies, level, epsilon)
                self.assertGreaterEqual(soft, 0.5)
                self.assertLess(int(np.sum(energies <= level + shift)), 1)
                right_witnesses += 1
                # Cote gauche : les `n` observations sont dans
                # `L_n(a - s')` mais `S < n - 1/2`.
                energies = np.full(count, level - shift)
                soft = self._soft(energies, level, epsilon)
                self.assertGreaterEqual(int(np.sum(energies <= level - shift)), count)
                self.assertLess(soft, count - 0.5)
                left_witnesses += 1
        # La constante EXACTE, elle, ne doit pas produire de temoin.
        for count in (4, 12, 30):
            exact_shift = epsilon * math.log(2.0 * count - 1.0)
            energies = np.full(count, level - exact_shift)
            self.assertGreaterEqual(
                self._soft(energies, level, epsilon), count - 0.5 - 1e-9
            )
        self.assertGreaterEqual(right_witnesses, 9)
        self.assertGreaterEqual(left_witnesses, 9)


class TestMutants(unittest.TestCase):
    """Chaque mutant doit etre tue par au moins une inclusion du theoreme 1."""

    def test_mutants_tues(self):
        kills = {name: {"inner": 0, "outer": 0} for name in ramp_exact.MUTANTS}
        self.assertEqual(frozenset(item[0] for item in GEOMETRIES), REQUIRED_DIMENSIONS)
        for dimension, count, seed in GEOMETRIES:
            for energies, order, level, epsilon, _critical in corpus(dimension, count, seed):
                for name in ramp_exact.MUTANTS:
                    verdict = ramp_exact.check_bracket(energies, order, level, epsilon, name)
                    kills[name]["inner"] += int(not verdict["inner_ok"])
                    kills[name]["outer"] += int(not verdict["outer_ok"])
        # Planchers cales sur le tiers du mesure (646 / 886 / 18), et non sur
        # un simple << au moins une mort >> : un mutant qui ne mourrait plus
        # que trois fois signalerait un corpus appauvri.
        floors = {"no_epsilon": 200, "tailed": 290, "shifted": 6}
        for name in ramp_exact.MUTANTS:
            total = kills[name]["inner"] + kills[name]["outer"]
            self.assertGreaterEqual(total, floors[name], msg=name + " : " + str(kills))
        self.assertGreaterEqual(kills["no_epsilon"]["inner"], 200)
        self.assertGreaterEqual(kills["tailed"]["inner"], 290)
        self.assertGreaterEqual(kills["tailed"]["outer"], 3)
        self.assertGreaterEqual(kills["shifted"]["outer"], 6)
        # La faute `no_epsilon` est INVISIBLE a droite : c'est le choix de
        # `epsilon < 1` dans le corpus qui la tue, et une porte a
        # `epsilon >= 1` seulement la laisserait passer entierement.
        self.assertEqual(kills["no_epsilon"]["outer"], 0)
        self.assertEqual(kills["shifted"]["inner"], 0)
        small = {"inner": 0, "outer": 0}
        for dimension, count, seed in GEOMETRIES:
            for energies, order, level, epsilon, _critical in corpus(dimension, count, seed):
                if epsilon >= 1:
                    continue
                verdict = ramp_exact.check_bracket(
                    energies, order, level, epsilon, "no_epsilon"
                )
                small["inner"] += int(not verdict["inner_ok"])
                small["outer"] += int(not verdict["outer_ok"])
        self.assertEqual(small["inner"], kills["no_epsilon"]["inner"])
        self.assertGreaterEqual(small["inner"], 200)

    def test_mutant_inconnu_refuse(self):
        energies = (Fraction(1), Fraction(4))
        self.assertRaises(
            ValueError, ramp_exact.ramp_weight, Fraction(1), Fraction(2), Fraction(1), "absent"
        )
        self.assertRaises(ValueError, ramp_exact.soft_count_exact, energies, 1, 0)


if __name__ == "__main__":
    unittest.main()
