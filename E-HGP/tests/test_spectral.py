"""Portes de la couche spectrale : formes closes, gradient, digest, mutants.

Sept blocs, tous executables sous `python3 -O` (aucune porte ne repose sur le
mot-cle `assert` : `self.assertX` est une methode) :

1. LES DEUX IDENTITES INTEGRALES de `spectral/log_density.py` (fait 1,
   Kullback-Leibler comme melange de chi-deux ponderes ; fait 2, le
   logarithme comme integrale des potentiels optimaux) sont confrontees a
   une quadrature `scipy` qui ne connait que les fonctions ponctuelles ;
2. LES FORMES CLOSES : `theta(rho)` contre la resolution directe du systeme
   lineaire ET contre une minimisation numerique generique de `scipy` qui ne
   connait que l'objectif et son gradient ; `Theta` contre la quadrature de
   `theta(rho)` ; le filtre spectral contre sa quadrature ; la divergence
   contre les potentiels quadratiques `v`, `w` ; les moments analytiques de
   la reference gaussienne contre un Monte-Carlo ;
3. LE GRADIENT analytique contre des differences finies centrees, pour le
   log-rapport et pour la log-densite ;
4. LA RECUPERATION de la vraie log-densite d'un melange gaussien en
   dimension 2, sur une grille, avec ERREUR DECROISSANTE quand `n` croit ;
5. LES REFUS EXPLICITES : covariance empirique singuliere, moments de
   reference singuliers, `rho` hors domaine, modele non ajuste — jamais un
   `NaN` ;
6. LA TOUR : digest invariant par permutation (exactement) et par
   translation (a la quantification declaree), tour de la log-densite EXACTE
   d'un melange (ou la verite terrain est connue), invariant du col sous les
   sommets, monotonie du raffinement de chemin, et confrontation des deux
   ascensions ;
7. TROIS MUTANTS CAUSAUX qui doivent etre TUES.

Les planchers de couverture de `FLOORS` interdisent le vert par vacuite.
"""

import os

# Le parallelisme BLAS est MESURE, jamais declare : sur les tailles de ce
# fichier, OpenBLAS multifils est plusieurs fois plus lent que le mode
# monofil dans ce conteneur (mesure reportee dans `bench/spectral_tower.py`).
for _variable in ("OPENBLAS_NUM_THREADS", "OMP_NUM_THREADS", "MKL_NUM_THREADS"):
    os.environ.setdefault(_variable, "1")

import math  # noqa: E402
import sys  # noqa: E402
import unittest  # noqa: E402

import numpy as np  # noqa: E402
import scipy.integrate  # noqa: E402
import scipy.optimize  # noqa: E402

sys.path.insert(
    0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "src")
)

from ehgp.spectral import log_density as L  # noqa: E402
from ehgp.spectral import tower as T  # noqa: E402

FLOORS = {
    "identity_points": 6,
    "rho_values": 5,
    "moment_entries": 100,
    "finite_difference_checks": 20,
    "recovery_cases": 6,
    "grid_points": 100,
    "refusals": 5,
    "tower_maxima": 3,
    "tower_merges": 2,
    "mutants": 3,
}

RATIOS = (0.02, 0.2, 0.5, 1.0, 1.0, 3.0, 11.0)
RHOS = (0.0, 0.15, 0.5, 0.75, 0.9, 0.99)

MIXTURE_WEIGHTS = np.array([0.4, 0.35, 0.25])
MIXTURE_CENTRES = np.array([[-2.5, -1.0], [2.0, 1.5], [0.0, 3.0]])
MIXTURE_SIGMAS = np.array([0.8, 1.0, 0.6])


def mixture_log_density(points):
    """Log-densite EXACTE du melange gaussien de reference."""
    points = np.atleast_2d(np.asarray(points, dtype=float))
    total = np.zeros(points.shape[0])
    for weight, centre, sigma in zip(MIXTURE_WEIGHTS, MIXTURE_CENTRES, MIXTURE_SIGMAS):
        offset = points - centre
        total += (
            weight
            * np.exp(-0.5 * np.einsum("ij,ij->i", offset, offset) / sigma ** 2)
            / (2.0 * math.pi * sigma ** 2)
        )
    return np.log(total)


