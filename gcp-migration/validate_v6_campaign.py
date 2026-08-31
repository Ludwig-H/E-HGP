#!/usr/bin/env python3
"""Validation LOCALE de la campagne v6 : seule cette validation decide de
campaign_status — jamais le lanceur, jamais la VM. `complete` exige :
  - les TROIS plans annonces (conf_plan.txt, bench_plan.txt, queue_plan.txt),
    chaque sequence RECALCULEE depuis les parametres et identique a l'annonce
    (bench : ordre ABBA contrebalance par parite de configuration) ;
  - un statut et une sortie pour CHAQUE run annonce : code 0, pin identique
    (commit, sha256 du payload, manifeste du protocole), pic RSS > 0, sortie
    complete de GNU time, commande gravee coherente avec le nom, aucun motif
    interdit, aucune troncature gravee ;
  - phase conformite : la reference v5 porte exactement un digest_all et la
    ligne tower_scope ; le juge imprime `identiques (objet)` ;
  - phase bench : AUCUN digest (ni drapeau ni ligne), moteur du run coherent
    avec la ligne payload= (mhgp5/mhgp6), et compteurs deterministes — les
    lignes de compteurs, `generation` et les dix `cardinalites` IDENTIQUES
    entre les deux runs d'un meme (famille, n, moteur) ;
  - phase queue : chaque compteur du grand-livre exige (sweep, vwspd,
    octaves_q4, vcensus, p_factor, ledger_paires), aucun digest ;
  - des codes de session (ssh distant, scp) nuls.
Il ecrit bench_resume.txt et queue_resume.txt (murs / RSS — completude et
dispersion, JAMAIS une conclusion d'acceleration ni de pente).

Usage : validate_v6_campaign.py OUT_DIR SOURCE_COMMIT SOURCE_PAYLOAD_SHA256 \\
        PROTOCOL_MANIFEST_SHA256 REMOTE_CAMPAIGN_RC SCP_RC
Sortie : 0 si complete, 1 sinon (les preuves partielles restent sur place).
"""
import os
import re
import sys

KMAX = 10
AUX = ("topologie.txt", "conf_plan.txt", "bench_plan.txt", "queue_plan.txt",
       "bench_resume.txt", "queue_resume.txt",
       "conf_tronquee.txt", "bench_tronquee.txt", "queue_tronquee.txt")
FORBIDDEN = re.compile(r"REFUS|INVARIANT|DIVERGENCE|PLANCHER|Killed|bad_alloc|AddressSanitizer")
COUNTERS = re.compile(r"boules_uniques=\d+.*evenements=\d+.*facettes=\d+")
IDENT = re.compile(r"^famille=(\S+) n=(\d+) coord=\d+ s=(\d+) smax=(\d+) seed=(\d+) threads=(\d+)", re.M)
TOWER = re.compile(r"^tower_scope=profile_complete_k10 smax_requested=11 smax_effective=11$", re.M)
ANY_DIGEST = re.compile(r"^digest_(balls|forest_K\d+|all|candidates_v5_compat|postprefilter)=", re.M)
# Grand-livre v6 exige sur chaque run de queue (sonde E6) : nom -> motif.
QUEUE_COUNTERS = [
    ("W_sweep1_evals_coeur", r"tests_coeur=(\d+)"),
    ("W_scan_q3", r"tests_prof_q3=(\d+)"),
    ("W_sweep2_evals_passe2", r"tests_passe2=(\d+)"),
    ("tri_comparaisons", r"tri_comparaisons=(\d+)"),
    ("vwspd_noeuds", r"vwspd nœuds_temoins=(\d+)"),
    ("vwspd_coins", r"coins=(\d+) h_rect"),
    ("m_anchor_q4", r"m_anchor=\d+/\d+/(\d+)"),
    ("iters_coeur", r"iters_coeur=(\d+)"),
    ("iters_passe2", r"iters_passe2=(\d+)"),
    ("octaves_ancres", r"octaves_q4 ancres=([0-9,]+)"),
    ("octaves_seeds", r"octaves_q4 ancres=[0-9,]+ seeds=([0-9,]+)"),
    ("octaves_w1", r"octaves_q4 ancres=[0-9,]+ seeds=[0-9,]+ w1=([0-9,]+)"),
    ("vcensus_noeuds", r"vcensus nœuds=(\d+)"),
    ("p_factor_q4", r"p_factor=\d+/\d+/(\d+)"),
    ("ledger_emis", r"ledger_paires emis=(\d+)"),
]


