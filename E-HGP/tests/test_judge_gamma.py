"""Porte de confrontation : juge independant contre `ehgp.exact.tower`.

Trois implementations de `pi_0(Gamma_k(a))` sont confrontees :

1. le SUJET, `ehgp.exact.tower.FullTower` (balayage incremental, union-find,
   cofaces generatrices, miniball par Gram des vecteurs differences) ;
2. le JUGE, `ehgp.judge.gamma_bfs.GammaJudge` (graphe reconstruit a chaque
   niveau, parcours en largeur, adjacence par paires, miniball par systeme
   borde de la matrice de distances) ;
3. la machinerie du depot, `reference/morsehgp3d_oracle/`, alimentee par
   notre miniball exacte injectee, sans modifier un seul de ses fichiers.

La porte ne repose sur aucun `assert` du langage : seules les methodes
`self.assert*` de `unittest` decident, et elles tiennent sous `python3 -O`.
Des planchers de couverture explicites interdisent le vert par vacuite.
"""

import random
import sys
import unittest
from fractions import Fraction
from itertools import combinations

SOURCE_ROOT = "/workspaces/E-HGP/E-HGP/src"
REFERENCE_ROOT = "/workspaces/E-HGP/reference"
for _root in (SOURCE_ROOT, REFERENCE_ROOT):
    if _root not in sys.path:
        sys.path.insert(0, _root)

from ehgp.exact import meb
from ehgp.exact.tower import FullTower
from ehgp.judge import gamma_bfs


# Planchers de couverture. Une campagne qui passe en dessous est vide de sens
# et doit echouer, meme si elle ne trouve aucun desaccord.
# Le corpus est grave et deterministe (fixtures a coordonnees exactes, graines
# fixees, arithmetique rationnelle) : les chiffres sont donc reproductibles au
# bit, et les planchers sont poses juste sous la mesure pour qu'ils MORDENT.
# Un plancher a 3 % de la mesure serait decoratif.
MINIMUM_CLOUDS = 20
MINIMUM_FIXTURES = 6
MINIMUM_COMPARISONS = 4500          # mesure : 4650
MINIMUM_CRITICAL_LEVELS = 1200      # mesure : 1225
MINIMUM_BETA_CALLS = 3800           # mesure : 3822
MINIMUM_BETA_CHECKED = 3800         # mesure : 3822
MINIMUM_EDGE_TESTS = 550000         # mesure : 577603
MINIMUM_BFS_VISITS = 98000          # mesure : 101358
MINIMUM_WELZL_SUBSETS = 750         # mesure : 790
MINIMUM_INJECTION_COMPARISONS = 150  # mesure : 162
MINIMUM_PLAIN_COMPARISONS = 30      # mesure : 39
MINIMUM_MUTANTS = 5
MINIMUM_MULTI_COMPONENT = 1900      # mesure : 1957
MINIMUM_MERGED = 3800               # mesure : 3909
REQUIRED_DIMENSIONS = (2, 3, 5, 20)

# Refus exacts que `reference/morsehgp3d_oracle/gamma.py` oppose a la grande
# dimension. Les portes les exigent AU MOT : un `assertRaises(ValueError)` nu
# passerait sur n'importe quel autre refus, donc pour une mauvaise raison.
FIRST_GATE_MESSAGE = "MorseHGP3D points must have exactly three coordinates"
SECOND_GATE_MESSAGE = "a miniball center must have three coordinates"


