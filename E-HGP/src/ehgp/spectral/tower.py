"""Tour E-HGP des ensembles de sur-niveau d'un modele de log-densite.

== 1. L'OBJET ==

La tour exacte de `exact/tower.py` est indexee par l'ORDRE `k` et par le
NIVEAU `a` : `L_k(a) = { y : a_k(y) <= a }` est la region couverte par au
moins `k` boules de rayon `sqrt(a)`, et la sortie est `pi_0(L_k(a))`. Le
comptage de boules est un estimateur a noyau INDICATRICE DE BOULE de la
densite : `#{ i : ||y - x_i||^2 <= a } ~ n V_d a^{d/2} p(y)` (cf.
`order_to_log_density_level` dans `log_density.py`). Les deux axes sont donc
le meme axe dans deux echelles, et

    { y : a_k(y) <= a }   correspond a   { y : p(y) >= k / (n V_d a^{d/2}) } .

Ce module calcule le second membre pour un modele `f` de `log p` : l'arbre
de fusion des ensembles de SUR-NIVEAU `{ f >= lambda }`, avec l'affectation
de chaque observation a son maximum local (son bassin).

== 2. CONVENTION DE SENS, ET CE QUI EST MAJORANT DE QUOI ==

L'axe `a` de HGP croit et les composantes FUSIONNENT quand `a` croit.
L'axe `lambda` de E-HGP decroit et les composantes fusionnent quand
`lambda` decroit, puisque `{ f >= lambda }` grandit. La calibration du
paragraphe 1 est decroissante en `a`, donc les deux sens se correspondent.

Pour deux maxima locaux `c` et `c'`, le niveau de fusion vrai est

    lambda*(c, c') = max sur les chemins gamma de c a c'
                        de min sur gamma de f  ,

le NIVEAU DE COL (theoreme du col de montagne). Un chemin CONTINU explicite
donne donc un MINORANT de `lambda*` : c'est un certificat de connexite de
`{ f >= min_gamma f }`, jamais de deconnexite. Consequence de sens :
minorant sur l'axe `lambda` = MAJORANT sur l'axe `a` de HGP, exactement
comme le maximum de `a_k` sur un segment dans `engine/segment.py` est un
majorant du niveau de fusion. Les deux modules font la meme chose dans les
deux echelles, et aucun des deux ne pretend a l'egalite.

CE QUE LE CODE CALCULE N'EST PAS CE MINORANT, ET LE SENS DE L'ERREUR EST LE
MAUVAIS. Le minimum sur un ECHANTILLON FINI du chemin MAJORE le minimum du
chemin continu : echantillonner pousse la valeur publiee vers le HAUT, donc
du cote non certifie. Le fait est MESURE, contre une minimisation
unidimensionnelle bornee independante sur le meme segment, sur le melange de
reference de `tests/test_spectral.py` : a 33 noeuds l'exces du minimum
echantillonne sur le minimum reel du segment vaut `+2,0 e-2`, `+1,2 e-3` et
`+2,3 e-4` selon la paire, et a 513 noeuds `+3,0 e-6`, `+7,3 e-5`,
`+5,8 e-6`. Sur deux des trois paires, cet exces suffit a passer AU-DESSUS
du niveau de col vrai, encadre independamment par etiquetage de composantes
connexes sur une grille (`+1,1 e-3` et `+2,2 e-4` au-dessus). Le mot
« certifie » serait donc faux, et c'est pourquoi il n'est pas employe.

Ce que le module fait, et qui est exactement dans le bon sens : le minimum
echantillonne est RAFFINE LE LONG DE L'AXE DU CHEMIN
(`refine_along_path`), par grilles emboitees qui ne gardent que la plus
petite valeur vue. La valeur renvoyee est donc DECROISSANTE en `path_refine`
et converge vers le minimum reel du chemin, c'est-a-dire vers un vrai
minorant. Le nombre de tours est publie dans l'enregistrement
(`path_refine`), comme toute regularisation de ce chantier.

Le raffinement TRANSVERSE (`raise_path`, hors defaut) fait l'autre moitie du
travail : il cherche un MEILLEUR chemin, donc il AUGMENTE legitimement le
minorant. Les deux se composent par un MAXIMUM sur les chemins essayes,
puisque `lambda*` est un max sur les chemins : c'est ce que fait
`_find_saddles`. Aucune borne superieure n'est produite : dire que la fusion
n'a pas lieu au-dessus d'un niveau demanderait un argument global sur `f`,
que ce module n'a pas.

== 3. ARCHITECTURE, IDENTIQUE A CELLE DU NOYAU EXACT ==

(a) MAXIMA. Ascension de gradient a recherche lineaire depuis chaque
    observation. C'est l'exact analogue de la descente `y <- m(y)` de
    `soft/fermi.py` et de la descente MEB-Lloyd de `engine/critical.py` :
    les points fixes sont les points critiques du modele. Les points
    d'arrivee sont agglomeres a un rayon declare.

(b) COLS. Pour chaque paire de maxima, un chemin, puis le minimum de `f`
    le long de ce chemin. Le chemin de depart est le SEGMENT DROIT
    echantillonne ; le raffinement relaxe les noeuds interieurs vers le
    haut, en n'acceptant que les pas qui augmentent le minimum.

(c) LIAISON SIMPLE. Les maxima sont fusionnes par ordre DECROISSANT de
    niveau de col : c'est la liaison simple sur la dissimilarite
    `- lambda*`, donc l'arbre de fusion de `pi_0({ f >= lambda })` restreint
    aux composantes qui contiennent un maximum trouve.

== 4. CE QUI EST EXACT, CE QUI EST APPROCHE ==

EXACT : la combinatoire de l'arbre une fois les niveaux de col donnes ;
l'ordre canonique ; le digest.

APPROCHE, et jamais autrement : la liste des maxima (une ascension peut en
manquer un, et deux ascensions peuvent aboutir au meme maximum par deux
points distincts) ; les niveaux de col (minorants, paragraphe 2). En
particulier le NOMBRE de composantes annonce est un nombre de composantes
TROUVEES, jamais `pi_0` certifie. Rien ici ne promeut un statut public.

== 5. CANONICITE ET DIGEST ==

Le modele et la tour travaillent dans l'ORDRE CANONIQUE des observations
(tri lexicographique des coordonnees), donc la permutation des entrees ne
change aucune somme flottante : l'invariance par permutation est exacte, pas
approchee. Les positions sont publiees RELATIVEMENT a la moyenne de
l'echantillon, donc la translation ne change pas l'enregistrement, aux
arrondis pres. Le digest quantifie les flottants a un nombre de chiffres
declare ; `structural_digest` n'utilise que la topologie, les niveaux
quantifies et la partition, et se passe des positions.
"""

