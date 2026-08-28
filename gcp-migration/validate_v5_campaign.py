#!/usr/bin/env python3
"""Validation LOCALE de la campagne v5 : seule cette validation decide de
campaign_status — jamais le lanceur, jamais la VM. `complete` exige l'ensemble
exact des fichiers, code=0 partout, la ligne de compteurs v5 presente, la
conformite v4 (`balls=egal all=egal`) sur chaque run de phase 1, un digest_all
sur chaque run de phase 2, aucun motif interdit, le MEME pin (commit, sha256 du
payload, manifeste du protocole) dans chaque statut, et des codes de session
(ssh distant, scp) nuls.

Phase optionnelle SCALE_THREADS (P0 de l'audit « rendement GPU et multi-CPU »
du 28 aout 2026) : si `scale_threads_plan.txt` est present, le validateur
RECALCULE la sequence contrebalancee depuis les parametres du plan, exige un
statut et une sortie pour CHAQUE run annonce (code 0, pin, RSS, GNU time
complet, fils/inflight/digest honores par la commande gravee et par la sortie
du pilote), le MEME digest_all pour toutes les combinaisons d'une famille
lorsque digest=1 (et aucun digest lorsque digest=0), la MEME ligne
`generation ...` et les MEMES `cardinalites K=...` quels que soient les fils et
inflight, la topologie gravee (topologie.txt), puis ecrit le tableau
`scale_threads_resume.txt` (famille, fils, inflight, digest, mur median / min /
max, RSS max). Il ne conclut JAMAIS sur une acceleration : completude et
egalite seulement.

Usage : validate_v5_campaign.py OUT_DIR SOURCE_COMMIT SOURCE_PAYLOAD_SHA256 \\
        PROTOCOL_MANIFEST_SHA256 REMOTE_CAMPAIGN_RC SCP_RC
Sortie : 0 si complete, 1 sinon (les preuves partielles restent sur place).
"""
import os
import re
import sys

FAMILIES = ("uniform", "terrain", "eight_clusters", "scanline_single_pass")
KMAX = 10
SCALE_PLAN = "scale_threads_plan.txt"
SCALE_RESUME = "scale_threads_resume.txt"
TOPOLOGY = "topologie.txt"
# Fichiers .txt auxiliaires de la phase SCALE_THREADS (jamais des runs).
SCALE_AUX = (SCALE_PLAN, SCALE_RESUME, TOPOLOGY)


def expected_names():
    names = ["gpu_witness", "gpu_lane", "gpu_mutant"]
    for n in (8000, 16000, 32000):
        for fam in FAMILIES:
            names.append(f"conf_{fam}_n{n}")
    for fam in FAMILIES:
        names.append(f"contrat_{fam}_n50000")
    for fam in FAMILIES:
        names.append(f"contrat_gpu_{fam}_n50000")
    for fam in ("eight_clusters", "scanline_single_pass"):
        names.append(f"contrat_gpuad_{fam}_n50000")
    return names


def read_text(path):
    with open(path, encoding="utf-8", errors="replace") as fh:
        return fh.read()


def scale_sequence(params):
    """Sequence contrebalancee EXACTE du runner : pour chaque repetition r, la
    liste des fils dans l'ordre donne (r impair) ou inversee (r pair), boucles
    famille > inflight > digest > fils. Retourne des dicts (seq, name, ...)."""
    seq = []
    threads = params["threads_list"].split()
    for r in range(1, int(params["repeats"]) + 1):
        order = threads if r % 2 == 1 else list(reversed(threads))
        for fam in params["families"].split():
            for infl in params["inflight_list"].split():
                for dig in params["digest_list"].split():
                    for t in order:
                        seq.append({
                            "seq": str(len(seq) + 1),
                            "name": f"scale_{fam}_n{params['n']}_t{t}_f{infl}_d{dig}_r{r}",
                            "family": fam, "threads": t, "inflight": infl,
                            "digest": dig, "repeat": str(r)})
    return seq


