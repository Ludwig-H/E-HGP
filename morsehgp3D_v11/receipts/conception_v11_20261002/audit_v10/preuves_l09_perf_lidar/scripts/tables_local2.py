#!/usr/bin/env python3
"""Blocs Markdown des sondes locales L09 (lots 2 et 3) : TSC, moments, descentes, scenes, composition, ISA."""
import json, os, re, statistics
R2 = "/tmp/v11-audit/l09_perf_lidar/runs2"; R3 = "/tmp/v11-audit/l09_perf_lidar/runs3"
def load(d, name):
    p = os.path.join(d, name + ".json")
    js = []
    if os.path.exists(p):
        for line in open(p):
            line = line.strip()
            if line.startswith("{"):
                try: js.append(json.loads(line))
                except Exception: pass
    t = {}
    tp = os.path.join(d, name + ".time")
    if os.path.exists(tp):
        for line in open(tp):
            m = re.match(r"\s*(.+?): (.+)$", line)
            if m: t[m.group(1).strip()] = m.group(2).strip()
    return js, t
def fr(x, d=1): return f"{x:.{d}f}".replace(".", ",")
def sp(n): return f"{int(n):,}".replace(",", " ")
def cpu(t): return float(t["User time (seconds)"]) + float(t["System time (seconds)"])
print("### S1 TSC catalogue")
print("| Trame | K | Filtre des nœuds | Feuilles | Reste | Masques de dominance | Paires | Triplets | Quadruplets | dont jugements (recensement et émission) | Frontière (1 fil) / tâches |")
print("| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |")
for f in ("00", "01", "02"):
    for k in (5, 10):
        js, t = load(R2, f"tsc_l{f}_k{k}_w1")
        if len(js) < 2: continue
        x = js[1]["l09"]; tot = x[27]; fil = x[20]; leaf = x[21]
        print(f"| {f} | {k} | {fr(100*fil/tot)} % | {fr(100*leaf/tot)} % | {fr(100*(tot-fil-leaf)/tot)} % | {fr(100*x[22]/leaf)} % | {fr(100*x[23]/leaf)} % | {fr(100*x[24]/leaf)} % | {fr(100*x[25]/leaf)} % | {fr(100*x[26]/leaf)} % | {fr(100*x[7]/tot,2)} % |")
print()
print("### S2 moments")
print("| Trame | K | Ancres (Σm) | Hors de la boîte fermée | Rejets par dominance individuelle (θ2 / θ3 / θ4) | Rejets nouveaux, groupe entier | Rejets nouveaux, meilleur de 7 groupes | Rejets nouveaux, meilleur préfixe | Crédit certifié : préfixes / dominance | Nœuds : sites gardés | Rejets, réservoir Y | Rejets, préfixes de Y | Rejets, liste parente |")
print("| --- | ---: | ---: | ---: | --- | --- | --- | --- | ---: | ---: | ---: | ---: | ---: |")
for f in ("00", "01", "02"):
    for k in (5, 10):
        js, t = load(R2, f"mom_l{f}_k{k}_w3")
        if len(js) < 2: continue
        x = js[1]["l09"]
        print(f"| {f} | {k} | {sp(x[30])} | {fr(100*x[31]/x[30])} % | {sp(x[35])} / {sp(x[36])} / {sp(x[37])} | {x[38]} / {x[39]} / {x[40]} | {x[41]} / {x[42]} / {x[43]} | {x[53]} / {x[54]} / {x[55]} | {fr(x[56]/x[57],2)} | {sp(x[44])} | {x[46]} | {x[49]} | {sp(x[47])} ({fr(100*x[47]/x[44],2)} %) |")
print()
print("### S3 descentes")
print("| Trame | K | Descentes | Recherche du semis | MEB | Recensement au catalogue (dont juge 1/32) | Boule fermée par l'arbre | Saut K-NN | Support et mémo | Premier représentant | Non attribué | Part des descentes dans la tour (1 fil) |")
print("| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |")
for f in ("00", "01", "02"):
    for k in (5, 10):
        js, t = load(R2, f"ttsc_l{f}_k{k}_w1")
        if len(js) < 2: continue
        j, x = js[0], js[1]["l09t"]; tot = x[0]; st = j["stages"]
        acc = sum(x[1:8])
        print(f"| {f} | {k} | {sp(st['join']['resolves'])} | " + " | ".join(f"{fr(100*x[i]/tot)} %" for i in range(1, 8)) + f" | {fr(100*(tot-acc)/tot)} % | {fr(100*st['t_resolve']/j['tower_s'])} % |")
