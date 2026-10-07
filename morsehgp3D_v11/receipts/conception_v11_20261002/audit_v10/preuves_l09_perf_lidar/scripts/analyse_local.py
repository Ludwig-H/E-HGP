#!/usr/bin/env python3
"""Analyse des mesures locales L09 (compteurs deterministes, repartition relative, memoire)."""
import json, os, re, sys, glob
O = "/tmp/v11-audit/l09_perf_lidar/runs"
def load(name):
    p = os.path.join(O, name + ".json")
    if not os.path.exists(p): return None, None
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
    return (js[0] if js else None), t
def cpu(t):
    try: return float(t["User time (seconds)"]), float(t["System time (seconds)"]), int(t["Maximum resident set size (kbytes)"]), int(t["Minor (reclaiming a frame) page faults"])
    except Exception: return None, None, None, None
out = []
P = out.append
P("== A. Catalogue : compteurs deterministes (identiques a 1 et 4 fils ?) et etages")
for f in ("00", "01", "02"):
    for k in (5, 10):
        j1, t1 = load(f"cat_l{f}_k{k}_w1"); j4, t4 = load(f"cat_l{f}_k{k}_w4")
        if not j1 or not j4: P(f"l{f} K{k}: manquant"); continue
        keys = ["balls","levels","nodes","leaves","sum_m","max_m","skipped_bbox","filter_tests","leaf_dominance_tests","pair_tests","triple_tests","line_hits","quad_tests","judged","extended","max_shell","stalled_leaves"]
        same = all(j1[x] == j4[x] for x in keys) and j1["by_q_p"] == j4["by_q_p"]
        n = j1["sites"]; b = j1["balls"]
        P(f"l{f} K={k} sites={n} boules={b} ({b/n:.1f}/site) niveaux={j1['levels']} ({j1['levels']/b:.3f}/boule) compteurs_1fil==4fils: {same}")
        P(f"   noeuds={j1['nodes']} ({j1['nodes']/b:.3f}/boule, {j1['nodes']/n:.1f}/site) feuilles={j1['leaves']} ({j1['leaves']/b:.3f}/boule) m_moyen={j1['sum_m']/j1['leaves']:.2f} m_max={j1['max_m']} ignores={j1['skipped_bbox']}")
        P(f"   tests_filtre={j1['filter_tests']} ({j1['filter_tests']/j1['nodes']:.0f}/noeud, {j1['filter_tests']/b:.0f}/boule) dom_feuille={j1['leaf_dominance_tests']} paires={j1['pair_tests']} triplets={j1['triple_tests']} droites={j1['line_hits']} quadruplets={j1['quad_tests']} juges={j1['judged']} ({j1['judged']/b:.2f}/boule) etendues={j1['extended']}")
        pop = 0
        for q in (2, 3, 4):
            for p_, c in enumerate(j1["by_q_p"][f"q{q}"]): pop += c * (p_ + q)
        P(f"   population I+U (hors coquilles etendues) = {pop} ({pop/b:.2f}/boule) ; par q: " + ", ".join(f"q{q}={sum(j1['by_q_p'][f'q{q}'])}" for q in (2,3,4)))
        for w, j, t in ((1, j1, t1), (4, j4, t4)):
            cs = j["catalogue_stages"]; u, s, rss, pf = cpu(t)
            tot = j["catalogue_s"]; st = cs["t_frontier"]+cs["t_boxes"]+cs["t_order"]+cs["t_assemble"]
            P(f"   w={w}: cat={tot:.3f}s frontiere={cs['t_frontier']:.3f} ({100*cs['t_frontier']/tot:.1f}%) boites={cs['t_boxes']:.3f} ({100*cs['t_boxes']/tot:.1f}%) ordre={cs['t_order']:.3f} ({100*cs['t_order']/tot:.1f}%) assemblage={cs['t_assemble']:.3f} ({100*cs['t_assemble']/tot:.1f}%) hors_etages={tot-st:.3f} ({100*(tot-st)/tot:.1f}%) | CPU user={u} sys={s} rss_kB={rss} ({rss*1024/b:.1f} o/boule) defauts_page={pf} taches={cs['tasks']} max_sites_tache={cs['max_task_sites']}")
