#!/usr/bin/env python3
"""Piste : premières COUVERTURES qualifiées, puis LCA de tous les ex aequo.

Modèle autonome borné, Gram/Fraction/Gamma ; aucun moteur, fit, GCP ni
exécution des 17 cas de la campagne. Sites distincts de poids 1.

Définition. À k fixé et à la coupe FERMÉE beta, pour chaque composante C de
FULL_k, sa couverture E_C est l'union des k-parties de sa composante Gamma.
Qualifier C lorsque |E_C| >= m AVANT toute attribution exclusive des points.
Pour x, alpha est la première coupe avec une couverture qualifiée contenant
x. Prendre TOUS les propriétaires C de cette même coupe, puis leur LCA dans
la forêt FULL complète. Attribuer x une seule fois à ce LCA ; son entrée est
max(alpha, niveau de naissance du LCA), avec tous les événements de la date
déjà activés. Le point suit ensuite les ancêtres de ce propriétaire figé.
Cette définition ne filtre ni les seules populations individuelles des
boules ni les tailles des groupes de points après projection.

Les sous-arbres à k fixé donnent une famille laminaire. Les ex aequo ne sont
pas départagés par les IDs. La taille qualifiante porte sur E_C, pas sur la
masse exclusive ensuite attribuée : les groupes de points peuvent avoir
moins de m membres. Une entrée différée au LCA exige de connaître les
ancêtres futurs, et peut être très postérieure à alpha. La qualification
est géométrique, indépendante des labels d'évaluation.

Deux témoins. Dans l'équilatéral entier face à face, les couvertures ABC et
DEF qualifient m=3 à k=2, beta=2/3 ; la couverture CD de taille2 ne qualifie
pas. Les deux triangles restent présents après les premières attaches.
Pour X=[(6,2),(0,0),(0,4),(12,0),(12,4)], les deux couvertures qualifiées
{0,1,2} et {0,3,4} apparaissent à beta=100/9. Site0 est ambigu, donc son
LCA est la racine de naissance beta=36. À beta=100/9, seuls les groupes
{1,2} et {3,4} sont actifs : la percolation de la fermeture qualifiée est
évitée, mais les deux masses exclusives valent2<m=3.

Contre-exemple de stabilité. Déplacer seulement site0 vers (6-delta,2),
0<delta suffisamment petit. La couverture gauche qualifie seule d'abord :
beta_L = ((6-delta)^2+4)^2 / (4*(6-delta)^2).
La droite qualifie plus tard avec la même formule en 6+delta. Site0 reste
attaché à gauche, avec entrée et coassociation u(0,1) à beta_L. Lorsque
delta -> 0+, leurs rayons tendent vers10/3, au lieu de6 au nuage symétrique :
saut8/3 pour un déplacement tendant vers0. Donc aucune borne uniforme
1-epsilon, ni continuité générale, pour cette projection dure. Le calcul
exact delta=1/1000 ci-dessous vérifie beta_L < (6-delta)^2, ce qui prouve
6-sqrt(beta_L)>delta sans approximation de racine. Ces coordonnées
rationnelles peuvent être rendues entières par une similitude commune ;
ce modèle n'est pas une exécution native du profil d'entrée.

Toute couverture FULL_k contient au moins k sites. Ainsi, pour m<=k,
premières qualifiées et premières couvertures ordinaires coïncident
exactement, y compris LCA, entrées et coupes. La piste ne change donc rien
à k5,m3 ; ses changements concernent m>k, notamment k2,m3. L'identité est
comparée à une voie indépendante par premières k-parties, sur huit petits
nuages et tous leurs k,m<=k. Aucune stabilité ou cohérence multi-K n'est
prouvée, aucune garantie statistique ou sélection finale n'est acquise.

La brique Gram/Fraction et Gamma est notre Oracle de test_qualified.py ;
son main et ses références produit optionnelles ne sont jamais exécutés.
La définition et l'attache qualifiée ci-dessous n'utilisent ni les records
forts exportés ni le premier propriétaire arbitraire de cet Oracle.
"""

from fractions import Fraction as Q
from hashlib import sha256
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "experiment"))
from test_qualified import Oracle  # noqa: E402; own bounded Gram/Fraction model

