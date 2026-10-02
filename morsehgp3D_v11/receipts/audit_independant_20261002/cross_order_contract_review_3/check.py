"""Temoin Γ collineaire : familles de descendants core a K1/K2."""
from fractions import Fraction as F
import json
from model import model


def require(ok, note):
    if not ok:
        raise RuntimeError(note)


def render(nodes, entries):
    return {"nodes": [{"beta": str(n["birth"]), "children": n["children"],
                       "core_points": [xs[i] for i in sorted(n["points"])]}
                      for n in nodes],
            "entries": [{"x": xs[i], "beta": str(a), "node": n}
                        for i, a, n in entries]}


xs = (0, 10, 11, 26, 27, 45, 46)
first, e1 = model(xs, 1)
second, e2 = model(xs, 2)
u = {0, 1, 2}
v = {1, 2, 3, 4}
a = next(i for i, n in enumerate(first) if n["points"] == u)
b = next(i for i, n in enumerate(second) if n["points"] == v)
pa = next(n for n in first if a in n["children"])
pb = next(n for n in second if b in n["children"])
require(first[a]["birth"] == 25 and pa["birth"] == F(225, 4), "branche K1")
require(second[b]["birth"] == 64 and pb["birth"] == F(361, 4), "branche K2")
require(dict((i, lev) for i, lev, _ in e2) == {0: F(100), **{i: F(1) for i in range(1,7)}}, "dates core K2")
require(all(a == 0 for _, a, _ in e1), "dates core K1")
require(u & v == {1, 2} and u-v == {0} and v-u == {3, 4}, "croisement strict")
require(pb["birth"] < 100, "0 arrive apres le parent K2")
require(max(xs) < 2**18, "profil entier u18")
for nodes in (first,second):
    for x in nodes:
        for y in nodes:
            require(not (x["points"] & y["points"] and x["points"]-y["points"] and y["points"]-x["points"]),
                    "chaque ordre reste laminaire")
print(json.dumps({"status": "PASS", "points": xs, "k1": render(first,e1),
                  "k2": render(second,e2), "crossing": {"k1_node": a, "k2_node": b,
                  "k1_group": [xs[i] for i in sorted(u)], "k2_group": [xs[i] for i in sorted(v)]},
                  "scope": "independent collinear Gamma only; no native/reference call"}, sort_keys=True, indent=2))
