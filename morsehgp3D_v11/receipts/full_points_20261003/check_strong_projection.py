#!/usr/bin/env python3
"""Fermeture des points par les seules populations fortes : preuve et gardes.

Portée : sites distincts, de poids 1 ; 2 <= k <= n ; seuil de transmission
1 <= m <= k. Ce résultat calcule le QUOTIENT de fermeture sur les points,
jamais la tour FULL, ses naissances, multifusions, parents ou verticales.
Il ne réhabilite donc ni le fold v4 ni son échec E5.

Énoncé. À rayon carré beta, prendre toutes les boules critiques positives du
catalogue complet qui vérifient p + qmin <= k <= p + |U|, à un niveau <= beta.
Fermer transitivement leurs populations COMPLÈTES I union U donne exactement
la fermeture des couvertures des composantes FULL_k. Les points inactifs
restent des singletons dans la partition totale, avec une entrée séparée.
Les dates d'entrée et les hauteurs de coassociation sont donc les mêmes.

Preuve, première étape. La couverture d'une composante FULL est l'union des
k-parties actives de sa composante Gamma_k. Pour k >= 2, une arête Gamma_k
relie des k-parties qui partagent k-1 >= 1 sites. Leur fermeture sur les
points les réunit déjà, sans avoir à calculer cette arête. Réciproquement,
fermer chacune des couvertures réunit toutes ses k-parties. Ces deux
fermetures sont égales. Chaque couverture contient au moins k sites : tous
les seuils m <= k qualifient donc exactement les mêmes couvertures.

Deuxième étape : les forts engendrent toutes les k-parties actives. Induire
sur les niveaux, en nombre fini, des MEB des k-parties. Soit F une k-partie,
b = MEB(F), c son centre et I/U ses populations dans le nuage GLOBAL.
Son support local est dans F, donc qmin(b) <= k ; p + |U| >= k.
Si p + qmin <= k, b est forte et sa population contient F.
Sinon k < p + qmin, donc p >= 1. Fixer H inclus dans I de cardinal
h = min(p, k-1) > 0. Alors t = k-h < qmin : soit h=p et t=k-p<qmin,
soit h=k-1 et t=1<qmin (rayon positif et sites distincts).
Pour chaque x dans F, choisir une k-partie F_x dans I union U contenant H
et x, en complétant par t sites hors H. Elle contient au plus t<qmin sites
de U. Le centre c n'est pas dans leur enveloppe convexe : sinon un support
positif de cardinal <qmin existerait. Une direction séparatrice rapproche
strictement c de tous les sites de coquille choisis ; les intérieurs
restent strictement intérieurs pour un déplacement suffisamment petit.
Ainsi beta(F_x) < beta(F). Par induction, chaque F_x est reliée par les
populations fortes antérieures. Toutes partagent H non vide : F est reliée.
Le rayon nul ne survient pas pour une k-partie de sites distincts, k>=2.

Réciproque. La population d'une boule forte contient au moins k sites.
Chaque paire de cette population appartient à une k-partie contenue dans
la même boule, de MEB <= beta(b). Sa fermeture par les k-parties actives
contient donc toute la population, sans imposer de liaison supplémentaire.
L'égalité des ensembles actifs prouve également l'égalité des entrées.
Tous ces forts appartiennent à Cat_K dès K>=k, car p+qmin<=k<=K.

Limites. À k=1, les vertex de Gamma sont disjoints : X={0,2}, beta=1, m=1
est un contre-exemple (FULL couvre les deux sites ensemble ; les forts k1
sont seulement les singletons de niveau 0).

Nuance m=k+1. L'identité connue avec la fermeture d'ordre k+1 autorise les
forts d'ordre k+1 SANS catalogue supplémentaire : leurs conditions sont
exactement celles des faibles à k, p+qmin <= k+1 <= p+|U|. Ils sont déjà
dans Cat_k de FULL, dont l'admission est p+qmin <= k+1, même si Kmax=k.
Au rayon positif, qmin>=2 impose p<=k-1 : aucun refus du census à p>=k.
Le k-ième voisin est sur U, donc les listes k-certifiées avec TOUS les ex
aequo contiennent I et U complets. Au rayon nul, un site distinct isolé a
qmin=1 et une population de cardinal1 : il ne peut être fort d'ordre k+1.
Ni forêt FULL_{k+1} ni nouveaux événements géométriques ne sont donc requis
pour ce quotient. Ce fait ne transfère aucune qualification à un nouveau
code/API. Si k>=n, seuil k+1 et forts d'ordre k+1 sont tous deux inactifs.
Pour m>k+1, aucun remplacement de FULL n'est prouvé ; qualifier les blocs
du quotient pourrait percoler trop tôt.

Recoupement c40 : src/catalogue/leaf.cpp lignes152,168,186, admission et
rejet strict au-delà de theta_q=k+1-q sur la présentation canonique ;
reference/hgp11_ref/constructive.py applique p+qmin<=K+1 après census p<K.
Le programme ne charge ni n'exécute ces sources produit. Le complément
compare Cat_k théorique et ses faibles aux forts k+1, puis leurs coupes
aux couvertures qualifiées FULL_k, sur les dix mêmes fixtures.

Programme borné : Gram/Fraction et Gamma indépendants de l'implémentation
produit, réutilisés depuis experiment/test_qualified.py (sa fonction main
et ses imports optionnels de références ne sont pas exécutés). Les forts
sont recensés directement depuis les centres critiques et TOUT le nuage,
sans utiliser les propriétaires ni les records exportés par le modèle.
734 gardes = 458 coupes fermées (partitions, activité ET entrées cumulées)
et 276 k-parties vérifiées à leur propre date. Aucun moteur, fit ou GCP.
"""