def read_scale_plan(out, bad):
    """Lit le plan annonce ; retourne (params, runs) ou (None, []) si absent.
    Le plan est juge : parametres complets, sequence identique a la sequence
    recalculee, nombre de runs coherent."""
    path = os.path.join(out, SCALE_PLAN)
    if not os.path.exists(path):
        return None, []
    params, listed = {}, []
    for line in read_text(path).splitlines():
        if line.startswith("seq="):
            listed.append(dict(kv.split("=", 1) for kv in line.split()))
        elif "=" in line:
            k, v = line.split("=", 1)
            params[k] = v
    for key in ("threads_list", "families", "n", "inflight_list", "digest_list", "repeats", "runs"):
        if key not in params or not params[key].strip():
            bad.append(f"{SCALE_PLAN}: parametre {key} absent")
            return None, []
    if params.get("scale_threads_plan") != "v1":
        bad.append(f"{SCALE_PLAN}: version de plan inconnue")
    try:
        want = scale_sequence(params)
    except ValueError:
        bad.append(f"{SCALE_PLAN}: parametres non entiers")
        return None, []
    if not params["runs"].isdigit() or len(want) != int(params["runs"]):
        bad.append(f"{SCALE_PLAN}: runs={params['runs']} != {len(want)} runs recalcules")
    if [(x["seq"], x["name"]) for x in listed] != [(x["seq"], x["name"]) for x in want]:
        bad.append(f"{SCALE_PLAN}: sequence annoncee != sequence contrebalancee recalculee")
    return params, want


