"""Maximum exact de `a_k` le long d'un segment, libre en dimension.

Soit `y(t) = p + t (q - p)` pour `t` dans `[0, 1]`. Les energies

    e_l(t) = ||y(t) - x_l||^2 = A t^2 + B_l t + C_l ,
    A = ||q - p||^2 ,  B_l = 2 <q - p, p - x_l> ,  C_l = ||p - x_l||^2 ,

PARTAGENT le coefficient dominant `A`. Consequence exploitee ici : l'ordre
des `e_l(t)` est celui des fonctions AFFINES `B_l t + C_l`, donc

    a_k(y(t)) = A t^2 + g_k(t) ,

ou `g_k` est la k-ieme plus petite de `n` fonctions affines : une fonction
affine par morceaux, dont les ruptures sont des croisements de droites.

Sur chaque morceau, `A t^2 + (s t + c)` est CONVEXE, donc son maximum sur
l'intervalle est atteint a une extremite. Le maximum de `a_k` sur le segment
est donc atteint en `t = 0`, `t = 1`, ou a un croisement de droites. Il
suffit d'evaluer `a_k` sur cet ensemble fini de temps candidats : c'est
exact, ne suppose rien sur `d`, et ne fait appel a aucun flottant.

Ce fait est la brique qui remplace, en grande dimension, la geometrie des
spheres critiques a `d + 1` points de MorseHGP3D : un chemin droit entre
deux observations est certifie par un calcul a une variable.
"""

from fractions import Fraction

from ..exact.meb import to_rational_cloud


def affine_parts_points(cloud, origin, endpoint):
    """Renvoie `(A, [(B_l, C_l)])` pour le segment `[origin, endpoint]`.

    Les energies sont calculees par rapport aux OBSERVATIONS `cloud` ; les
    extremites du segment peuvent etre des points quelconques (temoins), qui
    n'entrent PAS dans le nuage. Confondre les deux changerait `a_k`.
    """
    points = cloud
    direction = tuple(b - a for a, b in zip(origin, endpoint))
    leading = sum(component * component for component in direction)
    parts = []
    for point in points:
        offset = tuple(a - b for a, b in zip(origin, point))
        linear = 2 * sum(u * v for u, v in zip(direction, offset))
        constant = sum(component * component for component in offset)
        parts.append((linear, constant))
    return leading, parts


def affine_parts(cloud, source, target):
    """Variante indexee : le segment joint deux observations du nuage."""
    return affine_parts_points(cloud, cloud[source], cloud[target])


def candidate_times(parts):
    """Temps candidats dans `[0, 1]` : croisements des fonctions affines."""
    times = {Fraction(0), Fraction(1)}
    count = len(parts)
    for left in range(count):
        slope_left, intercept_left = parts[left]
        for right in range(left + 1, count):
            slope_right, intercept_right = parts[right]
            slope_gap = slope_left - slope_right
            if slope_gap == 0:
                continue
            time = Fraction(intercept_right - intercept_left, slope_gap)
            if 0 < time < 1:
                times.add(time)
    return sorted(times)


def order_statistic(leading, parts, time, order):
    """`a_order` au temps `time` : `A t^2` plus la k-ieme plus petite affine."""
    values = sorted(slope * time + intercept for slope, intercept in parts)
    return leading * time * time + values[order - 1]


def segment_maximum_points(cloud, origin, endpoint, order):
    """Maximum exact de `a_order` sur `[origin, endpoint]`, temoins admis.

    `cloud` est le nuage des observations (il fixe `a_order`) ; `origin` et
    `endpoint` sont des points quelconques de l'espace.
    """
    leading, parts = affine_parts_points(cloud, origin, endpoint)
    best_level = None
    best_time = None
    for time in candidate_times(parts):
        level = order_statistic(leading, parts, time, order)
        if best_level is None or level > best_level:
            best_level = level
            best_time = time
    return best_level, best_time


def segment_maximum(cloud, source, target, order):
    """Maximum exact de `a_order` sur le segment, avec son temps.

    Renvoie `(niveau, temps)`. Le niveau est le plus petit `a` tel que le
    segment entier soit contenu dans `L_order(a)` : c'est donc un MAJORANT
    certifie du niveau de fusion des deux observations a l'ordre `order`.
    """
    return segment_maximum_points(cloud, cloud[source], cloud[target], order)


def entry_levels(cloud, k_max):
    """`entry[i][k] = a_k(x_i)` : niveau d'entree de l'observation dans `L_k`."""
    points = cloud
    count = len(points)
    table = []
    for index in range(count):
        distances = sorted(
            sum((a - b) * (a - b) for a, b in zip(points[index], points[other]))
            for other in range(count)
        )
        table.append([distances[order - 1] for order in range(1, min(k_max, count) + 1)])
    return table


def rational_cloud(cloud):
    """Nuage rationnel canonique partage par le moteur."""
    return to_rational_cloud(cloud)
