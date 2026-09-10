"""Auditeur : K1 de la tour == single-linkage exact (rationnel) sur le nuage genere.
Aucune dependance au C++ produit ; refus explicites, jamais assert."""
import json, sys
from collections import Counter
from fractions import Fraction

def fail(msg):
    print(json.dumps({"status": "refused", "reason": msg}))
    sys.exit(1)

def main(path):
    d = json.load(open(path))
    pts = {p[0]: (p[1], p[2], p[3]) for p in d["points"]}
    if len(pts) != len(d["points"]):
        fail("ids dupliques")
    ids = sorted(pts)
    n = len(ids)
    # reference : toutes les paires, distances carrees exactes
    pairs = {}
    for i in range(n):
        xi, yi, zi = pts[ids[i]]
        for j in range(i + 1, n):
            xj, yj, zj = pts[ids[j]]
            d2 = (xi - xj) ** 2 + (yi - yj) ** 2 + (zi - zj) ** 2
            if d2 == 0:
                fail("positions dupliquees")
            pairs.setdefault(d2, []).append((ids[i], ids[j]))
    parent = {i: i for i in ids}
    members = {i: frozenset([i]) for i in ids}
    def find(a):
        while parent[a] != a:
            parent[a] = parent[parent[a]]
            a = parent[a]
        return a
    ref_events = Counter()
    equal_level_lots = 0
    for d2 in sorted(pairs):
        edges = pairs[d2]
        if len(edges) > 1:
            equal_level_lots += 1
        touched = {}
        for a, b in edges:
            ra, rb = find(a), find(b)
            for r in (ra, rb):
                touched.setdefault(r, members[r])
            if ra != rb:
                parent[rb] = ra
        groups = {}
        for old_root, cover in touched.items():
            groups.setdefault(find(old_root), []).append(cover)
        for new_root, covers in groups.items():
            if len(covers) >= 2:
                ref_events[(Fraction(d2, 4), frozenset(covers))] += 1
                members[new_root] = frozenset().union(*covers)
    # tour K1
    order = [o for o in d["orders"] if o["K"] == 1]
    if len(order) != 1:
        fail("ordre K1 absent")
    nodes = order[0]["nodes"]; contribs = order[0]["contributions"]
    def level(o):
        num = o["num"][0] + (o["num"][1] << 64) + (o["num"][2] << 128)
        den = int(o["den"])
        if den <= 0:
            fail("denominateur non positif")
        return Fraction(num, den)
    cover = {}
    births = 0
    for c in contribs:
        if level(c) != 0:
            fail("contribution K1 hors niveau zero")
        seg = c["segment"]
        if seg in cover:
            fail("deux contributions pour le meme segment K1")
        if len(c["points"]) != 1:
            fail("naissance K1 non singleton")
        cover[seg] = frozenset(c["points"])
    tower_events = Counter()
    for idx, nd in enumerate(nodes):
        if not nd["parents"]:
            births += 1
            if idx not in cover:
                fail("naissance K1 sans contribution")
            continue
        covers = []
        for p in nd["parents"]:
            if p >= idx or p not in cover:
                fail("parent non anterieur ou sans couverture")
            covers.append(cover[p])
        cover[idx] = frozenset().union(*covers)
        tower_events[(level(nd), frozenset(covers))] += 1
    if births != n or set().union(*[cover[i] for i in range(births)]) != set(ids):
        fail("naissances K1 != domaine")
    missing = ref_events - tower_events
    extra = tower_events - ref_events
    result = {"status": "passed" if not missing and not extra else "divergent", "n": n,
              "pairs": n * (n - 1) // 2, "distinct_levels": len(pairs), "equal_level_lots": equal_level_lots,
              "reference_events": sum(ref_events.values()), "tower_events": sum(tower_events.values()),
              "multifusions_arity_ge3": sum(v for (lv, cv), v in ref_events.items() if len(cv) >= 3),
              "missing_in_tower": len(missing), "extra_in_tower": len(extra),
              "balls": d["balls"], "extra_records": d["extra_records"]}
    if missing or extra:
        result["example_missing"] = [[str(lv), sorted(sorted(c) for c in cv)] for (lv, cv) in list(missing)[:2]]
        result["example_extra"] = [[str(lv), sorted(sorted(c) for c in cv)] for (lv, cv) in list(extra)[:2]]
    print(json.dumps(result, sort_keys=True))
    return 0 if result["status"] == "passed" else 1

if __name__ == "__main__":
    sys.exit(main(sys.argv[1]))