def check_scale_runs(out, params, runs, commit, payload_sha, manifest_sha, bad):
    """Juge chaque run annonce et l'egalite de l'objet par famille."""
    forbidden = re.compile(r"REFUS|INVARIANT|DIVERGENCE|PLANCHER|Killed|bad_alloc|AddressSanitizer")
    counters = re.compile(r"boules_uniques=\d+.*evenements=\d+.*facettes=\d+")
    topo = os.path.join(out, TOPOLOGY)
    if not os.path.exists(topo):
        bad.append(f"{TOPOLOGY}: ABSENT (topologie non gravee)")
    else:
        tb = read_text(topo)
        if not re.search(r"^nproc=\d+$", tb, re.M):
            bad.append(f"{TOPOLOGY}: nproc absent")
        if not re.search(r"^affinite_runner=\S+$", tb, re.M):
            bad.append(f"{TOPOLOGY}: affinite du runner absente")
        if "--- lscpu ---" not in tb:
            bad.append(f"{TOPOLOGY}: lscpu absent")
    # Par famille : digest_all (digest=1), ligne generation, cardinalites.
    per_family = {}
    measures = {}  # (family, threads, inflight, digest) -> liste de (mur_ms, duree_s, rss_kb)
    for run in runs:
        name = run["name"]
        status = os.path.join(out, name + ".status")
        txt = os.path.join(out, name + ".txt")
        if not os.path.exists(status):
            bad.append(f"{name}: .status ABSENT (run annonce non execute)")
            continue
        st = read_text(status)
        if "finished=1" not in st:
            bad.append(f"{name}: status incomplet")
        m = re.search(r"^code=(\d+)$", st, re.M)
        if not m or m.group(1) != "0":
            bad.append(f"{name}: code={m.group(1) if m else '?'} (attendu 0)")
        kb = re.search(r"^peak_rss_kb=(\d+)$", st, re.M)
        if not kb or int(kb.group(1)) <= 0:
            bad.append(f"{name}: pic RSS absent ou nul")
        for field, want in (("source_commit", commit), ("source_payload_sha256", payload_sha),
                            ("protocol_manifest_sha256", manifest_sha)):
            fm = re.search(rf"^{field}=(\S+)$", st, re.M)
            if not fm or fm.group(1) != want:
                bad.append(f"{name}: {field} absent ou different du pin")
        for field, want in (("timing_scope", "scale_threads"), ("threads", run["threads"]),
                            ("fold_inflight", run["inflight"]), ("digest", run["digest"]),
                            ("family", run["family"]), ("n", params["n"]),
                            ("repeat", run["repeat"]), ("seq", run["seq"])):
            fm = re.search(rf"^{field}=(\S+)$", st, re.M)
            if not fm or fm.group(1) != want:
                bad.append(f"{name}: {field}={fm.group(1) if fm else '?'} != {want} (annonce)")
        cmd = re.search(r"^commande=(.*)$", st, re.M)
        if not cmd:
            bad.append(f"{name}: commande absente")
        else:
            argv = cmd.group(1).split()
            want_args = {f"--family={run['family']}", f"--n={params['n']}", "--s=8", "--smax=11", "--seed=3",
                         f"--threads={run['threads']}", f"--fold-inflight={run['inflight']}"}
            if not want_args.issubset(argv):
                bad.append(f"{name}: commande gravee sans les arguments contractuels du nom")
            if ("--digest" in argv) != (run["digest"] == "1"):
                bad.append(f"{name}: commande gravee et digest={run['digest']} incoherents")
            if "--gpu" in argv:
                bad.append(f"{name}: SCALE_THREADS est une campagne CPU (--gpu refuse)")
        duree = re.search(r"^duree_s=(\d+)$", st, re.M)
        gtime = status + ".time"
        if not os.path.exists(gtime) or "Maximum resident set size" not in read_text(gtime):
            bad.append(f"{name}: sortie complete de GNU time absente ({os.path.basename(gtime)})")
        if not os.path.exists(txt):
            bad.append(f"{name}: .txt ABSENT")
            continue
        body = read_text(txt)
        fb = forbidden.search(body)
        if fb:
            bad.append(f"{name}: motif interdit ({fb.group(0)})")
        if not counters.search(body):
            bad.append(f"{name}: ligne de compteurs absente")
        ident = re.search(r"^famille=(\S+) n=(\d+) coord=\d+ s=(\d+) smax=(\d+) seed=(\d+) threads=(\d+) ", body, re.M)
        if not ident:
            bad.append(f"{name}: ligne d'identite absente")
        elif (ident.group(1), ident.group(2), ident.group(3), ident.group(4), ident.group(5), ident.group(6)) != \
                (run["family"], params["n"], "8", "11", "3", run["threads"]):
            bad.append(f"{name}: identite imprimee ({'/'.join(ident.groups())}) != nom du run")
        # Ligne du fold : format courant `fold_inflight=<I>, pic_mesure_en_vol=<P>`
        # (domaine [1, kFoldInflightMax] valide par le pilote) ; l'ancien
        # `<I> ordre(s) en vol` reste accepte pour rejouer un reçu anterieur.
        infl = re.search(r"^temps_fold_mur_ms=[0-9.]+ \(etages A et B, (?:fold_inflight=(\d+), pic_mesure_en_vol=\d+|(\d+) ordre\(s\) en vol)\)$", body, re.M)
        infl_seen = (infl.group(1) or infl.group(2)) if infl else None
        if infl_seen != run["inflight"]:
            bad.append(f"{name}: fold_inflight imprime {infl_seen or '?'} != {run['inflight']} demande")
        if "--gpu" in body or re.search(r"^gpu=1", body, re.M) or re.search(r"^backend=override", body, re.M):
            bad.append(f"{name}: sortie device sur une campagne CPU")
        gen = re.search(r"^generation .*$", body, re.M)
        if not gen:
            bad.append(f"{name}: ligne generation absente")
        cards = re.findall(r"^cardinalites K=(\d+) (.*)$", body, re.M)
        for k in range(1, KMAX + 1):
            c = sum(1 for kk, _ in cards if int(kk) == k)
            if c != 1:
                bad.append(f"{name}: cardinalites K={k} presente {c} fois (attendu 1)")
        digests = re.findall(r"^digest_all=([0-9a-f]{64})$", body, re.M)
        any_digest = re.search(r"^digest_(balls|forest_K\d+|all)=", body, re.M)
        if run["digest"] == "1":
            if len(digests) != 1:
                bad.append(f"{name}: digest_all present {len(digests)} fois (attendu 1)")
        elif any_digest:
            bad.append(f"{name}: digest imprime alors que digest=0 (le drapeau n'est pas honore)")
        mur = re.search(r"^temps_mur_ms=([0-9.]+)", body, re.M)
        if not mur:
            bad.append(f"{name}: temps_mur_ms absent")
        fam = per_family.setdefault(run["family"], {"digest": {}, "generation": {}, "cards": {}})
        if digests:
            fam["digest"].setdefault(digests[0], []).append(name)
        if gen:
            fam["generation"].setdefault(gen.group(0), []).append(name)
        fam["cards"].setdefault(tuple(sorted(f"K={k} {v}" for k, v in cards)), []).append(name)
        key = (run["family"], run["threads"], run["inflight"], run["digest"])
        measures.setdefault(key, []).append((
            float(mur.group(1)) if mur else None,
            int(duree.group(1)) if duree else None,
            int(kb.group(1)) if kb else None))
    for fam, sig in sorted(per_family.items()):
        for label, table in (("digest_all", sig["digest"]), ("ligne generation", sig["generation"]),
                             ("cardinalites", sig["cards"])):
            if len(table) > 1:
                groups = sorted(table.values(), key=len, reverse=True)
                bad.append(f"{fam}: {label} DIFFERENT entre fils/inflight/repetitions "
                           f"(majorite {groups[0][0]}..., dissidents {', '.join(groups[1])})")
    return measures


