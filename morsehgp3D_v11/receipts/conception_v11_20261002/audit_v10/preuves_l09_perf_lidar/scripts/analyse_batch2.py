#!/usr/bin/env python3
"""Analyse du lot 2 : repartition TSC (catalogue, tour), compteurs de moments, scenes des demos."""
import json, os, re
O = "/tmp/v11-audit/l09_perf_lidar/runs2"
def load(name):
    p = os.path.join(O, name + ".json")
    if not os.path.exists(p): return [], {}
    js = []
    for line in open(p):
        line = line.strip()
        if line.startswith("{"):
            try: js.append(json.loads(line))
            except Exception: pass
    t = {}
    tp = os.path.join(O, name + ".time")
    if os.path.exists(tp):
        for line in open(tp):
            m = re.match(r"\s*(.+?): (.+)$", line)
            if m: t[m.group(1).strip()] = m.group(2).strip()
    return js, t
out = []; P = out.append
P("== 1. Repartition TSC du catalogue a 1 fil (tics ; parts de l'etage des taches = t_boxes)")
for f in ("00", "01", "02"):
    for k in (5, 10):
        js, t = load(f"tsc_l{f}_k{k}_w1")
        if len(js) < 2: P(f"l{f} K{k}: manquant"); continue
        j, x = js[0], js[1]["l09"]
        tot = x[27]; fil = x[20]; leaf = x[21]
        P(f"l{f} K={k}: taches={tot/1e9:.2f} Gtic filtre={100*fil/tot:.1f}% feuilles={100*leaf/tot:.1f}% reste(ajustement, allocations)={100*(tot-fil-leaf)/tot:.1f}% | dans la feuille: masques={100*x[22]/leaf:.1f}% paires={100*x[23]/leaf:.1f}% triplets={100*x[24]/leaf:.1f}% quadruplets={100*x[25]/leaf:.1f}% ; dont jugements (recensement+emission)={100*x[26]/leaf:.1f}% de la feuille, {100*x[26]/tot:.1f}% de l'etage | frontiere: process={x[7]/1e9:.2f} Gtic dont filtre={100*x[0]/max(1,x[7]):.1f}% feuilles={100*x[1]/max(1,x[7]):.1f}% ; frontiere/taches={100*x[7]/tot:.2f}% | t_boxes={j['catalogue_stages']['t_boxes']:.2f}s cpu_user={t.get('User time (seconds)')}")
        b = j["balls"]; cyc = tot / b
        P(f"      tics par boule (etage des taches)={cyc:.0f} ; par test de filtre={fil/j['filter_tests']:.2f} ; par feuille={leaf/j['leaves']:.0f} ; par jugement={x[26]/j['judged']:.0f}")
P("")
P("== 2. Certificat de moments de groupe (compteurs ; aucune decision modifiee)")
for f in ("00", "01", "02"):
    for k in (5, 10):
        js, t = load(f"mom_l{f}_k{k}_w3")
        if len(js) < 2: P(f"l{f} K{k}: manquant"); continue
        j, x = js[0], js[1]["l09"]
        P(f"l{f} K={k}: boules={j['balls']} feuilles={j['leaves']} ancres={x[30]} hors_boite_fermee={x[31]} ({100*x[31]/max(1,x[30]):.1f}%)")
        P(f"      rejets Dom individuelle (seuils th2,th3,th4)={x[35]},{x[36]},{x[37]} | moments groupe entier={x[32]},{x[33]},{x[34]} dont nouveaux={x[38]},{x[39]},{x[40]} | meilleur de 7 groupes, nouveaux={x[41]},{x[42]},{x[43]} | prefixes par distance={x[50]},{x[51]},{x[52]} dont nouveaux={x[53]},{x[54]},{x[55]}")
        P(f"      credit certifie total : prefixes={x[56]} contre Dom={x[57]} (rapport {x[56]/max(1,x[57]):.3f}) ; ancres ou le prefixe depasse Dom={x[58]} ({100*x[58]/max(1,x[31]):.2f}% des ancres eligibles)")
        P(f"      noeuds : sites lus={x[48]} gardes={x[44]} dont hors boite fermee={x[45]} ; rejets moments Y={x[46]} prefixes de Y={x[49]} liste parente={x[47]} ({100*x[47]/max(1,x[44]):.3f}% des gardes)")