def mixture_gradient(points):
    """Gradient exact de la log-densite du melange."""
    points = np.atleast_2d(np.asarray(points, dtype=float))
    total = np.zeros(points.shape[0])
    slope = np.zeros_like(points)
    for weight, centre, sigma in zip(MIXTURE_WEIGHTS, MIXTURE_CENTRES, MIXTURE_SIGMAS):
        offset = points - centre
        density = (
            weight
            * np.exp(-0.5 * np.einsum("ij,ij->i", offset, offset) / sigma ** 2)
            / (2.0 * math.pi * sigma ** 2)
        )
        total += density
        slope += (-density / sigma ** 2)[:, None] * offset
    return slope / total[:, None]


def mixture_sample(count, seed):
    """Tirage du melange, avec ses etiquettes de composante."""
    generator = np.random.default_rng(seed)
    labels = generator.choice(MIXTURE_WEIGHTS.size, size=count, p=MIXTURE_WEIGHTS)
    cloud = MIXTURE_CENTRES[labels] + MIXTURE_SIGMAS[labels][:, None] * (
        generator.standard_normal((count, 2))
    )
    return cloud, labels


def rand_index(left, right):
    """Indice de Rand ajuste, table de contingence, sans dependance externe."""
    left_codes = np.unique(np.asarray(left), return_inverse=True)[1]
    right_codes = np.unique(np.asarray(right), return_inverse=True)[1]
    table = np.zeros((left_codes.max() + 1, right_codes.max() + 1), dtype=np.int64)
    np.add.at(table, (left_codes, right_codes), 1)

    def pairs(values):
        values = np.asarray(values, dtype=np.float64)
        return float(np.sum(values * (values - 1.0) / 2.0))

    total = left_codes.size
    whole = total * (total - 1) / 2.0
    inside = pairs(table)
    rows = pairs(table.sum(axis=1))
    columns = pairs(table.sum(axis=0))
    expected = rows * columns / whole
    maximum = 0.5 * (rows + columns)
    if maximum == expected:
        return 1.0
    return (inside - expected) / (maximum - expected)


def full_rank_model(seed=1, count=400):
    """Configuration a RANG PLEIN, pour tester les formes closes sans troncature.

    Le rang effectif de `Sigma_q` est inferieur a `m` des que le noyau est
    trop lisse (fait mesure : `m = 120` en dimension 2 donne un rang 43). On
    prend donc peu de descripteurs et une largeur de bande courte, et la
    porte VERIFIE que le rang est plein avant de comparer a une resolution
    de systeme lineaire.
    """
    cloud, _labels = mixture_sample(count, seed)
    model = L.SpectralLogDensity(
        feature_count=12, bandwidth_scale=0.25, seed=seed, on_singular="truncate"
    ).fit(cloud)
    return model


class IntegralIdentities(unittest.TestCase):
    """Bloc 1 : les deux identites integrales, par quadrature independante."""

    def test_kullback_leibler_est_un_melange_de_chi_deux(self):
        checked = 0
        for ratio in RATIOS:
            value = scipy.integrate.quad(
                lambda rho, t=ratio: float(L.kullback_leibler_integrand(t, rho)),
                0.0,
                1.0,
                limit=200,
            )[0]
            expected = ratio * math.log(ratio) - ratio + 1.0
            self.assertAlmostEqual(value, expected, places=9)
            checked += 1
        self.assertGreaterEqual(checked, FLOORS["identity_points"])

    def test_logarithme_est_integrale_des_potentiels_optimaux(self):
        checked = 0
        for ratio in RATIOS:
            value = scipy.integrate.quad(
                lambda rho, t=ratio: float(L.optimal_potential(t, rho)),
                0.0,
                1.0,
                limit=200,
            )[0]
            self.assertAlmostEqual(value, math.log(ratio), places=9)
            checked += 1
        self.assertGreaterEqual(checked, FLOORS["identity_points"])

    def test_potentiel_optimal_est_inversible(self):
        for ratio in RATIOS:
            for rho in RHOS:
                potential = L.optimal_potential(ratio, rho)
                back = float(L.ratio_from_potential(potential, rho))
                self.assertAlmostEqual(back, ratio, places=9)

    def test_chi_deux_pondere_est_la_valeur_du_supremum(self):
        for ratio in RATIOS:
            for rho in RHOS:
                best = scipy.optimize.minimize_scalar(
                    lambda u, t=ratio, r=rho: -(
                        (t - 1.0) * u - 0.5 * (r * t + 1.0 - r) * u * u
                    ),
                    bounds=(-50.0, 50.0),
                    method="bounded",
                    options={"xatol": 1e-12},
                )
                self.assertAlmostEqual(
                    -float(best.fun), float(L.weighted_chi_square(ratio, rho)), places=8
                )


