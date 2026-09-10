#!/usr/bin/env python3
"""Analyse des nuages ou le mutant drop_extra_ball reste silencieux : la boule
retiree est-elle inerte a tous ses rangs planifies (une seule composante
stricte locale couvrant S) ? Aucun assert ; refus explicites."""
import json
import sys
from itertools import combinations

import tower_corpus as tc


def local_strict(model, ball, k):
    """Composantes strictes locales de la boule a l'ordre k (facettes de S sans support)."""
    S = [i for i in range(model["n"]) if ball["closed"] >> i & 1]
    supports = ball["supports"]
    contains = lambda m: any(T & ~m == 0 for T in supports)
    verts = [sum(1 << i for i in f) for f in combinations(S, k)]
    verts = [m for m in verts if not contains(m)]
    parent = {v: v for v in verts}

    def find(v):
        while parent[v] != v:
            v = parent[v]
        return v
    for f in combinations(S, k + 1):
        m = sum(1 << i for i in f)
        if contains(m):
            continue
        faces = [m ^ (1 << i) for i in f]
        for g in faces[1:]:
            a, b = find(faces[0]), find(g)
            if a != b:
                parent[b] = a
    comps = {}
    for v in verts:
        comps.setdefault(find(v), 0)
        comps[find(v)] |= v
    return list(comps.values())


def main():
    d = json.load(open(sys.argv[1]))
    out = []
    for c in d["clouds"]:
        if not c["catalogue"]["mutated"] or "comparison" not in c or c["comparison"]["divergences"]:
            continue
        points = [tuple(p) for p in c["refusal"]["points"]] if "refusal" in c else None
        # points/ids du nuage : reconstitues depuis le resume (desc absent) -> relancer la generation
        out.append(c)
    # Regeneration deterministe des nuages (meme graine) pour retrouver points/ids
    import random
    rng = random.Random(d["seed"])
    clouds = []
    if d["fixtures"]:
        for name, points, kmax in tc.FIXTURES:
            n = len(points)
            ids = [4294967295, 17, 0, 902, 2147483648, 3, 65536, 42][:n]
            clouds.append(dict(name=name, n=n, points=points, ids=ids, kmax=kmax, cube=0, mode="fixture"))
    for _ in range(len(d["clouds"]) - len(clouds)):
        clouds.append(tc.make_cloud(rng) if d["family"] == "random" else tc.make_cocircular(rng))
    report = []
    silent = 0
    all_inert = 0
    all_explained = 0
    for cloud, summary in zip(clouds, d["clouds"]):
        if not summary["catalogue"]["mutated"] or "comparison" not in summary or summary["comparison"]["divergences"]:
            continue
        silent += 1
        model = tc.build_model(cloud["points"])
        rows, _ = tc.catalogue(model, cloud["kmax"], cloud["ids"])
        extra = [r for r in rows if r["u"] > r["arity"]][0]
        ball = model["balls"][extra["key"]]
        p, u, q = extra["p"], extra["u"], extra["arity"]
        lo, hi = p + q - 1, min(cloud["kmax"], p + u)
        ranks = []
        inert = True
        for k in range(lo, hi + 1):
            comps = local_strict(model, ball, k)
            covers_S = len(comps) == 1 and comps[0] | ball["interior"] == ball["closed"]
            ranks.append(dict(k=k, strict_components=len(comps), single_covering_S=covers_S))
            inert = inert and covers_S
        all_inert += inert
        # Verdict global : au niveau de la boule, pour chaque rang planifie, la
        # reference (independante du catalogue) change-t-elle entre la coupe
        # ouverte et la coupe fermee ? Si non, aucun evenement global a ce
        # niveau : la boule retiree etait globalement inerte (seule son ancre
        # manque, et elle n'a pas ete demandee par le resolver).
        radii = sorted({b["radius"] for b in model["balls"].values()})
        cuts = sorted({tc.Q(0)} | set(radii) | {radii[-1] + 1})
        ref = tc.reference(model, cloud["kmax"], cuts)
        index = cuts.index(extra["radius"])
        globally_silent = all(ref["snapshots"][(k, index, 0)] == ref["snapshots"][(k, index, 1)]
                              for k in range(lo, hi + 1))
        for r in ranks:
            r["open_equals_closed_globally"] = ref["snapshots"][(r["k"], index, 0)] == ref["snapshots"][(r["k"], index, 1)]
        # Verdict fin : a chaque rang planifie non inerte localement, les
        # composantes strictes locales ont-elles UN SEUL parent global a la coupe
        # ouverte (pont exterieur, README section 3) et couvrent-elles S (aucune
        # contribution) ? Alors le bloc n'a aucun effet public a ce rang.
        fine = True
        for r in ranks:
            if r["single_covering_S"]:
                continue
            k = r["k"]
            comps = local_strict(model, ball, k)
            level = model["level"]
            verts = [m for m in range(1, model["full"] + 1) if tc.popcount(m) == k and level[m] < extra["radius"]]
            parent = {v: v for v in verts}

            def find(v):
                while parent[v] != v:
                    v = parent[v]
                return v
            for m in range(1, model["full"] + 1):
                if tc.popcount(m) == k + 1 and level[m] < extra["radius"]:
                    faces = [m ^ (1 << i) for i in range(model["n"]) if m >> i & 1]
                    for g in faces[1:]:
                        a, b = find(faces[0]), find(g)
                        if a != b:
                            parent[b] = a
            S = [i for i in range(model["n"]) if ball["closed"] >> i & 1]
            supports = ball["supports"]
            roots = set()
            for f in combinations(S, k):
                m = sum(1 << i for i in f)
                if not any(T & ~m == 0 for T in supports):
                    roots.add(find(m))
            union_cover = 0
            for c in comps:
                union_cover |= c
            r["global_parents_open"] = len(roots)
            r["local_strict_cover_is_S"] = (union_cover | ball["interior"]) == ball["closed"]
            fine = fine and len(roots) == 1 and r["local_strict_cover_is_S"]
        explained = inert or globally_silent or fine
        all_explained += explained
        report.append(dict(n=cloud["n"], kmax=cloud["kmax"], points=cloud["points"], ids=cloud["ids"],
                           dropped_key=list(extra["key"]), radius=str(extra["radius"]), p=p, u=u, q_min=q,
                           scheduled_ranks=[lo, hi], ranks=ranks, inert_at_all_scheduled_ranks=inert,
                           globally_silent_at_level=globally_silent, explained=explained))
    print(json.dumps(dict(silent_clouds=silent, inert_at_all_scheduled_ranks=all_inert,
                          explained=all_explained, cases=report), sort_keys=True, separators=(",", ":")))
    return 0 if silent == all_explained else 1


if __name__ == "__main__":
    try:
        sys.exit(main())
    except tc.Refusal as r:
        print("REFUSAL", r.reason, file=sys.stderr)
        sys.exit(r.code)
