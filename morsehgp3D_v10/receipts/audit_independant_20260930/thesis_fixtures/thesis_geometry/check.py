#!/usr/bin/env python3
"""Petite contre-vérification exacte des dates, unités et exports déjà acquis."""
from dataclasses import dataclass
from fractions import Fraction as F
from pathlib import Path
import hashlib
import json
import math
import struct


def require(ok, message):
    if not ok:
        raise ValueError(message)


@dataclass(frozen=True)
class Q3:
    a: F = F(0)
    b: F = F(0)

    def __post_init__(self):
        object.__setattr__(self, "a", F(self.a))
        object.__setattr__(self, "b", F(self.b))

    @staticmethod
    def lift(x):
        return x if isinstance(x, Q3) else Q3(F(x))

    def __add__(self, x):
        x = self.lift(x)
        return Q3(self.a + x.a, self.b + x.b)

    __radd__ = __add__

    def __neg__(self):
        return Q3(-self.a, -self.b)

    def __sub__(self, x):
        return self + -self.lift(x)

    def __rsub__(self, x):
        return self.lift(x) - self

    def __mul__(self, x):
        x = self.lift(x)
        return Q3(self.a*x.a + 3*self.b*x.b, self.a*x.b + self.b*x.a)

    __rmul__ = __mul__

    def __truediv__(self, x):
        x = self.lift(x)
        n = x.a*x.a - 3*x.b*x.b
        require(n != 0, "division par zéro")
        return self * Q3(x.a/n, -x.b/n)

    def sign(self):
        if self.b == 0:
            return (self.a > 0) - (self.a < 0)
        if self.a == 0:
            return (self.b > 0) - (self.b < 0)
        sa, sb = (self.a > 0) - (self.a < 0), (self.b > 0) - (self.b < 0)
        if sa == sb:
            return sa
        n = self.a*self.a - 3*self.b*self.b
        require(n != 0, "égalité irrationnelle impossible avec coefficients rationnels non nuls")
        return sa if n > 0 else sb

    def text(self):
        return f"({self.a})+({self.b})sqrt(3)"


def dist2(a, b):
    return sum(((x-y)*(x-y) for x, y in zip(a, b)), Q3())


def midpoint(a, b):
    return tuple((x+y)/2 for x, y in zip(a, b))


def exact_ideal():
    s = Q3(0, 1)
    p = {"A": (-s, Q3(1)), "B": (-s, Q3(-1)), "C": (Q3(), Q3()),
         "D": (Q3(2), Q3()), "E": (2+s, Q3(1)), "F": (2+s, Q3(-1))}
    first = ["AB", "AC", "BC", "CD", "DE", "DF", "EF"]
    for ab in first:
        require(dist2(p[ab[0]], p[ab[1]]) == Q3(4), f"côté/pont {ab}")
    all_pairs = [a+b for i, a in enumerate(p) for b in list(p)[i+1:]]
    for ab in all_pairs:
        if ab not in first:
            require((dist2(p[ab[0]], p[ab[1]])-4).sign() > 0, f"autre paire {ab}")
    centres = [(tuple("ABC"), (-2*s/3, Q3())),
               (tuple("DEF"), (2+2*s/3, Q3()))]
    beta_tri = Q3(F(4, 3))
    for labels, c in centres:
        for label in labels:
            require(dist2(p[label], c) == beta_tri, "rayon triangle")
        for label in p:
            if label not in labels:
                require((dist2(p[label], c)-beta_tri).sign() > 0, "pas de point supplémentaire cercle")
    beta_global = Q3(2, 1)
    global_pairs = ["AD", "BD", "CE", "CF"]
    interiors = {"AD": "C", "BD": "C", "CE": "D", "CF": "D"}
    for ab in global_pairs:
        c = midpoint(p[ab[0]], p[ab[1]])
        require(dist2(p[ab[0]], c) == beta_global, "rayon global")
        inside, shell = [], []
        for label in p:
            sign = (dist2(p[label], c)-beta_global).sign()
            if sign < 0:
                inside.append(label)
            elif sign == 0:
                shell.append(label)
        require(inside == [interiors[ab]] and sorted(shell) == sorted(ab), "coquille globale complète")
    lambda_tri_z2 = Q3(F(3, 4))
    lambda_global_z2 = Q3(2, -1)
    require(beta_tri*lambda_tri_z2 == Q3(1), "lambda triangle z2")
    require(beta_global*lambda_global_z2 == Q3(1), "lambda global z2")
    lambda_tri_z1 = s/2
    require(lambda_tri_z1*lambda_tri_z1 == lambda_tri_z2, "lambda triangle z1")
    eta2 = F(81, 64)
    require((beta_tri-eta2).sign() > 0, "bande η1/8 sous fusion des triangles")
    # Les trois témoins de C sont AC/BC/CD. Après fusion ABC : deux voix gauche, une pont.
    require(F(2, 3) > F(1, 2), "majorité stricte idéale")
    return {"normalisation_r": 1, "K": 2, "premieres_paires": first,
            "beta_premiere": "1", "beta_triangle": "4/3", "beta_globale": beta_global.text(),
            "lambda_triangle_z2": lambda_tri_z2.text(),
            "lambda_globale_z2": lambda_global_z2.text(),
            "lambda_triangle_z1": lambda_tri_z1.text(),
            "lambda_globale_z1_squared": lambda_global_z2.text(),
            "fusions_globales": global_pairs, "interieurs_globales": interiors,
            "vote_C_triangle_uniforme_ou_1_beta": "2/3"}