import hashlib
import json
import math

import numpy as np


class AscentFailure(Exception):
    """Refus explicite : l'ascension ne converge pas dans le budget donne."""


def _canonical_order(cloud):
    """Permutation du tri lexicographique par coordonnees croissantes."""
    cloud = np.asarray(cloud, dtype=float)
    keys = tuple(cloud[:, index] for index in range(cloud.shape[1] - 1, -1, -1))
    return np.lexsort(keys)


def _quantise(value, digits):
    """Arrondi canonique d'un flottant, avec `-0.0` ramene a `0.0`."""
    rounded = round(float(value), digits)
    return 0.0 if rounded == 0.0 else rounded


class _Forest:
    """Union-find deterministe : la racine gardee est le plus petit indice."""

    def __init__(self, size):
        self.parent = list(range(size))

    def find(self, item):
        root = item
        while self.parent[root] != root:
            root = self.parent[root]
        while self.parent[item] != root:
            self.parent[item], item = root, self.parent[item]
        return root

    def union(self, left, right):
        left_root, right_root = self.find(left), self.find(right)
        if left_root == right_root:
            return False
        keep, drop = (
            (left_root, right_root) if left_root < right_root else (right_root, left_root)
        )
        self.parent[drop] = keep
        return True


def gradient_ascent(
    value,
    gradient,
    start,
    step,
    steps=400,
    tolerance=1e-9,
    growth=1.6,
    step_cap=None,
    gradient_floor=0.0,
):
    """Ascension de gradient a recherche lineaire par rebroussement.

    Renvoie `(positions, valeurs, pas_finaux)` pour un lot de departs. Le
    pas est propre a chaque depart : il croit d'un facteur `growth` quand il
    reussit et il est divise par deux quand il echoue, ce qui rend la
    methode insensible a l'echelle du gradient (donc a la dimension), sans
    depasser `step_cap`. Un depart quitte le lot ACTIF des que son pas tombe
    sous `tolerance` ou que la norme de son gradient tombe sous
    `gradient_floor` : le cout total est proportionnel au travail restant, et
    non au produit du nombre de departs par le nombre d'iterations. Les deux
    criteres sont relatifs a des echelles MESUREES sur les donnees (etendue
    du nuage, norme mediane du gradient aux observations), donc sans
    constante absolue cachee.

    FAIT MESURE, et c'est pourquoi ce n'est pas l'ascension par defaut : sur
    le modele spectral en dimension 50, cette methode laisse encore une
    norme de gradient mediane de `3 e-1` apres 160 iterations et annonce 53
    points d'arrivee distincts, alors que l'ascension quasi-Newton de
    `joint_ascent` converge a `|g| <= 3 e-6` et en trouve 3. Le zigzag du
    gradient normalise dans une vallee courbe, pas la geometrie du modele,
    produisait ces 53 faux maxima. Cette fonction reste comme SECOND TEMOIN
    (elle n'utilise que `value` et `gradient`, sans scipy) et comme mise en
    garde : un nombre de maxima annonce par une ascension non convergee est
    un artefact d'optimiseur.
    """
    positions = np.array(start, dtype=float, copy=True)
    count = positions.shape[0]
    ceiling = float("inf") if step_cap is None else float(step_cap)
    steps_taken = np.full(count, min(float(step), ceiling))
    values = np.asarray(value(positions), dtype=float).copy()
    active = np.arange(count)
    for _iteration in range(steps):
        if active.size == 0:
            break
        current = positions[active]
        slopes = np.asarray(gradient(current), dtype=float)
        norms = np.sqrt(np.einsum("ij,ij->i", slopes, slopes))
        moving = norms > gradient_floor
        directions = np.zeros_like(slopes)
        directions[moving] = slopes[moving] / norms[moving, None]
        proposed = current + steps_taken[active, None] * directions
        proposed_values = np.asarray(value(proposed), dtype=float)
        better = moving & (proposed_values > values[active])
        chosen = active[better]
        positions[chosen] = proposed[better]
        values[chosen] = proposed_values[better]
        steps_taken[active] = np.where(
            better,
            np.minimum(steps_taken[active] * growth, ceiling),
            steps_taken[active] * 0.5,
        )
        steps_taken[active[~moving]] = 0.0
        active = active[steps_taken[active] > tolerance]
    return positions, values, steps_taken


