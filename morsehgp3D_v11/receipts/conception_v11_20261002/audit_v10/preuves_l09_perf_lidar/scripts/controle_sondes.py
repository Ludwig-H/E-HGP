#!/usr/bin/env python3
"""Controle : les sondes instrumentees ne changent ni le grand livre du catalogue ni les comptes de la tour."""
import json, os
B = "/tmp/v11-audit/l09_perf_lidar"
keys = ["balls","levels","nodes","leaves","sum_m","max_m","skipped_bbox","filter_tests","leaf_dominance_tests","pair_tests","triple_tests","line_hits","quad_tests","judged","extended","max_shell","stalled_leaves","by_q_p"]
def first(p):
    if not os.path.exists(p): return None
    for line in open(p):
        if line.strip().startswith("{"): return json.loads(line)
    return None
bad = 0
for name in sorted(os.listdir(B + "/runs2")):
    if not name.endswith(".json"): continue
    if name.startswith("mom_") or name.startswith("tsc_"):
        f = name.split("_")[1][1:]; k = name.split("_")[2][1:]
        ref = first(f"{B}/runs/cat_l{f}_k{k}_w1.json"); j = first(f"{B}/runs2/{name}")
        if not ref or not j: print(name, "incomplet"); continue
        ok = all(ref[x] == j[x] for x in keys); bad += not ok
        print(name, "grand livre identique a la reference :", ok)
    if name.startswith("ttsc_"):
        f = name.split("_")[1][1:]; k = name.split("_")[2][1:]
        ref = first(f"{B}/runs/tow_l{f}_k{k}_w1_r2.json"); j = first(f"{B}/runs2/{name}")
        if not ref or not j: print(name, "incomplet"); continue
        ok = ref["balls"] == j["balls"] and [(o["nodes"], o["births"], o["merges"], o["joins"]) for o in ref["orders"]] == [(o["nodes"], o["births"], o["merges"], o["joins"]) for o in j["orders"]] and ref["stages"]["join"]["resolves"] == j["stages"]["join"]["resolves"] and ref["stages"]["join"]["meb"] == j["stages"]["join"]["meb"]
        bad += not ok
        print(name, "tour identique a la reference (noeuds, naissances, fusions, jonctions, descentes, MEB) :", ok)
print("ecarts :", bad)