NONE = (1 << 32)-1
CHECKS = 0


def require(value, reason):
    global CHECKS
    CHECKS += 1
    if not value:
        raise ValueError(reason)


def ancestors(tree, node):
    path = []
    while node != NONE:
        path.append(node)
        node = tree["nodes"][node]["parent"]
    return path


def owner_lca(tree, choices):
    paths = [ancestors(tree, owner) for owner in sorted(choices)]
    common = set(paths[0]).intersection(*map(set, paths[1:]))
    require(bool(common), "same-order choices have a common terminal ancestor")
    return next(node for node in paths[0] if node in common)


def anchored(oracle, k, first, choices):
    tree = oracle.trees[k]
    require(all(date is not None for date in first), "every point eventually has a qualified coverage")
    owners = [owner_lca(tree, group) for group in choices]
    entries = [max(date, oracle.levels[tree["nodes"][node]["rank"]])
               for date, node in zip(first, owners)]
    cuts, heights = [], {}
    previous = []
    for beta in oracle.levels:
        groups = {}
        for site, (date, owner) in enumerate(zip(entries, owners)):
            if date > beta:
                continue
            parent = tree["nodes"][owner]["parent"]
            while parent != NONE and oracle.levels[tree["nodes"][parent]["rank"]] <= beta:
                owner = parent
                parent = tree["nodes"][owner]["parent"]
            groups.setdefault(owner, []).append(site)
        current = sorted(groups.values())
        require(all(any(set(old) <= set(new) for new in current) for old in previous),
                "fixed-order anchored active groups never split")
        for group in current:
            for offset, site in enumerate(group):
                for other in group[offset+1:]:
                    heights.setdefault((site, other), beta)
        cuts.append((beta, current))
        previous = current
    return dict(first=first, choices=[sorted(group) for group in choices], owners=owners,
                entries=entries, cuts=cuts, pair_heights=heights)


def first_qualified(oracle, k, m):
    require(1 <= m <= oracle.n, "bounded positive transmission threshold")
    first = [None]*oracle.n
    choices = [set() for _ in oracle.points]
    for beta in oracle.levels:
        at_level = [set() for _ in oracle.points]
        for component in oracle.gamma(k, beta):
            coverage = {site for part in component for site in part}
            if len(coverage) < m:
                continue
            part = next(iter(component))
            owner = oracle.trees[k]["snapshots"][beta][part]
            for site in coverage:
                at_level[site].add(owner)
        for site, group in enumerate(at_level):
            if first[site] is None and group:
                first[site] = beta
                choices[site] = group
    return anchored(oracle, k, first, choices)


def ordinary_by_parts(oracle, k):
    """Independent first-event route: minimize MEB over k-parts containing x."""
    first, choices = [], []
    for site in range(oracle.n):
        parts = [part for part in oracle.parts[k] if site in part]
        beta = min(oracle.meb(part)[0] for part in parts)
        first.append(beta)
        choices.append({oracle.trees[k]["snapshots"][beta][part]
                        for part in parts if oracle.meb(part)[0] == beta})
    return anchored(oracle, k, first, choices)


def cut(answer, beta):
    return next(groups for level, groups in answer["cuts"] if level == beta)


def brief(answer):
    return dict(first_qualified=[str(date) for date in answer["first"]],
                entry_dates=[str(date) for date in answer["entries"]],
                tied_owners=answer["choices"], assigned_owners=answer["owners"],
                pair_heights={str(a)+":"+str(b): str(date)
                              for (a, b), date in answer["pair_heights"].items()})