def joint_ascent(value, gradient, start, max_iterations=600, tolerance=1e-8, ftol=1e-15):
    """Ascension quasi-Newton EMPILEE : L-BFGS-B sur la somme des valeurs.

    On minimise `- somme_i f(y_i)` sur le vecteur empile `(y_1, ..., y_N)`.
    L'objectif SE SEPARE en les `N` problemes independants, donc son gradient
    est le gradient bloc par bloc et ses points critiques sont exactement les
    N-uplets de points critiques : le probleme empile a les memes solutions
    que les `N` ascensions separees. Le couplage introduit par L-BFGS est
    dans la METRIQUE de recherche, pas dans l'ensemble des solutions, et le
    critere d'arret de L-BFGS-B porte sur le maximum des composantes du
    gradient, donc il est atteint bloc par bloc.

    Le gain est double : une seule evaluation vectorisee par iteration pour
    tous les departs, et une methode quasi-Newton insensible au
    conditionnement des vallees courbes (cf. le fait mesure documente dans
    `gradient_ascent`).

    Renvoie `(positions, valeurs, diagnostic)`.
    """
    import scipy.optimize

    start = np.asarray(start, dtype=float)
    count, dimension = start.shape
    calls = {"value": 0}

    def objective(flat):
        calls["value"] += 1
        points = flat.reshape(count, dimension)
        values = np.asarray(value(points), dtype=float)
        slopes = np.asarray(gradient(points), dtype=float)
        if not np.all(np.isfinite(values)) or not np.all(np.isfinite(slopes)):
            raise AscentFailure("modele non fini pendant l'ascension")
        return -float(values.sum()), -slopes.ravel()

    result = scipy.optimize.minimize(
        objective,
        start.ravel(),
        jac=True,
        method="L-BFGS-B",
        options={
            "maxiter": int(max_iterations),
            "maxfun": int(2 * max_iterations),
            "ftol": float(ftol),
            "gtol": float(tolerance),
        },
    )
    positions = result.x.reshape(count, dimension)
    values = np.asarray(value(positions), dtype=float)
    slopes = np.asarray(gradient(positions), dtype=float)
    norms = np.sqrt(np.einsum("ij,ij->i", slopes, slopes))
    diagnostic = {
        "iterations": int(result.nit),
        "evaluations": calls["value"],
        "gradient_max": float(norms.max()),
        "gradient_median": float(np.median(norms)),
        "message": str(result.message),
    }
    return positions, values, diagnostic


