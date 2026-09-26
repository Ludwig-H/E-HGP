"""Tour E-HGP projetee sur les observations : version flottante vectorisee.

Meme objet que `engine/point_tower.py` (la projection sur `X` de la tour
ordre-echelle), mais calcule en flottant par blocs `numpy`, donc utilisable
jusqu'a `n = 2000` la ou la version rationnelle plafonne vers `n = 40`.

BRIQUE PORTEE. Le long du segment `y(t) = x_i + t (x_j - x_i)`, les `n`
energies `e_l(t) = ||y(t) - x_l||^2 = A t^2 + B_l t + C_l` PARTAGENT le
coefficient dominant `A = ||x_j - x_i||^2`. Donc

    a_k(y(t)) = A t^2 + g_k(t) ,

ou `g_k` est la k-ieme plus petite de `n` fonctions AFFINES : affine par
morceaux, ruptures aux croisements de droites. Sur chaque morceau la
fonction est convexe, donc le maximum de `a_k` sur `[0, 1]` est atteint en
`t = 0`, `t = 1`, ou a un croisement (fait etabli par `engine/segment.py`).
Tout se lit dans la matrice des distances au carre :
`A = D_ij`, `C_l = D_il`, `B_l = D_jl - D_il - D_ij`, ce qui rend le moteur
libre en dimension au sens strict : `d` n'apparait QUE dans le produit de
Gram initial, en `O(n^2 d)`.

COMPROMIS RETENU, et pourquoi. Trois voies etaient possibles.

1. TOUS LES CROISEMENTS : exact, mais `O(n^2)` temps candidats par segment,
   donc `O(n^4)` pour la matrice complete. Implemente ici sous le nom
   `crossing_maximum`, reserve aux petits nuages : c'est le temoin flottant
   de la version rationnelle, pas le moteur.
2. BALAYAGE EVENEMENTIEL au bord du rang `k`. Le bas-de-liste `S_k(t)` ne
   change qu'aux croisements entre un membre et un non-membre ; au premier
   tel instant les deux lignes sont exactement la k-ieme et la (k+1)-ieme.
   Le nombre de ruptures de `g_k` est `O(n k^{1/3})`, mais chaque evenement
   coute `O(k (n - k))` a localiser et la boucle est sequentielle par
   segment : invectorisable telle quelle, donc ecartee pour ce chantier.
3. BALAYAGE ECHANTILLONNE A ENCADREMENT CERTIFIE : la voie retenue, seule
   entierement vectorisable. Elle ne rend pas un nombre mais un INTERVALLE
   `[w_lo, w_hi]` qui contient le maximum exact, avec un certificat
   d'exactitude quand l'intervalle est reduit a un point.

ENCADREMENT (demontre, pas mesure). Soit `0 = t_0 < ... < t_m = 1` les temps
d'echantillonnage et `S_r` l'ensemble des indices de plus petite energie au
temps `t_r`, de cardinal au moins `k`.

* MINORANT. `max_r a_k(y(t_r)) <= w` : un maximum sur un sous-ensemble de
  temps. En particulier `w >= max(a_k(x_i), a_k(x_j))`, borne toujours
  disponible puisque `t = 0` et `t = 1` sont des temps d'echantillonnage.
* MAJORANT. Sur `[t_r, t_{r+1}]`, comme `|S_r| >= k`,
  `a_k(y(t)) <= h_r(t) = max_{l dans S_r} e_l(t)`. Chaque `e_l` est convexe
  et toutes partagent `A`, donc `h_r` est convexe : son maximum sur
  l'intervalle est a une extremite. Or `h_r(t_r) = a_k(y(t_r))` exactement,
  puisque `S_r` est le bas-de-liste au temps `t_r`. D'ou
  `max_{[t_r, t_{r+1}]} a_k <= max(a_k(y(t_r)), M_r)` avec
  `M_r = max_{l dans S_r} e_l(t_{r+1})`, et symetriquement en partant de
  `t_{r+1}`. On retient le minimum des deux, puis le maximum sur `r`.
* CERTIFICAT. Si le bas-de-liste ne change pas sur `[t_r, t_{r+1}]` alors
  `M_r = a_k(y(t_{r+1}))` et la borne de cet intervalle vaut
  `max(a_k(t_r), a_k(t_{r+1}))`, deja atteinte par le minorant. Donc
  `w_hi = w_lo` implique `w = w_lo` : l'egalite des deux bornes est une
  PREUVE d'exactitude, verifiable a posteriori couple par couple. Aucun
  couple n'est declare exact sans elle. En flottant cette egalite est une
  egalite a quelques ULP pres : les couples ou l'arrondi inverse meme
  l'encadrement sont comptes et publies (`inverted_pairs`), et l'inversion
  RELATIVE maximale aussi (`order_violation_relative`).

RESTRICTION PAR TUBE (ce qui rend le calcul abordable). Le maximum ne depend
que des observations proches du segment, et la selection du bas-de-liste sur
les `n` colonnes a chaque temps est le seul poste couteux. Deux faits :

* MONOTONIE PAR SOUS-NUAGE. Pour `C` inclus dans `X`, `a_k^C >= a_k^X`
  (moins d'observations, k-ieme plus petite plus grande). Donc le MAJORANT
  calcule sur un sous-nuage `C` reste un majorant de `w` : le poids publie
  est valide quel que soit `C`.
* CERTIFICAT DE TUBE. Soit `U` un majorant de `w` et
  `q = min_{l hors de C} min_{t} e_l(t)` la distance au carre du segment a
  l'observation la plus proche hors de `C`. Si `q > U` alors
  `a_k^C = a_k^X` en tout point du segment : pour tout `t`, les membres du
  bas-de-liste verifient `e_l(t) <= a_k^X(t) <= w <= U < q`, donc ils sont
  dans `C`. Le MINORANT restreint devient alors lui aussi valide, et le
  certificat d'exactitude se transporte.

Le moteur prend donc `C_ij = kNN(x_i) union kNN(x_j)` (largeur `width`),
calcule l'encadrement sur `C_ij` (colonnes peu nombreuses, donc tri complet
vectorise), puis certifie le tube en UN passage `O(n)` par couple. Les
couples non certifies gardent un majorant valide et retombent, pour le
minorant, sur la borne d'extremites `max(a_k(x_i), a_k(x_j))`, toujours
valide. La part des couples certifies est publiee, jamais supposee.
En `mode="full"` toutes les colonnes sont gardees : c'est le chemin de
reference, et sur les couples certifies le mode tube rend des flottants
BIT-A-BIT identiques, parce que la formule est la meme, le jeu des `k` plus
petites valeurs est le meme, et les egalites sont departagees par l'indice
du point (voir `_selection` : sans ce departage, le milieu d'un segment, ou
les deux extremites sont a egale distance, suffit a faire diverger deux
majorants tous deux valides).

NATURE DU RESULTAT. Le poids publie est `w_hi` : comme dans
`engine/point_tower.py`, un segment contenu dans `L_k(a)` prouve que ses
extremites sont dans la meme composante, donc le niveau de fusion vrai est
AU PLUS le maximum du segment, lui-meme au plus `w_hi`. La tour rendue est
un MAJORANT certifie de la projection exacte, jamais une approximation non
gardee. La garantie est ecrite en arithmetique reelle ; en flottant elle
tient a l'arrondi pres, et `certify_against_exact` mesure la violation
maximale contre la version rationnelle de `engine/segment.py`.
"""