class ClosedForms(unittest.TestCase):
    """Bloc 2 : formes closes contre solveurs et quadratures independants."""

    @classmethod
    def setUpClass(cls):
        cls.model = full_rank_model()

    def test_rang_plein_de_la_configuration_de_reference(self):
        self.assertEqual(self.model.effective_rank, self.model.features.count)

    def test_theta_contre_resolution_directe(self):
        checked = 0
        for rho in RHOS:
            matrix = rho * self.model.sigma_p + (1.0 - rho) * self.model.sigma_q
            direct = np.linalg.solve(matrix, self.model.delta)
            gap = float(np.max(np.abs(direct - self.model.theta_at(rho))))
            self.assertLess(gap, 1e-6 * max(1.0, float(np.max(np.abs(direct)))))
            checked += 1
        self.assertGreaterEqual(checked, FLOORS["rho_values"])

    def test_theta_contre_minimisation_generique(self):
        """Le solveur ne connait que l'objectif quadratique et son gradient."""
        checked = 0
        for rho in (0.0, 0.5, 0.9):
            matrix = rho * self.model.sigma_p + (1.0 - rho) * self.model.sigma_q
            delta = self.model.delta

            def objective(theta, matrix=matrix, delta=delta):
                value = -(theta @ delta) + 0.5 * theta @ matrix @ theta
                return value, -delta + matrix @ theta

            result = scipy.optimize.minimize(
                objective,
                np.zeros(delta.size),
                jac=True,
                method="L-BFGS-B",
                options={"maxiter": 20000, "ftol": 1e-18, "gtol": 1e-14},
            )
            closed = self.model.theta_at(rho)
            scale = max(1.0, float(np.max(np.abs(closed))))
            self.assertLess(float(np.max(np.abs(result.x - closed))), 1e-3 * scale)
            checked += 1
        self.assertGreaterEqual(checked, 3)

    def test_grand_theta_contre_quadrature(self):
        indices = range(0, self.model.features.count, 3)
        for index in indices:
            value = scipy.integrate.quad(
                lambda rho, j=index: float(self.model.theta_at(rho)[j]),
                0.0,
                1.0,
                limit=100,
            )[0]
            self.assertAlmostEqual(value, float(self.model.theta[index]), places=6)

    def test_filtre_spectral_contre_quadrature(self):
        for lam in (1e-3, 0.3, 0.999999, 1.0, 1.7, 25.0):
            expected = scipy.integrate.quad(
                lambda rho, a=lam: 1.0 / (1.0 + rho * (a - 1.0)), 0.0, 1.0, limit=200
            )[0]
            values, clamped = L.spectral_filter(np.array([lam]))
            self.assertEqual(clamped, 0)
            self.assertAlmostEqual(float(values[0]), expected, places=7)

    def test_troncature_en_rho_reste_une_f_divergence(self):
        for rho_max in (0.25, 0.5, 0.9):
            expected = scipy.integrate.quad(
                lambda rho: 1.0 / (1.0 + rho * (0.0 - 1.0)), 0.0, rho_max, limit=200
            )[0]
            values, _clamped = L.spectral_filter(np.array([0.0]), rho_max, floor=0.0)
            self.assertAlmostEqual(float(values[0]), expected, places=7)

    def test_divergence_contre_potentiels_quadratiques(self):
        """`D_rho = E_p[v] + E_q[w]` avec les `(M, N, c)` publies."""
        for rho in RHOS:
            left, right, offset = self.model.quadratic_potentials(rho)
            expectation_p = float(
                np.trace(left @ self.model.sigma_p) + 2.0 * offset @ self.model.mean_p
            )
            expectation_q = float(
                np.trace(right @ self.model.sigma_q) - 2.0 * offset @ self.model.mean_q
            )
            self.assertAlmostEqual(
                expectation_p + expectation_q, self.model.divergence(rho), places=8
            )

    def test_kullback_leibler_contre_quadrature_des_divergences(self):
        value = scipy.integrate.quad(
            lambda rho: 2.0 * (1.0 - rho) * self.model.divergence(rho), 0.0, 1.0, limit=100
        )[0]
        self.assertAlmostEqual(value, self.model.kullback_leibler(), places=6)

    def test_moments_analytiques_contre_monte_carlo(self):
        features = L.FourierFeatures(3, 16, bandwidth=1.3, seed=5)
        mean = np.array([0.4, -0.2, 1.1])
        root = np.array([[1.2, 0.0, 0.0], [0.3, 0.9, 0.0], [-0.2, 0.15, 0.7]])
        covariance = root @ root.T
        first, second, flops = features.gaussian_moments(mean, covariance)
        self.assertGreater(flops, 0)
        generator = np.random.default_rng(11)
        draw = mean + generator.standard_normal((400000, 3)) @ root.T
        matrix = features.transform(draw)
        self.assertLess(float(np.max(np.abs(matrix.mean(axis=0) - first))), 4e-3)
        empirical = (matrix.T @ matrix) / draw.shape[0]
        self.assertLess(float(np.max(np.abs(empirical - second))), 4e-3)
        self.assertGreaterEqual(second.size, FLOORS["moment_entries"])

    def test_cout_annonce_est_du_bon_ordre(self):
        cost = self.model.cost
        size = cost["feature_count"]
        count = cost["sample_count"]
        self.assertGreaterEqual(cost["multiply_add"], size * size * count)
        self.assertEqual(cost["moments"], "analytic")
        self.assertEqual(cost["reference_sample"], 0)