def _graven_clouds():
    """Fixtures gravees aux coordonnees exactes, toutes degenerees a dessein."""
    fixtures = []
    # Positions dupliquees : deux paires confondues plus un point isole.
    fixtures.append(
        (
            "doublons_d3",
            [(0, 0, 0), (0, 0, 0), (5, 0, 0), (5, 0, 0), (2, 3, 0)],
            4,
        )
    )
    # Quatre points cocycliques (carre inscrit) et leur centre : plateaux
    # cospheriques et niveaux egaux en pagaille.
    fixtures.append(
        (
            "quatre_cocycliques_d2",
            [(5, 0), (0, 5), (-5, 0), (0, -5), (0, 0)],
            4,
        )
    )
    # Contre-famille du depot : sept points colineaires.
    fixtures.append(
        ("sept_colineaires_d3", [(index, 0, 0) for index in range(7)], 4)
    )
    # Simplexe regulier a coordonnees entieres en dimension cinq : les cinq
    # vecteurs de base, toutes distances au carre egales a deux.
    fixtures.append(
        (
            "simplexe_regulier_d5",
            [tuple(1 if axis == index else 0 for axis in range(5)) for index in range(5)],
            4,
        )
    )
    # Deux amas distants en grande dimension.
    far = 100
    cluster_a = [
        tuple(0 for _ in range(20)),
        tuple(1 if axis == 0 else 0 for axis in range(20)),
        tuple(1 if axis == 1 else 0 for axis in range(20)),
    ]
    cluster_b = [
        tuple(far for _ in range(20)),
        tuple(far + (1 if axis == 2 else 0) for axis in range(20)),
        tuple(far + (1 if axis == 3 else 0) for axis in range(20)),
    ]
    fixtures.append(("deux_amas_d20", cluster_a + cluster_b, 3))
    # Deux amas distants en dimension trois, pour que la meme forme soit
    # jugee dans le regime ou le juge par grille du depot existe aussi.
    fixtures.append(
        (
            "deux_amas_d3",
            [(0, 0, 0), (1, 0, 0), (0, 1, 0), (50, 50, 50), (51, 50, 50), (50, 51, 50)],
            4,
        )
    )
    return fixtures


def _random_clouds():
    """Nuages entiers reproductibles : graine fixee, dimensions 2, 3, 5 et 20."""
    plan = (
        (2, 9, 4, 101),
        (2, 7, 3, 102),
        (2, 8, 4, 113),
        (3, 9, 4, 103),
        (3, 8, 4, 104),
        (3, 7, 4, 112),
        (3, 9, 4, 114),
        (5, 8, 4, 105),
        (5, 6, 4, 106),
        (5, 9, 4, 111),
        (5, 9, 4, 115),
        (20, 7, 4, 107),
        (20, 6, 4, 108),
        (20, 9, 3, 109),
        (20, 8, 4, 110),
    )
    clouds = []
    for dimension, count, k_max, seed in plan:
        generator = random.Random(seed)
        cloud = [
            tuple(generator.randint(0, 6) for _ in range(dimension))
            for _ in range(count)
        ]
        name = "alea_d%d_n%d_k%d_g%d" % (dimension, count, k_max, seed)
        clouds.append((name, cloud, k_max))
    return clouds


def _canonical_from_cut(cut):
    """Forme canonique du depot ramenee a celle de la tour et du juge."""
    return tuple(
        sorted(
            (component.facet_point_ids[0], component.covered_point_ids)
            for component in cut.components
        )
    )


class _InjectedBall:
    """Vue attendue par `reference/.../gamma.py` sur une miniball exacte."""

    def __init__(self, center, squared_radius, support_ids):
        self.center = center
        self.squared_radius = squared_radius
        self.support_ids = support_ids


def _injected_ball_function(points, subset_ids):
    """Adaptateur : notre miniball exacte, indices remis dans le repere global."""
    subset = tuple(subset_ids)
    center, squared_radius, support = meb.minimum_enclosing_ball(
        [points[index] for index in subset]
    )
    return _InjectedBall(
        center, squared_radius, tuple(subset[local] for local in support)
    )


def _dimension_free_coordinates(point):
    """Remplacant libre en dimension de `gamma._point_coordinates`."""
    return tuple(Fraction(value) for value in point)


def _dimension_free_coerce(ball, subset):
    """Remplacant libre en dimension de `gamma._coerce_ball`."""
    from morsehgp3d_oracle import gamma as gamma_module

    center = tuple(Fraction(value) for value in ball.center)
    squared_radius = Fraction(ball.squared_radius)
    support = gamma_module.canonical_label(ball.support_ids)
    if squared_radius < 0:
        raise ValueError("rayon au carre negatif")
    if not support or not set(support) <= set(subset):
        raise ValueError("support hors du label")
    return center, squared_radius, support


_CAMPAIGN = None