print()
print("### S4 scenes")
print("| Scène | Sol | Sites | K | Boules | Boules par site | Nœuds | Feuilles | m moyen | Jugements par boule | Pic RSS du catalogue (o/boule) |")
print("| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |")
for s, sol in (("03_pieton_contre_facade", "retiré"), ("01_velos_en_rang", "retiré"), ("05_temoin_voitures_en_file", "retiré"), ("02_velos_contre_facade", "retiré"), ("04_avec_sol", "gardé")):
    for k in (5, 10):
        for suf in ("cat_w4", "cat_w3"):
            js, t = load(R2, f"demo_{s}_k{k}_{suf}")
            if not js or js[0].get("status") != "ok": continue
            j = js[0]; b = j["balls"]; n = j["sites"]; rss = int(t["Maximum resident set size (kbytes)"])
            print(f"| {s[:2]} | {sol} | {sp(n)} | {k} | {sp(b)} | {fr(b/n)} | {sp(j['nodes'])} | {sp(j['leaves'])} | {fr(j['sum_m']/j['leaves'],2)} | {fr(j['judged']/b,2)} | {fr(rss*1024/b)} |")
print()
print("### S5 composition")
print("| Trame | K | Fils | CPU base (s), médiane [min ; max] | CPU composition (s), médiane [min ; max] | Rapport | Boules, nœuds, naissances, fusions et jonctions par ordre |")
print("| --- | ---: | ---: | --- | --- | ---: | --- |")
for f, k, reps, w in (("01", 5, (1, 2), 2), ("02", 5, (1, 2), 2), ("01", 10, (1,), 3)):
    A = []; B = []
    for r in reps:
        for tag, lst in (("A", A), ("A2", A), ("B", B), ("B2", B)):
            js, t = load(R3, f"{tag}_l{f}_k{k}_r{r}")
            if js and t: lst.append((cpu(t), js[0]))
    if not A or not B: continue
    ca = [x[0] for x in A]; cb = [x[0] for x in B]; ja = A[0][1]; jb = B[0][1]
    same = ja["balls"] == jb["balls"] and [(o["nodes"], o["births"], o["merges"], o["joins"]) for o in ja["orders"]] == [(o["nodes"], o["births"], o["merges"], o["joins"]) for o in jb["orders"]]
    print(f"| {f} | {k} | {w} | {fr(statistics.median(ca),2)} [{fr(min(ca),2)} ; {fr(max(ca),2)}] ({len(ca)} passes) | {fr(statistics.median(cb),2)} [{fr(min(cb),2)} ; {fr(max(cb),2)}] ({len(cb)} passes) | ×{fr(statistics.median(ca)/statistics.median(cb),2)} | {'identiques' if same else 'DIFFERENTS'} |")
print()
print("### S6 ISA")
print("| Trame | K | CPU défaut (s) | CPU `x86-64-v3` (s) | Rapport | Compteurs et boules |")
print("| --- | ---: | ---: | ---: | ---: | --- |")
for f, k, tags in (("01", 5, ("isaA", "isaA2", "isaB", "isaB2")), ("01", 10, ("isaA", "isaB"))):
    A = []; B = []
    for tag in tags:
        js, t = load(R3, f"{tag}_l{f}_k{k}")
        if js and t: (A if "A" in tag else B).append((cpu(t), js[0]))
    if not A or not B: continue
    ja = A[0][1]; jb = B[0][1]
    keys = ["balls","levels","nodes","leaves","sum_m","filter_tests","pair_tests","triple_tests","quad_tests","judged"]
    same = all(ja[x] == jb[x] for x in keys) and ja["by_q_p"] == jb["by_q_p"]
    ca = statistics.median(x[0] for x in A); cb = statistics.median(x[0] for x in B)
    print(f"| {f} | {k} | {fr(ca,2)} | {fr(cb,2)} | ×{fr(ca/cb,2)} | {'identiques' if same else 'DIFFERENTS'} |")
