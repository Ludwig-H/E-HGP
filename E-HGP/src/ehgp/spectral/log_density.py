"""Estimateur spectral du log-rapport de densites, libre en dimension.

Ce module implemente la couche STATISTIQUE de E-HGP : il remplace le
comptage empirique de boules (inutilisable en grande dimension, cf.
`docs/OBSTRUCTION_GRANDE_DIMENSION.md`) par un modele de log-densite
regularise, evaluable PARTOUT dans `R^d`, avec son gradient analytique.

La source est le billet de Francis Bach « Spectral log-density estimation »
(https://francisbach.com/spectral_log_density_estimation/). Tout ce qui
suit est redemontre ici, parce que la chaine doit etre verifiable sans le
billet.

== 1. REPRESENTATION VARIATIONNELLE DES f-DIVERGENCES ==

Pour `f` convexe avec `f(1) = 0`, la f-divergence est
`D_f(p||q) = integrale de f(dp/dq) dq`, et la dualite de Fenchel donne

    D_f(p||q) = sup_v { E_p[v] - E_q[f*(v)] } ,

le supremum portant sur les fonctions mesurables `v` (le potentiel).
Sous la forme a deux potentiels, `D_f = sup { E_p[v] + E_q[w] }` sous la
contrainte ponctuelle `w(x) <= -f*(v(x))`.

== 2. LA FAMILLE DE CHI-DEUX PONDERES REND LE PROBLEME QUADRATIQUE ==

Pour `rho` dans `[0, 1]`,

    f_rho(t) = (1/2) (t - 1)^2 / (rho t + 1 - rho) ,

avec `rho = 0` Pearson, `rho = 1/2` Le Cam, `rho = 1` Neyman. Son point
cle est l'identite ponctuelle « quadratique plus affine »

    f_rho(t) = sup_u { (t - 1) u - (1/2) (rho t + 1 - rho) u^2 } ,

dont l'optimum est `u = (t - 1) / (rho t + 1 - rho)` (deriver en `u`, la
parabole est concave). En integrant contre `q` et en notant `t = dp/dq` :

    D_rho(p||q)
      = sup_u { E_p[u] - E_q[u] - (rho/2) E_p[u^2] - ((1-rho)/2) E_q[u^2] } ,

objectif QUADRATIQUE CONCAVE en `u`. Avec un modele lineaire
`u(x) = theta^T phi(x)` pour un plongement `phi : R^d -> R^m`, en posant

    mu_p = E_p[phi] , mu_q = E_q[phi] ,
    Sigma_p = E_p[phi phi^T] , Sigma_q = E_q[phi phi^T] ,

l'objectif devient `theta^T (mu_p - mu_q) - (1/2) theta^T S(rho) theta`
avec `S(rho) = rho Sigma_p + (1 - rho) Sigma_q`, dont l'unique maximum est

    theta(rho) = S(rho)^{-1} (mu_p - mu_q) ,                          (A)

et la valeur `(1/2) (mu_p - mu_q)^T S(rho)^{-1} (mu_p - mu_q)`.

== 3. LES DEUX IDENTITES INTEGRALES ==

FAIT 1 (Kullback-Leibler comme melange de chi-deux ponderes). Pour tout
`t > 0`,

    t log t - t + 1 = integrale de 0 a 1 de 2 (1 - rho) f_rho(t) d rho .

Preuve. Posons `s = t - 1`, donc `rho t + 1 - rho = 1 + rho s`. Comme
`1 - rho = ((1 + s) - (1 + rho s)) / s`, l'integrale vaut
`s^2 * (1/s) * [ (1+s) * (1/s) log(1+s) - 1 ]` soit
`(1+s) log(1+s) - s = t log t - t + 1`. CQFD.

FAIT 2 (le LOGARITHME est l'integrale des potentiels optimaux). Pour tout
`t > 0`,

    log t = integrale de 0 a 1 de u*(rho, t) d rho ,
    u*(rho, t) = (t - 1) / (rho t + 1 - rho) .

Preuve. `integrale de 0 a 1 de s / (1 + rho s) d rho = log(1 + s)`. CQFD.

Le fait 2 est le pivot de ce module : le log-rapport de densites est
l'INTEGRALE EN rho des potentiels optimaux de la famille quadratique, et
chacun de ces potentiels a la forme close (A). Donc, avec le modele
lineaire,

    log (dp/dq) (x) ~ Theta^T phi(x) ,
    Theta = integrale de 0 a 1 de theta(rho) d rho .                  (B)

== 4. LA FORME CLOSE SPECTRALE ==

Soit la decomposition en valeurs propres GENERALISEE du couple
`(Sigma_p, Sigma_q)` : une base `v_1, ..., v_m` de `R^m` avec

    v_i^T Sigma_q v_j = 1 si i = j et 0 sinon ,
    Sigma_p v_i = lambda_i Sigma_q v_i .

Alors `S(rho) = rho Sigma_p + (1-rho) Sigma_q` se diagonalise dans cette
base et `S(rho)^{-1} = somme_i v_i v_i^T / (1 + rho (lambda_i - 1))`.
En integrant (B) terme a terme :

    Theta = somme_i G(lambda_i) (v_i^T delta) v_i ,  delta = mu_p - mu_q ,
    G(lambda) = integrale de 0 a rho_max de d rho / (1 + rho (lambda - 1))
              = log(1 + rho_max (lambda - 1)) / (lambda - 1) ,
    G(1) = rho_max .                                                  (C)

Pour `rho_max = 1` on retrouve le FILTRE SPECTRAL `G(lambda) = log(lambda)
/ (lambda - 1)`, d'ou le nom « estimation spectrale de la log-densite ».
UNE SEULE decomposition generalisee donne donc tout : les `theta(rho)`,
les divergences, et l'estimateur du log-rapport.

C'est exactement la regularisation demandee au niveau de la CLASSE DE
FONCTIONS, et non au niveau de l'axe des niveaux : `G(lambda)` decroit
comme `log(lambda)/lambda`, donc les directions ou `Sigma_p` domine
`Sigma_q` (les directions de variance empirique elevee, celles qui
surajustent) sont AMORTIES, et non pas seulement translatees. Le membre
`rho = 0` seul (chi-deux de Pearson) correspondrait a `G == 1`, c'est-a-dire
a AUCUN amortissement : c'est le mutant tue par `tests/test_spectral.py`.

La divergence se lit dans la meme base (formule du billet) :

    F(p||q, phi) = somme_i (v_i^T delta)^2 f(lambda_i) / (lambda_i - 1)^2 ,

qui redonne bien `(1/2) delta^T S(rho)^{-1} delta` pour `f = f_rho`.

Potentiels quadratiques. En separant l'identite du paragraphe 2 selon `t`,
`(t-1)u - (1/2)(rho t + 1 - rho) u^2 = t [u - (rho/2) u^2] + [-u -
((1-rho)/2) u^2]`, donc `D_rho = E_p[v] + E_q[w]` avec

    v(x) = phi(x)^T M phi(x) + 2 c^T phi(x) , M = -(rho/2) theta theta^T ,
    w(x) = phi(x)^T N phi(x) - 2 c^T phi(x) , N = -((1-rho)/2) theta theta^T ,
    c = theta / 2 .

== 5. CE QUI EST EXACT, CE QUI EST ESTIME ==

EXACT (algebre lineaire, aux erreurs d'arrondi flottantes pres) : la forme
close (A), la forme close spectrale (C), les moments `mu_q` et `Sigma_q`
de la mesure de reference quand elle est gaussienne et le plongement de
Fourier (formules closes du paragraphe 6), les potentiels `v`, `w`, le
gradient analytique.

ESTIME : `mu_p` et `Sigma_p`, moyennes empiriques sur l'echantillon
(erreur `O(n^{-1/2})`) ; le log-rapport lui-meme, deux fois — d'abord par
le biais d'approximation (le vrai `log dp/dq` n'est pas dans l'espace
engendre par `phi`), ensuite par la variance statistique. AUCUN resultat
de ce module n'est un certificat, et rien ici ne promeut un statut public.

REGULARISATIONS DECLAREES, toutes comptees et publiees, jamais silencieuses :
la TRONCATURE DE RANG de `Sigma_q` (`effective_rank`, paragraphe de
`_decompose`), le PLANCHER sur les valeurs propres generalisees
(`clamped_directions`) ou la troncature `rho_max < 1` qui le remplace
proprement, le PLANCHER de covariance de la reference (`floored`,
`GaussianReference`), et le `ridge` optionnel. Chacune change l'objet
estime ; aucune n'est un detail numerique.

EQUIVARIANCES. Le plongement et la reference travaillent dans les
coordonnees CENTREES sur la moyenne de l'echantillon, donc le modele est
equivariant par translation ; les observations sont triees par ordre
lexicographique avant toute somme, donc le modele est invariant par
permutation de facon EXACTE (bit a bit), et non a une tolerance pres. En
flottant, l'equivariance par translation est limitee par l'amplification de
la decomposition tronquee : ecart MESURE de l'ordre de `1 e-7` sur des
log-densites de l'ordre de `3`.

Role de la dimension `d` dans les garanties, honnetement. La VARIANCE est
libre en dimension : elle se gouverne par `m` et `n` (regime `m/n` fixe).
Le COUT ne l'est pas, contrairement a ce qu'un raccourci en `O(m^2 n + m^3)`
laisserait croire : l'ajustement compte

    O(n d^2)      covariance empirique de l'echantillon ,
    O(n m d)      plongement des observations ,
    O(m^2 d)      moments analytiques de la reference (paragraphe 6) ,
    O(m^2 n)      moments empiriques `Sigma_p` ,
    O(m^3)        decomposition generalisee ,

et c'est bien cette somme que publie `self.cost["multiply_add"]`. MESURE
(temps CPU du processus, BLAS monofil, meilleur de trois) : a `m = 128`
fixe et `n = 2000`, doubler `d` de 25 a 400 multiplie le temps par `2^1,06`
au dernier doublement, donc le cout est LINEAIRE en `d` et non independant
de `d` ; a `d = 50` et `m = 128`, doubler `n` de 500 a 8000 donne une pente
`1,00` (lineaire en `n`) ; a `d = 50` et `n = 2000`, doubler `m` de 32 a 512
donne une pente `1,86` (entre `m^2` et `m^3`, conforme a `m^2 n` dominant).

Le BIAIS, lui, reste entierement dimensionnel : approcher un element de la
boule du RKHS gaussien a precision `eta` demande en general `m` de l'ordre
de `eta^{-d}` descripteurs, et la largeur de bande de l'heuristique de la
mediane croit comme `sqrt(d)`. Ce que ce module obtient est donc precis :
la dimension entre dans le cout POLYNOMIALEMENT (`n d^2 + n m d`) et jamais
par un `C(n, k)`, elle sort de la VARIANCE, et elle reste entiere dans le
BIAIS D'APPROXIMATION. C'est un deplacement, pas une victoire.

== 6. MOMENTS ANALYTIQUES DE LA REFERENCE GAUSSIENNE ==

Plongement de Fourier : `phi_j(x) = sqrt(2/m) cos(w_j^T x + b_j)`, avec
`w_j` gaussiennes de covariance `bandwidth^{-2} I` et `b_j` uniformes sur
`[0, 2 pi)`, de sorte que `phi(x)^T phi(y)` approche le noyau gaussien
`exp(- ||x - y||^2 / (2 bandwidth^2))`.

Pour `x` de loi `N(m_0, C)`, `E[exp(i w^T x)] = exp(i w^T m_0 - w^T C w/2)`
donne

    E[cos(w^T x + b)] = exp(- w^T C w / 2) cos(w^T m_0 + b) =: h(w, b) ,

et `cos(a) cos(b) = (cos(a - b) + cos(a + b)) / 2` donne

    mu_q[j] = sqrt(2/m) h(w_j, b_j) ,
    Sigma_q[j,k] = (1/m) ( h(w_j - w_k, b_j - b_k) + h(w_j + w_k, b_j + b_k) ) .

Les `m^2` formes quadratiques se calculent en `O(m^2 d)` par la matrice de
Gram de `W L` ou `C = L L^T`. Aucun Monte-Carlo n'intervient.

== 7. LA MESURE DE REFERENCE `q` ==

Defaut : GAUSSIENNE AJUSTEE AUX MOMENTS de l'echantillon, dans les
coordonnees centrees (donc de moyenne nulle et de covariance la covariance
empirique, eventuellement dilatee par `inflation` et relevee par
`covariance_floor`). Quatre raisons, toutes utilisees ailleurs dans le
chantier :

1. c'est la loi d'entropie MAXIMALE a moments d'ordre deux donnes, donc
   `log(dp/dq)` est exactement la PART NON GAUSSIENNE de la log-densite :
   la multimodalite, c'est-a-dire precisement le signal que la tour cherche ;
2. son support est `R^d` tout entier, donc `dp/dq` est defini partout et la
   tour peut etre evaluee hors du nuage, ce qu'exige la recherche de cols ;
3. sa log-densite ET son gradient sont en forme close, donc
   `log p = log(dp/dq) + log q` est evaluable et differentiable partout ;
4. ses moments de Fourier sont analytiques (paragraphe 6).

Alternative fournie : `uniform_box`, uniforme sur la boite englobante
dilatee, avec moments par Monte-Carlo (taille d'echantillon declaree). Elle
rend `log q` constant a l'interieur, donc la tour de `log p` et celle du
log-rapport y coincident ; en revanche `dp/dq` n'est pas defini hors de la
boite et le gradient de `log q` y est nul, ce qui rend la recherche de cols
aveugle a l'exterieur. C'est pourquoi ce n'est pas le defaut.

Arithmetique. Ce module travaille en FLOTTANT : c'est un PROPOSEUR, comme
`soft/fermi.py`. Il ne certifie rien ; les decisions exactes du chantier
restent celles de `exact/` et de `engine/segment.py`.
"""