P("")
P("== B. Tour FULL sans attaches, 2 passes (derniere passe), 1/2/4 fils")
for f in ("00", "01", "02"):
    for k in (5, 10):
        for w in (1, 2, 4):
            j, t = load(f"tow_l{f}_k{k}_w{w}_r2")
            if not j: P(f"l{f} K{k} w{w}: manquant"); continue
            st = j["stages"]; cs = j["catalogue_stages"]; u, s, rss, pf = cpu(t)
            tw = j["tower_s"]
            keys = ['t_prepare','t_local','t_seeds','t_resolve','t_kruskal','t_points','t_vertical']
            P(f"l{f} K={k} w={w}: prepare={1000*j['prepare_s']:.1f}ms cat={j['catalogue_s']:.3f}s tour={tw:.3f}s (tour/cat={tw/j['catalogue_s']:.2f}) | " + " ".join(f"{x[2:]}={1000*st[x]:.1f}ms({100*st[x]/tw:.0f}%)" for x in keys if x != 't_points') + f" | kruskal: etage={1000*st['t_kruskal']:.1f} somme_ordres={1000*st['sum_order_kruskal']:.1f} max_ordre={1000*max(o['t_kruskal'] for o in j['orders']):.1f} | vertical: etage={1000*st['t_vertical']:.1f} somme_ordres={1000*st['sum_order_vertical']:.1f} | CPU user={u} sys={s}")
            if w == 1:
                jn = st["join"]; b = j["balls"]
                nodes = sum(o["nodes"] for o in j["orders"]); births = sum(o["births"] for o in j["orders"]); merges = sum(o["merges"] for o in j["orders"]); joins = sum(o["joins"] for o in j["orders"])
                P(f"   compteurs (1 fil, deterministes): cellules={st['local_cells']} ({st['local_cells']/b:.2f}/boule) noeuds_tous_ordres={nodes} ({nodes/b:.2f}/boule) naissances={births} jonctions={joins} fusions={merges} ({100*merges/joins:.0f}% des jonctions) ; resolutions={jn['resolves']} ({jn['resolves']/b:.2f}/boule) pas={jn['steps']} semis={jn['seed_hits']} ({100*jn['seed_hits']/jn['resolves']:.1f}%) memo={jn['memo_hits']} naissance={jn['birth_hits']} MEB={jn['meb']} recherches={jn['lookups']} recens_cat={jn['census_cat']} boules_fermees={jn['closed_balls']} sauts_knn={jn['knn_jumps']} locales={jn['local_calls']} pas_remontee={st['walk_steps']} replis_meb={st['meb_fallbacks']}")
                P("   noeuds par ordre: " + " ".join(f"k{o['k']}:{o['nodes']}" for o in j["orders"]))
P("")
P("== C. Memoire, une passe, 4 fils (pic RSS du processus ; rss_kib interne)")
for f in ("00", "01", "02"):
    for k in (5, 10):
        for name in ("tow1", "towcover", "towcore"):
            j, t = load(f"{name}_l{f}_k{k}_w4")
            if not j: P(f"{name} l{f} K{k}: manquant"); continue
            u, s, rss, pf = cpu(t); b = j["balls"]; r = j["rss_kib"]; st = j["stages"]
            P(f"{name:8s} l{f} K={k}: boules={b} pic={rss} kB ({rss*1024/b:.1f} o/boule) apres_catalogue={r['after_catalogue']} ({r['after_catalogue']*1024/b:.1f} o/boule) pic_apres_cat={r['peak_after_catalogue']} apres_tour={r['after_tower']} ({r['after_tower']*1024/b:.1f} o/boule) | cat={j['catalogue_s']:.3f} tour={j['tower_s']:.3f} points={1000*st['t_points']:.1f}ms ({100*st['t_points']/j['tower_s']:.0f}% de la tour) defauts_page={pf}")
            if name != "tow1":
                pt = st["point"]
                P(f"      attaches: resolutions={pt['resolves']} pas={pt['steps']} MEB={pt['meb']} knn_queries={pt['knn_queries']} boules_fermees={pt['closed_balls']} pas_remontee={st['walk_steps']}")
open(os.path.join(O, "ANALYSE.txt"), "w").write("\n".join(out) + "\n")
print("\n".join(out))