def write_scale_resume(out, params, runs, measures):
    """Tableau RESUME (completude et dispersion) — jamais une acceleration."""
    def median(xs):
        xs = sorted(xs)
        if not xs:
            return None
        mid = len(xs) // 2
        return xs[mid] if len(xs) % 2 else (xs[mid - 1] + xs[mid]) / 2.0

    def fmt(x, unit=1.0):
        return "NA" if x is None else f"{x / unit:.1f}"

    lines = [
        "# scale_threads_resume — tableau de COMPLETUDE et de DISPERSION, produit par validate_v5_campaign.py.",
        "# Aucune conclusion de speedup n'est ecrite ici : le validateur juge seulement la presence de",
        "# tous les runs annonces et l'egalite des digests / compteurs de travail par famille.",
        f"# n={params['n']} threads_list={params['threads_list']} inflight_list={params['inflight_list']} "
        f"digest_list={params['digest_list']} repeats={params['repeats']} runs_annonces={len(runs)}",
        "# mur = temps_mur_ms interne du pilote ; duree = duree_s externe (GNU time / runner) ; rss = pic RSS GNU time.",
        "famille\tfils\tinflight\tdigest\truns\tmur_median_ms\tmur_min_ms\tmur_max_ms\tduree_med_s\trss_max_kb",
    ]
    seen = []
    for run in runs:
        key = (run["family"], run["threads"], run["inflight"], run["digest"])
        if key not in seen:
            seen.append(key)
    for key in sorted(seen, key=lambda k: (k[0], int(k[1]), int(k[2]), int(k[3]))):
        vals = measures.get(key, [])
        murs = [v[0] for v in vals if v[0] is not None]
        durees = [v[1] for v in vals if v[1] is not None]
        rss = [v[2] for v in vals if v[2] is not None]
        lines.append("\t".join([
            key[0], key[1], key[2], key[3], str(len(vals)),
            fmt(median(murs)), fmt(min(murs) if murs else None), fmt(max(murs) if murs else None),
            fmt(median(durees)), str(max(rss)) if rss else "NA"]))
    tmp = os.path.join(out, SCALE_RESUME + ".tmp")
    with open(tmp, "w", encoding="utf-8") as fh:
        fh.write("\n".join(lines) + "\n")
    os.replace(tmp, os.path.join(out, SCALE_RESUME))