import math
import time

import numpy as np


class SingularReference(Exception):
    """Refus explicite : les moments de reference ne sont pas inversibles.

    Leve plutot que de renvoyer des `NaN` ou un resultat silencieusement
    faux. Le message porte le conditionnement mesure et le seuil viole.
    """


class DegenerateSample(Exception):
    """Refus explicite : l'echantillon ne permet pas d'ajuster la reference."""


def weighted_chi_square(ratio, rho):
    """`f_rho(t) = (1/2) (t-1)^2 / (rho t + 1 - rho)`, vectorise."""
    ratio = np.asarray(ratio, dtype=float)
    denominator = rho * ratio + (1.0 - rho)
    return 0.5 * (ratio - 1.0) ** 2 / denominator


def optimal_potential(ratio, rho):
    """Potentiel optimal ponctuel `u*(rho, t) = (t-1)/(rho t + 1 - rho)`."""
    ratio = np.asarray(ratio, dtype=float)
    return (ratio - 1.0) / (rho * ratio + (1.0 - rho))


def ratio_from_potential(potential, rho):
    """Inverse de `optimal_potential` : `t = (1 + (1-rho) u) / (1 - rho u)`."""
    potential = np.asarray(potential, dtype=float)
    return (1.0 + (1.0 - rho) * potential) / (1.0 - rho * potential)