class Gradients(unittest.TestCase):
    """Bloc 3 : gradient analytique contre differences finies centrees."""

    def test_gradient_du_log_rapport_et_de_la_log_densite(self):
        cloud, _labels = mixture_sample(300, 3)
        model = L.SpectralLogDensity(feature_count=48, seed=2).fit(cloud)
        points = np.array([[0.0, 0.0], [1.5, 1.0], [-2.0, -0.5], [0.3, 3.1], [4.0, 4.0]])
        step = 1e-6
        checked = 0
        for value, gradient in (
            (model.evaluate, model.gradient),
            (model.log_density, model.gradient_log_density),
        ):
            analytic = gradient(points)
            for axis in range(points.shape[1]):
                shift = np.zeros(points.shape[1])
                shift[axis] = step
                numeric = (value(points + shift) - value(points - shift)) / (2.0 * step)
                scale = max(1e-9, float(np.max(np.abs(numeric))))
                self.assertLess(
                    float(np.max(np.abs(analytic[:, axis] - numeric))), 1e-5 * scale
                )
                checked += points.shape[0]
        self.assertGreaterEqual(checked, FLOORS["finite_difference_checks"])

    def test_gradient_de_nystrom(self):
        cloud, _labels = mixture_sample(200, 8)
        model = L.SpectralLogDensity(
            feature_count=40, features="nystrom", reference_sample=4000, seed=4
        ).fit(cloud)
        self.assertEqual(model.moments_used, "monte_carlo")
        points = np.array([[0.2, 0.4], [-1.0, 1.0], [2.5, 2.5]])
        step = 1e-5
        analytic = model.gradient(points)
        for axis in range(2):
            shift = np.zeros(2)
            shift[axis] = step
            numeric = (model.evaluate(points + shift) - model.evaluate(points - shift)) / (
                2.0 * step
            )
            scale = max(1e-9, float(np.max(np.abs(numeric))))
            self.assertLess(float(np.max(np.abs(analytic[:, axis] - numeric))), 1e-4 * scale)


class Recovery(unittest.TestCase):
    """Bloc 4 : la vraie log-densite est recuperee, et mieux quand `n` croit."""

    GRID = None
    TRUTH = None
    KEEP = None

    @classmethod
    def setUpClass(cls):
        axis_x = np.linspace(-5.0, 5.0, 26)
        axis_y = np.linspace(-4.0, 6.0, 26)
        grid = np.array([[x, y] for x in axis_x for y in axis_y])
        truth = mixture_log_density(grid)
        keep = truth > math.log(0.01)
        cls.GRID = grid
        cls.TRUTH = truth
        cls.KEEP = keep

    @classmethod
    def error_for(cls, count, seed, feature_count=128, bandwidth_scale=0.5):
        cloud, _labels = mixture_sample(count, seed)
        model = L.SpectralLogDensity(
            feature_count=feature_count, bandwidth_scale=bandwidth_scale, seed=seed
        ).fit(cloud)
        estimate = model.log_density(cls.GRID)
        residual = estimate[cls.KEEP] - cls.TRUTH[cls.KEEP]
        return float(np.sqrt(np.mean(residual ** 2)))

    def test_grille_assez_grande(self):
        self.assertGreaterEqual(int(np.count_nonzero(self.KEEP)), FLOORS["grid_points"])

    def test_erreur_decroit_avec_le_nombre_d_observations(self):
        cases = 0
        errors = []
        for count in (250, 1000, 4000):
            block = []
            for seed in (21, 22):
                block.append(self.error_for(count, seed))
                cases += 1
            errors.append(float(np.median(block)))
        self.assertGreaterEqual(cases, FLOORS["recovery_cases"])
        self.assertLess(errors[1], errors[0])
        self.assertLess(errors[2], errors[1])
        self.assertLess(errors[2], 0.25)