def main():
    out, commit, payload_sha, manifest_sha, remote_rc, scp_rc = sys.argv[1:7]
    forbidden = re.compile(r"REFUS|INVARIANT|DIVERGENCE|PLANCHER|Killed|bad_alloc|AddressSanitizer")
    counters = re.compile(r"boules_uniques=\d+.*evenements=\d+.*facettes=\d+")
    conformity = re.compile(r"conformite_v4 .*balls=egal all=egal")
    digest = re.compile(r"^digest_all=[0-9a-f]{64}$", re.M)
    bad = []
    if remote_rc != "0":
        bad.append(f"session distante : remote_campaign_rc={remote_rc}")
    if scp_rc != "0":
        bad.append(f"rapatriement : scp_rc={scp_rc}")
    # Runs d'extension (contrat_<fam>_n<N>, N != 50000) : optionnels, mais s'ils sont presents ils sont juges.
    extra = sorted(os.path.basename(f)[:-7] for f in os.listdir(out) if f.endswith(".status")
                   and re.match(r"^contrat_[a-z_]+_n\d+$", os.path.basename(f)[:-7])
                   and os.path.basename(f)[:-7] not in expected_names())
    for name in expected_names() + extra:
        txt = os.path.join(out, name + ".txt")
        status = os.path.join(out, name + ".status")
        if not os.path.exists(status):
            bad.append(f"{name}: .status ABSENT")
            continue
        st = open(status).read()
        if "finished=1" not in st:
            bad.append(f"{name}: status incomplet")
        m = re.search(r"^code=(\d+)$", st, re.M)
        want_code = "4" if name == "gpu_mutant" else "0"  # le mutant du temoin doit etre TUE (code 4)
        if not m or m.group(1) != want_code:
            bad.append(f"{name}: code={m.group(1) if m else '?'} (attendu {want_code})")
        kb = re.search(r"^peak_rss_kb=(\d+)$", st, re.M)
        if not kb or int(kb.group(1)) <= 0:
            bad.append(f"{name}: pic RSS absent ou nul")
        for field, want in (("source_commit", commit), ("source_payload_sha256", payload_sha),
                            ("protocol_manifest_sha256", manifest_sha)):
            fm = re.search(rf"^{field}=(\S+)$", st, re.M)
            if not fm or fm.group(1) != want:
                bad.append(f"{name}: {field} absent ou different du pin")
        if not os.path.exists(txt):
            bad.append(f"{name}: .txt ABSENT")
            continue
        body = open(txt, errors="replace").read()
        fb = forbidden.search(body)
        if fb:
            bad.append(f"{name}: motif interdit ({fb.group(0)})")
        if name == "gpu_mutant" and "DESACCORD device/hote" not in body:
            bad.append(f"{name}: le mutant du temoin n'a pas ete tue sur le device")
        if name not in ("gpu_witness", "gpu_lane", "gpu_mutant") and not counters.search(body):
            bad.append(f"{name}: ligne de compteurs absente")
        if name == "gpu_witness":
            if not re.search(r"^nvcc=\S+$", body, re.M) or "release 12.9" not in body:
                bad.append(f"{name}: provenance nvcc absente ou pas 12.9")
            if not re.search(r"^NVIDIA .*, \d+\.\d+", body, re.M):
                bad.append(f"{name}: provenance nvidia-smi absente (fail-closed)")
            # Les deux familles, leurs planchers et desaccords=0 partout — pas
            # la seule sous-chaine finale.
            arith = re.search(r"^arith cas=(\d+) desaccords=(\d+)$", body, re.M)
            if not arith or int(arith.group(1)) < 1000 or int(arith.group(2)) != 0:
                bad.append(f"{name}: lot arithmetique absent, sous le plancher ou en desaccord")
            for fam in ("uniform", "eight_clusters"):
                m = re.search(rf"^scan famille={fam} ancres=\d+ seeds=(\d+) sites=\d+ morts=\d+ desaccords=(\d+) kernel_ms=", body, re.M)
                if not m or int(m.group(1)) < 1000 or int(m.group(2)) != 0:
                    bad.append(f"{name}: scan {fam} absent, sous le plancher (1000 seeds) ou en desaccord")
            if "device_witness OK" not in body:
                bad.append(f"{name}: temoin device non conforme")
        if name.startswith("contrat_gpu_") or name.startswith("contrat_gpuad_"):
            if not re.search(r"^gpu=1 kernel_ms=", body, re.M):
                bad.append(f"{name}: le run n'annonce pas gpu=1")
            cpu_txt = os.path.join(out, name.replace("contrat_gpuad_", "contrat_").replace("contrat_gpu_", "contrat_") + ".txt")
            cpu_body = open(cpu_txt, encoding="utf-8", errors="replace").read() if os.path.exists(cpu_txt) else ""
            # Les DEUX digests (boules et forets) doivent etre identiques au contrat CPU.
            for key in ("digest_balls", "digest_all"):
                d_gpu = re.search(rf"^{key}=([0-9a-f]{{64}})$", body, re.M)
                d_cpu = re.search(rf"^{key}=([0-9a-f]{{64}})$", cpu_body, re.M)
                if not d_gpu or not d_cpu or d_gpu.group(1) != d_cpu.group(1):
                    bad.append(f"{name}: {key} DIFFERENT du contrat CPU de la meme famille (ou absent)")
            if not re.search(r"^backend=override_experimental", body, re.M):
                bad.append(f"{name}: le run --gpu doit s'annoncer backend=override_experimental (non autoritaire)")
            g = re.search(r"^gpu=1 kernel_ms=[0-9.]+ lancements=(\d+) min_sites=(\d+) routage_q3=(\d+)/(\d+) ancres \(seeds (\d+)/(\d+)\) routage_q4=(\d+)/(\d+) ancres \(seeds (\d+)/(\d+)\)", body, re.M)
            if not g:
                bad.append(f"{name}: ligne gpu= (lancements, min_sites, routage) absente ou mal formee")
            else:
                launches, min_sites = int(g.group(1)), int(g.group(2))
                s3d, s3h, s4d, s4h = int(g.group(5)), int(g.group(6)), int(g.group(9)), int(g.group(10))
                if launches < 1:
                    bad.append(f"{name}: aucun lancement device")
                if name.startswith("contrat_gpuad_"):
                    if min_sites != 256:
                        bad.append(f"{name}: adaptatif attendu a min_sites=256, vu {min_sites}")
                    if s3d < 1 or s3h < 1 or s4d < 1 or s4h < 1:
                        bad.append(f"{name}: adaptatif — les deux routes doivent avoir des seeds (q3 {s3d}/{s3h}, q4 {s4d}/{s4h})")
                elif min_sites != 1:
                    bad.append(f"{name}: tout-device attendu a min_sites=1, vu {min_sites}")
        if name == "gpu_lane":
            # Triples EXACTS famille/taille/fils, planchers (100000 candidats a
            # 8 k, seeds et morts non vides, lancements > 0), desaccords = 0,
            # et une occurrence unique de chaque triple.
            want = {("uniform", "1200", "1"): 200, ("eight_clusters", "1200", "4"): 200, ("uniform", "8000", "8"): 100000,
                    ("uniform", "300", "1"): 200}  # cocirculaire (coord=40) : replis exacts exerces
            for lane, cand_key in (("q3_lane_device", "candidats_q3"), ("q4_lane_device", "candidats_q4")):
                rx = re.compile(rf"^{lane} famille=(\S+) n=(\d+) fils=(\d+) (.*) desaccords_vecteur=(\d+) desaccords_compteurs=(\d+)$", re.M)
                seen = {}
                for f, n, t, mid, v, k in rx.findall(body):
                    kv = dict(re.findall(r"(\w+)=([0-9.]+)", mid))
                    key = (f, n, t)
                    seen[key] = seen.get(key, 0) + 1
                    if key not in want:
                        bad.append(f"{name}: {lane} — triple inattendu {key}")
                        continue
                    if int(v) != 0 or int(k) != 0:
                        bad.append(f"{name}: {lane} {key} — desaccords")
                    if int(kv.get(cand_key, 0)) < want[key]:
                        bad.append(f"{name}: {lane} {key} — plancher de candidats {want[key]} non atteint")
                    if int(kv.get("seeds", 0)) < 1 or int(kv.get("tues", kv.get("coeur_tues", 0))) < 1 or int(kv.get("lancements", 0)) < 1:
                        bad.append(f"{name}: {lane} {key} — seeds, morts ou lancements vides")
                for key in want:
                    if seen.get(key, 0) != 1:
                        bad.append(f"{name}: {lane} {key} — {seen.get(key, 0)} occurrence(s), une attendue")
                if body.count(f"{lane} OK") != 4:
                    bad.append(f"{name}: {lane} — quatre OK attendus, {body.count(lane + ' OK')} vus")
        if name.startswith("conf_") and not conformity.search(body):
            bad.append(f"{name}: conformite v4 non etablie")
        if name.startswith("contrat_") and not digest.search(body):
            bad.append(f"{name}: digest_all absent")
    # Phase SCALE_THREADS (optionnelle) : jugee si un plan a ete annonce ; des
    # runs scale_* sans plan sont des fichiers inattendus (jamais un
    # sous-ensemble juge a la place de l'annonce).
    scale_params, scale_runs = read_scale_plan(out, bad)
    scale_note = ""
    if scale_params is not None:
        measures = check_scale_runs(out, scale_params, scale_runs, commit, payload_sha, manifest_sha, bad)
        write_scale_resume(out, scale_params, scale_runs, measures)
        scale_note = f", scale_threads {len(scale_runs)} runs annonces -> {SCALE_RESUME}"
    known = set(expected_names()) | set(extra) | {r["name"] for r in scale_runs}
    for f in sorted(os.listdir(out)):
        if f.endswith(".txt") and f[:-4] not in known and f not in SCALE_AUX:
            bad.append(f"{f}: fichier inattendu")
    if bad:
        print("campaign_status=partial_or_failed")
        for b in bad:
            print("  -", b)
        return 1
    print(f"campaign_status=complete ({len(known)} runs valides, source_commit={commit[:12]}{scale_note})")
    print("=== CAMPAGNE COMPLETE ===")
    return 0


if __name__ == "__main__":
    sys.exit(main())