def kullback_leibler_integrand(ratio, rho):
    """Integrande `2 (1 - rho) f_rho(t)` du fait 1."""
    return 2.0 * (1.0 - rho) * weighted_chi_square(ratio, rho)


def spectral_filter(eigenvalues, rho_max=1.0, floor=1e-10):
    """Filtre `G(lambda) = log(1 + rho_max (lambda - 1)) / (lambda - 1)`.

    Renvoie `(valeurs, nombre_de_valeurs_plaquees)`. Les valeurs propres
    generalisees sont positives (les deux matrices sont semi-definies
    positives) mais peuvent etre nulles, et l'arithmetique flottante en rend
    de legerement NEGATIVES (mesure : 33 des 146 directions retenues en
    dimension 2 avec `m = 256`). `G` diverge alors pour `rho_max = 1`, ce qui
    est la divergence VRAIE de l'integrale de Kullback-Leibler.

    Deux regularisations sont disponibles et declarees, et elles NE SE
    CUMULENT PAS, ce qui serait une regularisation silencieuse :

    * la projection exacte sur le cone semi-defini positif (`max(lambda, 0)`)
      est toujours appliquee : ce n'est pas un reglage, c'est la correction du
      seul artefact flottant ;
    * le plancher `floor` n'est applique QUE pour `rho_max >= 1`, la ou `G`
      diverge en zero. Pour `rho_max < 1` la troncature de l'integrale suffit
      a rendre `G` finie en zero (`1 + rho_max (lambda - 1) >= 1 - rho_max`),
      et c'est la plus honnete des deux puisqu'elle reste une f-divergence
      exacte (un melange de chi-deux ponderes sur `[0, rho_max]`).

    `clamped` compte les valeurs REELLEMENT modifiees, jamais les valeurs
    simplement petites.
    """
    eigenvalues = np.asarray(eigenvalues, dtype=float)
    safe = np.maximum(eigenvalues, 0.0)
    if rho_max >= 1.0:
        safe = np.maximum(safe, floor)
    clamped = int(np.count_nonzero(safe != eigenvalues))
    shifted = 1.0 + rho_max * (safe - 1.0)
    if np.any(shifted <= 0.0):
        raise SingularReference(
            "filtre spectral non defini : 1 + rho_max (lambda - 1) <= 0"
        )
    gap = safe - 1.0
    ratio = np.log(shifted) / np.where(gap == 0.0, 1.0, gap)
    values = np.where(np.abs(gap) < 1e-12, rho_max, ratio)
    return values, clamped