import hashlib

import numpy as np

from .segment import rational_cloud, segment_maximum


def as_cloud(points):
    """Nuage `numpy` float64 de forme `(n, d)`."""
    cloud = np.asarray(points, dtype=np.float64)
    if cloud.ndim != 2:
        raise ValueError("le nuage doit etre de forme (n, d)")
    if cloud.shape[0] < 2:
        raise ValueError("au moins deux observations sont requises")
    return cloud


def squared_distances(cloud):
    """Matrice des `||x_i - x_l||^2` : seul endroit ou la dimension entre."""
    cloud = as_cloud(cloud)
    gram = cloud @ cloud.T
    norms = np.diag(gram).copy()
    distances = norms[:, None] - 2.0 * gram + norms[None, :]
    distances = 0.5 * (distances + distances.T)
    np.maximum(distances, 0.0, out=distances)
    np.fill_diagonal(distances, 0.0)
    return distances


def nearest_table(distances, span):
    """`(indices, valeurs)` des `span` plus proches de chaque observation.

    Les valeurs triees donnent directement les niveaux d'entree :
    `valeurs[i, k - 1] = a_k(x_i)`, la k-ieme distance au carre, avec
    `a_1(x_i) = 0` puisque l'observation se compte elle-meme (convention de
    la specification ; c'est la distance de coeur de HDBSCAN pour
    `minPts = k`, au carre).
    """
    count = distances.shape[0]
    reach = min(int(span), count)
    # Meme departage canonique que `_selection`, et pour la meme raison : a
    # distance egale la colonne de plus petit indice gagne, sinon le tube
    # `C_ij` depend du chemin de calcul.
    keys = distances + 1j * np.arange(count, dtype=np.float64)
    picked = np.argpartition(keys, reach - 1, axis=1)[:, :reach]
    ranking = np.argsort(np.take_along_axis(keys, picked, axis=1), axis=1)
    index = np.take_along_axis(picked, ranking, axis=1)
    return index, np.take_along_axis(distances, index, axis=1)