def _run_campaign():
    """Confronte juge et tour sur tout le corpus, une seule fois par session."""
    fixtures = _graven_clouds()
    corpus = fixtures + _random_clouds()
    report = {
        "clouds": 0,
        "fixtures": len(fixtures),
        "dimensions": set(),
        "comparisons": 0,
        "critical_levels": 0,
        "beta_calls": 0,
        "edge_tests": 0,
        "bfs_visits": 0,
        "level_set_agreements": 0,
        "level_set_mismatches": [],
        "mismatches": [],
        "beta_checked": 0,
        "beta_mismatches": [],
        "welzl_subsets": 0,
        "welzl_undecided": 0,
        "welzl_mismatches": [],
        "multi_component": 0,
        "merged": 0,
        "per_cloud": [],
    }
    for name, cloud, k_max in corpus:
        tower = FullTower(cloud, k_max)
        judge = gamma_bfs.GammaJudge(cloud, k_max)
        report["clouds"] += 1
        report["dimensions"].add(len(cloud[0]))
        report["critical_levels"] += len(tower.levels)
        if tuple(tower.levels) == judge.levels:
            report["level_set_agreements"] += 1
        else:
            report["level_set_mismatches"].append(
                (name, tuple(tower.levels), judge.levels)
            )
        local = 0
        for order in range(1, tower.effective + 1):
            for level in tower.levels:
                report["comparisons"] += 1
                local += 1
                expected = tower.partition_at(order, level)
                observed = judge.partition(order, level)
                if expected != observed:
                    report["mismatches"].append((name, order, level, expected, observed))
                if len(observed) >= 2:
                    report["multi_component"] += 1
                if any(len(covered) > order for _minimum, covered in observed):
                    report["merged"] += 1
        # Confrontation de l'arithmetique seule : niveau du juge contre niveau
        # du catalogue de supports du sujet, sur tous les sous-ensembles utiles.
        for size in range(1, judge.max_subset_size + 1):
            for subset in combinations(range(judge.count), size):
                mask = 0
                for index in subset:
                    mask |= 1 << index
                report["beta_checked"] += 1
                expected_level = tower.catalog.beta_mask(mask)
                if judge.beta(subset) != expected_level:
                    report["beta_mismatches"].append((name, subset, expected_level))
        if judge.count <= 7:
            compared, undecided, disagreements = judge.welzl_cross_check()
            report["welzl_subsets"] += compared
            report["welzl_undecided"] += undecided
            report["welzl_mismatches"].extend((name,) + item for item in disagreements)
        report["beta_calls"] += judge.beta_calls
        report["edge_tests"] += judge.edge_tests
        report["bfs_visits"] += judge.bfs_visits
        report["per_cloud"].append((name, len(cloud), len(cloud[0]), k_max, local))
    return report


def campaign():
    """Resultat de campagne mis en cache pour toute la session de tests."""
    global _CAMPAIGN
    if _CAMPAIGN is None:
        _CAMPAIGN = _run_campaign()
    return _CAMPAIGN


class TestJudgeAgreesWithFullTower(unittest.TestCase):
    """Le juge et la tour doivent coincider a chaque ordre et chaque niveau."""

    def test_partitions_coincide(self):
        report = campaign()
        self.assertEqual(report["mismatches"], [])

    def test_critical_level_sets_coincide(self):
        report = campaign()
        self.assertEqual(report["level_set_mismatches"], [])
        self.assertEqual(report["level_set_agreements"], report["clouds"])

    def test_independent_beta_arithmetic_coincides(self):
        report = campaign()
        self.assertEqual(report["beta_mismatches"], [])
        self.assertGreaterEqual(report["beta_checked"], MINIMUM_BETA_CHECKED)

    def test_two_judge_miniballs_coincide(self):
        report = campaign()
        self.assertEqual(report["welzl_mismatches"], [])
        self.assertGreaterEqual(report["welzl_subsets"], MINIMUM_WELZL_SUBSETS)