P("")
P("== 3. Repartition TSC de resolve (tour, 1 fil, sans attaches)")
names = {1: "semis", 2: "MEB", 3: "recensement catalogue (+juge 1/32)", 4: "boule fermee (arbre)", 5: "saut K-NN", 6: "support/memo", 7: "premier representant"}
for f in ("00", "01", "02"):
    for k in (5, 10):
        js, t = load(f"ttsc_l{f}_k{k}_w1")
        if len(js) < 2: P(f"l{f} K{k}: manquant"); continue
        j, x = js[0], js[1]["l09t"]
        st = j["stages"]; tot = x[0]
        parts = " ".join(f"{names[i]}={100*x[i]/tot:.1f}%" for i in range(1, 8))
        acc = sum(x[1:8])
        P(f"l{f} K={k}: resolve={tot/1e9:.2f} Gtic ; {parts} ; non attribue={100*(tot-acc)/tot:.1f}% | etage G (corps)={x[10]/1e9:.2f} Gtic | tour={j['tower_s']:.2f}s: " + " ".join(f"{s[2:]}={100*st[s]/j['tower_s']:.1f}%" for s in ('t_prepare','t_local','t_seeds','t_resolve','t_kruskal','t_vertical')))
        jn = st["join"]
        P(f"      tics par resolution={tot/jn['resolves']:.0f} ; par MEB={x[2]/max(1,jn['meb']):.0f} ; par boule fermee={x[4]/max(1,jn['closed_balls']-jn['census_cat']//32):.0f} (approx.) ; resolutions={jn['resolves']} MEB={jn['meb']} boules_fermees={jn['closed_balls']} recens_cat={jn['census_cat']}")
P("")
P("== 4. Scenes des demos (compteurs ; 3 fils, les noms de fichiers gardent le suffixe w4 du plan initial)")
for s in ("03_pieton_contre_facade", "01_velos_en_rang", "05_temoin_voitures_en_file", "02_velos_contre_facade", "04_avec_sol"):
    for k in (5, 10):
        for suf in ("cat_w4", "cat_w3"):
            js, t = load(f"demo_{s}_k{k}_{suf}")
            if not js: continue
            j = js[0]
            if j.get("status") != "ok": P(f"{s} K={k}: {j}"); continue
            b = j["balls"]; n = j["sites"]
            rss = int(t.get("Maximum resident set size (kbytes)", 0))
            P(f"{s} K={k}: sites={n} boules={b} ({b/n:.1f}/site) niveaux={j['levels']} noeuds={j['nodes']} feuilles={j['leaves']} m_moyen={j['sum_m']/j['leaves']:.2f} m_max={j['max_m']} tests_filtre={j['filter_tests']} juges={j['judged']} etendues={j['extended']} bloquees={j['stalled_leaves']} | cpu_user={t.get('User time (seconds)')} pic_rss={rss} kB ({rss*1024/b:.1f} o/boule)")
        js, t = load(f"demo_{s}_k{k}_tow_w4")
        if js:
            j = js[0]
            if j.get("status") == "ok":
                st = j["stages"]; b = j["balls"]
                nodes = sum(o["nodes"] for o in j["orders"])
                rss = int(t.get("Maximum resident set size (kbytes)", 0))
                P(f"      tour: noeuds_tous_ordres={nodes} ({nodes/b:.2f}/boule) resolutions={st['join']['resolves']} cellules={st['local_cells']} | pic_rss={rss} kB ({rss*1024/b:.1f} o/boule) cpu_user={t.get('User time (seconds)')}")
            else:
                P(f"      tour: {j}")
open(os.path.join(O, "ANALYSE2.txt"), "w").write("\n".join(out) + "\n")
print("\n".join(out))