def entry_levels(cloud, k_max):
    """`entry[i, k - 1] = a_k(x_i)` pour `k = 1..k_max`."""
    distances = squared_distances(cloud)
    return nearest_table(distances, k_max)[1]


def upper_pairs(count):
    """Indices `(ii, jj)` du triangle superieur strict, ordre lexicographique."""
    return np.triu_indices(count, k=1)


def sample_times(intervals):
    """Temps d'echantillonnage : `intervals + 1` valeurs dont `0`, `1/2`, `1`.

    Le nombre d'intervalles est force pair pour que `t = 1/2` soit un temps
    d'echantillonnage : c'est l'argmax exact a l'ordre `k = 1` quand aucune
    autre observation n'intervient, donc le regime de reference.
    """
    if intervals < 2:
        raise ValueError("au moins deux intervalles sont requis")
    if intervals % 2 == 1:
        intervals += 1
    return np.linspace(0.0, 1.0, intervals + 1)


def _selection(energies, order_max):
    """Les `order_max` plus petites energies par ligne, triees, avec indices.

    Deux regimes : tri complet quand les colonnes sont peu nombreuses (le
    tri vectorise de `numpy` est alors bien plus rapide qu'une selection
    partielle indirecte), selection partielle sinon.

    DEPARTAGE CANONIQUE. A energie egale, la colonne de plus petit indice
    gagne. Ce n'est pas cosmetique : au milieu d'un segment les deux
    extremites sont a EGALE distance, donc le bas-de-liste est ambigu, et
    deux ambiguites differentes donnent deux majorants differents (tous deux
    valides). Le departage rend la sortie independante du chemin de calcul,
    donc identique en `mode="tube"` et en `mode="full"`.

    Le departage doit porter sur la SELECTION elle-meme, pas seulement sur
    l'ordre des colonnes retenues. Une selection partielle appliquee aux
    energies choisit un jeu arbitraire parmi les colonnes ex aequo au rang
    `order_max` ; les retrier ensuite ne repare rien, puisque la colonne
    perdue n'est plus la. Fixture : nuage entier `d = 5`, `n = 44`, couple
    `(1, 28)`, ordre `10`, ou deux colonnes portent l'energie `18` au temps
    `t = 1` ; le mode complet publiait `18.4375` et le mode tube `19` (les
    deux valides, le maximum exact etant `18.08`). La selection partielle
    porte donc sur la CLE lexicographique `energie + i * indice`, qui est un
    ordre total sans ex aequo : `numpy` trie et partitionne les complexes
    lexicographiquement, donc le jeu retenu est le jeu canonique. Le tri
    complet, lui, est stable, donc deja canonique sans cle.
    """
    width = energies.shape[1]
    if width <= 4 * order_max:
        index = np.argsort(energies, axis=1, kind="stable")[:, :order_max]
    else:
        keys = energies + 1j * np.arange(width, dtype=np.float64)
        picked = np.argpartition(keys, order_max - 1, axis=1)[:, :order_max]
        index = np.take_along_axis(
            picked,
            np.argsort(np.take_along_axis(keys, picked, axis=1), axis=1),
            axis=1,
        )
    return index, np.take_along_axis(energies, index, axis=1)