class TestCoverageFloors(unittest.TestCase):
    """Sans plancher, un juge vert ne prouve rien."""

    def test_floors(self):
        report = campaign()
        self.assertGreaterEqual(report["clouds"], MINIMUM_CLOUDS)
        self.assertGreaterEqual(report["fixtures"], MINIMUM_FIXTURES)
        self.assertGreaterEqual(report["comparisons"], MINIMUM_COMPARISONS)
        self.assertGreaterEqual(report["critical_levels"], MINIMUM_CRITICAL_LEVELS)
        self.assertGreaterEqual(report["beta_calls"], MINIMUM_BETA_CALLS)
        self.assertGreaterEqual(report["edge_tests"], MINIMUM_EDGE_TESTS)
        self.assertGreaterEqual(report["bfs_visits"], MINIMUM_BFS_VISITS)
        self.assertGreaterEqual(report["multi_component"], MINIMUM_MULTI_COMPONENT)
        self.assertGreaterEqual(report["merged"], MINIMUM_MERGED)

    def test_required_dimensions_present(self):
        report = campaign()
        for dimension in REQUIRED_DIMENSIONS:
            self.assertIn(dimension, report["dimensions"])

    def test_every_cloud_contributed_comparisons(self):
        report = campaign()
        for name, _count, _dimension, _k_max, local in report["per_cloud"]:
            self.assertGreater(local, 0, name)


class TestMutantsAreKilled(unittest.TestCase):
    """Chaque faute causale injectee dans le juge doit etre detectee."""

    MUTANT_CLOUDS = (
        ("d3_n4", [(0, 0, 0), (3, 0, 0), (0, 4, 0), (1, 1, 2)], 3),
        ("quatre_cocycliques_d2", [(5, 0), (0, 5), (-5, 0), (0, -5), (0, 0)], 3),
        ("sept_colineaires_d3", [(index, 0, 0) for index in range(7)], 3),
        (
            "simplexe_regulier_d5",
            [tuple(1 if axis == index else 0 for axis in range(5)) for index in range(5)],
            3,
        ),
    )

    def _disagreements(self, mutant):
        """Nombre de desaccords du juge mute, par nuage."""
        killed = {}
        for name, cloud, k_max in self.MUTANT_CLOUDS:
            tower = FullTower(cloud, k_max)
            judge = gamma_bfs.GammaJudge(cloud, k_max, mutant=mutant)
            count = 0
            for order in range(1, tower.effective + 1):
                for level in tower.levels:
                    if tower.partition_at(order, level) != judge.partition(order, level):
                        count += 1
            killed[name] = count
        return killed

    def test_all_mutants_die(self):
        survivors = []
        for mutant in gamma_bfs.MUTANTS:
            killed = self._disagreements(mutant)
            if sum(killed.values()) == 0:
                survivors.append((mutant, killed))
        self.assertEqual(survivors, [])

    def test_mutant_count_is_not_empty(self):
        self.assertGreaterEqual(len(gamma_bfs.MUTANTS), MINIMUM_MUTANTS)


