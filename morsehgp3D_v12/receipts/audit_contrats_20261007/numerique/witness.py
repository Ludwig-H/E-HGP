#!/usr/bin/env python3
"""Témoins exacts indépendants pour le contrat numérique e264de6f2.

Python standard uniquement. Aucun moteur natif n'est exécuté. Les assertions
utilisent des exceptions explicites et restent actives avec python -O.
"""
import json
from fractions import Fraction


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def cross(a, b):
    return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])


def sub(a, b):
    return tuple(x-y for x, y in zip(a, b))


def extent(points):
    return max(max(p[j] for p in points)-min(p[j] for p in points) for j in range(3)).bit_length()


def q3(points):
    u, v = sub(points[1], points[0]), sub(points[2], points[0])
    w = cross(u, v)
    t = tuple(dot(u, u)*v[j]-dot(v, v)*u[j] for j in range(3))
    return 2*dot(w, w), cross(t, w)


def power_cert(d, n, s):
    return 0 < d < (1 << (123-2*s)) and all(abs(x) < (1 << (124-s)) for x in n)


def main():
    out = {"scope": "exact_python_models_not_native_v12", "contract_commit": "e264de6f2"}

    # CST certificat : chaque requête est dans la garde, mais le certificat
    # de support seul autorise un produit signé i128 réellement trop grand.
    m = 1 << 20
    h = m-1
    support = [(0, 0, 0), (h, h, 0), (h, 0, h)]
    query = (3*m-1,)*3
    d, n = q3(support)
    norm = dot(query, query)
    first = d*norm
    result = first-2*dot(n, query)
    acute = all(dot(sub(support[(i+1)%3], support[i]), sub(support[(i+2)%3], support[i])) > 0 for i in range(3))
    require(extent(support) == 20 and acute, "support aigu s20")
    require(all(-2*m < x < 3*m for x in query), "requête dans garde ouverte")
    require(power_cert(d, n, 20) and not power_cert(d, n, 22), "certificats s/s+2")
    require(first >= 1 << 127 and abs(result) < 1 << 127, "débordement intermédiaire i128")
    out["certificate_guard"] = dict(support=support, query=query, s=20, query_extent=extent(support+[query]),
        denominator=d, numerator=n, first_product=first, final_power=result,
        first_product_bits=first.bit_length(), final_power_bits=result.bit_length(),
        support_certificate=True, guarded_certificate=False)

    # Candidate non critique : les points et la requête sont tous en u32.
    # Le cercle exact du triangle obtus passe par la requête rejetée par la garde.
    support = [(419, 0, 0), (435, 15, 0), (434, 14, 0)]
    query = (0, 479, 0)
    d, n = q3(support)
    center = tuple(Fraction(support[0][j])+Fraction(n[j], d) for j in range(3))
    delta = sub(query, support[0])
    value = d*dot(delta, delta)-2*dot(n, delta)
    s = extent(support)
    anchor = tuple(min(p[j] for p in support) for j in range(3))
    passes = all(anchor[j]-2*(1 << s) < query[j] < anchor[j]+3*(1 << s) for j in range(3))
    require(s == 5 and value == 0 and not passes, "garde avant positivité perd coquille")
    out["noncritical_candidate"] = dict(support=support, query=query, s=s, center=[str(x) for x in center],
        power=value, passes_guard=passes, qualification="noncritical_exact_circumsphere")

    # Un nœud index contenant des points de la boule dépasse le pavé : il
    # n'est pas disjoint. Une règle 'pas inclus dans le pavé' est incorrecte.
    ball_support = [(100, 100, 100), (102, 100, 100)]
    box = [[0, 0, 0], [200, 200, 200]]
    s = extent(ball_support)
    anchor = [100]*3
    guard = [[x-2*(1 << s) for x in anchor], [x+3*(1 << s) for x in anchor]]
    require(s == 2 and any(box[0][j] <= guard[0][j] or box[1][j] >= guard[1][j] for j in range(3)), "boîte partielle")
    out["partial_box"] = dict(support=ball_support, box=box, guard=guard, contains_shell=True)

    # Centre continu et minimum entier ne sont pas le même argument.
    out["lattice_domain"] = dict(support=[[0, 0, 0], [1, 0, 0]], continuous_min_power="-1/2",
        lattice_min_power=0, continuous_projection=["1/2", "0", "0"])
    require(2*Fraction(1, 2)**2-2*Fraction(1, 2) == Fraction(-1, 2), "minimum continu q2")

    # Repère s30, G1 seul sûr i64 mais présélection de témoins non couverte.
    s = 30
    p = (1 << s)-1
    reservoir = 3*(2*p-1)**2
    require(reservoir >= 1 << 63, "réservoir dépasse i64 au palier G1")
    out["reservoir"] = dict(site=[p]*3, box=[[0]*3, [1]*3], s=s,
        value=reservoir, value_bits=reservoir.bit_length(), correct_i64_cutoff=29)

    # Le modèle reprend seulement les décisions key!=previous.key de
    # cloud.cpp:96-100 et 110-126 ; aucune exécution v12 n'est revendiquée.
    records = [((0, 0, 0), 1), ((1, 0, 0), 2), (((1 << 32)-1, 0, 0), 3)]
    shift = 11
    def key(p):
        return sum(((p[j] >> (i+shift)) & 1) << (3*i+j) for i in range(21) for j in range(3))
    ordered = sorted(records, key=lambda r: (key(r[0]), r[1]))
    model_sites = sum(i == 0 or key(r[0]) != key(ordered[i-1][0]) for i, r in enumerate(ordered))
    require(len(set(p for p, _ in records)) == 3 and model_sites == 2, "collision Morton fusionne positions")
    interleaved = [((0, 0, 0), 1), ((1, 0, 0), 2), ((0, 0, 0), 3)]
    require(all(interleaved[i][0] != interleaved[i-1][0] for i in range(1, 3)), "vrais doublons non adjacents")
    out["morton_identity"] = dict(records=records, keys=[key(p) for p, _ in records], shift=shift,
        exact_distinct_sites=3, inherited_key_grouping_sites=model_sites, interleaved_duplicates=interleaved)

    # Recalcul exhaustif des seuils garantis de la table, pas une preuve
    # de suffisance des formules : cette preuve figure dans REPORT.md.
    formulas = dict(dot=(2, 2), cross=(2, 1), determinant=(3, 3), g1=(2, 3),
        numerator3=(5, 5), denominator3=(4, 5), numerator4=(4, 5), denominator4=(3, 4),
        side=(6, 8), orientation=(7, 9), level_numerator=(8, 12), level_denominator=(6, 8),
        level_comparison=(14, 20), reservoir=(2, 4))
    out["cutoffs"] = {name: {"i64": (63-b)//a, "i128": (127-b)//a} for name, (a, b) in formulas.items()}
    out["absolute_center_max_u32"] = dict(numerator_bits=32+4*32+6, comparison_bits=32+8*32+11)
    print(json.dumps(out, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