from fractions import Fraction
from hashlib import sha256
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "experiment"))
from test_qualified import Oracle  # noqa: E402; own bounded Gram/Fraction model


def require(value, reason):
    if not value:
        raise ValueError(reason)


def closure(n, populations):
    parent = list(range(n))
    active = set()

    def find(site):
        while parent[site] != site:
            site = parent[site]
        return site

    for population in populations:
        population = tuple(population)
        active.update(population)
        for site in population[1:]:
            parent[find(site)] = find(population[0])
    groups = {}
    for site in range(n):
        groups.setdefault(find(site), set()).add(site)
    return {frozenset(group) for group in groups.values()}, active


def strong_events(oracle, k):
    """No owner lookup: exact global I/U census of all critical centers."""
    events = []
    for (beta, center), support in sorted(oracle.critical.items()):
        inner, shell = [], []
        for site, point in enumerate(oracle.points):
            distance = sum((x-y)**2 for x, y in zip(point, center))
            if distance < beta:
                inner.append(site)
            elif distance == beta:
                shell.append(site)
        p, qmin, shell_size = len(inner), len(support), len(shell)
        if p+qmin <= k <= p+shell_size and (k > 1 or beta == 0):
            events.append((beta, tuple(inner+shell)))
    return events


def weak_events_from_cat(oracle, k):
    """Actual Cat_k admission contract, followed by the weak(k) condition."""
    events = []
    for (beta, center), support in sorted(oracle.critical.items()):
        if beta == 0:  # Native positive catalogue starts at qmin2.
            continue
        inner, shell = [], []
        for site, point in enumerate(oracle.points):
            distance = sum((x-y)**2 for x, y in zip(point, center))
            if distance < beta:
                inner.append(site)
            elif distance == beta:
                shell.append(site)
        p, qmin = len(inner), len(support)
        if p >= k or p+qmin > k+1:
            continue
        if k+1 <= p+len(shell):
            events.append((beta, tuple(inner+shell)))
    return events