class FourierFeatures:
    """Descripteurs de Fourier aleatoires du noyau gaussien.

    `phi_j(x) = sqrt(2/m) cos(w_j^T x + b_j)` avec `w_j` de loi
    `N(0, bandwidth^{-2} I)`. Le produit `phi(x)^T phi(y)` est un
    estimateur sans biais de `exp(-||x - y||^2 / (2 bandwidth^2))`.
    """

    name = "fourier"

    def __init__(self, dimension, count, bandwidth, seed=0):
        if count < 1:
            raise ValueError("il faut au moins un descripteur")
        if not bandwidth > 0.0:
            raise ValueError("la largeur de bande doit etre strictement positive")
        generator = np.random.default_rng(seed)
        self.dimension = int(dimension)
        self.count = int(count)
        self.bandwidth = float(bandwidth)
        self.frequencies = generator.standard_normal((self.count, self.dimension)) / self.bandwidth
        self.offsets = generator.uniform(0.0, 2.0 * math.pi, size=self.count)
        self.scale = math.sqrt(2.0 / self.count)

    def transform(self, points):
        """Matrice `(N, m)` des descripteurs."""
        points = np.asarray(points, dtype=float)
        angles = points @ self.frequencies.T + self.offsets
        return self.scale * np.cos(angles)

    def directional(self, points, weights):
        """Gradient en `x` de `weights^T phi(x)`, matrice `(N, d)`.

        `d/dx somme_j theta_j sqrt(2/m) cos(w_j^T x + b_j)
        = - sqrt(2/m) somme_j theta_j sin(w_j^T x + b_j) w_j`.
        """
        points = np.asarray(points, dtype=float)
        angles = points @ self.frequencies.T + self.offsets
        return -self.scale * (np.sin(angles) * weights) @ self.frequencies

    def gaussian_moments(self, mean, covariance):
        """Moments analytiques sous `N(mean, covariance)` (paragraphe 6)."""
        mean = np.asarray(mean, dtype=float)
        covariance = np.asarray(covariance, dtype=float)
        root = np.linalg.cholesky(covariance)
        projected = self.frequencies @ root
        norms = np.einsum("ij,ij->i", projected, projected)
        gram = projected @ projected.T
        shifts = self.frequencies @ mean + self.offsets
        sum_norms = norms[:, None] + norms[None, :]
        minus_energy = sum_norms - 2.0 * gram
        plus_energy = sum_norms + 2.0 * gram
        minus_phase = shifts[:, None] - shifts[None, :]
        plus_phase = shifts[:, None] + shifts[None, :]
        first_moment = self.scale * np.exp(-0.5 * norms) * np.cos(shifts)
        second_moment = (1.0 / self.count) * (
            np.exp(-0.5 * np.maximum(minus_energy, 0.0)) * np.cos(minus_phase)
            + np.exp(-0.5 * np.maximum(plus_energy, 0.0)) * np.cos(plus_phase)
        )
        second_moment = 0.5 * (second_moment + second_moment.T)
        flops = self.count * self.count * self.dimension + 6 * self.count * self.count
        return first_moment, second_moment, flops