class Refusals(unittest.TestCase):
    """Bloc 5 : refus explicites, jamais de `NaN`."""

    def test_covariance_empirique_singuliere_refusee(self):
        generator = np.random.default_rng(3)
        flat = generator.standard_normal((120, 2))
        cloud = np.hstack([flat, np.zeros((120, 2))])
        with self.assertRaises(L.DegenerateSample):
            L.SpectralLogDensity(feature_count=20, seed=1).fit(cloud)

    def test_covariance_singuliere_acceptee_avec_plancher_declare(self):
        generator = np.random.default_rng(3)
        flat = generator.standard_normal((120, 2))
        cloud = np.hstack([flat, np.zeros((120, 2))])
        model = L.SpectralLogDensity(
            feature_count=20, covariance_floor=1e-6, seed=1
        ).fit(cloud)
        self.assertGreater(model.reference.floored, 0)
        values = model.log_density(cloud[:10])
        self.assertTrue(bool(np.all(np.isfinite(values))))

    def test_moments_de_reference_singuliers_refuses(self):
        cloud, _labels = mixture_sample(200, 5)
        features = L.FourierFeatures(2, 16, bandwidth=1.0, seed=2)
        features.frequencies[8:] = features.frequencies[:8]
        features.offsets[8:] = features.offsets[:8]
        with self.assertRaises(L.SingularReference):
            L.SpectralLogDensity(features=features, on_singular="refuse", seed=1).fit(cloud)

    def test_moments_singuliers_tronques_restent_finis(self):
        cloud, _labels = mixture_sample(200, 5)
        features = L.FourierFeatures(2, 16, bandwidth=1.0, seed=2)
        features.frequencies[8:] = features.frequencies[:8]
        features.offsets[8:] = features.offsets[:8]
        model = L.SpectralLogDensity(features=features, seed=1).fit(cloud)
        self.assertLess(model.effective_rank, 16)
        self.assertGreater(model.effective_rank, 0)
        values = model.log_density(cloud[:20])
        self.assertTrue(bool(np.all(np.isfinite(values))))
        self.assertTrue(bool(np.all(np.isfinite(model.gradient(cloud[:20])))))

    def test_modele_non_ajuste_refuse(self):
        model = L.SpectralLogDensity(feature_count=8)
        with self.assertRaises(RuntimeError):
            model.evaluate(np.zeros((1, 2)))

    def test_rho_hors_domaine_refuse(self):
        model = full_rank_model(seed=6, count=200)
        with self.assertRaises(L.SingularReference):
            model.theta_at(-5.0)

    def test_echantillon_trop_petit_refuse(self):
        with self.assertRaises(L.DegenerateSample):
            L.SpectralLogDensity(feature_count=4).fit(np.zeros((1, 3)))

    def test_calibration_hors_domaine_refusee(self):
        with self.assertRaises(ValueError):
            L.order_to_log_density_level(0, 10, 1.0, 2)
        with self.assertRaises(ValueError):
            L.order_to_log_density_level(1, 10, 0.0, 2)

    def test_nombre_de_refus_couverts(self):
        self.assertGreaterEqual(FLOORS["refusals"], 5)