def export_checks(base, name, dx):
    d = json.loads((base/f"{name}.json").read_text())
    raw = (base/f"{name}.u32le").read_bytes()
    coords = [(268, 3000, 0), (268, 1000, 0), (2000, 2000, 0),
              (4000+dx, 2000, 0), (5732+dx, 3000, 0), (5732+dx, 1000, 0)]
    require(list(struct.iter_unpack("<III", raw)) == coords, "identité des six sites du brut")
    require(d["K"] == d["kmax_catalogue"] == 2 and d["n_sites"] == d["n_points"] == 6,
            "univers export")
    require(d["orders_built"] == "only_order" and d["coordinate_bits"] == 18, "profil export")
    require(d["engine_commit"] == "e9eab2754f3d9f2a9e16d8c542b908c19b725c9a", "provenance déclarée")
    sites = d["sites"]
    require(len(sites) == 6 and sorted(x[0] for x in sites) == list(range(6)), "permutation IDs")
    for original, *xyz in sites:
        require(tuple(xyz) == coords[original], "identité site export")
    levels = [F(int(a), int(b)) for a, b in d["levels"]]
    h, r0 = 1732, 1000
    side2 = h*h+r0*r0
    beta_side, beta_base = F(side2, 4), F(r0*r0)
    beta_bridge = F((2*r0+dx)**2, 4)
    beta_tri = F(side2*side2, 4*h*h)
    beta_global = F((2*r0+h+dx)**2+r0*r0, 4)
    require(set(levels) == {F(0), beta_side, beta_base, beta_bridge, beta_tri, beta_global}, "dates export")
    balls = d["balls"]
    order = d["orders"][0]
    require(order["k"] == 2 and len(order["nodes"]) == 10 and len(balls) == 13, "taille export")
    pairs_expected = {tuple(sorted(pair)) for pair in [(0,1),(0,2),(1,2),(2,3),(3,4),(3,5),(4,5)]}
    pairsets, nb_tri, nb_global = set(), 0, 0
    for ball in balls:
        c = tuple(F(int(a), int(ball["c"][-1])) for a in ball["c"][:-1])
        beta = levels[ball["lv"]]
        inside, shell = [], []
        for i, site in enumerate(sites):
            e = sum((F(v)-u)**2 for v, u in zip(site[1:], c))-beta
            if e < 0:
                inside.append(i)
            elif e == 0:
                shell.append(i)
        require(inside == ball["I"] and shell == ball["U"], "incidences exactes indépendantes")
        require(len(inside) == ball["p"] and len(shell) == ball["u"], "populations")
        labels = tuple(sorted(sites[i][0] for i in ball["S"]))
        if ball["p"]+ball["q"] <= 2:
            require(ball["q"] == 2 and beta <= beta_base, "univers fort de vote")
            pairsets.add(labels)
        elif ball["q"] == 3:
            require(ball["p"] == 0 and beta == beta_tri, "fusion triangle K+1")
            nb_tri += 1
        else:
            require(ball["q"] == 2 and ball["p"] == 1 and beta == beta_global, "fusion globale K+1")
            nb_global += 1
    require(pairsets == pairs_expected and nb_tri == 2 and nb_global == 4, "catalogue observé")
    nodes = order["nodes"]
    roots = [i for i, n in enumerate(nodes) if n[1] == -1]
    require(len(roots) == 1, "racine unique")
    root = roots[0]
    require(levels[nodes[root][0]] == beta_global and len(nodes[root][3]) == 3, "plateau trois parents")
    joins = [i for i, n in enumerate(nodes) if levels[n[0]] == beta_tri]
    require(len(joins) == 2 and all(len(nodes[j][3]) == 3 and nodes[j][1] == root for j in joins),
            "fusions triangles")
    eta2 = F(81, 64)
    min_c = min(beta_side, beta_bridge)
    require(beta_base <= eta2*min_c < beta_tri, "bande de C/D inclut toutes premières paires uniquement")
    require(beta_tri < F(13,10)**2*r0*r0 < F(17,10)**2*r0*r0 < beta_global,
            "intervalle 1.3r..1.7r dans plateau séparé FULL")
    require(order["core_node"] == [root]*6, "core retardé observé")
    for i, site in enumerate(sites):
        nearest = min(sum((F(x)-F(y))**2 for x,y in zip(site[1:], other[1:]))
                      for j,other in enumerate(sites) if i != j)
        require(nearest == order["core_level"][i], "distance core exacte")
        require(nearest > beta_global, "entrée core après fusion globale")
    return {"fixture": name, "translation_DEF_x": dx, "r_echelle": r0,
            "beta_inclinees": str(beta_side), "beta_verticales": str(beta_base),
            "beta_pont": str(beta_bridge), "beta_triangles": str(beta_tri),
            "beta_globale": str(beta_global), "rayon_triangles_affichage": math.sqrt(float(beta_tri)),
            "rayon_global_affichage": math.sqrt(float(beta_global)),
            "delta_cote_incline_vs_2000_affichage": 2000-math.sqrt(side2),
            "lambda_triangles_z2": str(1/beta_tri), "lambda_globale_z2": str(1/beta_global),
            "bornes_bande_C_beta": [str(min_c), str(eta2*min_c)],
            "boules_fortes_vote": len(pairsets), "boules_fusion_triangles": nb_tri,
            "boules_fusion_globale": nb_global, "enfants_racine": len(nodes[root][3])}


def verify_payloads(base):
    manifest = base/"SHA256SUMS"
    if manifest.exists():
        count = 0
        for row in manifest.read_text().splitlines():
            expected, filename = row.split("  ", 1)
            require(hashlib.sha256((base/filename).read_bytes()).hexdigest() == expected, "SHA256 "+filename)
            count += 1
        return count
    return 0


def main():
    base = Path(__file__).resolve().parent
    verify_payloads(base)
    result = {"scope": "formules exactes; lecture indépendante de deux exports hérités, sans appel moteur",
              "ideal": exact_ideal(),
              "exports": [export_checks(base/"inputs", "aretes_plus_courtes", 0),
                          export_checks(base/"inputs", "pont_plus_court", -2)]}
    print(json.dumps(result, sort_keys=True, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
