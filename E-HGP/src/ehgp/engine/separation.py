"""Certificat de SEPARATION : la borne inferieure qui manquait.

Le moteur `witness_tower.py` produit un MAJORANT certifie du niveau de fusion
(un chemin polygonal contenu dans `L_k(a)` prouve la connexite). Il manquait
la direction opposee : prouver que deux observations ne sont PAS dans la meme
composante. Le certificat suivant la fournit, exactement et sans dependance
en dimension.

> **Certificat de tranche.** Soit `H` un hyperplan qui separe strictement
> `x_i` de `x_j`. Si au plus `k - 1` observations sont a distance au carre au
> plus `a` de `H`, alors `H` ne rencontre pas `L_k(a)`, donc `x_i` et `x_j`
> sont dans des composantes distinctes de `L_k(a)`.

*Preuve.* Un point `y` de `H` n'est couvert que par les boules
`B(x_l, racine de a)` dont le centre est a distance au carre au plus `a` de
`H` ; il y en a au plus `k - 1`, donc `a_k(y) > a` et `y` n'est pas dans
`L_k(a)`. Tout chemin continu de `x_i` a `x_j` traverse `H` : il sort donc de
`L_k(a)`. $\\square$

La quantite utile est donc, pour un hyperplan `H`,

    A_k(H) = k-ieme plus petite des distances au carre des observations a H ,

et le certificat vaut exactement pour tout `a < A_k(H)`. Le niveau de fusion
vrai est donc MINORE par `A_k(H)`, pour tout `H` separant. En maximisant sur
une famille d'hyperplans on obtient une borne inferieure certifiee ; avec le
majorant de `witness_tower.py`, cela donne un ENCADREMENT certifie, et
l'egalite des deux bornes CERTIFIE la valeur exacte.

Remarquable : `A_k(H)` est la meme statistique d'ordre que `a_k`, prise sur
les distances a un hyperplan au lieu d'un point. Le primal (points) et le
dual (hyperplans) partagent donc exactement la structure entropique du § 1 de
`docs/REGULARISATION_ENTROPIQUE.md`.

Pour la direction naturelle `n = x_j - x_i`, le decalage optimal se calcule
exactement : en notant `t_l` le produit scalaire de `n` avec `x_l`, la
fonction `c -> k-ieme plus petite des (t_l - c)^2` est quadratique par
morceaux et son maximum sur un intervalle est atteint a un croisement
`c = (t_l + t_m)/2` ou a une extremite. Il suffit donc d'evaluer ces
candidats : `O(n^2)` par paire, en rationnels exacts.
"""

from fractions import Fraction

from .segment import rational_cloud


def hyperplane_order_statistic(cloud, normal, offset, order):
    """`A_order(H)` pour `H = {y : <normal, y> = offset}`, en rationnels exacts.

    Renvoie la k-ieme plus petite des distances au CARRE des observations a
    l'hyperplan, c'est-a-dire `(<normal, x> - offset)^2 / ||normal||^2`.
    """
    squared_norm = sum(component * component for component in normal)
    if squared_norm == 0:
        raise ValueError("normale nulle")
    values = []
    for point in cloud:
        gap = sum(a * b for a, b in zip(normal, point)) - offset
        values.append(Fraction(gap * gap, 1) / squared_norm)
    values.sort()
    return values[order - 1]


def best_offset(cloud, normal, low, high, order):
    """Decalage qui maximise `A_order` sur l'intervalle ouvert `(low, high)`.

    Renvoie `(niveau, decalage)`, ou `(None, None)` si l'intervalle est vide.
    Les candidats sont les croisements `(t_l + t_m)/2` interieurs ; les
    extremites sont exclues car l'hyperplan doit separer STRICTEMENT.
    """
    if low > high:
        low, high = high, low
    projections = [sum(a * b for a, b in zip(normal, point)) for point in cloud]
    candidates = set()
    count = len(projections)
    for left in range(count):
        for right in range(left, count):
            middle = Fraction(projections[left] + projections[right], 2)
            if low < middle < high:
                candidates.add(middle)
    candidates.add(Fraction(low + high, 2))
    best_level = None
    best_point = None
    for offset in sorted(candidates):
        level = hyperplane_order_statistic(cloud, normal, offset, order)
        if best_level is None or level > best_level:
            best_level = level
            best_point = offset
    return best_level, best_point