def read_text(path):
    with open(path, encoding="utf-8", errors="replace") as fh:
        return fh.read()


def read_plan(out, fname, version_key, param_keys, bad):
    """Lit un plan annonce : parametres complets + lignes seq. Retourne
    (params, listed) ou (None, []) si invalide. L'ABSENCE d'un plan est une
    faute (les plans sont annonces avant le premier run)."""
    path = os.path.join(out, fname)
    if not os.path.exists(path):
        bad.append(f"{fname}: ABSENT (plan non annonce)")
        return None, []
    params, listed = {}, []
    for line in read_text(path).splitlines():
        if line.startswith("seq="):
            listed.append(dict(kv.split("=", 1) for kv in line.split()))
        elif "=" in line:
            k, v = line.split("=", 1)
            params[k] = v
    if params.get(version_key) != "v1":
        bad.append(f"{fname}: version de plan inconnue")
        return None, []
    for key in param_keys + ("runs",):
        if key not in params or not params[key].strip():
            bad.append(f"{fname}: parametre {key} absent")
            return None, []
    return params, listed


def conf_sequence(params):
    seq = []
    for fam in params["families"].split():
        seq.append({"seq": str(len(seq) + 1), "name": f"v5ref_{fam}_n{params['n']}",
                    "family": fam, "kind": "v5ref"})
        seq.append({"seq": str(len(seq) + 1), "name": f"conf_{fam}_n{params['n']}",
                    "family": fam, "kind": "conf"})
    return seq


def bench_sequence(params):
    seq, cfg = [], 0
    for n in params["n_list"].split():
        for fam in params["families"].split():
            cfg += 1
            order = ["v5", "v6", "v6", "v5"] if cfg % 2 == 1 else ["v6", "v5", "v5", "v6"]
            reps = {"v5": 0, "v6": 0}
            for pos, eng in enumerate(order, 1):
                reps[eng] += 1
                seq.append({"seq": str(len(seq) + 1),
                            "name": f"bench_{fam}_n{n}_{eng}_r{reps[eng]}",
                            "family": fam, "n": n, "engine": eng,
                            "pos": str(pos), "repeat": str(reps[eng])})
    return seq


def queue_sequence(params):
    seq = []
    for fam in params["families"].split():
        for n in params["n_list"].split():
            for seed in params["seeds"].split():
                seq.append({"seq": str(len(seq) + 1), "name": f"queue_{fam}_n{n}_s{seed}",
                            "family": fam, "n": n, "seed": seed})
    return seq


def check_plan(fname, params, listed, want, bad):
    if params is None:
        return
    if not params["runs"].isdigit() or len(want) != int(params["runs"]):
        bad.append(f"{fname}: runs={params['runs']} != {len(want)} runs recalcules")
    if [(x["seq"], x["name"]) for x in listed] != [(x["seq"], x["name"]) for x in want]:
        bad.append(f"{fname}: sequence annoncee != sequence recalculee")


def check_common(out, name, commit, payload_sha, manifest_sha, threads, bad):
    """Verifications communes a tout run annonce. Retourne le corps .txt (ou
    None), le statut (ou None) et (mur_ms, duree_s, rss_kb)."""
    status = os.path.join(out, name + ".status")
    txt = os.path.join(out, name + ".txt")
    if not os.path.exists(status):
        bad.append(f"{name}: .status ABSENT (run annonce non execute)")
        return None, None, (None, None, None)
    st = read_text(status)
    if "finished=1" not in st:
        bad.append(f"{name}: status incomplet")
    m = re.search(r"^code=(\d+)$", st, re.M)
    if not m or m.group(1) != "0":
        bad.append(f"{name}: code={m.group(1) if m else '?'} (attendu 0)")
    kb = re.search(r"^peak_rss_kb=(\d+)$", st, re.M)
    if not kb or int(kb.group(1)) <= 0:
        bad.append(f"{name}: pic RSS absent ou nul")
    th = re.search(r"^threads=(\d+)$", st, re.M)
    if not th or th.group(1) != threads:
        bad.append(f"{name}: threads={th.group(1) if th else '?'} != {threads} (plan)")
    for field, want in (("source_commit", commit), ("source_payload_sha256", payload_sha),
                        ("protocol_manifest_sha256", manifest_sha)):
        fm = re.search(rf"^{field}=(\S+)$", st, re.M)
        if not fm or fm.group(1) != want:
            bad.append(f"{name}: {field} absent ou different du pin")
    duree = re.search(r"^duree_s=(\d+)$", st, re.M)
    gtime = status + ".time"
    if not os.path.exists(gtime) or "Maximum resident set size" not in read_text(gtime):
        bad.append(f"{name}: sortie complete de GNU time absente")
    if not os.path.exists(txt):
        bad.append(f"{name}: .txt ABSENT")
        return None, st, (None, None, None)
    body = read_text(txt)
    fb = FORBIDDEN.search(body)
    if fb:
        bad.append(f"{name}: motif interdit ({fb.group(0)})")
    mur = re.search(r"^temps_mur_ms=([0-9.]+)", body, re.M)
    return body, st, (float(mur.group(1)) if mur else None,
                      int(duree.group(1)) if duree else None,
                      int(kb.group(1)) if kb else None)


