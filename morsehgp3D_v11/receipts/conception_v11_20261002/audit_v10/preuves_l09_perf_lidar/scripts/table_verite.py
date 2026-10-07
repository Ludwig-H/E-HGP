#!/usr/bin/env python3
"""Table de verite des temps G4 v10 (CPU seul, 48 fils) a partir des sorties brutes des recus (lecture seule).
Usage : python3 table_verite.py <racine des recus morsehgp3D_v10/receipts> > TABLE_VERITE_G4.md
Aucune valeur n'est recopiee a la main : tout vient des fichiers results/cmd/*/stdout des sessions 1 a 4."""
import json, os, sys
ROOT = sys.argv[1] if len(sys.argv) > 1 else "/workspaces/E-HGP/build/v11-worktree/morsehgp3D_v10/receipts"
SESS = {"s1": "g4_session1_20260929", "s2": "g4_session2_perf_20260929", "s3": "g4_session3_j2_20260929", "s4": "g4_session4_j2c_20260929"}
COMMIT = {"s1": "8b8d66f6e", "s2": "f8e78ad94", "s3": "82fc2a6b5", "s4": "777406b82"}
def load(sess, cmd):
    p = os.path.join(ROOT, SESS[sess], "results", "cmd", cmd, "stdout")
    for line in open(p):
        line = line.strip()
        if line.startswith("{"):
            return json.loads(line)
def meta(sess, cmd):
    m = {}
    for line in open(os.path.join(ROOT, SESS[sess], "results", "cmd", cmd, "meta.txt")):
        if "=" in line:
            k, v = line.strip().split("=", 1); m[k] = v
    return m
def ms(x, d=1): return f"{1000*x:.{d}f}".replace(".", ",")
def fr(x, d=1): return f"{x:.{d}f}".replace(".", ",")
out = []
P = out.append
P("# Table de vérité des temps G4 de la v10 (CPU seul), extraite des sorties brutes des reçus")
P("")
P("Produite par `table_verite.py` à partir de `receipts/g4_session{1..4}_*/results/cmd/*/stdout` ; millisecondes ; VM `g4-standard-48` (AMD EPYC 9B45, 24 cœurs, 48 fils), g++ 11.4, `-O3 -DNDEBUG` sans `-march`. Aucun calcul sur GPU.")
P("")
P("## 1. Tour FULL 1..K sans attaches (`mhgp10_tower --no-points --repeat=3`, dernière passe), session 4 (`777406b82`), 48 fils")
P("")
P("| Trame | Sites | K | Boules | Préparation (froide, hors passes) | Catalogue | dont frontière | dont boîtes | dont ordre | dont assemblage | dont hors étages | Tour | dont index (`t_prepare`) | dont atlas (`t_local`) | dont semis | dont descentes (`t_resolve`) | dont Kruskal | dont verticales | Catalogue + tour | Trois passes catalogue + tour |")
P("| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |")
T = {}
for f, cmds in (("00", ("008_tower_lidar00_k5_w48", "009_tower_lidar00_k10_w48")), ("01", ("010_tower_lidar01_k5_w48", "011_tower_lidar01_k10_w48")), ("02", ("012_tower_lidar02_k5_w48", "013_tower_lidar02_k10_w48"))):
    for c in cmds:
        j = load("s4", c); cs = j["catalogue_stages"]; st = j["stages"]
        hs = j["catalogue_s"] - (cs["t_frontier"] + cs["t_boxes"] + cs["t_order"] + cs["t_assemble"])
        tot = j["catalogue_s"] + j["tower_s"]
        passes = " / ".join(ms(a + b) for a, b in zip(j["passes_catalogue_s"], j["passes_tower_s"]))
        T[(f, j["K"])] = j
        P(f"| {f} | {j['n']} | {j['K']} | {j['balls']} | {ms(j['prepare_s'])} | {ms(j['catalogue_s'])} | {ms(cs['t_frontier'])} | {ms(cs['t_boxes'])} | {ms(cs['t_order'])} | {ms(cs['t_assemble'])} | {ms(hs)} | {ms(j['tower_s'])} | {ms(st['t_prepare'])} | {ms(st['t_local'])} | {ms(st['t_seeds'])} | {ms(st['t_resolve'])} | {ms(st['t_kruskal'])} | {ms(st['t_vertical'])} | **{ms(tot)}** | {passes} |")