def _block_bracket(distances, left, right, columns, blocked, orders, times):
    """Encadrement d'un bloc de couples sur un jeu de colonnes par ligne.

    `columns` est `(P, W)` : les observations retenues pour chaque couple.
    `blocked` marque les colonnes a ignorer (doublons de la reunion des deux
    listes de voisins) ; elles recoivent une energie infinie a tout temps,
    donc ne peuvent entrer dans aucun bas-de-liste.
    """
    order_max = orders[-1]
    leading = distances[left, right]
    base = distances[left[:, None], columns]
    far = distances[right[:, None], columns]
    slope = far - base - leading[:, None]
    if blocked is not None:
        base = np.where(blocked, np.inf, base)
        slope = np.where(blocked, 0.0, slope)
    size = left.size
    lower = np.zeros((len(orders), size), dtype=np.float64)
    upper = np.zeros((len(orders), size), dtype=np.float64)
    previous_energies = None
    previous_index = None
    previous_values = None
    for time in times:
        energies = base + time * slope
        energies += leading[:, None] * (time * time)
        np.maximum(energies, 0.0, out=energies)
        index, values = _selection(energies, order_max)
        for slot, order in enumerate(orders):
            np.maximum(lower[slot], values[:, order - 1], out=lower[slot])
        if previous_energies is not None:
            forward = np.maximum.accumulate(
                np.take_along_axis(energies, previous_index, axis=1), axis=1
            )
            backward = np.maximum.accumulate(
                np.take_along_axis(previous_energies, index, axis=1), axis=1
            )
            for slot, order in enumerate(orders):
                ahead = np.maximum(previous_values[:, order - 1], forward[:, order - 1])
                behind = np.maximum(values[:, order - 1], backward[:, order - 1])
                np.minimum(ahead, behind, out=ahead)
                np.maximum(upper[slot], ahead, out=upper[slot])
        previous_energies = energies
        previous_index = index
        previous_values = values
    return lower, upper


def _tube_clearance(distances, left, right, columns):
    """`q` : distance au carre du segment a l'observation la plus proche hors `C`.

    Pour chaque couple, `min_t e_l(t)` a la forme close
    `A t*^2 + B_l t* + C_l` avec `t* = clip(-B_l / (2 A), 0, 1)`. Les
    colonnes de `C` sont ecartees en les poussant a l'infini.
    """
    leading = distances[left, right]
    base = distances[left, :].copy()
    slope = distances[right, :] - base - leading[:, None]
    safe = np.where(leading > 0.0, leading, 1.0)[:, None]
    station = np.clip(-slope / (2.0 * safe), 0.0, 1.0)
    value = base + station * slope
    value += leading[:, None] * station * station
    np.maximum(value, 0.0, out=value)
    np.put_along_axis(value, columns, np.inf, axis=1)
    return value.min(axis=1)