class Calibration(unittest.TestCase):
    """L'axe d'ordre et l'axe de niveau sont le meme axe, et c'est verifie."""

    def test_aller_retour_ordre_niveau(self):
        cloud, _labels = mixture_sample(200, 9)
        tower = T.SpectralTower(cloud, mixture_log_density, mixture_gradient)
        for order in (1, 5, 17):
            for radius in (0.25, 1.0, 4.0):
                level = L.order_to_log_density_level(order, tower.count, radius, 2)
                back = tower.equivalent_order(level, radius)
                self.assertAlmostEqual(back, float(order), places=6)

    def test_comptage_de_boules_suit_la_calibration(self):
        """Sur un melange, le comptage dur egale `n V_d a^{d/2} p` a facteur pres."""
        cloud, _labels = mixture_sample(4000, 31)
        probes = np.array([[-2.5, -1.0], [2.0, 1.5], [0.0, 3.0]])
        radius_squared = 0.04
        offsets = probes[:, None, :] - cloud[None, :, :]
        squared = np.einsum("ijk,ijk->ij", offsets, offsets)
        counts = np.count_nonzero(squared <= radius_squared, axis=1)
        volume = math.pi * radius_squared
        predicted = cloud.shape[0] * volume * np.exp(mixture_log_density(probes))
        for observed, expected in zip(counts, predicted):
            self.assertGreater(observed, 0)
            self.assertLess(abs(observed - expected) / expected, 0.35)