def _agglomerate(positions, values, radius):
    """Agglomere les points d'arrivee : glouton par valeur decroissante."""
    order = np.argsort(-values, kind="stable")
    representatives = []
    labels = np.full(positions.shape[0], -1, dtype=int)
    squared = radius * radius
    for index in order:
        placed = False
        for slot, centre in enumerate(representatives):
            gap = positions[index] - centre
            if float(gap @ gap) <= squared:
                labels[index] = slot
                placed = True
                break
        if not placed:
            representatives.append(positions[index].copy())
            labels[index] = len(representatives) - 1
    return np.array(representatives), labels


def polyline_point(nodes, position):
    """Point de la ligne brisee `nodes` au parametre `position`.

    `position` parcourt `[0, len(nodes) - 1]` : l'entier `j` donne le noeud
    `j`, et la partie fractionnaire interpole lineairement vers le suivant.
    """
    total = nodes.shape[0] - 1
    clipped = min(max(float(position), 0.0), float(total))
    lower = min(int(math.floor(clipped)), total - 1)
    fraction = clipped - lower
    return nodes[lower] + fraction * (nodes[lower + 1] - nodes[lower])


def refine_along_path(value, nodes, start, rounds=5, probes=8):
    """Descend le minimum ECHANTILLONNE vers le minimum REEL du chemin.

    Pourquoi c'est necessaire, et pas cosmetique : le minimum sur un
    echantillon fini d'un chemin MAJORE le minimum du chemin, donc il pousse
    la valeur publiee du cote NON certifie (cf. paragraphe 2 du module). Le
    raffinement se fait donc le long de l'AXE DU CHEMIN, par grilles
    emboitees autour du noeud le plus faible : a chaque tour on sonde
    `probes + 1` positions dans le bracket courant, on garde la plus petite
    valeur vue, et on resserre le bracket d'un facteur `2 / probes`.

    La valeur renvoyee est donc DECROISSANTE en `rounds` par construction (on
    ne garde jamais une valeur plus grande), et elle converge vers le minimum
    reel du chemin. Renvoie `(valeur, position)`.

    Les extremites du chemin sont les maxima : ce sont les plus grandes
    valeurs du chemin, donc le minimum ne peut pas s'y deplacer, et aucune
    exclusion explicite n'est necessaire.
    """
    total = nodes.shape[0] - 1
    best_position = min(max(float(start), 0.0), float(total))
    best_value = float(
        np.asarray(value(polyline_point(nodes, best_position)[None, :]), dtype=float)[0]
    )
    span = 1.0
    calls = 1
    for _round in range(int(rounds)):
        offsets = np.linspace(-span, span, int(probes) + 1)
        positions = best_position + offsets
        positions = positions[(positions >= 0.0) & (positions <= float(total))]
        if positions.size == 0:
            break
        points = np.array([polyline_point(nodes, position) for position in positions])
        values = np.asarray(value(points), dtype=float)
        calls += 1
        slot = int(np.argmin(values))
        if float(values[slot]) < best_value:
            best_value = float(values[slot])
            best_position = float(positions[slot])
        span *= 2.0 / float(probes)
    return best_value, best_position, calls


def path_minimum(value, left, right, samples, refine=5, probes=8):
    """Minimum de `f` sur le segment droit, echantillonne puis RAFFINE.

    Renvoie `(niveau, noeuds, evaluations)`. Les extremites sont exclues du
    minimum echantillonne : ce sont les maxima, et le niveau de col est un
    minimum INTERIEUR. Le niveau renvoye est ensuite descendu par
    `refine_along_path`, donc il est INFERIEUR OU EGAL au minimum
    echantillonne : c'est le sens exige par le paragraphe 2 du module.
    """
    times = np.linspace(0.0, 1.0, samples)
    nodes = left[None, :] + times[:, None] * (right - left)[None, :]
    values = np.asarray(value(nodes), dtype=float)
    evaluations = int(samples)
    if samples > 2:
        weakest = 1 + int(np.argmin(values[1:-1]))
    else:
        weakest = int(np.argmin(values))
    level = float(values[weakest])
    if refine > 0 and samples > 2:
        refined, _position, calls = refine_along_path(value, nodes, weakest, refine, probes)
        level = min(level, refined)
        evaluations += calls * (int(probes) + 1)
    return level, nodes, evaluations