P("")
P("Lecture : « catalogue + tour » exclut la lecture du fichier et la préparation (tri de Morton, `SiteTree`, création du pool), mesurée une seule fois par processus. Ce sont les seules mesures G4 de la tour FULL 1..K au code courant.")
P("")
P("## 2. Étages peu parallèles : plancher hors « boîtes » et « descentes » (session 4, 48 fils)")
P("")
P("| Trame | K | Catalogue + tour | Boîtes + descentes | Reste (frontière, ordre, assemblage, hors étages, index, atlas, semis, Kruskal, verticales) | Part du reste | Kruskal de l'ordre K (tâche séquentielle) | Fusions verticales de l'ordre K (tâche séquentielle) |")
P("| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |")
for (f, k), j in sorted(T.items(), key=lambda kv: (kv[0][1], kv[0][0])):
    cs = j["catalogue_stages"]; st = j["stages"]; tot = j["catalogue_s"] + j["tower_s"]
    par = cs["t_boxes"] + st["t_resolve"]
    o = j["orders"][-1]
    P(f"| {f} | {k} | {ms(tot)} | {ms(par)} | {ms(tot - par)} | {fr(100*(tot-par)/tot,0)} % | {ms(o['t_kruskal'])} | {ms(o['t_vertical'])} |")
P("")
P("## 3. Attaches des points (entrée `cover`, `mhgp10_tower --entry=cover --repeat`, dernière passe, 48 fils)")
P("")
P("| Session (commit) | Trame | K | Catalogue | Tour avec attaches | dont attaches (`t_points`) | Catalogue + tour avec attaches |")
P("| --- | --- | ---: | ---: | ---: | ---: | ---: |")
for sess, cmds in (("s4", ("014_tower_lidar00_k5_cover_w48", "016_tower_lidar01_k5_cover_w48", "018_tower_lidar02_k5_cover_w48")),
                   ("s1", ("009_c1_lidar00_k5_cover_w48", "011_c1_lidar01_k5_cover_w48", "013_c1_lidar02_k5_cover_w48", "010_c1_lidar00_k10_cover_w48", "012_c1_lidar01_k10_cover_w48", "014_c1_lidar02_k10_cover_w48"))):
    for c in cmds:
        j = load(sess, c); st = j["stages"]
        f = c.split("lidar")[1][:2]
        P(f"| {sess} (`{COMMIT[sess]}`) | {f} | {j['K']} | {ms(j['catalogue_s'])} | {ms(j['tower_s'])} | {ms(st['t_points'])} | {ms(j['catalogue_s'] + j['tower_s'])} |")
P("")
P("L'entrée `core` (C∩X) n'a jamais été mesurée sur G4 sur trame entière. À K = 10, les attaches n'ont été mesurées qu'en session 1 (catalogue d'avant J1, J2 et J2c ; le code de la tour n'a pas changé depuis sur ce chemin).")
P("")
P("## 4. Chaîne `mhgp10_cluster` (un seul ordre K = 5, entrée `cover`, tête EOM ; processus froid, préparation comprise dans `catalogue_s`)")
P("")
P("| Session (commit) | Trame | `catalogue_s` (préparation + index + catalogue) | `tower_s` (ordre 5 seul, attaches comprises, sans verticales) | `head_s` | Somme | Mur du processus | Amas |")
P("| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |")
for sess, cmds in (("s2", ("014_cluster_lidar00_k5_cover_w48", "016_cluster_lidar01_k5_cover_w48", "018_cluster_lidar02_k5_cover_w48")),
                   ("s3", ("015_cluster_lidar00_k5_cover_w48", "017_cluster_lidar01_k5_cover_w48", "019_cluster_lidar02_k5_cover_w48")),
                   ("s4", ("015_cluster_lidar00_k5_cover_w48", "017_cluster_lidar01_k5_cover_w48", "019_cluster_lidar02_k5_cover_w48"))):
    for c in cmds:
        j = load(sess, c); m = meta(sess, c)
        f = c.split("lidar")[1][:2]
        P(f"| {sess} (`{COMMIT[sess]}`) | {f} | {ms(j['catalogue_s'],0)} | {ms(j['tower_s'],0)} | {ms(j['head_s'],0)} | {ms(j['catalogue_s']+j['tower_s']+j['head_s'],0)} | {ms(float(m['wall_seconds']),0)} | {j['clusters']} |")