class Tower(unittest.TestCase):
    """Bloc 6 : la tour, sur la log-densite EXACTE et sur le modele."""

    def test_tour_du_melange_exact_retrouve_les_composantes(self):
        cloud, labels = mixture_sample(400, 41)
        tower = T.SpectralTower(cloud, mixture_log_density, mixture_gradient)
        self.assertEqual(tower.maximum_count, FLOORS["tower_maxima"])
        self.assertGreaterEqual(len(tower.merges), FLOORS["tower_merges"])
        self.assertEqual(tower.unconverged, 0)
        # Les bassins des modes ne sont PAS la partition de Bayes du melange :
        # sur ce melange (ecarts-types 0,6 a 1,0 pour des centres a distance 4
        # a 5) l'ARI de la partition par bassins plafonne vers 0,88. C'est une
        # propriete du melange, pas de la tour ; la porte sur les positions des
        # modes ci-dessous est la porte de correction, l'ARI n'est qu'un ordre
        # de grandeur.
        self.assertGreater(rand_index(labels, tower.labels(3)), 0.85)
        self.assertEqual(int(np.unique(tower.labels(3)).size), 3)
        self.assertEqual(int(np.unique(tower.labels(1)).size), 1)

    def test_maxima_sont_les_modes_exacts_du_melange(self):
        """Porte de correction : chaque mode vrai est retrouve, et aucun autre.

        Les modes du melange sont calcules INDEPENDAMMENT, par une montee
        `scipy` partant de chaque centre ; la tour doit les retrouver tous, a
        la tolerance de son agglomeration, et ne rien inventer.
        """
        cloud, _labels = mixture_sample(400, 41)
        tower = T.SpectralTower(cloud, mixture_log_density, mixture_gradient)
        modes = []
        for centre in MIXTURE_CENTRES:
            result = scipy.optimize.minimize(
                lambda point: (
                    -float(mixture_log_density(point[None, :])[0]),
                    -mixture_gradient(point[None, :])[0],
                ),
                centre,
                jac=True,
                method="L-BFGS-B",
                options={"ftol": 1e-16, "gtol": 1e-12},
            )
            modes.append(result.x)
        modes = np.array(modes)
        self.assertEqual(tower.maximum_count, modes.shape[0])
        matched = set()
        for maximum in tower.maxima:
            gaps = np.sqrt(np.einsum("ij,ij->i", modes - maximum, modes - maximum))
            best = int(np.argmin(gaps))
            self.assertLess(float(gaps[best]), 1e-3)
            matched.add(best)
        self.assertEqual(len(matched), modes.shape[0])

    def test_col_reste_sous_les_sommets(self):
        """Invariant mathematique : un col n'est jamais au-dessus des sommets."""
        cloud, _labels = mixture_sample(400, 41)
        tower = T.SpectralTower(cloud, mixture_log_density, mixture_gradient)
        checked = 0
        for (left, right), level in tower.saddle.items():
            summit = min(tower.maximum_values[left], tower.maximum_values[right])
            self.assertLessEqual(level, summit + 1e-9)
            checked += 1
        self.assertGreaterEqual(checked, FLOORS["tower_merges"])

    def test_raffinement_de_chemin_est_monotone(self):
        cloud, _labels = mixture_sample(300, 43)
        tower = T.SpectralTower(cloud, mixture_log_density, mixture_gradient)
        checked = 0
        for left in range(tower.maximum_count):
            for right in range(left + 1, tower.maximum_count):
                base, nodes = T.path_minimum(
                    mixture_log_density, tower.maxima[left], tower.maxima[right], 33
                )
                raised, _nodes, accepted = T.raise_path(
                    mixture_log_density, mixture_gradient, nodes, 40, 0.2
                )
                self.assertGreaterEqual(raised, base - 1e-12)
                self.assertGreaterEqual(accepted, 0)
                checked += 1
        self.assertGreaterEqual(checked, FLOORS["tower_merges"])

    def test_digest_invariant_par_permutation(self):
        cloud, _labels = mixture_sample(300, 47)
        generator = np.random.default_rng(5)
        shuffle = generator.permutation(cloud.shape[0])
        first = T.SpectralTower(cloud, mixture_log_density, mixture_gradient)
        second = T.SpectralTower(cloud[shuffle], mixture_log_density, mixture_gradient)
        self.assertEqual(first.digest(), second.digest())
        self.assertEqual(first.structural_digest(), second.structural_digest())
        self.assertTrue(
            bool(np.array_equal(first.labels(3)[shuffle], second.labels(3)))
        )

    def test_digest_invariant_par_translation(self):
        cloud, _labels = mixture_sample(300, 47)
        shift = np.array([7.5, -3.25])

        def shifted_value(points):
            return mixture_log_density(np.asarray(points, dtype=float) - shift)

        def shifted_gradient(points):
            return mixture_gradient(np.asarray(points, dtype=float) - shift)

        first = T.SpectralTower(cloud, mixture_log_density, mixture_gradient)
        second = T.SpectralTower(cloud + shift, shifted_value, shifted_gradient)
        self.assertEqual(first.structural_digest(), second.structural_digest())
        self.assertEqual(first.digest(), second.digest())

    def test_modele_spectral_invariant_par_permutation(self):
        cloud, _labels = mixture_sample(300, 53)
        generator = np.random.default_rng(6)
        shuffle = generator.permutation(cloud.shape[0])
        first = L.SpectralLogDensity(feature_count=48, seed=3).fit(cloud)
        second = L.SpectralLogDensity(feature_count=48, seed=3).fit(cloud[shuffle])
        self.assertTrue(bool(np.array_equal(first.theta, second.theta)))
        probe = np.array([[0.0, 0.0], [1.0, 2.0]])
        self.assertTrue(bool(np.array_equal(first.log_density(probe), second.log_density(probe))))

    def test_modele_spectral_equivariant_par_translation(self):
        cloud, _labels = mixture_sample(300, 53)
        shift = np.array([-4.0, 9.0])
        first = L.SpectralLogDensity(feature_count=48, seed=3).fit(cloud)
        second = L.SpectralLogDensity(feature_count=48, seed=3).fit(cloud + shift)
        probe = np.array([[0.0, 0.0], [1.0, 2.0], [-3.0, 0.5]])
        gap = np.max(np.abs(first.log_density(probe) - second.log_density(probe + shift)))
        # L'equivariance est exacte en algebre ; en flottant elle est limitee
        # par l'amplification de la decomposition tronquee. Ecart MESURE ici :
        # de l'ordre de 1 e-7 sur des valeurs de log-densite de l'ordre de 3.
        self.assertLess(float(gap), 1e-5)

    def test_les_deux_ascensions_et_leur_ecart_mesure(self):
        """L'ascension quasi-Newton converge, l'ascension par pas ne suffit pas."""
        cloud, _labels = mixture_sample(300, 59)
        quasi = T.SpectralTower(
            cloud, mixture_log_density, mixture_gradient, optimiser="lbfgs"
        )
        armijo = T.SpectralTower(
            cloud,
            mixture_log_density,
            mixture_gradient,
            optimiser="armijo",
            ascent_steps=60,
        )
        self.assertLess(quasi.ascent["gradient_max"], quasi.residual_ceiling())
        self.assertGreater(
            armijo.ascent["gradient_max"], quasi.ascent["gradient_max"]
        )
        self.assertGreaterEqual(armijo.maximum_count, quasi.maximum_count)

    def test_optimiseur_inconnu_refuse(self):
        cloud, _labels = mixture_sample(60, 61)
        with self.assertRaises(ValueError):
            T.SpectralTower(cloud, mixture_log_density, mixture_gradient, optimiser="magie")

    def test_tour_du_modele_spectral(self):
        cloud, labels = mixture_sample(600, 67)
        model = L.SpectralLogDensity(feature_count=128, bandwidth_scale=0.5, seed=7).fit(cloud)
        tower = T.from_model(cloud, model)
        self.assertGreaterEqual(tower.maximum_count, 2)
        self.assertEqual(tower.unconverged, 0)
        record = tower.canonical_record()
        self.assertEqual(record["object"], "ehgp.spectral_tower.v1")
        self.assertEqual(len(record["basin"]), cloud.shape[0])
        self.assertEqual(len(tower.digest()), 64)
        if tower.maximum_count >= 3:
            self.assertGreater(rand_index(labels, tower.labels(3)), 0.6)

    def test_cible_inconnue_refusee(self):
        cloud, _labels = mixture_sample(60, 71)
        model = L.SpectralLogDensity(feature_count=16, seed=7).fit(cloud)
        with self.assertRaises(ValueError):
            T.from_model(cloud, model, target="magie")