class NystromFeatures:
    """Descripteurs de Nystrom du noyau gaussien sur `m` points de reference.

    `phi(x) = K_ZZ^{-1/2} k(Z, x)` ou `Z` sont `m` observations tirees sans
    remise. Le produit `phi(x)^T phi(y)` est l'approximation de Nystrom de
    `k(x, y)`. Les moments de la reference se calculent par Monte-Carlo :
    aucune forme close n'est disponible ici, et c'est declare.

    Le spectre de `K_ZZ` decroit geometriquement pour un noyau lisse, donc
    l'inverse de sa racine est catastrophiquement mal conditionne. Les
    directions sous `rank_tolerance` fois la plus grande valeur propre sont
    donc SUPPRIMEES, pas inversees avec un plancher : le nombre de
    descripteurs effectifs `count` est le rang retenu, et il peut etre
    strictement inferieur au nombre de points de reference. C'est la meme
    decision que la troncature de `SpectralLogDensity._decompose`, prise au
    meme endroit conceptuel : la classe de fonctions, pas l'axe des niveaux.
    """

    name = "nystrom"

    def __init__(self, landmarks, bandwidth, ridge=0.0, rank_tolerance=1e-8):
        landmarks = np.asarray(landmarks, dtype=float)
        if landmarks.ndim != 2:
            raise ValueError("les points de reference forment une matrice (m, d)")
        self.landmarks = landmarks
        self.dimension = landmarks.shape[1]
        self.landmark_count = landmarks.shape[0]
        self.bandwidth = float(bandwidth)
        gram = self._kernel(landmarks)
        if ridge > 0.0:
            scale = float(np.trace(gram)) / max(1, self.landmark_count)
            gram = gram + ridge * scale * np.eye(self.landmark_count)
        values, vectors = np.linalg.eigh(0.5 * (gram + gram.T))
        largest = float(values.max())
        if largest <= 0.0:
            raise DegenerateSample("matrice de Gram des points de reference nulle")
        kept = values > rank_tolerance * largest
        self.count = int(np.count_nonzero(kept))
        if self.count == 0:
            raise DegenerateSample("aucune direction de Nystrom au-dessus du seuil")
        self.whitener = (vectors[:, kept] / np.sqrt(values[kept])).T

    def _kernel(self, points):
        points = np.asarray(points, dtype=float)
        left = np.einsum("ij,ij->i", self.landmarks, self.landmarks)
        right = np.einsum("ij,ij->i", points, points)
        cross = self.landmarks @ points.T
        squared = np.maximum(left[:, None] + right[None, :] - 2.0 * cross, 0.0)
        return np.exp(-0.5 * squared / (self.bandwidth ** 2))

    def transform(self, points):
        return (self.whitener @ self._kernel(points)).T

    def directional(self, points, weights):
        """Gradient de `weights^T phi(x)` : chaine sur le noyau gaussien."""
        points = np.asarray(points, dtype=float)
        kernel = self._kernel(points)
        coefficients = self.whitener.T @ weights
        scaled = kernel * coefficients[:, None]
        total = scaled.sum(axis=0)
        return (scaled.T @ self.landmarks - total[:, None] * points) / (self.bandwidth ** 2)


class GaussianReference:
    """Reference gaussienne ajustee aux moments, avec dilatation declaree.

    Une covariance empirique SINGULIERE n'est pas un accident : c'est le cas
    de toutes les donnees de dimension intrinseque inferieure a `d` (un nuage
    LiDAR sur une surface, un melange dont les centres vivent dans un plan).
    La reference gaussienne exige un rang plein, donc deux comportements
    declares :

    * `floor = 0` (defaut) : REFUS explicite par `DegenerateSample` ;
    * `floor > 0` : les valeurs propres sous `floor * lambda_max` sont
      relevees a `floor * lambda_max`. La reference reste une gaussienne de
      support `R^d` tout entier, mais elle est DILATEE dans les directions
      ou l'echantillon est plat. C'est une regularisation, elle est comptee
      dans `floored`, et elle change la mesure de reference donc le
      log-rapport : elle ne doit jamais etre silencieuse.
    """

    name = "gaussian"

    def __init__(self, mean, covariance, inflation=1.0, floor=0.0):
        self.mean = np.asarray(mean, dtype=float)
        self.inflation = float(inflation)
        self.floor = float(floor)
        raw = inflation * np.asarray(covariance, dtype=float)
        values, vectors = np.linalg.eigh(0.5 * (raw + raw.T))
        largest = float(values.max())
        if largest <= 0.0:
            raise DegenerateSample("covariance empirique nulle")
        threshold = self.floor * largest
        self.floored = int(np.count_nonzero(values < threshold))
        if values.min() <= 0.0 and self.floored == 0:
            raise DegenerateSample(
                "covariance empirique singuliere : valeur propre minimale "
                + repr(float(values.min()))
                + " (passer covariance_floor > 0 pour dilater la reference)"
            )
        values = np.maximum(values, threshold)
        self.covariance = vectors @ np.diag(values) @ vectors.T
        self.precision = vectors @ np.diag(1.0 / values) @ vectors.T
        self.log_normaliser = 0.5 * (
            self.mean.size * math.log(2.0 * math.pi) + float(np.sum(np.log(values)))
        )

    def sample(self, count, seed):
        generator = np.random.default_rng(seed)
        root = np.linalg.cholesky(self.covariance)
        return self.mean + generator.standard_normal((count, self.mean.size)) @ root.T

    def log_density(self, points):
        centred = np.asarray(points, dtype=float) - self.mean
        whitened = centred @ self.precision
        quadratic = np.einsum("ij,ij->i", whitened, centred)
        return -0.5 * quadratic - self.log_normaliser

    def gradient_log_density(self, points):
        centred = np.asarray(points, dtype=float) - self.mean
        return -centred @ self.precision


class UniformBoxReference:
    """Reference uniforme sur la boite englobante dilatee.

    `log q` est constant a l'interieur et `- infini` a l'exterieur. La
    convention retenue ici PROLONGE la valeur constante a tout `R^d` pour
    que l'ascension de gradient ne rencontre pas de falaise : c'est un
    choix declare, pas une propriete de la mesure, et il rend la tour de
    `log p` identique a celle du log-rapport.
    """

    name = "uniform_box"

    def __init__(self, lower, upper, inflation=1.0):
        lower = np.asarray(lower, dtype=float)
        upper = np.asarray(upper, dtype=float)
        centre = 0.5 * (lower + upper)
        half = 0.5 * inflation * (upper - lower)
        if np.any(half <= 0.0):
            raise DegenerateSample("boite englobante d'interieur vide")
        self.lower = centre - half
        self.upper = centre + half
        self.inflation = float(inflation)
        self.log_normaliser = float(np.sum(np.log(2.0 * half)))

    def sample(self, count, seed):
        generator = np.random.default_rng(seed)
        return generator.uniform(self.lower, self.upper, size=(count, self.lower.size))

    def log_density(self, points):
        points = np.asarray(points, dtype=float)
        return np.full(points.shape[0], -self.log_normaliser)

    def gradient_log_density(self, points):
        points = np.asarray(points, dtype=float)
        return np.zeros_like(points)