def raise_path(value, gradient, nodes, steps, step, tolerance=1e-12):
    """Relaxation TRANSVERSE du chemin : n'accepte que ce qui MONTE le minimum.

    A chaque iteration, le noeud interieur qui realise le minimum fait un
    pas de gradient ; le pas est accepte seulement si le minimum sur le
    chemin augmente strictement, sinon il est divise par deux. La valeur
    renvoyee est donc superieure ou egale au minimum ECHANTILLONNE du chemin
    initial : c'est la recherche d'un MEILLEUR chemin, donc la moitie
    legitime du travail (paragraphe 2).

    Attention au sens : cette valeur est un minimum echantillonne, elle porte
    donc le meme biais vers le haut que `path_minimum` avant raffinement.
    C'est `_find_saddles` qui compose les deux en prenant le MAXIMUM des
    minima RAFFINES des chemins essayes, ce qui est exactement la definition
    de `lambda*` comme max sur les chemins.
    """
    nodes = np.array(nodes, dtype=float, copy=True)
    values = np.asarray(value(nodes), dtype=float).copy()
    if nodes.shape[0] <= 2:
        return float(values.min()), nodes, 0
    current = float(values[1:-1].min())
    length = float(step)
    accepted = 0
    for _iteration in range(steps):
        if length <= tolerance:
            break
        weakest = 1 + int(np.argmin(values[1:-1]))
        slope = np.asarray(gradient(nodes[weakest][None, :]), dtype=float)[0]
        norm = float(np.sqrt(slope @ slope))
        if norm <= 0.0:
            break
        candidate = nodes[weakest] + length * slope / norm
        trial = np.array(nodes, copy=True)
        trial[weakest] = candidate
        trial_values = np.asarray(value(trial), dtype=float)
        proposal = float(trial_values[1:-1].min())
        if proposal > current + tolerance:
            nodes = trial
            values = trial_values
            current = proposal
            length *= 1.6
            accepted += 1
        else:
            length *= 0.5
    return current, nodes, accepted