def run():
    fixtures = {
        "pair": [(0, 0, 0), (2, 0, 0)],
        "line024": [(x, 0, 0) for x in (0, 2, 4)],
        "line01269": [(x, 0, 0) for x in (0, 1, 2, 6, 9)],
        "square": [(0, 0, 0), (2, 0, 0), (2, 2, 0), (0, 2, 0)],
        "octa": [(3, 2, 2), (1, 2, 2), (2, 3, 2), (2, 1, 2),
                 (2, 2, 3), (2, 2, 1)],
        "generic6": [(0, 0, 0), (2, 1, 0), (0, 3, 1), (4, 0, 2),
                     (2, 4, 3), (6, 2, 1)],
        "equilateral": [(1, 1, 2), (1, 2, 1), (2, 2, 2), (3, 3, 2),
                        (4, 4, 2), (4, 3, 3)],
        "triangle_inner": [(0, 0, 0), (6, 0, 0), (3, 4, 0),
                           (2, 1, 0), (4, 1, 0)],
        "two_pairs": [(x, 0, 0) for x in (0, 2, 100, 102)],
        "shared_boundary": [(6, 2, 0), (0, 0, 0), (0, 4, 0),
                            (12, 0, 0), (12, 4, 0)],
    }
    checks = 0
    cuts = 0
    summaries = []
    k1_guard = None
    weak_catalogue_checks = 0
    weak_cut_checks = 0
    for name, points in fixtures.items():
        oracle = Oracle(points, len(points))
        case_cuts = 0
        entries_by_order = {}
        for k in range(2, oracle.n+1):
            events = strong_events(oracle, k)
            entries = [[None]*oracle.n for _ in range(3)]
            for beta in oracle.levels:
                from_strong = closure(oracle.n, [population for level, population
                                                 in events if level <= beta])
                from_parts = closure(oracle.n, [part for part in oracle.parts[k]
                                                if oracle.meb(part)[0] <= beta])
                from_full = closure(oracle.n, [set(site for part in component for site in part)
                                               for component in oracle.gamma(k, beta)])
                for dates, (_groups, active) in zip(entries, (from_strong, from_parts, from_full)):
                    for site in active:
                        if dates[site] is None:
                            dates[site] = beta
                require(from_strong == from_parts == from_full and entries[0] == entries[1] == entries[2],
                        (name, k, str(beta), "strong/parts/FULL partition, activity or entries"))
                checks += 1
                cuts += 1
                case_cuts += 1
            require(all(date is not None for date in entries[0]), (name, k, "terminal entries"))
            entries_by_order[str(k)] = [str(date) for date in entries[0]]
            for part in oracle.parts[k]:
                beta = oracle.meb(part)[0]
                partition, active = closure(oracle.n, [population for level, population
                                                       in events if level <= beta])
                require(set(part) <= active and any(set(part) <= group for group in partition),
                        (name, k, part, "active k-part generated at its own MEB date"))
                checks += 1
        if name == "pair":
            beta = Fraction(1)
            from_strong = closure(oracle.n, [population for level, population
                                             in strong_events(oracle, 1) if level <= beta])
            from_full = closure(oracle.n, [set(site for part in component for site in part)
                                           for component in oracle.gamma(1, beta)])
            require(from_strong != from_full, "k1 counterexample missing")
            k1_guard = dict(points=points, k=1, m=1, beta=str(beta),
                            strong_blocks=sorted(map(sorted, from_strong[0])),
                            full_cover_blocks=sorted(map(sorted, from_full[0])))
        summaries.append(dict(name=name, points=points, closed_cut_checks=case_cuts,
                              common_entry_dates=entries_by_order))
        weak_entries = {}
        for k in range(1, oracle.n+1):
            weak = weak_events_from_cat(oracle, k)
            next_strong = strong_events(oracle, k+1)
            require(weak == next_strong, (name, k, "Cat_k weak events exactly strong(k+1)"))
            weak_catalogue_checks += 1
            entries = [[None]*oracle.n for _ in range(3)]
            for beta in oracle.levels:
                from_weak = closure(oracle.n, [population for level, population in weak if level <= beta])
                from_next = closure(oracle.n, [part for part in oracle.parts.get(k+1, ())
                                               if oracle.meb(part)[0] <= beta])
                qualified_cover = [set(site for part in component for site in part)
                                   for component in oracle.gamma(k, beta)]
                from_full = closure(oracle.n, [cover for cover in qualified_cover if len(cover) >= k+1])
                for dates, (_groups, active) in zip(entries, (from_weak, from_next, from_full)):
                    for site in active:
                        if dates[site] is None:
                            dates[site] = beta
                require(from_weak == from_next == from_full and entries[0] == entries[1] == entries[2],
                        (name, k, str(beta), "weak Cat_k/next parts/qualified FULL_k"))
                weak_cut_checks += 1
            weak_entries[str(k)] = [None if date is None else str(date) for date in entries[0]]
        summaries[-1]["weak_next_common_entry_dates"] = weak_entries
    require(checks == 734 and cuts == 458 and len(fixtures) == 10, "guard census changed")
    dependencies = {str(path.relative_to(HERE)): sha256(path.read_bytes()).hexdigest()
                    for path in (Path(__file__).resolve(), HERE / "experiment/test_qualified.py",
                                 HERE / "experiment/qualified.py")}
    return dict(status="pass", schema="ehgp.v11.strong_projection_exact_check.v1",
                scope="point quotient only; not FULL; unit distinct sites; k>=2; 1<=m<=k",
                checks=checks, closed_cut_checks=cuts, kpart_birth_checks=checks-cuts,
                weak_catalogue_set_checks=weak_catalogue_checks, weak_next_closed_cut_checks=weak_cut_checks,
                total_checks=checks+weak_catalogue_checks+weak_cut_checks,
                fixtures=summaries, k1_counterexample=k1_guard, dependencies=dependencies,
                strong_source="direct critical-center global census; no owners or exported records",
                product_or_reference_imports=False, native_runs=0, fits=0,
                next_order="m=k+1 uses weak events already present in complete Cat_k of FULL; no FULL_{k+1} needed; new code/API unqualified",
                larger_threshold="m>k+1 replacement of FULL not proved")


if __name__ == "__main__":
    print(json.dumps(run(), ensure_ascii=False, sort_keys=True, separators=(",", ":")))