P("")
P("Aucune mesure G4 de la tête à K = 10, ni d'une tête sur les K ordres d'une tour FULL.")
P("")
P("## 5. Catalogue seul selon le nombre de fils (trame 02, `mhgp10_catalogue`, une passe froide par processus)")
P("")
P("| Session (commit) | K | Fils | Catalogue | Frontière | Boîtes | Ordre | Assemblage | Hors étages | Accélération du catalogue | Accélération des boîtes | Tâches | Mur du processus | RSS max (Kio) | Octets par boule au pic |")
P("| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |")
for sess, cmds in (("s2", ("001_cat_lidar02_k5_w1", "002_cat_lidar02_k5_w12", "003_cat_lidar02_k5_w24", "004_cat_lidar02_k5_w48", "005_cat_lidar02_k10_w24", "006_cat_lidar02_k10_w48")),
                   ("s3", ("001_cat_lidar02_k5_w1", "003_cat_lidar02_k5_w12", "004_cat_lidar02_k5_w24", "005_cat_lidar02_k5_w48", "002_cat_lidar02_k10_w1", "006_cat_lidar02_k10_w24", "007_cat_lidar02_k10_w48")),
                   ("s4", ("001_cat_lidar02_k5_w1", "003_cat_lidar02_k5_w12", "004_cat_lidar02_k5_w24", "005_cat_lidar02_k5_w48", "002_cat_lidar02_k10_w1", "006_cat_lidar02_k10_w24", "007_cat_lidar02_k10_w48"))):
    base = {}
    for c in cmds:
        j = load(sess, c)
        if j["threads"] == 1: base[j["K"]] = j
    for c in cmds:
        j = load(sess, c); m = meta(sess, c); cs = j["catalogue_stages"]
        hs = j["catalogue_s"] - (cs["t_frontier"] + cs["t_boxes"] + cs["t_order"] + cs["t_assemble"])
        b = base.get(j["K"])
        acc = fr(b["catalogue_s"] / j["catalogue_s"], 1) if b else "—"
        accb = fr(b["catalogue_stages"]["t_boxes"] / cs["t_boxes"], 1) if b else "—"
        rss = int(m["max_rss_kb"])
        P(f"| {sess} (`{COMMIT[sess]}`) | {j['K']} | {j['threads']} | {ms(j['catalogue_s'])} | {ms(cs['t_frontier'])} | {ms(cs['t_boxes'])} | {ms(cs['t_order'])} | {ms(cs['t_assemble'])} | {ms(hs)} | {("×" + acc) if b else "—"} | {("×" + accb) if b else "—"} | {cs.get('tasks','—')} | {ms(float(m['wall_seconds']),0)} | {rss} | {fr(rss*1024/j['balls'],1)} |")
P("")
P("## 6. Tour selon le nombre de fils (trame 02, K = 5, sans attaches)")
P("")
P("| Session | Fils | Tour | index | atlas | semis | descentes | Kruskal (étage) | Kruskal (somme des ordres) | verticales (étage) | verticales (somme des ordres) |")
P("| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |")
for sess, c in (("s1", "008_c0_lidar02_k5_w1"), ("s1", "007_c0_lidar02_k5_w24"), ("s1", "003_c0_lidar02_k5_w48"), ("s4", "020_tower_lidar02_k5_w24"), ("s4", "012_tower_lidar02_k5_w48")):
    j = load(sess, c); st = j["stages"]
    P(f"| {sess} (`{COMMIT[sess]}`) | {j['threads']} | {ms(j['tower_s'])} | {ms(st['t_prepare'])} | {ms(st['t_local'])} | {ms(st['t_seeds'])} | {ms(st['t_resolve'])} | {ms(st['t_kruskal'])} | {ms(st['sum_order_kruskal'])} | {ms(st['t_vertical'])} | {ms(st['sum_order_vertical'])} |")
P("")
P("## 7. Compteurs déterministes du catalogue (trame 02, session 4)")
P("")
P("| K | Boules | Niveaux distincts | Nœuds | Feuilles | Candidats (Σm) | m moyen | m max | Tests du filtre | Dominance de feuille | Paires | Triplets | Droites touchant la boîte | Quadruplets | Jugements | Coquilles étendues |")
P("| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |")
for c in ("005_cat_lidar02_k5_w48", "007_cat_lidar02_k10_w48"):
    j = load("s4", c)
    P(f"| {j['K']} | {j['balls']} | {j['levels']} | {j['nodes']} | {j['leaves']} | {j['sum_m']} | {fr(j['sum_m']/j['leaves'],2)} | {j['max_m']} | {j['filter_tests']} | {j['leaf_dominance_tests']} | {j['pair_tests']} | {j['triple_tests']} | {j['line_hits']} | {j['quad_tests']} | {j['judged']} | {j['extended']} |")
P("")
print("\n".join(out))
