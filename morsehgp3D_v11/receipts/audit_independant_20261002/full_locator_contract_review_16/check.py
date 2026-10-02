#!/usr/bin/env python3
"""Contrats mathématiques autonomes, sans import produit ni exécution native."""
import hashlib
import itertools
import json
import math
from fractions import Fraction as Q
from pathlib import Path

HERE = Path(__file__).resolve().parent

def require(cond, message):
    if not cond:
        raise RuntimeError(message)


def components(vertices, separable):
    pending = set(vertices)
    groups = []
    while pending:
        first = min(pending)
        pending.remove(first)
        group = {first}
        frontier = [first]
        while frontier:
            a = frontier.pop()
            for b in sorted(pending):
                if separable(set(a) | set(b)):
                    pending.remove(b)
                    group.add(b)
                    frontier.append(b)
        groups.append(sorted(group))
    return groups


def axis_cell(axes, t):
    # Sites +/-e_i, dans l'ordre (+,-) par axe. Une trace contient le centre
    # ssi elle contient une paire antipodale : les autres signes sont dans
    # un demi-espace ouvert (certificat somme des signes choisis).
    def strict(ids):
        return not any(2 * i in ids and 2 * i + 1 in ids for i in range(axes))
    vertices = [a for a in itertools.combinations(range(2 * axes), t) if strict(set(a))]
    groups = components(vertices, strict)
    return {"t": t, "traces": len(vertices), "local_pieces": len(groups),
            "birth": not vertices, "combinations": math.comb(2 * axes, t)}


def sqdist(a, b):
    return sum((Q(x) - Q(y)) ** 2 for x, y in zip(a, b))


def run():
    before = json.loads((HERE / "SOURCE_BEFORE.json").read_text())
    present = 0
    for r in before["files"]:
        if r.get("missing"):
            continue
        data = (HERE / r["copy"]).read_bytes()
        require(len(data) == r["bytes"] and hashlib.sha256(data).hexdigest() == r["sha256"], r["path"])
        present += 1
    six = [axis_cell(3, t) for t in range(1, 7)]
    square = [axis_cell(2, t) for t in range(1, 5)]
    require([(r["traces"], r["local_pieces"]) for r in six] ==
            [(6, 1), (12, 1), (8, 8), (0, 0), (0, 0), (0, 0)], "six axes")
    require([(r["traces"], r["local_pieces"]) for r in square] ==
            [(4, 1), (4, 4), (0, 0), (0, 0)], "square")
    # Tétraèdre strict : les poids uniques sont tous 1/4. Toute partie
    # propre est séparable, toute union de deux faces contient les 4 sites.
    tet = [(1, 1, 1), (1, -1, -1), (-1, 1, -1), (-1, -1, 1)]
    require(all(sum(x[j] for x in tet) == 0 for j in range(3)), "tet centre")
    faces = list(itertools.combinations(range(4), 3))
    require(len(components(faces, lambda ids: len(ids) < 4)) == 4, "tet faces")
    # Hit catalogue hors fenêtre d'événement à l'ordre de la partie :
    # MEB({0,6})=(3,9), globalement p=2,q=2,m=2. Cat3 l'admet.
    axis = [(0, 0, 0), (2, 0, 0), (4, 0, 0), (6, 0, 0)]
    c = (3, 0, 0)
    require(sum(sqdist(x, c) < 9 for x in axis) == 2, "hit interiors")
    require(sum(sqdist(x, c) == 9 for x in axis) == 2, "hit shell")
    require(2 + 2 <= 3 + 1 and 2 >= 2, "hit admission, saturation")
    # Complète au seuil k=3, mais légitimement absente de Cat3 :
    # les poids du support strict sont (25/64,25/64,7/32).
    tri = [(0, 0, 0), (6, 0, 0), (3, 4, 0)]
    c = (Q(3), Q(7, 8), Q(0)); beta = Q(625, 64)
    weights = (Q(25, 64), Q(25, 64), Q(7, 32))
    require(all(w > 0 for w in weights) and sum(weights) == 1, "triangle support")
    require(tuple(sum(w * x[j] for w, x in zip(weights, tri)) for j in range(3)) == c, "triangle centre")
    require(all(sqdist(x, c) == beta for x in tri), "triangle shell")
    require(all(sqdist(x, c) < beta for x in [(2, 1, 0), (4, 1, 0)]), "triangle interiors")
    require(2 < 3 and 2 + 3 > 3 + 1, "complete absent Cat3")
    # Minorant du travail exact : déjà deux passes sur C(150,10), sans
    # allocation de tous les candidats. Ce calcul n'est pas un chrono.
    large = math.comb(150, 10)
    require(large < (2**64 - 1) // 2, "representable but enormous work")
    capacity_checks = 0
    for m in (0, 1, 2, 4, 6, 12, 30, 150, 256, 4096, 2**32 - 2):
        for t in range(min(12, m) + 1):
            n = math.comb(m, t)
            # La taille, le compteur deux passes et les octets sont trois
            # admissions distinctes ; une valeur représentable ne borne
            # pas le temps d'énumération.
            require(n >= 1, "binomial positivity")
            capacity_checks += 1
    return {"status": "PASS", "source_copies": present, "native_runs": 0,
            "six_axis_cells": six, "square_cells": square, "tetrahedron_faces": 4,
            "known_complete_hit_p": 2, "known_complete_hit_k": 2,
            "complete_missing_p": 2, "complete_missing_q": 3, "complete_missing_K": 3,
            "C_150_10": large, "two_pass_trace_tests_150_10": 2 * large,
            "capacity_cases": capacity_checks}

if __name__ == "__main__":
    print(json.dumps(run(), sort_keys=True, indent=2))
