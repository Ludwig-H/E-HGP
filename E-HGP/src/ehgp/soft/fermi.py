"""Niveau de Fermi : regularisation entropique de la k-ieme distance.

Pour `y` dans `R^d` et `X = {x_1..x_n}`, notons `e_i(y) = ||y - x_i||^2`. La
somme des `k` plus petites energies est la valeur du programme lineaire

    min { somme_i w_i e_i : 0 <= w_i <= 1 , somme_i w_i = k } ,

dont la solution met un poids 1 sur les `k` plus petites : c'est `k` fois le
carre de la distance-a-la-mesure, et `a_k(y)` est le MULTIPLICATEUR de la
contrainte de masse.

En ajoutant a ce programme une entropie separable `epsilon * somme_i phi(w_i)`
strictement convexe, l'optimum devient

    w_i = g((mu - e_i)/epsilon) ,   g = (phi')^{-1} tronque a [0, 1] ,

ou `mu = mu_k^epsilon(y)` est l'unique solution de `somme_i g((mu - e_i)/eps)
= k` : le NIVEAU DE FERMI. Deux consequences exploitees par E-HGP.

1. Correspondance f-divergence / noyau. Le choix de `phi` fixe la FORME du
   noyau `g` par transformee de Legendre :

   * `phi(w) = w log w + (1 - w) log(1 - w)` (Fermi-Dirac, entropie de
     Shannon sous contrainte de capacite) donne `g = sigmoide` : noyau
     logistique, support infini, classe C-infini ;
   * `phi(w) = w^2 / 2` (chi-deux, cas `rho = 0` de la famille `f_rho` de
     Bach) donne `g(u) = clip(u, 0, 1)` : noyau RAMPE, support compact,
     lineaire par morceaux, donc EXACTEMENT representable en rationnels ;
   * `phi(w) = (w^alpha - w)/(alpha - 1)` (Tsallis) donne des noyaux en
     puissance, intermediaires.

2. Le gradient est un barycentre. En derivant l'equation implicite,

       grad mu(y) = 2 ( y - m(y) ) ,
       m(y) = somme_i s_i x_i / somme_i s_i ,  s_i = g'((mu - e_i)/epsilon) ,

   donc la descente de gradient a pas `1/2` est l'iteration

       y  <-  m(y) ,

   « aller au barycentre pondere de la coquille ». Ses points fixes sont les
   `y` egaux au barycentre des observations situees sur leur sphere de
   niveau : c'est EXACTEMENT la condition de criticite
   `c dans conv(U(c, a))` de `docs/SPECIFICATION_MORSEHGP3D.md` § 5, obtenue
   sans aucune enumeration combinatoire et sans dependance en `d`.

Ce module travaille en flottant : c'est un PROPOSEUR de candidats. Toute
decision est ensuite certifiee en rationnels exacts par
`engine/segment.py`. Aucun resultat publie ne repose sur ces flottants.
"""

import numpy as np

FAMILIES = ("logistic", "ramp")


def occupancy(values, family="logistic"):
    """Fonction d'occupation `g` de la famille demandee."""
    if family == "logistic":
        return 1.0 / (1.0 + np.exp(-np.clip(values, -700.0, 700.0)))
    if family == "ramp":
        return np.clip(values, 0.0, 1.0)
    raise ValueError("famille inconnue : " + str(family))


def occupancy_slope(values, family="logistic"):
    """Derivee `g'` de la fonction d'occupation."""
    if family == "logistic":
        weights = occupancy(values, family)
        return weights * (1.0 - weights)
    if family == "ramp":
        return ((values > 0.0) & (values < 1.0)).astype(float)
    raise ValueError("famille inconnue : " + str(family))


def soft_count(energies, level, epsilon, family="logistic"):
    """Comptage doux `somme_i g((level - e_i)/epsilon)`."""
    return float(np.sum(occupancy((level - energies) / epsilon, family)))