def separates(normal, offset, left_point, right_point):
    """Vrai si l'hyperplan separe STRICTEMENT les deux points."""
    left = sum(a * b for a, b in zip(normal, left_point)) - offset
    right = sum(a * b for a, b in zip(normal, right_point)) - offset
    return (left > 0 and right < 0) or (left < 0 and right > 0)


def certified_lower_bound(cloud, source, target, order, extra_normals=True):
    """Borne inferieure certifiee du niveau de fusion de deux observations.

    Explore la direction `x_target - x_source` et, en option, les directions
    fournies par les autres paires d'observations. Renvoie
    `(niveau, normale, decalage)` : le niveau de fusion vrai est au moins ce
    niveau. Renvoie `(Fraction(0), None, None)` si aucun certificat n'est
    trouve : l'absence de certificat ne prouve rien.
    """
    points = cloud
    count = len(points)
    directions = [
        tuple(b - a for a, b in zip(points[source], points[target]))
    ]
    if extra_normals:
        for left in range(count):
            for right in range(left + 1, count):
                if (left, right) == (source, target):
                    continue
                direction = tuple(
                    b - a for a, b in zip(points[left], points[right])
                )
                if any(component != 0 for component in direction):
                    directions.append(direction)
    best = (Fraction(0), None, None)
    for normal in directions:
        squared_norm = sum(component * component for component in normal)
        if squared_norm == 0:
            continue
        low = sum(a * b for a, b in zip(normal, points[source]))
        high = sum(a * b for a, b in zip(normal, points[target]))
        if low == high:
            continue
        level, offset = best_offset(points, normal, low, high, order)
        if level is None or offset is None:
            continue
        if not separates(normal, offset, points[source], points[target]):
            continue
        if level > best[0]:
            best = (level, normal, offset)
    return best


def certified_bracket(cloud, upper_ultrametric, order, extra_normals=True):
    """Encadrement certifie de chaque paire : `(inferieure, superieure, serre)`.

    `upper_ultrametric` est le majorant produit par `witness_tower.py`. La
    borne inferieure vient du certificat de tranche. Quand les deux bornes
    coincident, la valeur exacte est CERTIFIEE : ce n'est plus une mesure
    d'accord avec un oracle, c'est une preuve.
    """
    points = rational_cloud(cloud)
    count = len(points)
    result = {}
    for source in range(count):
        for target in range(source + 1, count):
            upper = upper_ultrametric.get((source, target))
            lower, normal, offset = certified_lower_bound(
                points, source, target, order, extra_normals=extra_normals
            )
            tight = upper is not None and lower == upper
            result[(source, target)] = {
                "lower": lower,
                "upper": upper,
                "tight": tight,
                "normal": normal,
                "offset": offset,
            }
    return result


def covers_sphere(squared_distance_to_centre, sphere_radius_squared, level):
    """La boule `B(x, racine(level))` couvre-t-elle un point de la sphere ?

    Avec `s` la distance au carre de l'observation au centre de la sphere et
    `R` le rayon au carre de la sphere, le point de la sphere le plus proche
    de l'observation est a distance au carre `(racine(s) - racine(R))^2`. La
    condition `level >= (racine(s) - racine(R))^2` s'ecrit SANS racine :

        (level - s - R)^2 <= 4 s R    ou bien    level > s + R .

    En effet la premiere inegalite caracterise
    `(racine(s) - racine(R))^2 <= level <= (racine(s) + racine(R))^2`, et la
    seconde couvre le cas ou la boule avale la sphere entiere. Tout est
    rationnel : aucune racine n'est jamais evaluee.
    """
    gap = level - squared_distance_to_centre - sphere_radius_squared
    if gap * gap <= 4 * squared_distance_to_centre * sphere_radius_squared:
        return True
    return level > squared_distance_to_centre + sphere_radius_squared


def sphere_threshold_below(squared_distance_to_centre, sphere_radius_squared, level):
    """Vrai si le seuil de couverture de l'observation est STRICTEMENT sous `level`."""
    if not covers_sphere(squared_distance_to_centre, sphere_radius_squared, level):
        return False
    gap = level - squared_distance_to_centre - sphere_radius_squared
    egalite = (
        gap * gap == 4 * squared_distance_to_centre * sphere_radius_squared
        and level <= squared_distance_to_centre + sphere_radius_squared
    )
    return not egalite