def run():
    exact = [(1, 1, 2), (1, 2, 1), (2, 2, 2), (3, 3, 2), (4, 4, 2), (4, 3, 3)]
    triangles = first_qualified(Oracle(exact, 2), 2, 3)
    require(cut(triangles, Q(2, 3)) == [[0, 1, 2], [3, 4, 5]], "both qualified triangles retained")
    require(triangles["entries"] == [Q(2, 3)]*6, "all triangle entries at qualification")

    points = [(6, 2, 0), (0, 0, 0), (0, 4, 0), (12, 0, 0), (12, 4, 0)]
    original = first_qualified(Oracle(points, 2), 2, 3)
    require(original["first"] == [Q(100, 9)]*5, "all first boundary qualifications tie")
    require(len(original["choices"][0]) == 2 and original["entries"][0] == 36,
            "ambiguous point delayed to root36")
    require(cut(original, Q(100, 9)) == [[1, 2], [3, 4]], "two exclusive pairs without premature percolation")
    require(original["pair_heights"][(0, 1)] == 36, "original ambiguous coassociation at root36")

    delta = Q(1, 1000)
    x = 6-delta
    left = (x*x+4)**2/(4*x*x)
    right = ((6+delta)**2+4)**2/(4*(6+delta)**2)
    displaced = first_qualified(Oracle([(x, 2, 0), *points[1:]], 2), 2, 3)
    require(left < right, "unique first qualified left branch")
    require(len(displaced["choices"][0]) == 1 and displaced["entries"][0] == left,
            "displaced point attaches permanently to left")
    require(displaced["pair_heights"][(0, 1)] == left, "displaced left coassociation at betaL")
    require(cut(displaced, right) == [[0, 1, 2], [3, 4]], "right remains an exclusive two-point branch")
    require(left < (6-delta)**2, "entry and pair radius change strictly exceeds point displacement")

    fixtures = {
        "pair": [(0, 0, 0), (2, 0, 0)],
        "line024": [(x, 0, 0) for x in (0, 2, 4)],
        "line01269": [(x, 0, 0) for x in (0, 1, 2, 6, 9)],
        "square": [(0, 0, 0), (2, 0, 0), (2, 2, 0), (0, 2, 0)],
        "octa": [(3, 2, 2), (1, 2, 2), (2, 3, 2), (2, 1, 2), (2, 2, 3), (2, 2, 1)],
        "generic6": [(0, 0, 0), (2, 1, 0), (0, 3, 1), (4, 0, 2), (2, 4, 3), (6, 2, 1)],
        "equilateral": exact,
        "triangle_inner": [(0, 0, 0), (6, 0, 0), (3, 4, 0), (2, 1, 0), (4, 1, 0)],
    }
    identities = []
    identity_checks = 0
    for name, coordinates in fixtures.items():
        oracle = Oracle(coordinates, len(coordinates))
        count = 0
        for k in range(1, oracle.n+1):
            ordinary = ordinary_by_parts(oracle, k)
            for m in range(1, k+1):
                require(first_qualified(oracle, k, m) == ordinary,
                        (name, k, m, "m<=k exactly equals ordinary first-cover LCA"))
                count += 1
                identity_checks += 1
        identities.append(dict(name=name, sites=len(coordinates), comparisons=count))
    dependencies = {str(path.relative_to(HERE)): sha256(path.read_bytes()).hexdigest()
                    for path in (Path(__file__).resolve(), HERE / "experiment/test_qualified.py",
                                 HERE / "experiment/qualified.py")}
    return dict(status="pass", schema="ehgp.v11.qualified_first_exact_check.v1", checks=CHECKS,
                scope="bounded mathematical proposal; not the native 17-case experiment",
                height_units="squared_grid_radius", dependencies=dependencies,
                triangles=dict(points=exact, result=brief(triangles), cut_beta_2over3=cut(triangles, Q(2, 3))),
                shared_boundary=dict(points=points, result=brief(original), cut_beta_100over9=cut(original, Q(100, 9))),
                perturbation=dict(delta=str(delta), beta_left=str(left), beta_right=str(right),
                                  result=brief(displaced), cut_at_right_qualification=cut(displaced, right),
                                  radius_difference_exceeds_delta=True, limit_radius="10/3",
                                  original_radius="6", limiting_radius_jump="8/3"),
                identities=dict(fixtures=identities, comparisons=identity_checks,
                                statement="for m<=k: first-qualified-LCA = ordinary-first-cover-LCA"),
                limits=["exclusive assigned mass can be below m", "no uniform 1-epsilon stability or continuity",
                        "no multi-K coherence proved", "no statistical quality or final selection acquired"],
                native_runs=0, fits=0, product_or_reference_imports=False)


if __name__ == "__main__":
    print(json.dumps(run(), ensure_ascii=False, sort_keys=True, separators=(",", ":")))
