#!/usr/bin/env python3
"""Deux gardes géométriques v11 : continuum de centres, masse exclusive.

Fraction, trois sites sur un axe ; aucun moteur/FULL natif importé ou appelé.
Le helper em_geo figé est confronté à des intervalles dérivés indépendamment.
"""
from fractions import Fraction as F
from itertools import combinations
from pathlib import Path
import hashlib
import importlib.util
import json


def require(ok, message):
    if not ok:
        raise ValueError(message)


def distance_interval(x, lo, hi):
    return max(lo-x, F(0), x-hi)


def pair_interval(a, b, radius):
    require(a <= b and 2*radius >= b-a, "lentille non vide")
    return b-radius, a+radius


def mature_independent(axis, pair, x, tau, radius):
    lo, hi = pair_interval(axis[pair[0]], axis[pair[1]], radius)
    return distance_interval(axis[x], lo, hi) <= tau*radius


def gamma_axis(axis, radius):
    beta = radius*radius
    pairs = [p for p in combinations(range(len(axis)), 2)
             if (axis[p[1]]-axis[p[0]])**2/4 <= beta]
    parent = list(range(len(pairs)))
    def root(i):
        while parent[i] != i:
            i = parent[i]
        return i
    for i,j in combinations(range(len(pairs)), 2):
        sites = sorted(set(pairs[i]) | set(pairs[j]))
        union_meb = (axis[sites[-1]]-axis[sites[0]])**2/4
        if union_meb <= beta:
            a,b = root(i),root(j)
            parent[b] = a
    groups = {}
    for i,pair in enumerate(pairs):
        groups.setdefault(root(i), []).append(pair)
    return sorted(tuple(sorted(pairs_)) for pairs_ in groups.values())


def source_helper(base):
    path = base/"sources"/"em_geo.py"
    spec = importlib.util.spec_from_file_location("maturity_review_frozen_geo", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def manifest_check(base):
    path = base/"SHA256SUMS"
    if path.exists():
        for row in path.read_text().splitlines():
            expected, relative = row.split("  ", 1)
            require(hashlib.sha256((base/relative).read_bytes()).hexdigest() == expected,
                    "SHA256 "+relative)


def run():
    base = Path(__file__).resolve().parent
    manifest_check(base)
    geo = source_helper(base)
    axis = tuple(map(F, (0,2,4)))
    points = [(x,F(0),F(0)) for x in axis]
    tau = F(1,2)
    left, right = (0,1),(1,2)
    birth, maturity, fusion = F(1),F(4,3),F(2)
    tests = 0
    rows = []
    for radius in (birth,F(5,4),maturity-F(1,1000),maturity,F(3,2),fusion-F(1,1000)):
        components = gamma_axis(axis,radius)
        require(components == [(left,),(right,)], "deux composantes distinctes avant fusion")
        witnesses = []
        for pair in (left,right):
            accepted = []
            for x in range(3):
                exact = mature_independent(axis,pair,x,tau,radius)
                observed = geo.mur(points,pair,x,tau,radius*radius)
                require(observed == exact, "em_geo vs intervalle indépendant")
                tests += 1
                if exact:
                    accepted.append(x)
            witnesses.append(accepted)
        rows.append({"radius": str(radius), "mature_site_ids": witnesses})
    require(gamma_axis(axis,fusion) == [((0,1),(0,2),(1,2))], "plateau fermé : racine à fusion")
    tests += 1
    require(rows[2]["mature_site_ids"] == [[],[]] and rows[3]["mature_site_ids"] == [[0,1],[1,2]],
            "contact exact à 4/3")
    tests += 1

    # Deux tailles géométriques ≥mcs=2 ne peuvent fonder deux blocs exclusifs de taille2 sur3 sites.
    choices = []
    for owners in ((0,a,1) for a in (0,1)):
        masses = [owners.count(0), owners.count(1)]
        require(sum(masses) == 3 and min(masses) < 2, "pigeonhole / conservation")
        choices.append({"owner_per_site": list(owners), "exclusive_masses": masses})
        tests += 1
    geometric_total = 4
    require(geometric_total > len(axis), "masse géométrique non exclusive")
    tests += 1

    # Une composante est un ensemble de centres, pas le seul centre MEB ni ses points couverts.
    lo,hi = pair_interval(F(0),F(2),maturity)
    continuum_distance = distance_interval(F(0),lo,hi)
    representative_distance = F(1)
    covered_points_distance = F(0)
    require(continuum_distance == F(2,3) == tau*maturity, "distance au continuum")
    require(representative_distance > tau*maturity and covered_points_distance < tau*maturity,
            "deux substituts donnent une autre décision/date")
    tests += 2

    # Projection valide retardée : x0 entre dans la branche01 à e=3/2, avant sa mort2.
    # La maturité intrinsèque est4/3 ; F/C exige ensuite m>=e pour cette projection.
    entry = F(3,2)
    require(birth <= entry < fusion and distance_interval(F(0), *pair_interval(F(0),F(2),entry)) <= entry,
            "propriétaire vivant et couvrant à l'entrée retardée")
    require(maturity < entry and max(maturity,entry) == entry, "plancher de projection nécessaire")
    tests += 2

    return {"status": "MATURITY_GEOMETRY_CONTRACT_PASS", "guards": tests,
            "sites": [[str(c) for c in p] for p in points], "K": 2, "tau": "1/2", "mcs": 2,
            "branch_birth_radius": str(birth), "intrinsic_maturity_radius": str(maturity),
            "branch_death_global_fusion_radius": str(fusion), "cuts": rows,
            "at_maturity_geometric_sizes": [2,2], "sum_geometric_sizes": geometric_total,
            "distinct_sites_in_union": 3, "possible_exclusive_assignments": choices,
            "distance_to_continuum": str(continuum_distance),
            "distance_to_MEB_representative": str(representative_distance),
            "distance_to_covered_point_set": str(covered_points_distance),
            "delayed_projection_entry_radius": str(entry),
            "causal_clamped_maturity_radius": str(max(entry,maturity)),
            "native_engine_calls": 0,
            "scope": "contrat v11; limitations reconnues; aucun défaut nouveau du helper actuel"}


if __name__ == "__main__":
    print(json.dumps(run(),ensure_ascii=False,sort_keys=True,indent=2))