class TestReferenceOracleInjection(unittest.TestCase):
    """La machinerie de `reference/` alimentee par notre miniball injectee.

    Fait verifie ligne a ligne, puis a l'execution : `gamma.py` n'a bien
    qu'une seule dependance de MODULE geometrique, injectable par `ball_fn`,
    et `hierarchy.py` n'importe que `gamma`. Mais `gamma.py` porte DEUX
    verrous de dimension trois qui lui sont propres et qu'aucune injection ne
    contourne : `_point_coordinates` et `_coerce_ball`. Ces deux verrous sont
    exerces ici, puis neutralises par un monkeypatch LOCAL, restaure en
    `tearDown`. Aucun fichier de `reference/` n'est modifie.
    """

    def setUp(self):
        from morsehgp3d_oracle import gamma as gamma_module

        self.gamma_module = gamma_module
        self.saved_coordinates = gamma_module._point_coordinates
        self.saved_coerce = gamma_module._coerce_ball

    def tearDown(self):
        self.gamma_module._point_coordinates = self.saved_coordinates
        self.gamma_module._coerce_ball = self.saved_coerce

    def _patch(self):
        self.gamma_module._point_coordinates = _dimension_free_coordinates
        self.gamma_module._coerce_ball = _dimension_free_coerce

    def _compare(self, cloud, k_max):
        """Compare les coupes fermees du depot aux partitions de la tour."""
        tower = FullTower(cloud, k_max)
        comparisons = 0
        mismatches = []
        for order in range(1, tower.effective + 1):
            filtration = self.gamma_module.build_gamma_filtration(
                cloud, order, ball_fn=_injected_ball_function
            )
            for level in tower.levels:
                comparisons += 1
                observed = _canonical_from_cut(filtration.cut(level))
                expected = tower.partition_at(order, level)
                if observed != expected:
                    mismatches.append((order, level, expected, observed))
        return comparisons, mismatches

    def test_first_dimension_gate_is_in_gamma_itself(self):
        cloud = [tuple(1 if axis == index else 0 for axis in range(5)) for index in range(5)]
        with self.assertRaisesRegex(ValueError, FIRST_GATE_MESSAGE):
            self.gamma_module.build_gamma_filtration(
                cloud, 2, ball_fn=_injected_ball_function
            )

    def test_second_dimension_gate_is_in_coerce_ball(self):
        cloud = [tuple(1 if axis == index else 0 for axis in range(5)) for index in range(5)]
        self.gamma_module._point_coordinates = _dimension_free_coordinates
        with self.assertRaisesRegex(ValueError, SECOND_GATE_MESSAGE):
            self.gamma_module.build_gamma_filtration(
                cloud, 2, ball_fn=_injected_ball_function
            )

    def test_injection_agrees_in_three_dimensions(self):
        cloud = [(0, 0, 0), (3, 0, 0), (0, 4, 0), (1, 1, 2), (2, 2, 2)]
        comparisons, mismatches = self._compare(cloud, 3)
        self.assertEqual(mismatches, [])
        self.assertGreaterEqual(comparisons, MINIMUM_PLAIN_COMPARISONS)

    def test_injection_agrees_in_high_dimension_once_patched(self):
        self._patch()
        total = 0
        clouds = (
            (
                [tuple(1 if axis == index else 0 for axis in range(5)) for index in range(5)],
                3,
            ),
            (_high_dimension_cloud(), 3),
        )
        for cloud, k_max in clouds:
            comparisons, mismatches = self._compare(cloud, k_max)
            self.assertEqual(mismatches, [])
            total += comparisons
        self.assertGreaterEqual(total, MINIMUM_INJECTION_COMPARISONS)

    def test_repository_hierarchy_runs_in_high_dimension_once_patched(self):
        from morsehgp3d_oracle import hierarchy as hierarchy_module

        self._patch()
        hierarchy = hierarchy_module.build_exhaustive_hierarchy(
            _high_dimension_cloud(), 3, "full_pi0", ball_fn=_injected_ball_function
        )
        self.assertEqual(hierarchy.k_eff, 3)
        self.assertEqual(len(hierarchy.forests), 3)


def _high_dimension_cloud():
    """Nuage entier reproductible en dimension vingt, six observations."""
    generator = random.Random(211)
    return [tuple(generator.randint(0, 6) for _ in range(20)) for _ in range(6)]


def _print_report():
    """Chiffres de la campagne, pour un recu. Renvoie un code de sortie."""
    report = campaign()
    ordered = (
        "clouds",
        "fixtures",
        "comparisons",
        "critical_levels",
        "beta_calls",
        "beta_checked",
        "edge_tests",
        "bfs_visits",
        "welzl_subsets",
        "welzl_undecided",
        "level_set_agreements",
        "multi_component",
        "merged",
    )
    for key in ordered:
        print("%-22s %d" % (key, report[key]))
    print("%-22s %s" % ("dimensions", sorted(report["dimensions"])))
    failures = (
        "mismatches",
        "level_set_mismatches",
        "beta_mismatches",
        "welzl_mismatches",
    )
    status = 0
    for key in failures:
        print("%-22s %d" % (key, len(report[key])))
        if report[key]:
            status = 1
    for name, count, dimension, k_max, local in report["per_cloud"]:
        print(
            "nuage %-26s n=%d d=%d K=%d comparaisons=%d"
            % (name, count, dimension, k_max, local)
        )
    return status


if __name__ == "__main__":
    if "--chiffres" in sys.argv:
        sys.exit(_print_report())
    unittest.main()