def median_bandwidth(cloud, sample=512, seed=0):
    """Heuristique de la mediane : mediane des distances par paires.

    La valeur croit comme `sqrt(d)` pour un nuage de variance fixee par
    coordonnee : c'est l'un des deux endroits ou la dimension entre.
    """
    cloud = np.asarray(cloud, dtype=float)
    count = cloud.shape[0]
    if count > sample:
        generator = np.random.default_rng(seed)
        cloud = cloud[generator.choice(count, size=sample, replace=False)]
    squared = np.einsum("ij,ij->i", cloud, cloud)
    matrix = squared[:, None] + squared[None, :] - 2.0 * cloud @ cloud.T
    upper = matrix[np.triu_indices(cloud.shape[0], 1)]
    upper = np.maximum(upper, 0.0)
    value = math.sqrt(float(np.median(upper))) if upper.size else 1.0
    return value if value > 0.0 else 1.0


class SpectralLogDensity:
    """Log-rapport de densites spectral, evaluable partout dans `R^d`.

    Sequence : `fit(cloud)` calcule les moments, la decomposition
    generalisee et le vecteur `Theta` de la forme close (C) ; `evaluate`
    et `gradient` repondent partout ; `log_density` ajoute `log q`.

    Cout : `O(n d^2 + n m d + m^2 d + m^2 n + m^3)`, compte terme a terme
    dans `self.cost["multiply_add"]` et MESURE au paragraphe 5 du module. Les
    deux termes en `d` ne sont pas negligeables : a `d = 400` ils dominent.
    Aucun terme en `C(n, k)` n'apparait, et c'est la seule affirmation de
    cout que ce module fait.
    """

    def __init__(
        self,
        feature_count=256,
        bandwidth=None,
        bandwidth_scale=1.0,
        features="fourier",
        reference="gaussian",
        inflation=1.0,
        covariance_floor=0.0,
        ridge=0.0,
        rho_max=1.0,
        eigenvalue_floor=1e-10,
        rank_tolerance=1e-10,
        on_singular="truncate",
        moments="auto",
        reference_sample=8192,
        canonical_order=True,
        seed=0,
    ):
        self.feature_count = int(feature_count)
        self.bandwidth = bandwidth
        self.bandwidth_scale = float(bandwidth_scale)
        self.features_kind = features
        self.reference_kind = reference
        self.inflation = float(inflation)
        self.covariance_floor = float(covariance_floor)
        self.ridge = float(ridge)
        self.rho_max = float(rho_max)
        self.eigenvalue_floor = float(eigenvalue_floor)
        self.rank_tolerance = float(rank_tolerance)
        self.on_singular = str(on_singular)
        self.moments_kind = moments
        self.reference_sample = int(reference_sample)
        self.canonical_order = bool(canonical_order)
        self.seed = int(seed)
        self.features = None
        self.reference = None
        self.fitted = False
        self.cost = {}

    # -- ajustement ----------------------------------------------------

    def fit(self, cloud):
        """Ajuste le modele sur l'echantillon `cloud` de forme `(n, d)`."""
        started = time.perf_counter()
        cloud = np.asarray(cloud, dtype=float)
        if cloud.ndim != 2 or cloud.shape[0] < 2:
            raise DegenerateSample("il faut au moins deux observations en matrice (n, d)")
        if not np.all(np.isfinite(cloud)):
            raise DegenerateSample("observations non finies")
        count, dimension = cloud.shape
        self.count = count
        self.dimension = dimension
        if self.canonical_order:
            keys = tuple(cloud[:, index] for index in range(dimension - 1, -1, -1))
            self.input_order = np.lexsort(keys)
            cloud = cloud[self.input_order]
        else:
            self.input_order = np.arange(count)
        self.sample_mean = cloud.mean(axis=0)
        centred = cloud - self.sample_mean
        self.sample_covariance = (centred.T @ centred) / count
        flops = count * dimension * dimension
        if self.bandwidth is None:
            bandwidth = self.bandwidth_scale * median_bandwidth(centred, seed=self.seed)
        else:
            bandwidth = self.bandwidth_scale * float(self.bandwidth)
        self.effective_bandwidth = bandwidth
        self.features = self._build_features(centred, bandwidth)
        self.reference = self._build_reference(centred)
        size = self.features.count
        matrix = self.features.transform(centred)
        flops += count * size * dimension + count * size * size
        self.mean_p = matrix.mean(axis=0)
        self.sigma_p = (matrix.T @ matrix) / count
        moments = self._moments_kind()
        if moments == "analytic":
            self.mean_q, self.sigma_q, reference_flops = self.features.gaussian_moments(
                self.reference.mean, self.reference.covariance
            )
        else:
            draw = self.reference.sample(self.reference_sample, self.seed + 1)
            reference_matrix = self.features.transform(draw)
            self.mean_q = reference_matrix.mean(axis=0)
            self.sigma_q = (reference_matrix.T @ reference_matrix) / self.reference_sample
            reference_flops = self.reference_sample * size * (size + dimension)
        flops += reference_flops
        self.moments_used = moments
        if self.ridge > 0.0:
            scale = float(np.trace(self.sigma_q)) / size
            self.sigma_q = self.sigma_q + self.ridge * scale * np.eye(size)
        self._decompose()
        flops += 10 * size ** 3
        self.cost = {
            "sample_count": count,
            "dimension": dimension,
            "feature_count": size,
            "effective_rank": self.effective_rank,
            "moments": moments,
            "reference_sample": 0 if moments == "analytic" else self.reference_sample,
            "multiply_add": int(flops),
            "seconds": time.perf_counter() - started,
        }
        self.fitted = True
        return self

    def _build_features(self, cloud, bandwidth):
        if not isinstance(self.features_kind, str):
            return self.features_kind
        if self.features_kind == "fourier":
            return FourierFeatures(cloud.shape[1], self.feature_count, bandwidth, self.seed)
        if self.features_kind == "nystrom":
            generator = np.random.default_rng(self.seed)
            size = min(self.feature_count, cloud.shape[0])
            chosen = generator.choice(cloud.shape[0], size=size, replace=False)
            return NystromFeatures(cloud[np.sort(chosen)], bandwidth)
        raise ValueError("plongement inconnu : " + str(self.features_kind))

    def _build_reference(self, cloud):
        if not isinstance(self.reference_kind, str):
            return self.reference_kind
        if self.reference_kind == "gaussian":
            return GaussianReference(
                np.zeros(self.dimension),
                self.sample_covariance,
                self.inflation,
                self.covariance_floor,
            )
        if self.reference_kind == "uniform_box":
            return UniformBoxReference(
                cloud.min(axis=0), cloud.max(axis=0), max(self.inflation, 1.0)
            )
        raise ValueError("reference inconnue : " + str(self.reference_kind))

    def _moments_kind(self):
        analytic = (
            getattr(self.features, "name", None) == "fourier"
            and getattr(self.reference, "name", None) == "gaussian"
            and hasattr(self.features, "gaussian_moments")
        )
        if self.moments_kind == "auto":
            return "analytic" if analytic else "monte_carlo"
        if self.moments_kind == "analytic":
            if not analytic:
                raise ValueError(
                    "moments analytiques disponibles seulement pour Fourier + gaussienne"
                )
            return "analytic"
        if self.moments_kind == "monte_carlo":
            return "monte_carlo"
        raise ValueError("mode de moments inconnu : " + str(self.moments_kind))

    def _decompose(self):
        """Decomposition generalisee du couple `(Sigma_p, Sigma_q)`.

        La matrice `Sigma_q` d'un plongement de Fourier est PRESQUE TOUJOURS
        de rang deficient : un noyau gaussien lisse en petite dimension a un
        spectre qui decroit geometriquement, donc `m` descripteurs engendrent
        un espace de rang effectif bien inferieur a `m`. Ce n'est pas un
        accident numerique, c'est le fait mesure que la classe de fonctions
        est plus petite que le nombre de descripteurs.

        Deux politiques declarees, selon `on_singular` :

        * `truncate` (defaut) : on RESTREINT la classe de fonctions au
          sous-espace engendre par les directions de variance de reference
          au-dessus de `rank_tolerance * lambda_max(Sigma_q)`. C'est une
          regularisation de la CLASSE DE FONCTIONS, pas de l'axe des
          niveaux, et le rang retenu est publie dans `effective_rank` ;
        * `refuse` : tout defaut de rang leve `SingularReference`.

        Dans le sous-espace retenu, le blanchiment `psi = S^{-1/2} U^T phi`
        rend `Sigma_q = I`, et la decomposition generalisee se reduit a une
        decomposition symetrique ordinaire : la base obtenue verifie bien
        `v_i^T Sigma_q v_j = 1_{i=j}` et `Sigma_p v_i = lambda_i Sigma_q v_i`.
        """
        spectrum, basis = np.linalg.eigh(self.sigma_q)
        largest = float(spectrum.max())
        smallest = float(spectrum.min())
        if largest <= 0.0:
            raise SingularReference("moments de reference nuls ou negatifs")
        kept = spectrum > self.rank_tolerance * largest
        rank = int(np.count_nonzero(kept))
        self.effective_rank = rank
        self.reference_spectrum = spectrum
        if rank == 0:
            raise SingularReference("aucune direction de reference au-dessus du seuil")
        if rank < spectrum.size and self.on_singular == "refuse":
            raise SingularReference(
                "moments de reference singuliers : rang effectif "
                + repr(rank)
                + " sur "
                + repr(int(spectrum.size))
                + ", valeur propre minimale "
                + repr(smallest)
                + ", maximale "
                + repr(largest)
                + ", seuil relatif "
                + repr(self.rank_tolerance)
            )
        if self.on_singular not in ("truncate", "refuse"):
            raise ValueError("politique inconnue : " + str(self.on_singular))
        whitener = basis[:, kept] / np.sqrt(spectrum[kept])
        reduced = whitener.T @ self.sigma_p @ whitener
        reduced = 0.5 * (reduced + reduced.T)
        values, rotation = np.linalg.eigh(reduced)
        vectors = whitener @ rotation
        if not np.all(np.isfinite(values)) or not np.all(np.isfinite(vectors)):
            raise SingularReference("decomposition generalisee non finie")
        self.eigenvalues = values
        self.eigenvectors = vectors
        self.delta = self.mean_p - self.mean_q
        self.projections = vectors.T @ self.delta
        filtered, clamped = spectral_filter(values, self.rho_max, self.eigenvalue_floor)
        self.filter_values = filtered
        self.clamped_directions = clamped
        self.theta = vectors @ (filtered * self.projections)

    # -- evaluation ----------------------------------------------------

    def _require_fit(self):
        if not self.fitted:
            raise RuntimeError("modele non ajuste : appeler fit d'abord")

    def centred(self, points):
        """Coordonnees centrees sur la moyenne de l'echantillon.

        Le plongement et la reference travaillent dans ces coordonnees : le
        modele est donc EQUIVARIANT PAR TRANSLATION, et son log-rapport est
        invariant par translation du nuage et du point d'evaluation.
        """
        self._require_fit()
        return np.atleast_2d(np.asarray(points, dtype=float)) - self.sample_mean

    def evaluate(self, points):
        """Estimation de `log (dp/dq)` aux points donnes, forme `(N,)`."""
        centred = self.centred(points)
        return self.features.transform(centred) @ self.theta

    def gradient(self, points):
        """Gradient analytique de l'estimation de `log (dp/dq)`, `(N, d)`."""
        centred = self.centred(points)
        return self.features.directional(centred, self.theta)

    def log_density(self, points):
        """Estimation de `log p = log(dp/dq) + log q`."""
        centred = self.centred(points)
        return self.features.transform(centred) @ self.theta + self.reference.log_density(centred)

    def gradient_log_density(self, points):
        """Gradient de l'estimation de `log p`."""
        centred = self.centred(points)
        return self.features.directional(centred, self.theta) + (
            self.reference.gradient_log_density(centred)
        )

    def theta_at(self, rho):
        """Forme close (A) `theta(rho)`, par la base generalisee.

        Quand `effective_rank < feature_count`, c'est la solution exacte du
        probleme RESTREINT au sous-espace retenu (celle de norme minimale
        parmi les solutions du probleme complet, qui n'est pas unique).
        """
        self._require_fit()
        denominator = 1.0 + rho * (self.eigenvalues - 1.0)
        if np.any(denominator <= 0.0):
            raise SingularReference("rho hors du domaine de definition de S(rho)")
        return self.eigenvectors @ (self.projections / denominator)

    def potential_at(self, rho, points):
        """Valeur du potentiel `u(rho, x) = theta(rho)^T phi(x)`."""
        centred = self.centred(points)
        return self.features.transform(centred) @ self.theta_at(rho)

    def quadratic_potentials(self, rho):
        """Triplet `(M, N, c)` des potentiels quadratiques du paragraphe 4."""
        theta = self.theta_at(rho)
        outer = np.outer(theta, theta)
        return -0.5 * rho * outer, -0.5 * (1.0 - rho) * outer, 0.5 * theta

    def divergence(self, rho):
        """Divergence `D_rho` du modele lineaire, en forme close."""
        self._require_fit()
        denominator = 1.0 + rho * (self.eigenvalues - 1.0)
        if np.any(denominator <= 0.0):
            raise SingularReference("rho hors du domaine de definition de S(rho)")
        return 0.5 * float(np.sum(self.projections ** 2 / denominator))

    def kullback_leibler(self):
        """Divergence de Kullback-Leibler du modele, base generalisee.

        `somme_i (v_i^T delta)^2 (lambda log lambda - lambda + 1)/(lambda-1)^2`.
        """
        self._require_fit()
        values = np.maximum(self.eigenvalues, self.eigenvalue_floor)
        gap = values - 1.0
        weight = np.where(
            np.abs(gap) < 1e-8,
            0.5,
            (values * np.log(np.maximum(values, 1e-300)) - values + 1.0)
            / np.where(gap == 0.0, 1.0, gap ** 2),
        )
        return float(np.sum(self.projections ** 2 * weight))

    def spectrum(self):
        """Valeurs propres generalisees et valeurs du filtre spectral."""
        self._require_fit()
        return self.eigenvalues.copy(), self.filter_values.copy()