def check_pipeline_run(name, body, fam, n, seed, engine, threads, bad):
    """Un run de pilote (v5 ou v6) : identite, portee de tour, compteurs."""
    if body is None:
        return
    payload = "mhgp5-forests-horizontal-v1" if engine == "v5" else "mhgp6-forests-horizontal-v1"
    if f"payload={payload}" not in body:
        bad.append(f"{name}: ligne payload={payload} absente (moteur incoherent avec le nom)")
    if not TOWER.search(body):
        bad.append(f"{name}: ligne tower_scope absente")
    if not COUNTERS.search(body):
        bad.append(f"{name}: ligne de compteurs absente")
    ident = IDENT.search(body)
    if not ident:
        bad.append(f"{name}: ligne d'identite absente")
    elif (ident.group(1), ident.group(2), ident.group(3), ident.group(4),
          ident.group(5), ident.group(6)) != (fam, n, "8", "11", seed, threads):
        bad.append(f"{name}: identite imprimee ({'/'.join(ident.groups())}) != nom du run")
    cards = re.findall(r"^cardinalites K=(\d+) ", body, re.M)
    for k in range(1, KMAX + 1):
        c = sum(1 for kk in cards if int(kk) == k)
        if c != 1:
            bad.append(f"{name}: cardinalites K={k} presente {c} fois (attendu 1)")
    if not re.search(r"^temps_mur_ms=", body, re.M):
        bad.append(f"{name}: temps_mur_ms absent")


def determinism_signature(body):
    """Lignes deterministes d'un run pilote : compteurs, generation, sweep,
    cardinalites — jamais les temps ni les RSS."""
    sig = []
    for pat in (r"^famille=.*$", r"^generation .*$", r"^sweep .*$", r"^cardinalites K=\d+ .*$"):
        sig.extend(re.findall(pat, body, re.M))
    return tuple(sig)


def fmt(x, unit=1.0):
    return "NA" if x is None else f"{x / unit:.1f}"


def median(xs):
    xs = sorted(xs)
    if not xs:
        return None
    mid = len(xs) // 2
    return xs[mid] if len(xs) % 2 else (xs[mid - 1] + xs[mid]) / 2.0