def segment_brackets(
    points,
    orders,
    intervals=8,
    mode="tube",
    width=None,
    certify=True,
    budget=4_000_000,
):
    """Encadrement certifie du maximum de `a_k` sur tous les segments.

    Renvoie un dictionnaire : `lower` et `upper` sont des matrices
    `(len(orders), n (n - 1) / 2)` indexees comme les distances condensees
    de `scipy`, avec `lower <= w_ij^{(k)} <= upper` ; `tube_certified` dit
    pour quels couples le tube est certifie (donc le minorant restreint
    valide et le certificat d'exactitude transportable) ; `exact` dit quels
    couples sont PROUVES exacts (`upper == lower` et tube certifie).
    """
    cloud = as_cloud(points)
    count = cloud.shape[0]
    wanted = sorted({int(order) for order in orders})
    if wanted[0] < 1:
        raise ValueError("les ordres doivent etre au moins 1")
    wanted = tuple(order for order in wanted if order <= count)
    order_max = wanted[-1]
    if mode not in ("tube", "full"):
        raise ValueError("mode inconnu : " + str(mode))
    distances = squared_distances(cloud)
    span = order_max + 6 if width is None else int(width)
    span = max(order_max, min(span, count))
    neighbours, entry = nearest_table(distances, max(span, order_max))
    rows, columns = upper_pairs(count)
    pairs = rows.size
    times = sample_times(intervals)
    lower = np.zeros((len(wanted), pairs), dtype=np.float64)
    upper = np.zeros((len(wanted), pairs), dtype=np.float64)
    certified = np.zeros((len(wanted), pairs), dtype=bool)
    block = max(1, int(budget // max(1, count)))
    blocks = 0
    for start in range(0, pairs, block):
        stop = min(pairs, start + block)
        blocks += 1
        left = rows[start:stop]
        right = columns[start:stop]
        if mode == "full":
            chosen = np.broadcast_to(np.arange(count), (left.size, count))
            blocked = None
        else:
            chosen = np.concatenate(
                [neighbours[left, :span], neighbours[right, :span]], axis=1
            )
            chosen = np.sort(chosen, axis=1)
            blocked = np.zeros(chosen.shape, dtype=bool)
            blocked[:, 1:] = chosen[:, 1:] == chosen[:, :-1]
        low, high = _block_bracket(
            distances, left, right, chosen, blocked, wanted, times
        )
        ends = np.maximum(entry[left][:, :order_max], entry[right][:, :order_max])
        if mode == "full":
            clear = np.full(left.size, np.inf)
        elif certify:
            clear = _tube_clearance(distances, left, right, chosen)
        else:
            clear = np.zeros(left.size)
        for slot, order in enumerate(wanted):
            # Certificat par ordre : le tube du couple doit degager le
            # majorant DE CET ORDRE, ce qui est plus fin qu'un certificat
            # commun pilote par l'ordre le plus grand.
            keep = clear > high[slot]
            certified[slot, start:stop] = keep
            floor = ends[:, order - 1]
            lower[slot, start:stop] = np.where(keep, np.maximum(low[slot], floor), floor)
            upper[slot, start:stop] = high[slot]
    # Les deux bornes sont valides en toute circonstance : le minorant vaut au
    # pire la borne d'extremites, le majorant est valide pour tout sous-nuage.
    # Leur egalite est donc a elle seule une preuve, sans certificat de tube.
    #
    # EN FLOTTANT, cette preuve est une egalite A QUELQUES ULP PRES, et il faut
    # le dire : en arithmetique reelle `upper >= lower` toujours (chaque borne
    # d'intervalle majore le `a_k` d'un temps d'echantillonnage), donc une
    # inversion stricte `upper < lower` ne peut venir que de l'arrondi. Ces
    # couples sont comptes et publies (`inverted_pairs`), et l'inversion
    # relative maximale l'est aussi (`order_violation_relative`) : c'est elle,
    # et non une borne absolue, qui est le bon garde-fou, puisque l'echelle des
    # poids varie de `10^2` a `10^4` selon la dimension. Mesure du 26 septembre
    # 2026 sur la famille variete, `n = 200`, mode complet : inversion relative
    # au pire `1.0e-15`, et de `0.2` a `3.6` pour cent des couples declares
    # exacts le sont par inversion stricte plutot que par egalite franche.
    exact = upper <= lower
    scale = np.maximum(np.abs(upper), 1e-300)
    gaps = (upper - lower) / scale
    inverted = upper < lower
    return {
        "orders": wanted,
        "count": count,
        "mode": mode,
        "width": int(span),
        "intervals": int(times.size - 1),
        "pairs": int(pairs),
        "blocks": int(blocks),
        "lower": lower,
        "upper": upper,
        "entry": entry[:, :order_max],
        "tube_certified": certified,
        "exact": exact,
        "tube_fraction": tuple(float(row.mean()) for row in certified),
        "exact_fraction": tuple(float(row.mean()) for row in exact),
        "gap_max": tuple(float(row.max()) for row in gaps),
        "gap_mean": tuple(float(row.mean()) for row in gaps),
        "order_violation": float(np.min(upper - lower)),
        "order_violation_relative": float(np.min(gaps)),
        "inverted_pairs": int(inverted.sum()),
        "inverted_exact_pairs": int(np.sum(inverted & exact)),
    }


def crossing_maximum(points, source, target, order):
    """Maximum flottant de `a_order` sur `[x_source, x_target]`, TOUS croisements.

    Temoin exact a l'arrondi pres de `engine/segment.py`, en `O(n^2)` temps
    candidats : jamais un moteur, seulement un juge pour petits nuages.
    """
    cloud = as_cloud(points)
    origin = cloud[source]
    direction = cloud[target] - origin
    leading = float(direction @ direction)
    offsets = origin[None, :] - cloud
    linear = 2.0 * (offsets @ direction)
    constant = np.einsum("ij,ij->i", offsets, offsets)
    slope_gap = linear[:, None] - linear[None, :]
    intercept_gap = constant[None, :] - constant[:, None]
    usable = np.abs(slope_gap) > 0.0
    crossings = np.where(usable, intercept_gap / np.where(usable, slope_gap, 1.0), -1.0)
    inside = crossings[(crossings > 0.0) & (crossings < 1.0)]
    candidates = np.concatenate([np.array([0.0, 1.0]), inside])
    values = leading * candidates * candidates + np.partition(
        candidates[:, None] * linear[None, :] + constant[None, :], order - 1, axis=1
    )[:, order - 1]
    best = int(np.argmax(values))
    return float(values[best]), float(candidates[best])


def certify_against_exact(points, orders, intervals=8, mode="tube", width=None):
    """Confronte l'encadrement flottant a la version rationnelle exacte.

    Renvoie les compteurs de la confrontation : couples examines, couples
    dont le maximum exact tombe dans `[lower, upper]`, couples certifies
    exacts dont la valeur est bien la bonne, violations relatives maximales
    des deux bornes, accord du temoin `crossing_maximum`.
    """
    cloud = as_cloud(points)
    count = cloud.shape[0]
    bracket = segment_brackets(
        cloud, orders, intervals=intervals, mode=mode, width=width
    )
    rounded = [tuple(int(round(value)) for value in row) for row in cloud]
    integral = np.array_equal(cloud, np.array(rounded, dtype=np.float64))
    # Le nuage rationnel reprend EXACTEMENT les flottants du moteur
    # (`Fraction` d'un `float` est exacte) : la confrontation ne melange donc
    # pas deux nuages. Les coordonnees entieres sont preferees quand elles
    # existent, pour garder des denominateurs courts.
    exact_cloud = rational_cloud(rounded if integral else cloud.tolist())
    rows, columns = upper_pairs(count)
    checked = 0
    inside = 0
    certified = 0
    certified_good = 0
    witness_agree = 0
    low_violation = 0.0
    high_violation = 0.0
    for slot, order in enumerate(bracket["orders"]):
        for position in range(rows.size):
            source = int(rows[position])
            target = int(columns[position])
            reference, _time = segment_maximum(exact_cloud, source, target, order)
            truth = float(reference)
            low = float(bracket["lower"][slot, position])
            high = float(bracket["upper"][slot, position])
            window = max(1.0, abs(truth))
            checked += 1
            if low <= truth + 1e-9 * window and truth <= high + 1e-9 * window:
                inside += 1
            low_violation = max(low_violation, (low - truth) / window)
            high_violation = max(high_violation, (truth - high) / window)
            if bool(bracket["exact"][slot, position]):
                certified += 1
                if abs(truth - low) <= 1e-9 * window:
                    certified_good += 1
            witness, _at = crossing_maximum(cloud, source, target, order)
            if abs(witness - truth) <= 1e-9 * window:
                witness_agree += 1
    return {
        "orders": bracket["orders"],
        "count": count,
        "mode": mode,
        "intervals": bracket["intervals"],
        "checked": checked,
        "inside": inside,
        "certified": certified,
        "certified_good": certified_good,
        "witness_agree": witness_agree,
        "low_violation": float(low_violation),
        "high_violation": float(high_violation),
        "tube_fraction": bracket["tube_fraction"],
        "exact_fraction": bracket["exact_fraction"],
        "gap_max": bracket["gap_max"],
    }


def mutual_reachability(entry, distances, order, quarter=True):
    """Poids de reachability mutuelle facon HDBSCAN, POUR COMPARAISON SEULEMENT.

    `mr_ij = max(a_k(x_i), a_k(x_j), rho D_ij)` avec `rho = 1/4` si
    `quarter` (l'echelle de la tour : a l'ordre `k = 1` le maximum de `a_1`
    sur le segment vaut `D_ij / 4`, atteint au milieu, des qu'aucune autre
    observation n'intervient), sinon `rho = 1` (l'echelle usuelle de
    HDBSCAN, au carre).

    CE N'EST PAS LE MEME OBJET que la tour E-HGP. La tour demande que le
    SEGMENT ENTIER soit couvert `k` fois, c'est-a-dire
    `max_t a_k(y(t)) <= a` ; la reachability mutuelle ne regarde que les
    deux extremites et leur distance, donc ignore tout ce que le segment
    traverse. Les deux coincident a l'ordre `k = 1` avec `quarter=True` hors
    intrusion, et divergent des que `k > 1` : a l'ordre `k`, un segment peut
    quitter `L_k` en son milieu alors que ses deux extremites y sont bien
    installees. Confronter les deux est exactement ce que mesure
    `bench/clustering_compare.py`.
    """
    entry = np.asarray(entry, dtype=np.float64)
    core = entry[:, order - 1]
    count = core.size
    rows, columns = upper_pairs(count)
    base = distances[rows, columns]
    if quarter:
        base = base / 4.0
    return np.maximum(base, np.maximum(core[rows], core[columns]))


class LinkageTower:
    """Arbre de liaison simple sur des poids condenses, plus des naissances.

    Un seul objet sert aux trois familles comparees : tour E-HGP
    (poids = maximum de segment), reachability mutuelle (poids = `mr`),
    liaison simple euclidienne (ordre `k = 1`, naissances nulles). Les
    naissances `a_k(x_i)` font partie de l'objet : une observation
    n'appartient a `L_k(a)` qu'a partir de `a_k(x_i)`, donc en dessous elle
    n'est dans AUCUNE composante. C'est le seul mecanisme de rejet de bruit
    de la tour, et il est commun a E-HGP et a `mr` au meme ordre : seule la
    regle de liaison les distingue.
    """

    def __init__(self, weights, births, count):
        self.count = int(count)
        self.births = np.asarray(births, dtype=np.float64).ravel()
        rows, columns = upper_pairs(self.count)
        weights = np.asarray(weights, dtype=np.float64)
        ranking = np.argsort(weights, kind="stable")
        self.edge_level = weights[ranking]
        self.edge_left = rows[ranking]
        self.edge_right = columns[ranking]
        parent = list(range(self.count))

        def find(item):
            root = item
            while parent[root] != root:
                root = parent[root]
            while parent[item] != root:
                parent[item], item = root, parent[item]
            return root

        levels = []
        self.merge_edges = []
        for position in range(self.edge_level.size):
            left = find(int(self.edge_left[position]))
            right = find(int(self.edge_right[position]))
            if left == right:
                continue
            parent[max(left, right)] = min(left, right)
            levels.append(float(self.edge_level[position]))
            self.merge_edges.append(position)
            if len(self.merge_edges) == self.count - 1:
                break
        self.merge_level = np.asarray(levels, dtype=np.float64)

    def _union_find(self, limit=None, level=None):
        parent = list(range(self.count))

        def find(item):
            root = item
            while parent[root] != root:
                root = parent[root]
            while parent[item] != root:
                parent[item], item = root, parent[item]
            return root

        chosen = self.merge_edges if limit is None else self.merge_edges[:limit]
        for position in chosen:
            if level is not None and float(self.edge_level[position]) > level:
                break
            left = find(int(self.edge_left[position]))
            right = find(int(self.edge_right[position]))
            if left != right:
                parent[max(left, right)] = min(left, right)
        return [find(index) for index in range(self.count)]

    def labels_fixed(self, clusters):
        """Coupe a exactement `clusters` groupes : tous les points etiquetes."""
        wanted = max(1, min(int(clusters), self.count))
        roots = self._union_find(limit=self.count - wanted)
        return self._relabel(roots, None)

    def labels_at(self, level, min_size=1):
        """Etiquettes au niveau `level` : naissances respectees, non-nes a `-1`.

        Un groupe de moins de `min_size` observations nees est declare bruit.
        C'est le seul parametre de forme, et il est le meme pour toutes les
        methodes dendrogrammes du banc.
        """
        roots = self._union_find(level=level)
        alive = self.births <= level
        roots = [roots[index] if alive[index] else -1 for index in range(self.count)]
        return self._relabel(roots, min_size)

    def _relabel(self, roots, min_size):
        sizes = {}
        for root in roots:
            if root < 0:
                continue
            sizes[root] = sizes.get(root, 0) + 1
        mapping = {}
        labels = np.full(self.count, -1, dtype=np.int64)
        for index, root in enumerate(roots):
            if root < 0:
                continue
            if min_size is not None and sizes[root] < min_size:
                continue
            if root not in mapping:
                mapping[root] = len(mapping)
            labels[index] = mapping[root]
        return labels

    def peak_level(self, min_size=5, max_clusters=25):
        """Niveau ou le nombre de groupes recevables est MAXIMAL.

        Seconde regle spontanee, celle qui utilise vraiment l'axe d'ordre :
        elle balaie les niveaux ou quelque chose change (naissances et
        fusions) et retient celui qui donne le plus de groupes d'au moins
        `min_size` observations nees. Bas, les observations isolees ne sont
        pas encore nees, donc rejetees comme bruit ; haut, tout a fusionne.
        Le maximum est un compromis intrinseque, sans seuil a regler. C'est
        l'analogue pauvre de l'exces de masse de HDBSCAN, mais il est
        applicable a l'identique aux trois arbres compares.
        """
        candidates = sorted(set(self.merge_level.tolist()) | set(self.births.tolist()))
        best_count = -1
        best_level = candidates[-1] if candidates else 0.0
        for level in candidates:
            labels = self.labels_at(level, min_size=min_size)
            groups = int(np.unique(labels[labels >= 0]).size)
            if 2 <= groups <= max_clusters and groups > best_count:
                best_count = groups
                best_level = level
        return float(best_level), max(best_count, 0)

    def spontaneous_level(self, max_clusters=25):
        """Niveau de coupe spontanee : plus grand saut de niveau de fusion.

        Regle unique pour toutes les methodes dendrogrammes du banc, donc
        neutre : on cherche le plus grand ecart `L_{r+1} - L_r` entre niveaux
        de fusion consecutifs, restreint aux coupes donnant entre `2` et
        `max_clusters` groupes, et on coupe juste apres `L_r`.
        """
        levels = self.merge_level
        if levels.size < 2:
            return (float(levels[-1]) if levels.size else 0.0), 1
        low = max(1, levels.size + 1 - int(max_clusters))
        high = levels.size
        if low >= high:
            low = 1
        gaps = levels[low:high] - levels[low - 1:high - 1]
        if gaps.size == 0:
            return float(levels[-1]), 1
        best = int(np.argmax(gaps)) + low
        return float(levels[best - 1]), self.count - best


class FastPointTower:
    """Tour E-HGP projetee, flottante : naissances, poids encadres, liaisons."""

    def __init__(
        self,
        points,
        orders=(1, 2, 5, 10),
        intervals=8,
        mode="tube",
        width=None,
        certify=True,
    ):
        self.cloud = as_cloud(points)
        self.count = self.cloud.shape[0]
        self.dimension = self.cloud.shape[1]
        self.bracket = segment_brackets(
            self.cloud,
            orders,
            intervals=intervals,
            mode=mode,
            width=width,
            certify=certify,
        )
        self.orders = self.bracket["orders"]
        self.entry = self.bracket["entry"]
        self.distances = squared_distances(self.cloud)
        self._towers = {}

    def weights(self, order, side="upper"):
        """Poids condenses de l'ordre demande (`upper` = majorant certifie)."""
        return self.bracket[side][self.orders.index(order)]

    def tower(self, order, side="upper"):
        """Arbre de liaison simple de la tour a l'ordre `order`."""
        key = ("ehgp", order, side)
        if key not in self._towers:
            self._towers[key] = LinkageTower(
                self.weights(order, side), self.entry[:, order - 1], self.count
            )
        return self._towers[key]

    def reachability_tower(self, order, quarter=True):
        """Arbre de liaison simple de la reachability mutuelle, meme ordre."""
        key = ("mr", order, quarter)
        if key not in self._towers:
            self._towers[key] = LinkageTower(
                mutual_reachability(self.entry, self.distances, order, quarter),
                self.entry[:, order - 1],
                self.count,
            )
        return self._towers[key]

    def digest(self, decimals=9):
        """Digest de reproductibilite des majorants arrondis, par ordre."""
        payload = []
        for slot, order in enumerate(self.orders):
            rounded = np.round(self.bracket["upper"][slot], decimals)
            payload.append(str(order) + ":" + rounded.tobytes().hex())
        return hashlib.sha256("|".join(payload).encode("ascii")).hexdigest()
