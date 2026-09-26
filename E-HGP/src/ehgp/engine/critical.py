"""Recherche des spheres critiques par descente, sans enumeration.

MorseHGP3D enumere son catalogue critique par la combinatoire des supports
de taille au plus `d + 1` (§ 5 de la specification) : en dimension `d`
grande, ce catalogue compte `C(n, d + 1)` candidats et l'enumeration meurt.
E-HGP le remplace par une DESCENTE, sans aucune combinatoire globale.

Iteration MEB-Lloyd, pour une masse entiere `m` :

    1. `N(y)` = les `m` observations les plus proches de `y` ;
    2. `c` = centre de la boule englobante minimale exacte de `N(y)` ;
    3. si `c = y` on s'arrete, sinon `y <- c` et on recommence.

Deux proprietes elementaires, valables en toute dimension :

* DECROISSANCE. Soit `a_m(y)` la m-ieme distance au carre. Si `y` a pour
  m-plus-proches `N(y)`, alors `a_m(y) = max_{x dans N(y)} ||y - x||^2`. Le
  centre `c` de la boule englobante minimale de `N(y)` minimise ce maximum,
  donc `max_{x dans N(y)} ||c - x||^2 = r^2 <= a_m(y)` ; et comme `N(y)`
  fournit `m` observations a distance au plus `r` de `c`, on a
  `a_m(c) <= r^2 <= a_m(y)`. La suite decroit, et comme il n'y a qu'un
  nombre fini de parties `N`, elle s'arrete.
* CRITICITE. En un point fixe, `y` est le centre de la boule englobante
  minimale de ses `m` plus proches. Donc `y` appartient a
  `conv(U)` (propriete du centre englobant), toute observation hors de
  `N(y)` est a distance au moins `r` (donc hors de la boule OUVERTE), et le
  rang ferme vaut `s = |I| + |U| = m` hors degenerescence cospherique. Avec
  la caracterisation de `docs/SPECIFICATION_MORSEHGP3D.md` § 5
  (`y` critique pour `D_k` si `y` est dans `relint conv(U)` et
  `|I| < k <= s`, d'indice `s - k`), on obtient :
      - `m = k`     : indice 0, une NAISSANCE de composante a l'ordre `k` ;
      - `m = k + 1` : indice 1, un evenement de FUSION a l'ordre `k`.

Le catalogue critique est donc l'ensemble des points fixes de cette
descente pour `m = k` et `m = k + 1`. La descente coute `O(n d)` par pas
plus une boule englobante de `m` points : rien d'exponentiel en `d`.

Ce que la descente NE donne PAS : la garantie d'avoir trouve TOUS les
points fixes. La completude reste une question de couverture mesuree, donc
`public_status=not_claimed` ; le nombre de candidats trouves, le nombre de
departs et la part du catalogue exact retrouvee doivent etre publies.
"""

from fractions import Fraction

from ..exact.meb import minimum_enclosing_ball, squared_distance, to_rational_cloud


def nearest_subset(cloud, position, mass):
    """Les `mass` observations les plus proches de `position`, par indices."""
    ranked = sorted(
        range(len(cloud)),
        key=lambda index: (squared_distance(position, cloud[index]), index),
    )
    return tuple(sorted(ranked[:mass]))


def meb_lloyd(cloud, start, mass, max_steps=200):
    """Descente MEB-Lloyd depuis `start` pour la masse `mass`.

    Renvoie un dictionnaire decrivant le point fixe atteint : centre,
    niveau (rayon au carre), support, interieur strict, coquille, rang
    ferme, nombre de pas, et si le point fixe a bien ete atteint.
    """
    position = tuple(Fraction(coordinate) for coordinate in start)
    previous = None
    steps = 0
    for steps in range(1, max_steps + 1):
        subset = nearest_subset(cloud, position, mass)
        if subset == previous:
            break
        previous = subset
        center, radius_squared, support = minimum_enclosing_ball([cloud[i] for i in subset])
        if center == position:
            break
        position = center
    subset = nearest_subset(cloud, position, mass)
    center, radius_squared, support = minimum_enclosing_ball([cloud[i] for i in subset])
    interior = []
    shell = []
    for index, point in enumerate(cloud):
        distance = squared_distance(center, point)
        if distance < radius_squared:
            interior.append(index)
        elif distance == radius_squared:
            shell.append(index)
    return {
        "center": center,
        "level": radius_squared,
        "subset": subset,
        "support": tuple(subset[i] for i in support),
        "interior": tuple(interior),
        "shell": tuple(shell),
        "closed_rank": len(interior) + len(shell),
        "steps": steps,
        "fixed": center == position,
    }


def descent_starts(cloud, pairs=True, triples=False):
    """Points de depart canoniques : observations, milieux, barycentres."""
    starts = [tuple(point) for point in cloud]
    count = len(cloud)
    if pairs:
        for left in range(count):
            for right in range(left + 1, count):
                starts.append(
                    tuple(
                        (a + b) / 2 for a, b in zip(cloud[left], cloud[right])
                    )
                )
    if triples:
        for left in range(count):
            for middle in range(left + 1, count):
                for right in range(middle + 1, count):
                    starts.append(
                        tuple(
                            (a + b + c) / 3
                            for a, b, c in zip(cloud[left], cloud[middle], cloud[right])
                        )
                    )
    return starts


def critical_catalogue(cloud, mass, starts=None, triples=False):
    """Catalogue des spheres critiques de rang ferme `mass` trouvees par descente.

    Renvoie `(catalogue, statistiques)`. Le catalogue est un dictionnaire
    indexe par `(centre, niveau)` canonique, donc deux departs qui tombent
    sur la meme sphere ne comptent qu'une fois.
    """
    points = to_rational_cloud(cloud)
    if starts is None:
        starts = descent_starts(points, pairs=True, triples=triples)
    catalogue = {}
    total_steps = 0
    for start in starts:
        record = meb_lloyd(points, start, mass)
        total_steps += record["steps"]
        key = (record["center"], record["level"])
        catalogue.setdefault(key, record)
    statistics = {
        "starts": len(starts),
        "distinct": len(catalogue),
        "steps": total_steps,
        "mass": mass,
    }
    return catalogue, statistics