def main():
    out, commit, payload_sha, manifest_sha, remote_rc, scp_rc = sys.argv[1:7]
    bad = []
    if remote_rc != "0":
        bad.append(f"session distante : remote_campaign_rc={remote_rc}")
    if scp_rc != "0":
        bad.append(f"rapatriement : scp_rc={scp_rc}")
    for tf in ("conf_tronquee.txt", "bench_tronquee.txt", "queue_tronquee.txt"):
        p = os.path.join(out, tf)
        if os.path.exists(p):
            bad.append(f"{tf}: campagne TRONQUEE ({read_text(p).strip().replace(chr(10), ' ; ')})")
    topo = os.path.join(out, "topologie.txt")
    if not os.path.exists(topo):
        bad.append("topologie.txt: ABSENT")
    else:
        tb = read_text(topo)
        if not re.search(r"^nproc=\d+$", tb, re.M) or "--- lscpu ---" not in tb:
            bad.append("topologie.txt: nproc ou lscpu absent")

    conf_params, conf_listed = read_plan(out, "conf_plan.txt", "conf_plan",
                                         ("families", "n", "threads"), bad)
    bench_params, bench_listed = read_plan(out, "bench_plan.txt", "bench_plan",
                                           ("families", "n_list", "threads"), bad)
    queue_params, queue_listed = read_plan(out, "queue_plan.txt", "queue_plan",
                                           ("families", "n_list", "seeds", "threads"), bad)
    conf_runs = conf_sequence(conf_params) if conf_params else []
    bench_runs = bench_sequence(bench_params) if bench_params else []
    queue_runs = queue_sequence(queue_params) if queue_params else []
    check_plan("conf_plan.txt", conf_params, conf_listed, conf_runs, bad)
    check_plan("bench_plan.txt", bench_params, bench_listed, bench_runs, bad)
    check_plan("queue_plan.txt", queue_params, queue_listed, queue_runs, bad)

    # PHASE 1 — conformite.
    conf_threads = conf_params["threads"] if conf_params else "0"
    v5ref_bodies = {}
    for run in conf_runs:
        name = run["name"]
        body, st, _ = check_common(out, name, commit, payload_sha, manifest_sha, conf_threads, bad)
        if body is None:
            continue
        if run["kind"] == "v5ref":
            check_pipeline_run(name, body, run["family"], conf_params["n"], "3", "v5", conf_threads, bad)
            digests = re.findall(r"^digest_all=([0-9a-f]{64})$", body, re.M)
            if len(digests) != 1:
                bad.append(f"{name}: digest_all present {len(digests)} fois (attendu 1)")
            cmd = re.search(r"^commande=(.*)$", st, re.M)
            if not cmd or "--digest" not in cmd.group(1).split():
                bad.append(f"{name}: commande gravee sans --digest (la reference exige les digests)")
            v5ref_bodies[run["family"]] = body
        else:
            if "conformite v5=v6" not in body or "identiques (objet)" not in body:
                bad.append(f"{name}: conformite v5=v6 non etablie (`identiques (objet)` absent)")
            cmd = re.search(r"^commande=(.*)$", st, re.M)
            if not cmd or f"v5ref_{run['family']}_n{conf_params['n']}.txt" not in cmd.group(1):
                bad.append(f"{name}: commande gravee sans la reference v5 de la meme famille")
            if run["family"] not in v5ref_bodies:
                bad.append(f"{name}: juge sans reference v5 valide")

    # PHASE 2 — bench apparie.
    bench_threads = bench_params["threads"] if bench_params else "0"
    signatures = {}  # (family, n, engine) -> {signature: [names]}
    measures = {}    # (family, n, engine) -> [(mur, duree, rss)]
    for run in bench_runs:
        name = run["name"]
        body, st, meas = check_common(out, name, commit, payload_sha, manifest_sha, bench_threads, bad)
        if body is None:
            continue
        check_pipeline_run(name, body, run["family"], run["n"], "3", run["engine"], bench_threads, bad)
        if ANY_DIGEST.search(body):
            bad.append(f"{name}: digest imprime sur un run de bench (mur non compare a armes egales)")
        cmd = re.search(r"^commande=(.*)$", st, re.M)
        if cmd:
            argv = cmd.group(1).split()
            if "--digest" in argv:
                bad.append(f"{name}: --digest dans la commande gravee d'un run de bench")
            want_args = {f"--family={run['family']}", f"--n={run['n']}", "--s=8", "--smax=11",
                         "--seed=3", f"--threads={bench_threads}"}
            if not want_args.issubset(argv):
                bad.append(f"{name}: commande gravee sans les arguments contractuels du nom")
        else:
            bad.append(f"{name}: commande absente")
        for field, want in (("engine", run["engine"]), ("family", run["family"]), ("n", run["n"]),
                            ("pos", run["pos"]), ("repeat", run["repeat"]), ("seq", run["seq"])):
            fm = re.search(rf"^{field}=(\S+)$", st, re.M)
            if not fm or fm.group(1) != want:
                bad.append(f"{name}: {field}={fm.group(1) if fm else '?'} != {want} (annonce)")
        key = (run["family"], run["n"], run["engine"])
        signatures.setdefault(key, {}).setdefault(determinism_signature(body), []).append(name)
        measures.setdefault(key, []).append(meas)
    for key, table in sorted(signatures.items()):
        if len(table) > 1:
            groups = sorted(table.values(), key=len, reverse=True)
            bad.append(f"bench {key}: compteurs NON deterministes entre repetitions "
                       f"(dissidents {', '.join(groups[1])})")

    # PHASE 3 — queue stationnaire.
    queue_threads = queue_params["threads"] if queue_params else "0"
    queue_rows = []
    for run in queue_runs:
        name = run["name"]
        body, st, meas = check_common(out, name, commit, payload_sha, manifest_sha, queue_threads, bad)
        if body is None:
            continue
        check_pipeline_run(name, body, run["family"], run["n"], run["seed"], "v6", queue_threads, bad)
        if ANY_DIGEST.search(body):
            bad.append(f"{name}: digest imprime sur un run de queue")
        vals = {}
        for cname, pat in QUEUE_COUNTERS:
            m = re.search(pat, body)
            if m is None:
                bad.append(f"{name}: compteur {cname} absent (grand-livre incomplet)")
            else:
                vals[cname] = m.group(1)
        for field, want in (("family", run["family"]), ("n", run["n"]),
                            ("seed", run["seed"]), ("seq", run["seq"])):
            fm = re.search(rf"^{field}=(\S+)$", st, re.M)
            if not fm or fm.group(1) != want:
                bad.append(f"{name}: {field}={fm.group(1) if fm else '?'} != {want} (annonce)")
        queue_rows.append((run, vals, meas))

    # Fichiers inattendus.
    known = {r["name"] for r in conf_runs + bench_runs + queue_runs}
    for f in sorted(os.listdir(out)):
        if f.endswith(".txt") and f[:-4] not in known and f not in AUX:
            bad.append(f"{f}: fichier inattendu")

    # RESUMES factuels (jamais une conclusion).
    lines = [
        "# bench_resume — COMPLETUDE et DISPERSION, produit par validate_v6_campaign.py.",
        "# Aucune conclusion de speedup : le validateur juge presence, pins, determinisme.",
        "famille\tn\tmoteur\truns\tmur_median_ms\tmur_min_ms\tmur_max_ms\tduree_med_s\trss_max_kb",
    ]
    for key in sorted(measures, key=lambda k: (k[0], int(k[1]), k[2])):
        vals = measures[key]
        murs = [v[0] for v in vals if v[0] is not None]
        durees = [v[1] for v in vals if v[1] is not None]
        rss = [v[2] for v in vals if v[2] is not None]
        lines.append("\t".join([key[0], key[1], key[2], str(len(vals)),
                                fmt(median(murs)), fmt(min(murs) if murs else None),
                                fmt(max(murs) if murs else None), fmt(median(durees)),
                                str(max(rss)) if rss else "NA"]))
    tmp = os.path.join(out, "bench_resume.txt.tmp")
    with open(tmp, "w", encoding="utf-8") as fh:
        fh.write("\n".join(lines) + "\n")
    os.replace(tmp, os.path.join(out, "bench_resume.txt"))

    lines = [
        "# queue_resume — grand-livre de la sonde E6 a l'echelle (compteurs bruts, pas de pente ici).",
        "famille\tn\tgraine\tW_sweep1\tW_sweep2\ttri_comp\tm_anchor_q4\tmur_ms\trss_kb",
    ]
    for run, vals, meas in queue_rows:
        lines.append("\t".join([run["family"], run["n"], run["seed"],
                                vals.get("W_sweep1_evals_coeur", "NA"),
                                vals.get("W_sweep2_evals_passe2", "NA"),
                                vals.get("tri_comparaisons", "NA"),
                                vals.get("m_anchor_q4", "NA"),
                                fmt(meas[0]), str(meas[2]) if meas[2] is not None else "NA"]))
    tmp = os.path.join(out, "queue_resume.txt.tmp")
    with open(tmp, "w", encoding="utf-8") as fh:
        fh.write("\n".join(lines) + "\n")
    os.replace(tmp, os.path.join(out, "queue_resume.txt"))

    if bad:
        print("campaign_status=partial_or_failed")
        for b in bad:
            print("  -", b)
        return 1
    print(f"campaign_status=complete ({len(known)} runs valides, source_commit={commit[:12]}, "
          f"resumes bench_resume.txt / queue_resume.txt)")
    print("=== CAMPAGNE COMPLETE ===")
    return 0


if __name__ == "__main__":
    sys.exit(main())