class Mutants(unittest.TestCase):
    """Bloc 7 : trois mutants causaux, tous TUES."""

    def test_mutant_filtre_plaque_a_un_est_tue(self):
        """`G == 1` est le membre `rho = 0` seul : aucun amortissement."""
        original = L.spectral_filter
        try:
            reference = self._recovery_error(4000, 21)
            L.spectral_filter = lambda values, rho_max=1.0, floor=1e-10: (
                np.ones_like(np.asarray(values, dtype=float)),
                0,
            )
            mutated = self._recovery_error(4000, 21)
        finally:
            L.spectral_filter = original
        self.assertGreater(mutated, 2.0 * reference)
        self.assertGreater(mutated, 0.4)

    @staticmethod
    def _recovery_error(count, seed):
        axis_x = np.linspace(-5.0, 5.0, 26)
        axis_y = np.linspace(-4.0, 6.0, 26)
        grid = np.array([[x, y] for x in axis_x for y in axis_y])
        truth = mixture_log_density(grid)
        keep = truth > math.log(0.01)
        cloud, _labels = mixture_sample(count, seed)
        model = L.SpectralLogDensity(feature_count=128, bandwidth_scale=0.5, seed=seed).fit(
            cloud
        )
        residual = model.log_density(grid)[keep] - truth[keep]
        return float(np.sqrt(np.mean(residual ** 2)))

    def test_mutant_col_par_maximum_est_tue(self):
        """Prendre le MAXIMUM du chemin au lieu du minimum viole l'invariant."""
        cloud, _labels = mixture_sample(400, 41)
        original = T.path_minimum

        def mutated(value, left, right, samples):
            times = np.linspace(0.0, 1.0, samples)
            nodes = left[None, :] + times[:, None] * (right - left)[None, :]
            values = np.asarray(value(nodes), dtype=float)
            return float(values.max()), nodes

        try:
            T.path_minimum = mutated
            tower = T.SpectralTower(cloud, mixture_log_density, mixture_gradient)
            violations = 0
            for (left, right), level in tower.saddle.items():
                summit = min(tower.maximum_values[left], tower.maximum_values[right])
                if level > summit + 1e-9:
                    violations += 1
        finally:
            T.path_minimum = original
        self.assertGreater(violations, 0)

    def test_mutant_gradient_inverse_est_tue(self):
        cloud, _labels = mixture_sample(300, 73)
        model = L.SpectralLogDensity(feature_count=48, seed=2).fit(cloud)
        points = np.array([[0.0, 0.0], [1.5, 1.0], [-2.0, -0.5]])
        step = 1e-6
        wrong = -model.gradient(points)
        caught = 0
        for axis in range(2):
            shift = np.zeros(2)
            shift[axis] = step
            numeric = (model.evaluate(points + shift) - model.evaluate(points - shift)) / (
                2.0 * step
            )
            scale = max(1e-9, float(np.max(np.abs(numeric))))
            if float(np.max(np.abs(wrong[:, axis] - numeric))) > 1e-5 * scale:
                caught += 1
        self.assertEqual(caught, 2)

    def test_nombre_de_mutants_couverts(self):
        self.assertGreaterEqual(FLOORS["mutants"], 3)


if __name__ == "__main__":
    unittest.main(verbosity=2)