def order_to_log_density_level(order, count, radius_squared, dimension):
    """Calibration explicite de l'axe d'ordre `k` vers l'axe de niveau.

    Le comptage dur `C(y) = #{ i : ||y - x_i||^2 <= a }` est `n` fois un
    estimateur a noyau INDICATRICE DE BOULE de la densite :

        C(y) = n * (1/n) somme_i 1[ ||y - x_i|| <= sqrt(a) ]
             ~ n * V_d * a^{d/2} * p(y) ,

    ou `V_d = pi^{d/2} / Gamma(d/2 + 1)` est le volume de la boule unite.
    Le comptage doux `somme_i g((a - e_i)/epsilon)` du module
    `soft/fermi.py` est le meme estimateur avec un noyau lisse de meme
    support nominal. Donc le seuil d'ordre `k` correspond au niveau

        lambda = log k - log n - log V_d - (d/2) log a

    sur `log p`. C'est une EGALITE DE CALIBRATION entre les deux axes, a
    l'erreur d'estimation a noyau pres ; elle dit exactement pourquoi
    l'axe d'ordre de HGP et l'axe de niveau de E-HGP sont le meme axe vu
    dans deux echelles.
    """
    if order < 1 or count < 1 or radius_squared <= 0.0 or dimension < 1:
        raise ValueError("arguments de calibration hors domaine")
    log_unit_volume = 0.5 * dimension * math.log(math.pi) - math.lgamma(0.5 * dimension + 1.0)
    return (
        math.log(order)
        - math.log(count)
        - log_unit_volume
        - 0.5 * dimension * math.log(radius_squared)
    )