class SpectralTower:
    """Arbre de fusion des ensembles de sur-niveau d'un modele evaluable.

    `value(points)` renvoie `f` sur un lot `(N, d)` et `gradient(points)`
    son gradient `(N, d)`. Le modele n'a pas besoin d'etre spectral : tout
    modele evaluable convient, ce qui rend ce module testable contre une
    densite de melange exacte.
    """

    def __init__(
        self,
        cloud,
        value,
        gradient,
        step=None,
        optimiser="lbfgs",
        ascent_steps=600,
        ascent_tolerance=1e-8,
        merge_radius=None,
        path_samples=33,
        path_refine=5,
        refine_steps=0,
        refine_step=None,
        pair_limit=96,
        neighbour_count=12,
        level_digits=6,
        position_digits=4,
    ):
        cloud = np.asarray(cloud, dtype=float)
        if cloud.ndim != 2 or cloud.shape[0] < 2:
            raise ValueError("il faut une matrice (n, d) d'au moins deux observations")
        self.order = _canonical_order(cloud)
        self.cloud = cloud[self.order]
        self.count, self.dimension = self.cloud.shape
        self.centre = self.cloud.mean(axis=0)
        spread = float(np.sqrt(np.mean(np.einsum("ij,ij->i", self.cloud - self.centre,
                                                 self.cloud - self.centre))))
        self.spread = spread if spread > 0.0 else 1.0
        self.value = value
        self.gradient = gradient
        self.step = 0.05 * self.spread if step is None else float(step)
        self.optimiser = str(optimiser)
        self.ascent_steps = int(ascent_steps)
        self.ascent_tolerance = float(ascent_tolerance)
        self.merge_radius = 0.02 * self.spread if merge_radius is None else float(merge_radius)
        self.path_samples = int(path_samples)
        self.path_refine = int(path_refine)
        self.refine_steps = int(refine_steps)
        self.refine_step = (
            0.10 * self.spread if refine_step is None else float(refine_step)
        )
        self.pair_limit = int(pair_limit)
        self.neighbour_count = int(neighbour_count)
        self.level_digits = int(level_digits)
        self.position_digits = int(position_digits)
        self.path_evaluations = 0
        self.refinement_accepted = 0
        self._find_maxima()
        self._find_saddles()
        self._link()

    # -- (a) maxima ----------------------------------------------------

    def _find_maxima(self):
        slopes = np.asarray(self.gradient(self.cloud), dtype=float)
        norms = np.sqrt(np.einsum("ij,ij->i", slopes, slopes))
        self.gradient_scale = float(np.median(norms))
        if self.optimiser == "lbfgs":
            positions, values, self.ascent = joint_ascent(
                self.value,
                self.gradient,
                self.cloud,
                max_iterations=self.ascent_steps,
                tolerance=self.ascent_tolerance,
            )
        elif self.optimiser == "armijo":
            positions, values, steps_taken = gradient_ascent(
                self.value,
                self.gradient,
                self.cloud,
                self.step,
                self.ascent_steps,
                tolerance=1e-7 * self.spread,
                step_cap=0.5 * self.spread,
                gradient_floor=1e-6 * self.gradient_scale,
            )
            residual = np.asarray(self.gradient(positions), dtype=float)
            residual_norms = np.sqrt(np.einsum("ij,ij->i", residual, residual))
            self.ascent = {
                "iterations": self.ascent_steps,
                "evaluations": 2 * self.ascent_steps,
                "gradient_max": float(residual_norms.max()),
                "gradient_median": float(np.median(residual_norms)),
                "message": "armijo",
                "still_moving": int(np.count_nonzero(steps_taken > 1e-7 * self.spread)),
            }
        else:
            raise ValueError("optimiseur inconnu : " + str(self.optimiser))
        self.unconverged = int(self.ascent["gradient_max"] > self.residual_ceiling())
        if not np.all(np.isfinite(values)):
            raise AscentFailure("valeurs non finies apres ascension")
        centres, labels = _agglomerate(positions, values, self.merge_radius)
        keys = tuple(centres[:, index] for index in range(self.dimension - 1, -1, -1))
        relabel = np.lexsort(keys)
        inverse = np.empty(relabel.size, dtype=int)
        inverse[relabel] = np.arange(relabel.size)
        self.maxima = centres[relabel]
        self.basin = inverse[labels]
        self.maximum_values = np.asarray(self.value(self.maxima), dtype=float)
        self.arrival = positions
        self.maximum_count = self.maxima.shape[0]

    def residual_ceiling(self):
        """Plafond declare sur la norme residuelle du gradient aux maxima.

        Un maximum trouve n'est retenu comme point critique que si la norme
        de son gradient est tombee sous `1 e-4` fois la norme MEDIANE mesuree
        aux observations : un plancher relatif, sans constante absolue. Le
        drapeau `unconverged` de l'enregistrement dit si ce plafond est
        franchi, et le fait mesure documente dans `gradient_ascent` dit ce
        qu'il se passe quand on l'ignore.
        """
        return 1e-4 * max(self.gradient_scale, 1e-300)

    # -- (b) cols ------------------------------------------------------

    def _candidate_pairs(self):
        total = self.maximum_count
        if total * (total - 1) // 2 <= self.pair_limit * self.pair_limit:
            self.pairs_restricted = False
            return [(left, right) for left in range(total) for right in range(left + 1, total)]
        self.pairs_restricted = True
        squared = np.einsum("ij,ij->i", self.maxima, self.maxima)
        matrix = squared[:, None] + squared[None, :] - 2.0 * self.maxima @ self.maxima.T
        np.fill_diagonal(matrix, np.inf)
        keep = min(self.neighbour_count, total - 1)
        pairs = set()
        for left in range(total):
            for right in np.argsort(matrix[left])[:keep]:
                pairs.add((min(left, int(right)), max(left, int(right))))
        order = np.argsort(matrix.ravel())
        for flat in order:
            left, right = divmod(int(flat), total)
            if left < right:
                pairs.add((left, right))
                if len(pairs) > self.pair_limit * self.pair_limit:
                    break
        return sorted(pairs)

    def _find_saddles(self):
        self.saddle = {}
        for left, right in self._candidate_pairs():
            level, nodes, evaluations = path_minimum(
                self.value,
                self.maxima[left],
                self.maxima[right],
                self.path_samples,
                self.path_refine,
            )
            self.path_evaluations += evaluations
            if self.refine_steps > 0:
                raised, relaxed, accepted = raise_path(
                    self.value,
                    self.gradient,
                    nodes,
                    self.refine_steps,
                    self.refine_step,
                )
                self.refinement_accepted += accepted
                # Chaque iteration de `raise_path` evalue tout le jeu de
                # noeuds : le compteur suit les evaluations REELLES, il n'est
                # pas une annonce.
                self.path_evaluations += (1 + self.refine_steps) * relaxed.shape[0]
                if self.path_refine > 0 and relaxed.shape[0] > 2:
                    values = np.asarray(self.value(relaxed), dtype=float)
                    weakest = 1 + int(np.argmin(values[1:-1]))
                    refined, _position, calls = refine_along_path(
                        self.value, relaxed, weakest, self.path_refine
                    )
                    self.path_evaluations += relaxed.shape[0] + 9 * calls
                    raised = min(float(values[weakest]), refined)
                # `lambda*` est un MAX sur les chemins : on garde le meilleur
                # minorant des deux chemins essayes, jamais le dernier calcule.
                level = max(level, raised)
            self.saddle[(left, right)] = level

    # -- (c) liaison simple --------------------------------------------

    def _link(self):
        edges = sorted(
            ((-level, left, right) for (left, right), level in self.saddle.items())
        )
        forest = _Forest(self.maximum_count)
        merges = []
        for negative, left, right in edges:
            root_left, root_right = forest.find(left), forest.find(right)
            if root_left == root_right:
                continue
            forest.union(left, right)
            merges.append((-negative, min(root_left, root_right), max(root_left, root_right)))
        self.merges = merges

    # -- lecture -------------------------------------------------------

    def partition(self, classes):
        """Etiquettes des maxima pour une coupe a `classes` composantes."""
        if classes < 1:
            raise ValueError("il faut au moins une classe")
        applied = max(0, self.maximum_count - classes)
        forest = _Forest(self.maximum_count)
        for level, left, right in self.merges[:applied]:
            del level
            forest.union(left, right)
        roots = {}
        labels = np.empty(self.maximum_count, dtype=int)
        for index in range(self.maximum_count):
            root = forest.find(index)
            if root not in roots:
                roots[root] = len(roots)
            labels[index] = roots[root]
        return labels

    def labels(self, classes):
        """Etiquettes des observations, dans l'ORDRE D'ENTREE du nuage."""
        maximum_labels = self.partition(classes)
        canonical = maximum_labels[self.basin]
        restored = np.empty(self.count, dtype=int)
        restored[self.order] = canonical
        return restored

    def achieved_classes(self, classes):
        """Nombre de classes reellement obtenu par `partition(classes)`.

        Ce n'est pas `min(classes, maximum_count)` : quand le graphe des
        paires candidates n'est pas connexe (possible des que
        `pairs_restricted` est vrai, donc au-dela de `pair_limit ** 2` paires),
        il manque des fusions et la coupe rend PLUS de classes que demande.
        Le compte honnete est donc le nombre de maxima moins le nombre de
        fusions REELLEMENT disponibles. Dans le cas connexe
        (`len(merges) == maximum_count - 1`) cette expression redonne
        exactement `min(classes, maximum_count)`.
        """
        wanted = max(0, self.maximum_count - max(1, int(classes)))
        return int(self.maximum_count - min(len(self.merges), wanted))

    def gap_classes(self, max_classes=12):
        """Nombre de classes propose spontanement par le plus grand saut.

        Regle declaree, ecrite pour etre applicable TELLE QUELLE a une tour
        dont l'axe va dans l'autre sens (c'est la condition pour que la
        comparaison de `bench/spectral_tower.py` soit juste) : appliquer les
        `j` premieres fusions laisse `P - j` composantes, et le saut qui
        justifie de s'arreter a `j` est `|niveau[j] - niveau[j-1]|`. On
        maximise ce saut sur un nombre de classes compris entre 2 et
        `max_classes`.

        Cette regle est FAIBLE, et c'est mesure : sur un reseau de centres
        equidistants elle propose 2 classes quand la verite est 4, parce que
        les dernieres fusions sont a des niveaux voisins. Le nombre de
        maxima trouves (`maximum_count`) et la regle de persistance de la
        sonde sont de meilleurs indicateurs ; celle-ci reste comme temoin
        minimal, sans modele.

        DOMAINE D'ARRIVEE, declare parce qu'il n'est pas `2 .. max_classes` :
        le saut a `c` classes demande `levels[P - c]` ET `levels[P - c - 1]`,
        donc `c = P` (aucune fusion appliquee, pas de niveau precedent) est
        inatteignable, et avec exactement deux maxima la regle ne peut jamais
        proposer 2. La valeur renvoyee vit donc dans `1 .. P - 1`, et le `1`
        signifie « aucun saut exploitable », pas « une seule composante ».
        C'est mesure dans la sonde : une cellule a `P = 2` y affiche
        `gap_s = 1` a cote de `per_s = 2`.
        """
        levels = [level for level, _left, _right in self.merges]
        total = self.maximum_count
        best_classes, best_gap = 1, -1.0
        for classes in range(2, min(max_classes, total - 1) + 1):
            index = total - classes
            if index < 1 or index > len(levels) - 1:
                continue
            gap = abs(float(levels[index]) - float(levels[index - 1]))
            if gap > best_gap:
                best_gap, best_classes = gap, classes
        return best_classes

    def equivalent_order(self, level, radius_squared):
        """Ordre `k` de HGP equivalent a un niveau `lambda`, a rayon donne.

        Inverse de `order_to_log_density_level` : calibration explicite de
        l'axe de niveau vers l'axe d'ordre (paragraphe 1).
        """
        if radius_squared <= 0.0:
            raise ValueError("rayon au carre positif exige")
        log_unit = 0.5 * self.dimension * math.log(math.pi) - math.lgamma(
            0.5 * self.dimension + 1.0
        )
        exponent = (
            level
            + math.log(self.count)
            + log_unit
            + 0.5 * self.dimension * math.log(radius_squared)
        )
        return math.exp(exponent)

    def canonical_record(self):
        """Enregistrement canonique JSON-serialisable de la tour."""
        relative = self.maxima - self.centre
        maxima = [
            {
                "index": index,
                "level": _quantise(self.maximum_values[index], self.level_digits),
                "position": [
                    _quantise(component, self.position_digits) for component in relative[index]
                ],
                "size": int(np.count_nonzero(self.basin == index)),
            }
            for index in range(self.maximum_count)
        ]
        merges = [
            {
                "level": _quantise(level, self.level_digits),
                "left": int(left),
                "right": int(right),
            }
            for level, left, right in self.merges
        ]
        return {
            "object": "ehgp.spectral_tower.v1",
            "count": self.count,
            "dimension": self.dimension,
            "maximum_count": self.maximum_count,
            "path_samples": self.path_samples,
            "path_refine": self.path_refine,
            "refine_steps": self.refine_steps,
            "pairs_restricted": bool(getattr(self, "pairs_restricted", False)),
            "unconverged": self.unconverged,
            "optimiser": self.optimiser,
            "level_digits": self.level_digits,
            "position_digits": self.position_digits,
            "maxima": maxima,
            "merges": merges,
            "basin": [int(label) for label in self.basin],
        }

    def structural_record(self):
        """Enregistrement sans position : topologie, niveaux, partition."""
        record = self.canonical_record()
        return {
            "object": "ehgp.spectral_tower.structure.v1",
            "count": record["count"],
            "dimension": record["dimension"],
            "maximum_count": record["maximum_count"],
            "levels": sorted(item["level"] for item in record["maxima"]),
            "merges": [[item["level"], item["left"], item["right"]] for item in record["merges"]],
            "basin": record["basin"],
            "sizes": sorted(item["size"] for item in record["maxima"]),
        }

    def digest(self):
        """Digest canonique sha256 de la tour complete."""
        return self._digest_of(self.canonical_record())

    def structural_digest(self):
        """Digest canonique sha256 de la seule structure."""
        return self._digest_of(self.structural_record())

    @staticmethod
    def _digest_of(record):
        payload = json.dumps(
            record, sort_keys=True, separators=(",", ":"), ensure_ascii=True
        )
        return hashlib.sha256(payload.encode("ascii")).hexdigest()


def from_model(cloud, model, target="log_density", **options):
    """Tour du modele spectral : raccourci sur `SpectralTower`.

    `target` vaut `log_density` (defaut, l'objet calibre du paragraphe 1)
    ou `log_ratio` (le potentiel brut `log dp/dq`).
    """
    if target == "log_density":
        value, gradient = model.log_density, model.gradient_log_density
    elif target == "log_ratio":
        value, gradient = model.evaluate, model.gradient
    else:
        raise ValueError("cible inconnue : " + str(target))
    return SpectralTower(cloud, value, gradient, **options)