def sphere_separates(cloud, centre, radius_squared, source, target):
    """Vrai si la sphere separe strictement les deux observations."""
    from ..exact.meb import squared_distance

    inside = squared_distance(centre, cloud[source])
    outside = squared_distance(centre, cloud[target])
    return (inside < radius_squared < outside) or (outside < radius_squared < inside)


def sphere_certificate(cloud, centre, radius_squared, order, level, source, target):
    """Certificat de tranche SPHERIQUE : la fusion n'a pas lieu avant `level`.

    Si la sphere separe les deux observations et si au plus `order - 1`
    observations ont leur seuil de couverture strictement sous `level`, alors
    pour tout `a < level` la sphere ne rencontre pas `L_order(a)` : tout
    chemin la traverse, donc les deux observations sont dans des composantes
    distinctes. Le niveau de fusion est donc au moins `level`.

    Une sphere est le bon obstacle pour un objet fait de boules : un
    hyperplan, en grande dimension, est traverse par presque toutes les
    boules (mesure : rapport majorant sur minorant de 8,5 en d = 20), alors
    qu'une sphere critique du catalogue epouse la geometrie de l'objet.
    """
    from ..exact.meb import squared_distance

    if not sphere_separates(cloud, centre, radius_squared, source, target):
        return False
    crossing = 0
    for point in cloud:
        if sphere_threshold_below(
            squared_distance(centre, point), radius_squared, level
        ):
            crossing += 1
            if crossing >= order:
                return False
    return True


def candidate_spheres(cloud, centres, precision=10 ** 12):
    """Spheres candidates autour de chaque centre.

    Le rayon optimal ne vit pas dans l'espace des rayons au CARRE mais dans
    celui des rayons : l'obstacle ideal entre deux coquilles de rayons `u` et
    `v` a pour rayon `(u + v)/2`, donc pour rayon au carre
    `(u + v)^2/4 = (s_u + s_v + 2 racine(s_u s_v))/4`, en general irrationnel.
    Trois familles de candidats RATIONNELS sont donc produites :

    * les milieux dans l'espace des rayons au carre (grossiers mais exacts) ;
    * les `s/4`, qui sont les optima exacts quand une coquille est de rayon
      nul, c'est-a-dire quand le centre est une observation : pour l'ordre 1
      ce candidat suffit a certifier le niveau de fusion vrai ;
    * les milieux de l'espace des rayons, calcules en flottant puis
      RATIONALISES. Le flottant ne sert qu'a proposer : le certificat, lui,
      est exact pour n'importe quel rayon rationnel, donc un candidat
      legerement sous-optimal donne une borne legerement plus faible, jamais
      une borne fausse.
    """
    from fractions import Fraction as _Fraction
    from math import sqrt

    from ..exact.meb import squared_distance

    spheres = []
    for centre in centres:
        distances = sorted({squared_distance(centre, point) for point in cloud})
        radii = set()
        for left, right in zip(distances, distances[1:]):
            radii.add((left + right) / 2)
        for value in distances:
            if value > 0:
                radii.add(value / 4)
        roots = [sqrt(float(value)) for value in distances]
        for index, left in enumerate(roots):
            for right in roots[index + 1:]:
                middle = 0.5 * (left + right)
                radii.add(_Fraction(middle * middle).limit_denominator(precision))
        if distances:
            radii.add(distances[-1] * 2)
        for radius in radii:
            if radius > 0:
                spheres.append((centre, radius))
    return spheres


def certified_exact_levels(cloud, upper_ultrametric, order, spheres):
    """Paires dont le niveau de fusion est CERTIFIE exact.

    `spheres` est une liste de couples `(centre, rayon_carre)` : les spheres
    critiques trouvees par la descente conviennent, ce qui fait des temoins
    une ressource partagee entre les chemins (majorant) et les barrieres
    (minorant). Une paire est certifiee lorsqu'un chemin atteint le niveau
    `U` et qu'une sphere interdit toute jonction sous `U`.
    """
    points = rational_cloud(cloud)
    certified = {}
    for (source, target), upper in upper_ultrametric.items():
        if upper is None:
            continue
        for centre, radius_squared in spheres:
            if sphere_certificate(
                points, centre, radius_squared, order, upper, source, target
            ):
                certified[(source, target)] = {
                    "level": upper,
                    "centre": centre,
                    "radius_squared": radius_squared,
                }
                break
    return certified
