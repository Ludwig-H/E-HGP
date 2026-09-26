"""Projection de la tour FULL exacte sur les observations.

Une observation `x_i` appartient a `L_k(a)` si et seulement si
`a_k(x_i) <= a`, ou `a_k(x_i)` est sa k-ieme distance au carre aux
observations (la premiere etant nulle). Dans ce cas, toute partie `F` de
cardinal `k` contenue dans `X inter B(x_i, sqrt(a))` est un sommet de
`Gamma_k(a)`, et deux telles parties sont adjacentes ou reliees par une
chaine de telles parties : elles vivent dans LA MEME composante, puisque
toutes contiennent `x_i` dans leur intersection de boules. La composante de
`x_i` est donc bien definie, et on peut prendre pour temoin les `k` plus
proches voisins de `x_i` (lui-meme inclus).

Ce module fournit l'ultrametrique de reference

    u_ij^{(k)} = min { a : x_i et x_j dans la meme composante de L_k(a) } ,

calculee a partir de l'oracle exact. C'est la verite contre laquelle le
moteur `engine/point_tower.py` est juge.
"""

from fractions import Fraction

from .tower import OrderState, _mask_of


def witness_vertex(cloud, index, order):
    """Partie de cardinal `order` temoin de l'appartenance de `x_index`."""
    ranked = sorted(
        range(len(cloud)),
        key=lambda other: (
            sum((a - b) * (a - b) for a, b in zip(cloud[index], cloud[other])),
            other,
        ),
    )
    return tuple(sorted(ranked[:order]))


def entry_level(cloud, index, order):
    """Niveau d'entree `a_order(x_index)`."""
    distances = sorted(
        sum((a - b) * (a - b) for a, b in zip(cloud[index], cloud[other]))
        for other in range(len(cloud))
    )
    return distances[order - 1]


def projected_ultrametric(tower, order):
    """Ultrametrique exacte des observations a l'ordre `order`.

    Le balayage porte sur l'UNION des niveaux critiques de la region (les
    `beta` des parties, ou la topologie de `Gamma_order` change) et des
    niveaux d'entree `a_order(x_i)` (ou une observation rejoint `L_order`).
    Ces derniers ne sont pas des `beta` : l'entree d'une observation n'est pas
    un changement de topologie de la region, mais elle change la projection.
    Entre deux niveaux consecutifs de cette union, l'application
    observation -> composante est constante.

    Les paires jamais reunies dans la fenetre balayee recoivent `None` ; la
    fenetre est explicitement bornee par le plus grand niveau utile
    (`max(beta, entrees)`), ce qui suffit car au-dela `Gamma_order` est
    complet et sa composante unique.
    """
    cloud = tower.catalog.cloud
    count = len(cloud)
    witnesses = [witness_vertex(cloud, index, order) for index in range(count)]
    entries = [entry_level(cloud, index, order) for index in range(count)]
    replay = OrderState(tower.catalog, order)
    event_levels = sorted(replay.events)
    probe_levels = sorted(set(event_levels) | set(entries))
    result = {}
    pending = {
        (left, right)
        for left in range(count)
        for right in range(left + 1, count)
    }
    cursor = 0
    for level in probe_levels:
        while cursor < len(event_levels) and event_levels[cursor] <= level:
            replay.apply_level(event_levels[cursor])
            cursor += 1
        if not pending:
            break
        roots = {}
        for index in range(count):
            if entries[index] > level:
                continue
            vertex_index = replay.index_of.get(_mask_of(witnesses[index]))
            if vertex_index is None or not replay.active[vertex_index]:
                continue
            roots[index] = replay.union_find.find(vertex_index)
        settled = []
        for pair in pending:
            left, right = pair
            if left in roots and right in roots and roots[left] == roots[right]:
                result[pair] = level
                settled.append(pair)
        for pair in settled:
            pending.discard(pair)
    for pair in pending:
        result[pair] = None
    return result


def compare_ultrametrics(exact, candidate):
    """Compare deux ultrametriques : accords, majorations strictes, violations.

    `candidate` doit majorer `exact` (un chemin droit est un chemin). Une
    valeur strictement plus petite est une VIOLATION : elle signale un bug
    ou une hypothese fausse, et devient une fixture permanente.
    """
    equal = 0
    strictly_above = 0
    violations = []
    for pair, reference in exact.items():
        proposal = candidate.get(pair)
        if reference is None:
            continue
        if proposal is None:
            violations.append((pair, reference, None))
            continue
        if proposal == reference:
            equal += 1
        elif proposal > reference:
            strictly_above += 1
        else:
            violations.append((pair, reference, proposal))
    return {
        "equal": equal,
        "strictly_above": strictly_above,
        "violations": violations,
        "total": equal + strictly_above + len(violations),
        "ratio_max": max(
            (Fraction(candidate[pair], exact[pair]) for pair in exact
             if exact[pair] not in (None, 0) and candidate.get(pair) is not None),
            default=None,
        ),
    }