def fermi_level(energies, mass, epsilon, family="logistic", tolerance=1e-12, steps=200):
    """Niveau de Fermi : unique `mu` tel que le comptage doux vaille `mass`.

    `energies` est le vecteur des `e_i(y)`, `mass` la masse visee (un reel,
    pas necessairement entier : l'axe d'ordre devient continu).
    """
    energies = np.asarray(energies, dtype=float)
    count = energies.size
    if not 0.0 < mass < count:
        raise ValueError("masse hors de (0, n)")
    low = float(energies.min()) - 1.0
    high = float(energies.max()) + 1.0
    span = max(1.0, high - low)
    while soft_count(energies, low, epsilon, family) > mass:
        low -= span
        span *= 2.0
    span = max(1.0, high - low)
    while soft_count(energies, high, epsilon, family) < mass:
        high += span
        span *= 2.0
    for _step in range(steps):
        middle = 0.5 * (low + high)
        if soft_count(energies, middle, epsilon, family) < mass:
            low = middle
        else:
            high = middle
        if high - low <= tolerance * max(1.0, abs(high)):
            break
    return 0.5 * (low + high)


def energies_at(cloud, position):
    """Vecteur des `||y - x_i||^2` pour un nuage `numpy`."""
    difference = cloud - position
    return np.einsum("ij,ij->i", difference, difference)


def shell_barycentre(cloud, position, mass, epsilon, family="logistic"):
    """Barycentre pondere `m(y)` de la coquille, et le niveau de Fermi."""
    energies = energies_at(cloud, position)
    level = fermi_level(energies, mass, epsilon, family)
    slopes = occupancy_slope((level - energies) / epsilon, family)
    total = slopes.sum()
    if total <= 0.0:
        return position.copy(), level, 0.0
    return (slopes @ cloud) / total, level, float(total)


def descend(cloud, start, mass, epsilon, family="logistic", steps=200, tolerance=1e-13):
    """Descente `y <- m(y)` vers un minimum local du niveau de Fermi.

    Renvoie `(position, niveau, deplacement_final)`. L'iteration ne fait
    intervenir que des produits matrice-vecteur : cout `O(n d)` par pas,
    aucune combinatoire, aucune dependance a la dimension au-dela du produit
    scalaire.
    """
    position = np.asarray(start, dtype=float).copy()
    level = float("inf")
    shift = float("inf")
    for _step in range(steps):
        target, level, weight = shell_barycentre(cloud, position, mass, epsilon, family)
        if weight <= 0.0:
            break
        shift = float(np.linalg.norm(target - position))
        position = target
        if shift <= tolerance * (1.0 + float(np.linalg.norm(position))):
            break
    return position, level, shift


def hard_order_statistic(cloud, position, order):
    """`a_order(y)` en flottant : reference de comparaison du niveau doux."""
    energies = energies_at(cloud, position)
    return float(np.partition(energies, order - 1)[order - 1])


def ramp_level(energies, mass, epsilon):
    """Niveau de Fermi de la famille RAMPE, en forme close.

    Pour `g(u) = clip(u, 0, 1)`, le comptage doux

        C(a) = somme_i clip((a - e_i)/epsilon, 0, 1)

    est affine par morceaux et croissant en `a`, avec ruptures en `e_i` et
    `e_i + epsilon`. On resout donc `C(a) = mass` exactement : on trie les
    `2n` ruptures, on localise l'intervalle, puis on inverse la fonction
    affine de cet intervalle. Cout `O(n log n)`, sans iteration, et
    transposable tel quel en arithmetique rationnelle exacte.
    """
    energies = np.asarray(energies, dtype=float)
    count = energies.size
    if not 0.0 < mass <= count:
        raise ValueError("masse hors de (0, n]")
    breaks = np.concatenate([energies, energies + epsilon])
    breaks.sort()
    matrix = (breaks[:, None] - energies[None, :]) / epsilon
    values = np.clip(matrix, 0.0, 1.0).sum(axis=1)
    index = int(np.searchsorted(values, mass, side="left"))
    if index == 0:
        low_point, low_value = float(breaks[0]) - epsilon, 0.0
        high_point, high_value = float(breaks[0]), float(values[0])
    elif index >= breaks.size:
        return float(breaks[-1])
    else:
        low_point, low_value = float(breaks[index - 1]), float(values[index - 1])
        high_point, high_value = float(breaks[index]), float(values[index])
    if high_value <= low_value:
        return high_point
    return low_point + (mass - low_value) * (high_point - low_point) / (high_value - low_value)


def ramp_band(energies, level, epsilon):
    """Indices de la bande de transition : `level - epsilon < e_i < level`."""
    energies = np.asarray(energies, dtype=float)
    return (energies > level - epsilon) & (energies < level)
