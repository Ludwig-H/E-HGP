#!/usr/bin/env python3
"""Validation LOCALE de la campagne v6 : seule cette validation decide de
campaign_status — jamais le lanceur, jamais la VM. `complete` exige :
  - les SIX plans annonces (conf, sweep, gpu, frontier, bench, queue),
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
  - phase FILS : moteur v6 seul, et les lignes deterministes INVARIANTES PAR
    FILS (generation, compteurs, cardinalites — jamais ouvriers ni identite)
    IDENTIQUES entre TOUS les fils et repetitions d'un meme (famille, n) —
    la doctrine "sorties bit-identiques quel que soit le nombre de fils"
    jugee a l'echelle ;
  - phase GPU (v5, contrats herites de validate_v5_campaign.py) : temoin
    device conforme (nvcc 12.9, lot arithmetique, scans sans desaccord),
    mutant du temoin TUE (code 4), lanes q3/q4 device aux planchers exacts,
    et par famille digest_balls + digest_all IDENTIQUES entre le contrat CPU
    et chacun des contrats --gpu / adaptatif / wire=index ;
  - phase FRONTIERE : TROIS classes fermees, mutuellement EXCLUSIVES
    (septieme tour) : code 0 = contrat pipeline complet + motifs interdits
    scannes ; code 2 = refus du pipeline a la grammaire fermee
    « REFUS resource_exhausted : ... » (exactement une ligne, jamais
    bad_alloc dans le corps) ; code 134 = abort a diagnostic exact
    « std::bad_alloc », PROUVE par le superviseur (« terminated by
    signal 6 » dans la sortie GNU time — coreutils timeout propage le
    signal du fils en se le renvoyant), SOUS RLIMIT_AS atteste
    (limit_kind/limit_kb exactement une fois, commande a correspondance
    EXACTE avec le wrapper ulimit du plan) et jamais une ligne REFUS.
    Un code 124 est une SORTIE NON ATTRIBUEE (indistinguable d'un
    exit 124 du binaire, aucun marqueur causal distinct) : il INVALIDE la
    phase, comme 3, 127, 139, un code absent/non decimal et tout signal
    non prouve. Meme correcte, cette mesure est une frontiere SOUS PLAFOND
    VIRTUEL RLIMIT_AS, pas le mur RAM natif de la VM. RSS et sortie GNU
    time exiges dans tous les cas ;
  - des codes de session (ssh distant, scp) nuls.
Il ecrit bench_resume.txt et queue_resume.txt (murs / RSS — completude et
dispersion, JAMAIS une conclusion d'acceleration ni de pente).

Le PROFIL DE CAMPAGNE (7e argument, obligatoire — audit GCP v6, P1) est le
fichier ecrit par le cycle de vie AVANT la campagne : la matrice attendue en
vient, jamais des axes declares par le runner lui-meme. Les plans annonces
doivent EGALER le profil (une matrice reduite n'est jamais `complete`).

Le PROFIL CANONIQUE (8e argument) est le fichier versionne du manifeste :
le profil de campagne doit porter son nom et son sha256 EXACTS (liaison,
audit troisieme tour). Seul un profil effectif == canonique == decision_v1
obtient `campaign_status=decision_complete` ; tout autre profil valide rend
`campaign_status=verifie_non_decisionnel` (code 0, jamais une decision).

Usage : validate_v6_campaign.py OUT_DIR SOURCE_COMMIT SOURCE_PAYLOAD_SHA256 \\
        PROTOCOL_MANIFEST_SHA256 REMOTE_CAMPAIGN_RC SCP_RC PROFIL_CAMPAGNE \\
        PROFIL_CANONIQUE MANIFESTE_REVALIDE
Sortie : 0 si valide, 1 sinon (les preuves partielles restent sur place).
"""
import hashlib
import math
import os
import re
import sys

KMAX = 10
# Les resumes sont ecrits A COTE de out/ (idempotence : le validateur ne
# modifie jamais l'inventaire qu'il juge).
AUX = ("topologie.txt", "conf_plan.txt", "bench_plan.txt", "queue_plan.txt",
       "sweep_plan.txt", "gpu_plan.txt", "frontier_plan.txt",
       "matrice_plan.txt", "attrib_plan.txt", "gpuv6_plan.txt",
       "gpuv6_inventaire.txt",
       "MANIFESTE_DISTANT.txt",
       "conf_tronquee.txt", "bench_tronquee.txt", "queue_tronquee.txt",
       "sweep_tronquee.txt", "gpu_tronquee.txt", "frontier_tronquee.txt",
       "matrice_tronquee.txt", "attrib_tronquee.txt", "gpuv6_tronquee.txt")
FORBIDDEN = re.compile(r"REFUS|INVARIANT|DIVERGENCE|PLANCHER|Killed|bad_alloc|AddressSanitizer")
# Motifs FATALS de la frontiere (sixieme tour) : appliques a TOUTES les
# classes d'issue — un motif de capacite ne peut pas les masquer.
FATAL_FRONT = re.compile(r"INVARIANT|DIVERGENCE|PLANCHER|AddressSanitizer|Sanitizer|Killed|"
                         r"command not found|Segmentation fault")
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
    ("h_scan_q4", r"h_scan=\d+/\d+/(\d+) m_anchor"),
    ("m_anchor_q4", r"m_anchor=\d+/\d+/(\d+) entrees_ancres"),
    ("entrees_ancres_q4", r"entrees_ancres=\d+/\d+/(\d+)"),
    ("iters_coeur", r"iters_coeur=(\d+)"),
    ("iters_passe2", r"iters_passe2=(\d+)"),
    ("octaves_ancres", r"octaves_q4 ancres=([0-9,]+)"),
    ("octaves_seeds", r"octaves_q4 ancres=[0-9,]+ seeds=([0-9,]+)"),
    ("octaves_w1", r"octaves_q4 ancres=[0-9,]+ seeds=[0-9,]+ w1=([0-9,]+)"),
    ("octaves_seeds_cellules", r"octaves_q4_seeds cellules=([0-9,]+) coeur="),
    ("octaves_seeds_passe2", r"octaves_q4_seeds cellules=[0-9,]+ coeur=[0-9,]+ corde=[0-9,]+ passe2=([0-9,]+)"),
    ("vcensus_prefiltre_noeuds", r"vcensus prefiltre_nœuds=(\d+)"),
    ("vcensus_census_noeuds", r"census_nœuds=(\d+)"),
    ("p_factor_q4", r"p_factor=\d+/\d+/(\d+)"),
    ("ledger_emis", r"ledger_paires emis=(\d+)"),
]


def read_text(path):
    with open(path, encoding="utf-8", errors="replace") as fh:
        return fh.read()


def read_plan(out, fname, version_key, param_keys, bad, version="v1"):
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
    if params.get(version_key) != version:
        bad.append(f"{fname}: version de plan inconnue")
        return None, []
    for key in param_keys + ("runs",):
        if key not in params or not params[key].strip():
            bad.append(f"{fname}: parametre {key} absent")
            return None, []
    return params, listed


def expand_axis(text):
    """Expansion unique d'un axe : liste de jetons, sentinelle `aucun`
    filtree — TOUTE phase vide passe par ici (bloquant cinquieme tour)."""
    return [tok for tok in text.split() if tok != "aucun"]


def conf_sequence(params):
    seq = []
    for spec in expand_axis(params["specs"]):
        fam, n = spec.split(":", 1)
        seq.append({"seq": str(len(seq) + 1), "name": f"v5ref_{fam}_n{n}",
                    "family": fam, "n": n, "kind": "v5ref"})
        seq.append({"seq": str(len(seq) + 1), "name": f"conf_{fam}_n{n}",
                    "family": fam, "n": n, "kind": "conf"})
    return seq


def bench_sequence(params):
    seq, cfg = [], 0
    for spec in expand_axis(params["specs"]):
        fam, n = spec.split(":", 1)
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
    for fam in expand_axis(params["families"]):
        for n in expand_axis(params["n_list"]):
            for seed in expand_axis(params["seeds"]):
                seq.append({"seq": str(len(seq) + 1), "name": f"queue_{fam}_n{n}_s{seed}",
                            "family": fam, "n": n, "seed": seed})
    return seq


def sweep_sequence(params):
    seq = []
    reps = int(params["repeats"]) if params["repeats"].isdigit() else 0
    for spec in expand_axis(params["specs"]):
        fam, n, tl = spec.split(":", 2)
        threads = tl.split(",")
        for r in range(1, reps + 1):
            order = threads if r % 2 == 1 else list(reversed(threads))
            for pos, t in enumerate(order, 1):
                seq.append({"seq": str(len(seq) + 1), "name": f"sweep_{fam}_n{n}_t{t}_r{r}",
                            "family": fam, "n": n, "sweep_threads": t,
                            "repeat": str(r), "pos": str(pos)})
    return seq


GPU_KINDS = ("cpu", "dev", "ad", "idx")


def gpu_sequence(params):
    seq = []
    if not expand_axis(params["specs"]):
        return seq
    for nm, kind in (("gpu_witness", "witness"), ("gpu_lane", "lane"), ("gpu_mutant", "mutant")):
        seq.append({"seq": str(len(seq) + 1), "name": nm, "kind": kind})
    for spec in expand_axis(params["specs"]):
        fam, n = spec.split(":", 1)
        for kind in GPU_KINDS:
            seq.append({"seq": str(len(seq) + 1), "name": f"gpu_{kind}_{fam}_n{n}",
                        "family": fam, "n": n, "kind": kind})
    return seq


def frontier_sequence(params):
    seq = []
    for spec in expand_axis(params["specs"]):
        fam, n = spec.split(":", 1)
        seq.append({"seq": str(len(seq) + 1), "name": f"front_{fam}_n{n}", "family": fam, "n": n})
    return seq


def matrice_passage_points(points, passage):
    """Permutation NOMMEE d'un passage (§ 5.12/5.13) : aller = ordre des
    points, retour = inverse, rotation8 = rotation cyclique fixe de 8."""
    if passage == "aller":
        return list(points)
    if passage == "retour":
        return list(reversed(points))
    if passage == "rotation8":
        if not points:
            return []
        rot = 8 % len(points)
        return list(points[rot:]) + list(points[:rot])
    return None


def matrice_sequence(params, bad):
    seq = []
    points = expand_axis(params["points"])
    if not points:
        return seq
    for pas_no, pas in enumerate(params["sequence"].split(), 1):
        ordered = matrice_passage_points(points, pas)
        if ordered is None:
            bad.append(f"matrice_plan.txt: passage inconnu {pas}")
            return []
        for pos, pt in enumerate(ordered, 1):
            parts = pt.split(":")
            if len(parts) != 6:
                bad.append(f"matrice_plan.txt: point mal forme {pt}")
                return []
            fam, n, t, i, j, d = parts
            seq.append({"seq": str(len(seq) + 1),
                        "name": f"mat_{fam}_n{n}_t{t}_i{i}_j{j}_{d}_p{pas_no}",
                        "family": fam, "n": n, "mat_threads": t, "inflight": i,
                        "join": j, "digest": d, "passage": str(pas_no), "pos": str(pos)})
    return seq


def attrib_sequence(params, bad):
    seq = []
    for pt in expand_axis(params["points"]):
        parts = pt.split(":")
        if len(parts) != 5:
            bad.append(f"attrib_plan.txt: point mal forme {pt}")
            return []
        fam, n, t, i, j = parts
        seq.append({"seq": str(len(seq) + 1), "name": f"attrib_{fam}_n{n}_t{t}_i{i}_j{j}",
                    "family": fam, "n": n, "mat_threads": t, "inflight": i, "join": j})
    return seq


def gpuv6_sequence(params):
    seq = []
    if not expand_axis(params["gate_names"]):
        return seq
    seq.append({"seq": "1", "name": "gpuv6_build", "kind": "build"})
    seq.append({"seq": "2", "name": "gpuv6_gates", "kind": "gates"})
    for spec in expand_axis(params["pilot_specs"]):
        fam, n = spec.split(":", 1)
        seq.append({"seq": str(len(seq) + 1), "name": f"pilote_{fam}_n{n}",
                    "family": fam, "n": n, "kind": "pilote"})
    return seq


def parse_cpu_list(text):
    """`taskset -pc` peut imprimer des plages (0-15) ou des listes (0,2,4) :
    normalisation en ensemble d'entiers ; None si illisible."""
    cpus = set()
    for tok in text.strip().split(","):
        tok = tok.strip()
        m = re.match(r"^(\d+)-(\d+)$", tok)
        if m:
            lo, hi = int(m.group(1)), int(m.group(2))
            if hi < lo:
                return None
            cpus.update(range(lo, hi + 1))
        elif re.match(r"^\d+$", tok):
            cpus.add(int(tok))
        else:
            return None
    return cpus


def recompute_cpu_list(topo_text, want):
    """Miroir Python de cpu_list_for du runner (§ 5.14.4) : lignes lscpu_p
    triees (socket, core, cpu) DANS le cpuset autorise — premier fil de
    chaque coeur, puis fils SMT restants. None si la topologie gravee ne
    permet pas le recalcul (le juge refuse alors, fail-closed)."""
    m = re.search(r"^cpuset_autorise=(\S+)$", topo_text, re.M)
    if not m:
        return None
    allowed = parse_cpu_list(m.group(1))
    if allowed is None:
        return None
    rows, grab = [], False
    for ln in topo_text.splitlines():
        if ln.strip() == "--- lscpu_p ---":
            grab = True
            continue
        if grab:
            mm = re.match(r"^(\d+),(\d+),(\d+)$", ln)
            if not mm:
                break
            rows.append((int(mm.group(3)), int(mm.group(2)), int(mm.group(1))))
    if not rows:
        return None
    rows.sort()
    firsts, smts, seen = [], [], set()
    for sock, core, cpu in rows:
        if cpu not in allowed:
            continue
        if (sock, core) not in seen:
            seen.add((sock, core))
            firsts.append(cpu)
        else:
            smts.append(cpu)
    ordered = firsts + smts
    if len(ordered) < want:
        return None
    return ",".join(str(c) for c in ordered[:want])


def load_pilote_juge():
    """Le juge des records du pilote est LE MEME que la porte stub locale
    (morsehgp3D_v6/tests/pilote_juge.py) — importe, jamais reimplemente
    (§ 5.13.1 : une grammaire causale unique). None si absent (fail-closed
    par l'appelant)."""
    import importlib.util
    path = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                        "morsehgp3D_v6", "tests", "pilote_juge.py")
    if not os.path.exists(path):
        return None
    spec = importlib.util.spec_from_file_location("pilote_juge", path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def check_plan(fname, params, listed, want, bad):
    if params is None:
        return
    if not params["runs"].isdigit() or len(want) != int(params["runs"]):
        bad.append(f"{fname}: runs={params['runs']} != {len(want)} runs recalcules")
    if [(x["seq"], x["name"]) for x in listed] != [(x["seq"], x["name"]) for x in want]:
        bad.append(f"{fname}: sequence annoncee != sequence recalculee")


TIME_BINS = []  # instrumentation observee sur chaque run (huitieme tour)


def check_common(out, name, commit, payload_sha, manifest_sha, threads, bad,
                 want_code="0", code_free=False, forbidden_free=False):
    """Verifications communes a tout run annonce. Retourne le corps .txt (ou
    None), le statut (ou None) et (mur_ms, duree_s, rss_kb). Chaque champ du
    statut est exige EXACTEMENT une fois, et le pic RSS est recoupe avec la
    sortie complete de GNU time (audit GCP v6, P1). code_free=True (phase
    frontiere) : le code est CLASSIFIE par l'appelant au lieu d'etre juge ici,
    et les motifs interdits ne sont pas scannes (le motif de capacite EST la
    donnee) ; RSS et sortie GNU time restent exiges."""
    status = os.path.join(out, name + ".status")
    txt = os.path.join(out, name + ".txt")
    if not os.path.exists(status):
        bad.append(f"{name}: .status ABSENT (run annonce non execute)")
        return None, None, (None, None, None)
    st = read_text(status)
    fields = {}
    for field in ("code", "duree_s", "peak_rss_kb", "threads", "timing_scope", "commande",
                  "time_bin",
                  "source_commit", "source_payload_sha256", "protocol_manifest_sha256", "finished"):
        ms = re.findall(rf"^{field}=(.*)$", st, re.M)
        if len(ms) != 1:
            bad.append(f"{name}: champ {field} absent ou duplique ({len(ms)}) dans le statut")
            fields[field] = None
        else:
            fields[field] = ms[0]
    if fields.get("finished") != "1":
        bad.append(f"{name}: status incomplet")
    if not code_free and fields.get("code") != want_code:
        bad.append(f"{name}: code={fields.get('code') or '?'} (attendu {want_code})")
    kb = fields.get("peak_rss_kb")
    if not kb or not kb.isdigit() or int(kb) <= 0:
        bad.append(f"{name}: pic RSS absent ou nul")
        kb = None
    if fields.get("threads") != threads:
        bad.append(f"{name}: threads={fields.get('threads') or '?'} != {threads} (plan)")
    if fields.get("time_bin") is not None:
        if not fields["time_bin"]:
            bad.append(f"{name}: time_bin VIDE (instrumentation non attestee)")
        TIME_BINS.append(fields["time_bin"])
    for field, want in (("source_commit", commit), ("source_payload_sha256", payload_sha),
                        ("protocol_manifest_sha256", manifest_sha)):
        if fields.get(field) != want:
            bad.append(f"{name}: {field} absent ou different du pin")
    duree = fields.get("duree_s")
    gtime = status + ".time"
    if not os.path.exists(gtime):
        bad.append(f"{name}: sortie complete de GNU time absente")
    else:
        tm = re.search(r"Maximum resident set size[^0-9]*(\d+)", read_text(gtime))
        if not tm:
            bad.append(f"{name}: pic RSS absent de la sortie GNU time")
        elif kb is not None and tm.group(1) != kb:
            bad.append(f"{name}: peak_rss_kb={kb} != GNU time ({tm.group(1)}) — statut non recoupe")
    if not os.path.exists(txt):
        bad.append(f"{name}: .txt ABSENT")
        return None, st, (None, None, None)
    body = read_text(txt)
    if not code_free and not forbidden_free:
        # forbidden_free (§ 5.14.4) : le transcript CTest agrege n'est PAS
        # une surface produit — la porte negative mhgp6_pilote_refus_n y
        # ecrit legitimement REFUS tout en etant Passed.
        fb = FORBIDDEN.search(body)
        if fb:
            bad.append(f"{name}: motif interdit ({fb.group(0)})")
    mur = re.search(r"^temps_mur_ms=([0-9.]+)", body, re.M)
    return body, st, (float(mur.group(1)) if mur else None,
                      int(duree) if duree and duree.isdigit() else None,
                      int(kb) if kb else None)


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
    for pat in (r"^famille=.*$", r"^generation .*$", r"^sweep .*$", r"^vwspd .*$", r"^octaves_q4 .*$",
                r"^octaves_q4_seeds .*$", r"^vcensus .*$", r"^p_factor=.*$", r"^ledger_paires .*$",
                r"^ouvriers .*$", r"^cardinalites K=\d+ .*$"):
        sig.extend(re.findall(pat, body, re.M))
    return tuple(sig)


# Ce que la phase FILS juge est l'INVARIANCE DU GRAND-LIVRE entre fils —
# compteurs, generation, cardinalites —, PAS la bit-identite de l'objet (les
# runs de fils n'impriment pas de digest ; deux forets differentes aux memes
# comptes passeraient). La bit-identite de l'objet reste prouvee par les
# portes a digest (conformite, contrats GPU) — cinquieme tour.
SWEEP_INVARIANT_SINGLE = (r"^generation .*$", r"^sweep .*$", r"^vwspd .*$", r"^octaves_q4 .*$",
                          r"^octaves_q4_seeds .*$", r"^vcensus .*$", r"^p_factor=.*$",
                          r"^ledger_paires .*$")


def thread_invariant_signature(body):
    """Lignes du grand-livre INVARIANTES PAR FILS — jamais l'identite (elle
    imprime threads=) ni les lignes ouvriers (parallelisme MESURE, par
    definition dependant des fils)."""
    sig = []
    for pat in SWEEP_INVARIANT_SINGLE + (r"^cardinalites K=\d+ .*$",):
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


def sha256_file(path):
    h = hashlib.sha256()
    with open(path, "rb") as fh:
        for chunk in iter(lambda: fh.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def main():
    if len(sys.argv) != 10:
        print("usage: validate_v6_campaign.py OUT COMMIT PAYLOAD MANIFEST REMOTE_RC SCP_RC PROFIL CANONIQUE MANIFESTE_REVALIDE",
              file=sys.stderr)
        return 2
    (out, commit, payload_sha, manifest_sha, remote_rc, scp_rc, profile_path,
     canonical_path, revalidated_manifest_path) = sys.argv[1:10]
    bad = []
    if remote_rc != "0":
        bad.append(f"session distante : remote_campaign_rc={remote_rc}")
    if scp_rc != "0":
        bad.append(f"rapatriement : scp_rc={scp_rc}")
    # PROFIL EPINGLE : la matrice attendue vient de lui, jamais du runner.
    profile = {}
    canon_axes = {}
    if not os.path.exists(profile_path):
        bad.append("profil de campagne ABSENT (matrice non epinglee)")
    else:
        for line in read_text(profile_path).splitlines():
            if "=" in line:
                k, v = line.split("=", 1)
                profile[k] = v
        for key in ("profil", "profil_canonique", "profil_canonique_sha256", "conf_specs", "bench_specs",
                    "queue_families", "queue_n", "queue_seeds", "threads",
                    "sweep_specs", "sweep_repeats", "gpu_specs", "frontier_specs", "frontier_timeout",
                    "gpu_build_timeout", "frontier_ulimit_kb"):
            if not profile.get(key, "").strip():
                bad.append(f"profil de campagne : cle {key} absente")
        for key in ("run_timeout", "v5_gate_min", "v6_gate_min"):
            if not profile.get(key, "").strip():
                bad.append(f"profil de campagne : cle {key} absente")
        # AXES SERIE C (§ 5.12/5.13) : toujours emis par le cycle de vie —
        # leur absence signale un profil de campagne fabrique autrement.
        for key in ("matrice_points", "matrice_sequence", "matrice_timeout",
                    "attrib_points", "attrib_timeout",
                    "gpuv6_gate_names", "gpuv6_build_timeout", "gpuv6_gate_timeout",
                    "gpuv6_pilot_specs", "gpuv6_pilot_min_lots", "gpuv6_pilot_timeout",
                    "session_max_run_seconds", "session_invite_minutes",
                    "max_run_seconds_effectif", "guest_shutdown_minutes_effectif"):
            if not profile.get(key, "").strip():
                bad.append(f"profil de campagne : cle {key} absente")
        # § 5.13.4 : les axes de duree du profil pilotent les VRAIS
        # coupe-circuits — un ecart entre l'axe et l'effectif signifie une
        # surcharge d'environnement (profil effectif != canonique).
        if profile.get("profil") == profile.get("profil_canonique"):
            if profile.get("max_run_seconds_effectif") != profile.get("session_max_run_seconds"):
                bad.append("profil de campagne : max_run_seconds_effectif != session_max_run_seconds "
                           "alors que le profil se dit canonique")
            if profile.get("guest_shutdown_minutes_effectif") != profile.get("session_invite_minutes"):
                bad.append("profil de campagne : guest_shutdown_minutes_effectif != session_invite_minutes "
                           "alors que le profil se dit canonique")
        # LIAISON au fichier canonique versionne (jamais un simple transport) :
        # le canon est PARSE (grammaire fermee, chaque axe exactement une
        # fois) et chaque axe du profil effectif lui est compare LITTERALEMENT
        # — l'audit du quatrieme tour a montre qu'un hash transporte sans
        # comparaison d'axes laissait passer une matrice reduite nommee
        # decision_v1.
        if not os.path.exists(canonical_path):
            bad.append("profil canonique ABSENT (liaison impossible)")
        else:
            if profile.get("profil_canonique_sha256", "") != sha256_file(canonical_path):
                bad.append("profil_canonique_sha256 != sha256 du fichier canonique fourni")
            axis_names = ("PROFIL_NOM", "CONF_SPECS", "BENCH_SPECS", "QUEUE_FAMILIES", "QUEUE_N",
                          "QUEUE_SEEDS", "RUN_TIMEOUT", "THREADS_VM", "V5_GATE_MIN", "V6_GATE_MIN",
                          "SWEEP_SPECS", "SWEEP_REPEATS", "GPU_SPECS", "FRONTIER_SPECS",
                          "FRONTIER_TIMEOUT", "GPU_BUILD_TIMEOUT", "FRONTIER_ULIMIT_KB")
            # Axes serie C OPTIONNELS dans le canon (les profils anterieurs
            # ne les declarent pas ; le cycle de vie leur donne des valeurs
            # par defaut « aucun » qui sautent les phases).
            optional_axes = ("MATRICE_POINTS", "MATRICE_SEQUENCE", "MATRICE_TIMEOUT",
                             "ATTRIB_POINTS", "ATTRIB_TIMEOUT",
                             "GPUV6_GATE_NAMES", "GPUV6_BUILD_TIMEOUT", "GPUV6_GATE_TIMEOUT",
                             "GPUV6_PILOT_SPECS", "GPUV6_PILOT_MIN_LOTS", "GPUV6_PILOT_TIMEOUT",
                             "SESSION_MAX_RUN_SECONDS", "SESSION_INVITE_MINUTES")
            # Grammaire TOTALE : commentaire, ligne vide, ou axe connu a
            # guillemets equilibres — TOUT le reste est refuse.
            axis_line = re.compile(r'^([A-Z0-9_]+)=(?:"([^"]*)"|([^"\s]*))$')
            for line in read_text(canonical_path).splitlines():
                if not line.strip() or line.lstrip().startswith("#"):
                    continue
                m = axis_line.match(line)
                if not m:
                    bad.append(f"profil canonique : ligne hors grammaire ({line[:60]})")
                    continue
                name = m.group(1)
                value = m.group(2) if m.group(2) is not None else m.group(3)
                if name not in axis_names and name not in optional_axes:
                    bad.append(f"profil canonique : axe inconnu {name}")
                    continue
                if name in canon_axes:
                    bad.append(f"profil canonique : axe {name} duplique")
                canon_axes[name] = value
            for axis in axis_names:
                if axis not in canon_axes:
                    bad.append(f"profil canonique : axe {axis} absent")
            # IDENTITE : le profil effectif doit designer le canon par SON nom
            # (cinquieme tour : jamais compare pour les non decisionnels).
            if canon_axes.get("PROFIL_NOM") and profile.get("profil_canonique") != canon_axes.get("PROFIL_NOM"):
                bad.append(f"profil_canonique={profile.get('profil_canonique', '?')} != PROFIL_NOM du canon "
                           f"({canon_axes.get('PROFIL_NOM')})")
            # LIAISON LITTERALE (§ 5.15.1) : DES QUE le profil se dit
            # canonique (profil == profil_canonique, quel que soit le nom),
            # TOUS les axes communs sont compares — axes historiques, treize
            # axes serie C et deux durees de session. Seul un axe optionnel
            # ABSENT d'un ancien canon est ignore. matrice_timeout 60 -> 61
            # sous un nom canonique doit etre refuse ici, pas seulement dans
            # la promotion decision_v1.
            if profile.get("profil") and profile.get("profil") == profile.get("profil_canonique"):
                full_axis_map = (
                    ("conf_specs", "CONF_SPECS"), ("bench_specs", "BENCH_SPECS"),
                    ("queue_families", "QUEUE_FAMILIES"), ("queue_n", "QUEUE_N"),
                    ("queue_seeds", "QUEUE_SEEDS"), ("run_timeout", "RUN_TIMEOUT"),
                    ("threads", "THREADS_VM"), ("v5_gate_min", "V5_GATE_MIN"),
                    ("v6_gate_min", "V6_GATE_MIN"),
                    ("sweep_specs", "SWEEP_SPECS"), ("sweep_repeats", "SWEEP_REPEATS"),
                    ("gpu_specs", "GPU_SPECS"), ("frontier_specs", "FRONTIER_SPECS"),
                    ("frontier_timeout", "FRONTIER_TIMEOUT"),
                    ("gpu_build_timeout", "GPU_BUILD_TIMEOUT"),
                    ("frontier_ulimit_kb", "FRONTIER_ULIMIT_KB"),
                    ("matrice_points", "MATRICE_POINTS"), ("matrice_sequence", "MATRICE_SEQUENCE"),
                    ("matrice_timeout", "MATRICE_TIMEOUT"),
                    ("attrib_points", "ATTRIB_POINTS"), ("attrib_timeout", "ATTRIB_TIMEOUT"),
                    ("gpuv6_gate_names", "GPUV6_GATE_NAMES"),
                    ("gpuv6_build_timeout", "GPUV6_BUILD_TIMEOUT"),
                    ("gpuv6_gate_timeout", "GPUV6_GATE_TIMEOUT"),
                    ("gpuv6_pilot_specs", "GPUV6_PILOT_SPECS"),
                    ("gpuv6_pilot_min_lots", "GPUV6_PILOT_MIN_LOTS"),
                    ("gpuv6_pilot_timeout", "GPUV6_PILOT_TIMEOUT"),
                    ("session_max_run_seconds", "SESSION_MAX_RUN_SECONDS"),
                    ("session_invite_minutes", "SESSION_INVITE_MINUTES"))
                for pk, ck in full_axis_map:
                    if ck not in canon_axes:
                        continue  # axe optionnel absent d'un ancien canon
                    if profile.get(pk, "").split() != canon_axes[ck].split():
                        bad.append(f"profil canonique : axe {pk} != {ck} du canon "
                                   f"({profile.get(pk, '?')!r} vs {canon_axes[ck]!r})")
    # LIAISON AU MANIFESTE REVALIDE (cinquieme tour) : le canon fourni doit
    # etre EXACTEMENT le fichier epingle du commit — chemin, hash et taille
    # dans le manifeste dont le sha256 EST le pin grave dans chaque statut.
    if not os.path.exists(revalidated_manifest_path):
        bad.append("manifeste revalide ABSENT (liaison canon impossible)")
    else:
        if sha256_file(revalidated_manifest_path) != manifest_sha:
            bad.append("manifeste revalide : sha256 != protocol_manifest_sha256 (pin)")
        mlines = read_text(revalidated_manifest_path).splitlines()
        entries = {}
        if not mlines or mlines[0] != "schema=e-hgp.protocol-manifest.v1":
            bad.append("manifeste revalide : schema absent ou inconnu")
        if len(mlines) < 2 or mlines[1] != f"commit={commit}":
            bad.append("manifeste revalide : commit != pin")
        for ln in mlines[2:]:
            m = re.match(r"^([0-9a-f]{64})\t(\d+)\t(\S+)$", ln)
            if not m:
                bad.append(f"manifeste revalide : ligne hors grammaire ({ln[:60]})")
                continue
            if m.group(3) in entries:
                bad.append(f"manifeste revalide : chemin duplique {m.group(3)}")
            entries[m.group(3)] = (m.group(1), m.group(2))
        canon_name = canon_axes.get("PROFIL_NOM", "")
        canon_rel = f"gcp-migration/profils/{canon_name}.env"
        if canon_name and os.path.exists(canonical_path):
            ent = entries.get(canon_rel)
            if not ent or ent[0] != sha256_file(canonical_path) \
               or ent[1] != str(os.path.getsize(canonical_path)):
                bad.append(f"profil canonique NON LIE au manifeste revalide ({canon_rel} : chemin, hash ou taille)")
    for tf in ("conf_tronquee.txt", "bench_tronquee.txt", "queue_tronquee.txt",
               "sweep_tronquee.txt", "gpu_tronquee.txt", "frontier_tronquee.txt",
               "matrice_tronquee.txt", "attrib_tronquee.txt", "gpuv6_tronquee.txt"):
        p = os.path.join(out, tf)
        if os.path.exists(p):
            bad.append(f"{tf}: campagne TRONQUEE ({read_text(p).strip().replace(chr(10), ' ; ')})")
    topo = os.path.join(out, "topologie.txt")
    topo_body = ""
    if not os.path.exists(topo):
        bad.append("topologie.txt: ABSENT")
    else:
        topo_body = read_text(topo)
        if not re.search(r"^nproc=\d+$", topo_body, re.M) or "--- lscpu ---" not in topo_body:
            bad.append("topologie.txt: nproc ou lscpu absent")

    conf_params, conf_listed = read_plan(out, "conf_plan.txt", "conf_plan",
                                         ("specs", "threads"), bad, version="v2")
    bench_params, bench_listed = read_plan(out, "bench_plan.txt", "bench_plan",
                                           ("specs", "threads"), bad, version="v2")
    queue_params, queue_listed = read_plan(out, "queue_plan.txt", "queue_plan",
                                           ("families", "n_list", "seeds", "threads"), bad)
    sweep_params, sweep_listed = read_plan(out, "sweep_plan.txt", "sweep_plan",
                                           ("specs", "repeats"), bad)
    gpu_params, gpu_listed = read_plan(out, "gpu_plan.txt", "gpu_plan",
                                       ("specs", "threads", "build_timeout"), bad)
    frontier_params, frontier_listed = read_plan(out, "frontier_plan.txt", "frontier_plan",
                                                 ("specs", "threads", "timeout", "ulimit_kb"), bad)
    matrice_params, matrice_listed = read_plan(out, "matrice_plan.txt", "matrice_plan",
                                               ("points", "sequence"), bad)
    attrib_params, attrib_listed = read_plan(out, "attrib_plan.txt", "attrib_plan",
                                             ("points",), bad)
    gpuv6_params, gpuv6_listed = read_plan(out, "gpuv6_plan.txt", "gpuv6_plan",
                                           ("gate_names", "pilot_specs", "min_lots"), bad)
    conf_runs = conf_sequence(conf_params) if conf_params else []
    bench_runs = bench_sequence(bench_params) if bench_params else []
    queue_runs = queue_sequence(queue_params) if queue_params else []
    sweep_runs = sweep_sequence(sweep_params) if sweep_params else []
    gpu_runs = gpu_sequence(gpu_params) if gpu_params else []
    frontier_runs = frontier_sequence(frontier_params) if frontier_params else []
    matrice_runs = matrice_sequence(matrice_params, bad) if matrice_params else []
    attrib_runs = attrib_sequence(attrib_params, bad) if attrib_params else []
    gpuv6_runs = gpuv6_sequence(gpuv6_params) if gpuv6_params else []
    check_plan("conf_plan.txt", conf_params, conf_listed, conf_runs, bad)
    check_plan("bench_plan.txt", bench_params, bench_listed, bench_runs, bad)
    check_plan("queue_plan.txt", queue_params, queue_listed, queue_runs, bad)
    check_plan("sweep_plan.txt", sweep_params, sweep_listed, sweep_runs, bad)
    check_plan("gpu_plan.txt", gpu_params, gpu_listed, gpu_runs, bad)
    check_plan("frontier_plan.txt", frontier_params, frontier_listed, frontier_runs, bad)
    check_plan("matrice_plan.txt", matrice_params, matrice_listed, matrice_runs, bad)
    check_plan("attrib_plan.txt", attrib_params, attrib_listed, attrib_runs, bad)
    check_plan("gpuv6_plan.txt", gpuv6_params, gpuv6_listed, gpuv6_runs, bad)
    # Les plans annonces doivent EGALER le profil epingle (matrice fixee
    # independamment des sorties jugees).
    if profile:
        for plan, params, checks in (
                ("conf_plan.txt", conf_params, (("specs", "conf_specs"), ("threads", "threads"))),
                ("bench_plan.txt", bench_params, (("specs", "bench_specs"), ("threads", "threads"))),
                ("queue_plan.txt", queue_params, (("families", "queue_families"), ("n_list", "queue_n"),
                                                  ("seeds", "queue_seeds"), ("threads", "threads"))),
                ("sweep_plan.txt", sweep_params, (("specs", "sweep_specs"), ("repeats", "sweep_repeats"))),
                ("gpu_plan.txt", gpu_params, (("specs", "gpu_specs"), ("threads", "threads"),
                                              ("build_timeout", "gpu_build_timeout"))),
                ("frontier_plan.txt", frontier_params, (("specs", "frontier_specs"), ("threads", "threads"),
                                                        ("timeout", "frontier_timeout"),
                                                        ("ulimit_kb", "frontier_ulimit_kb"))),
                ("matrice_plan.txt", matrice_params, (("points", "matrice_points"),
                                                      ("sequence", "matrice_sequence"))),
                ("attrib_plan.txt", attrib_params, (("points", "attrib_points"),)),
                ("gpuv6_plan.txt", gpuv6_params, (("gate_names", "gpuv6_gate_names"),
                                                  ("pilot_specs", "gpuv6_pilot_specs"),
                                                  ("min_lots", "gpuv6_pilot_min_lots")))):
            if params is None:
                continue
            for plan_key, prof_key in checks:
                if params.get(plan_key, "").split() != profile.get(prof_key, "").split():
                    bad.append(f"{plan}: {plan_key} != profil epingle ({prof_key})")

    # PHASE 1 — conformite (paires fam:n du profil, tailles mesurees incluses).
    conf_threads = conf_params["threads"] if conf_params else "0"
    v5ref_bodies = {}
    for run in conf_runs:
        name = run["name"]
        body, st, _ = check_common(out, name, commit, payload_sha, manifest_sha, conf_threads, bad)
        if body is None:
            continue
        if run["kind"] == "v5ref":
            check_pipeline_run(name, body, run["family"], run["n"], "3", "v5", conf_threads, bad)
            digests = re.findall(r"^digest_all=([0-9a-f]{64})$", body, re.M)
            if len(digests) != 1:
                bad.append(f"{name}: digest_all present {len(digests)} fois (attendu 1)")
            cmd = re.search(r"^commande=(.*)$", st, re.M)
            if not cmd or "--digest" not in cmd.group(1).split():
                bad.append(f"{name}: commande gravee sans --digest (la reference exige les digests)")
            v5ref_bodies[(run["family"], run["n"])] = body
        else:
            if "conformite v5=v6" not in body or "identiques (objet)" not in body:
                bad.append(f"{name}: conformite v5=v6 non etablie (`identiques (objet)` absent)")
            cmd = re.search(r"^commande=(.*)$", st, re.M)
            if not cmd or f"v5ref_{run['family']}_n{run['n']}.txt" not in cmd.group(1):
                bad.append(f"{name}: commande gravee sans la reference v5 de la meme paire")
            if (run["family"], run["n"]) not in v5ref_bodies:
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
            ms = re.findall(pat, body)
            if len(ms) != 1:
                bad.append(f"{name}: compteur {cname} absent ou duplique ({len(ms)} occurrences)")
            else:
                vals[cname] = ms[0]
        for field, want in (("family", run["family"]), ("n", run["n"]),
                            ("seed", run["seed"]), ("seq", run["seq"])):
            fm = re.search(rf"^{field}=(\S+)$", st, re.M)
            if not fm or fm.group(1) != want:
                bad.append(f"{name}: {field}={fm.group(1) if fm else '?'} != {want} (annonce)")
        queue_rows.append((run, vals, meas))


    # PHASE FILS — moteur v6, bit-identite entre fils jugee a l'echelle.
    sweep_signatures = {}   # (family, n) -> {signature: [names]}
    sweep_measures = {}     # (family, n, threads) -> [(mur, duree, rss)]
    for run in sweep_runs:
        name = run["name"]
        body, st, meas = check_common(out, name, commit, payload_sha, manifest_sha,
                                      run["sweep_threads"], bad)
        if body is None:
            continue
        check_pipeline_run(name, body, run["family"], run["n"], "3", "v6",
                           run["sweep_threads"], bad)
        if ANY_DIGEST.search(body):
            bad.append(f"{name}: digest imprime sur un run de fils (mur non compare a armes egales)")
        cmd = re.search(r"^commande=(.*)$", st, re.M)
        if cmd:
            argv = cmd.group(1).split()
            if "--digest" in argv:
                bad.append(f"{name}: --digest dans la commande gravee d'un run de fils")
            want_args = {f"--family={run['family']}", f"--n={run['n']}", "--s=8", "--smax=11",
                         "--seed=3", f"--threads={run['sweep_threads']}"}
            if not want_args.issubset(argv):
                bad.append(f"{name}: commande gravee sans les arguments contractuels du nom")
        else:
            bad.append(f"{name}: commande absente")
        for field, want in (("family", run["family"]), ("n", run["n"]),
                            ("sweep_threads", run["sweep_threads"]), ("repeat", run["repeat"]),
                            ("pos", run["pos"]), ("seq", run["seq"])):
            fm = re.search(rf"^{field}=(\S+)$", st, re.M)
            if not fm or fm.group(1) != want:
                bad.append(f"{name}: {field}={fm.group(1) if fm else '?'} != {want} (annonce)")
        # NON-VACUITE (cinquieme tour) : chaque ligne invariante annoncee doit
        # etre presente EXACTEMENT une fois — une signature vide n'egale pas
        # une signature vide.
        for pat in SWEEP_INVARIANT_SINGLE:
            cnt = len(re.findall(pat, body, re.M))
            if cnt != 1:
                bad.append(f"{name}: ligne invariante {pat} presente {cnt} fois (attendu 1)")
        key = (run["family"], run["n"])
        sweep_signatures.setdefault(key, {}).setdefault(thread_invariant_signature(body), []).append(name)
        sweep_measures.setdefault((run["family"], run["n"], run["sweep_threads"]), []).append(meas)
    for key, table in sorted(sweep_signatures.items()):
        if len(table) > 1:
            groups = sorted(table.values(), key=len, reverse=True)
            bad.append(f"fils {key}: INVARIANCE DU GRAND-LIVRE VIOLEE entre fils/repetitions "
                       f"(dissidents {', '.join(groups[1])})")

    # PHASE MATRICE (§ 5.12/5.13) — contrastes CPU pre-enregistres au binaire
    # v6 NON instrumente : affinite taskset DERIVEE et ATTESTEE (jamais
    # l'affinite inchangee du shell), bras digest exact, bit-identite de
    # l'objet (digest_all identique entre tous les points --digest d'une meme
    # paire famille/n) et invariance du grand-livre entre fils/inflight/join.
    matrice_digests = {}     # (family, n) -> {digest_all: [names]}
    matrice_signatures = {}  # (family, n) -> {signature: [names]}
    matrice_rows = []
    matrice_bin_shas, attrib_bin_shas, pilote_bin_shas = set(), set(), set()
    for run in matrice_runs:
        name = run["name"]
        body, st, meas = check_common(out, name, commit, payload_sha, manifest_sha,
                                      run["mat_threads"], bad)
        if body is None:
            continue
        check_pipeline_run(name, body, run["family"], run["n"], "3", "v6",
                           run["mat_threads"], bad)
        for field, want in (("family", run["family"]), ("n", run["n"]),
                            ("mat_threads", run["mat_threads"]), ("inflight", run["inflight"]),
                            ("join", run["join"]), ("digest", run["digest"]),
                            ("passage", run["passage"]), ("pos", run["pos"]), ("seq", run["seq"])):
            fm = re.search(rf"^{field}=(\S+)$", st, re.M)
            if not fm or fm.group(1) != want:
                bad.append(f"{name}: {field}={fm.group(1) if fm else '?'} != {want} (annonce)")
        # AFFINITE (§ 5.13.4) : liste demandee gravee, attestation effective
        # d'un shell confine EGALE a la demande (plages normalisees), et
        # cardinal == fils du point.
        dem = re.search(r"^affinite_demandee=(\S+)$", st, re.M)
        eff = re.search(r"^affinite_effective=(\S+)$", st, re.M)
        if not dem or not eff:
            bad.append(f"{name}: affinite_demandee/affinite_effective absentes du statut")
        else:
            sdem, seff = parse_cpu_list(dem.group(1)), parse_cpu_list(eff.group(1))
            if sdem is None or seff is None or sdem != seff:
                bad.append(f"{name}: affinite effective ({eff.group(1)}) != demandee ({dem.group(1)})")
            elif len(sdem) != int(run["mat_threads"]):
                bad.append(f"{name}: affinite de {len(sdem)} CPU != fils={run['mat_threads']}")
            # § 5.14.4 : le masque est RECALCULE ici depuis la topologie
            # gravee ((socket, core) dans le cpuset autorise) — jamais cru.
            want_mask = recompute_cpu_list(topo_body, int(run["mat_threads"]))
            if want_mask is None:
                bad.append(f"{name}: topologie gravee insuffisante pour recalculer le masque")
            elif dem.group(1) != want_mask:
                bad.append(f"{name}: affinite demandee ({dem.group(1)}) != masque recalcule ({want_mask})")
        bs = re.findall(r"^binaire_sha256=([0-9a-f]{64})$", st, re.M)
        if len(bs) != 1:
            bad.append(f"{name}: binaire_sha256 absent ou duplique")
        else:
            matrice_bin_shas.add(bs[0])
        cmd = re.search(r"^commande=(.*)$", st, re.M)
        argv = cmd.group(1).split() if cmd else []
        if len(argv) < 4 or argv[0] != "taskset" or argv[1] != "-c" \
           or (dem and argv[2] != dem.group(1)):
            bad.append(f"{name}: commande gravee sans confinement taskset -c <liste demandee>")
        elif os.path.basename(argv[3]) != "mhgp6":
            bad.append(f"{name}: binaire de matrice inattendu ({argv[3]}) — mhgp6 exige")
        want_args = {f"--family={run['family']}", f"--n={run['n']}", "--s=8", "--smax=11",
                     "--seed=3", f"--threads={run['mat_threads']}",
                     f"--fold-inflight={run['inflight']}", f"--fold-join={run['join']}"}
        if not want_args.issubset(argv):
            bad.append(f"{name}: commande gravee sans les arguments contractuels du point")
        if run["digest"] == "avec":
            if "--digest" not in argv:
                bad.append(f"{name}: bras avec-digest sans --digest dans la commande gravee")
            digests = re.findall(r"^digest_all=([0-9a-f]{64})$", body, re.M)
            if len(digests) != 1:
                bad.append(f"{name}: digest_all present {len(digests)} fois (attendu 1)")
            else:
                matrice_digests.setdefault((run["family"], run["n"]), {}) \
                    .setdefault(digests[0], []).append(name)
        else:
            if "--digest" in argv:
                bad.append(f"{name}: --digest dans la commande gravee d'un bras sans-digest")
            if ANY_DIGEST.search(body):
                bad.append(f"{name}: digest imprime sur un bras sans-digest (mur contamine)")
        for pat in SWEEP_INVARIANT_SINGLE:
            cnt = len(re.findall(pat, body, re.M))
            if cnt != 1:
                bad.append(f"{name}: ligne invariante {pat} presente {cnt} fois (attendu 1)")
        matrice_signatures.setdefault((run["family"], run["n"]), {}) \
            .setdefault(thread_invariant_signature(body), []).append(name)
        matrice_rows.append((run, meas))
    for key, table in sorted(matrice_digests.items()):
        if len(table) > 1:
            groups = sorted(table.values(), key=len, reverse=True)
            bad.append(f"matrice {key}: digest_all NON identique entre bras --digest "
                       f"(dissidents {', '.join(groups[1])})")
    for key, table in sorted(matrice_signatures.items()):
        if len(table) > 1:
            groups = sorted(table.values(), key=len, reverse=True)
            bad.append(f"matrice {key}: INVARIANCE DU GRAND-LIVRE VIOLEE entre points "
                       f"(dissidents {', '.join(groups[1])})")

    # PHASE ATTRIBUTION (§ 5.12) — mhgp6_profile : attribution seulement,
    # jamais un mur. L'autorite de grammaire complete est la porte
    # tests/profil_gate.py ; ici, les invariants transportables : signature
    # du build, fold_join signe, records par K, somme RECALCULEE aux seuils
    # serres du § 5.13 (0.0051 / 0.006).
    attrib_rows = []
    for run in attrib_runs:
        name = run["name"]
        body, st, meas = check_common(out, name, commit, payload_sha, manifest_sha,
                                      run["mat_threads"], bad)
        if body is None:
            continue
        check_pipeline_run(name, body, run["family"], run["n"], "3", "v6",
                           run["mat_threads"], bad)
        for field, want in (("family", run["family"]), ("n", run["n"]),
                            ("mat_threads", run["mat_threads"]), ("inflight", run["inflight"]),
                            ("join", run["join"]), ("seq", run["seq"]),
                            ("authority", "attribution_seulement")):
            fm = re.search(rf"^{field}=(\S+)$", st, re.M)
            if not fm or fm.group(1) != want:
                bad.append(f"{name}: {field}={fm.group(1) if fm else '?'} != {want} (annonce)")
        dem = re.search(r"^affinite_demandee=(\S+)$", st, re.M)
        eff = re.search(r"^affinite_effective=(\S+)$", st, re.M)
        if not dem or not eff:
            bad.append(f"{name}: affinite_demandee/affinite_effective absentes du statut")
        else:
            sdem, seff = parse_cpu_list(dem.group(1)), parse_cpu_list(eff.group(1))
            if sdem is None or seff is None or sdem != seff or len(sdem) != int(run["mat_threads"]):
                bad.append(f"{name}: affinite non attestee ou incoherente avec fils")
            want_mask = recompute_cpu_list(topo_body, int(run["mat_threads"]))
            if want_mask is None:
                bad.append(f"{name}: topologie gravee insuffisante pour recalculer le masque")
            elif dem.group(1) != want_mask:
                bad.append(f"{name}: affinite demandee ({dem.group(1)}) != masque recalcule ({want_mask})")
        bs = re.findall(r"^binaire_sha256=([0-9a-f]{64})$", st, re.M)
        if len(bs) != 1:
            bad.append(f"{name}: binaire_sha256 absent ou duplique")
        else:
            attrib_bin_shas.add(bs[0])
        # § 5.15.1 : la commande d'attribution est verifiee comme celle de
        # la matrice — confinement taskset, binaire PROFILE, arguments du
        # point ; retirer taskset -c ne passe plus malgre les attestations.
        cmd = re.search(r"^commande=(.*)$", st, re.M)
        argv = cmd.group(1).split() if cmd else []
        if len(argv) < 4 or argv[0] != "taskset" or argv[1] != "-c" \
           or (dem and argv[2] != dem.group(1)):
            bad.append(f"{name}: commande gravee sans confinement taskset -c <liste demandee>")
        elif os.path.basename(argv[3]) != "mhgp6_profile":
            bad.append(f"{name}: binaire d'attribution inattendu ({argv[3]}) — mhgp6_profile exige")
        want_args = {f"--family={run['family']}", f"--n={run['n']}", "--s=8", "--smax=11",
                     "--seed=3", f"--threads={run['mat_threads']}",
                     f"--fold-inflight={run['inflight']}", f"--fold-join={run['join']}"}
        if not want_args.issubset(argv):
            bad.append(f"{name}: commande gravee sans les arguments contractuels du point")
        kind = re.findall(r"^profil_kind=reduce_v2 fold_join=(\d) ", body, re.M)
        if len(kind) != 1 or kind[0] != run["join"]:
            bad.append(f"{name}: profil_kind=reduce_v2 fold_join={run['join']} absent ou multiple")
        reduce_rows = re.findall(r"^profil_reduce K=(\d+) (.*)$", body, re.M)
        intern_rows = re.findall(r"^profil_intern K=(\d+) .*$", body, re.M)
        # § 5.14.4 : ENSEMBLE EXACT des K (jamais un plancher) et finitude
        # de TOUS les champs — smax=11 => K1..10, aucun trou, aucun double.
        want_ks = [str(k) for k in range(1, KMAX + 1)]
        if [k for k, _ in reduce_rows] != want_ks or intern_rows != want_ks:
            bad.append(f"{name}: ensembles de K hors contrat "
                       f"(reduce={[k for k, _ in reduce_rows]}, intern={intern_rows}, attendu K1..{KMAX})")
        for k, row in reduce_rows:
            f = dict((kv.split("=", 1)[0], kv.split("=", 1)[1]) for kv in row.split())
            try:
                vals = {kk: float(vv) for kk, vv in f.items()}
                if any(not math.isfinite(v) for v in vals.values()):
                    bad.append(f"{name}: champ non fini dans profil_reduce K={k}")
                comp = sum(vals[kk] for kk in ("init", "touch", "pre", "unite",
                                               "post_remplissage", "materialisation_tri_copie",
                                               "liveness", "partition", "liberation"))
                if abs(comp - vals["somme"]) > 0.0051:
                    bad.append(f"{name}: somme imprimee != somme des neuf composantes (K={k})")
                if abs(comp + vals["residuel"] - vals["mur_reduce_interne"]) > 0.006:
                    bad.append(f"{name}: fermeture somme+residuel != mur_reduce_interne (K={k})")
            except (KeyError, ValueError):
                bad.append(f"{name}: record profil_reduce incomplet ou non numerique (K={k})")
        attrib_rows.append((run, meas))

    # PHASE GPUV6 (§ 5.12) — la serie C : build CUDA signe, inventaire EXACT
    # des portes (chaque nom Passed ET total == inventaire, jamais un
    # plancher), puis pilote juge par LE MEME juge que la porte stub
    # (tests/pilote_juge.py importe — fail-closed s'il est absent).
    pilote_bodies = {}
    if gpuv6_runs:
        juge_mod = load_pilote_juge()
        plan_min_lots = int(gpuv6_params["min_lots"]) if gpuv6_params["min_lots"].isdigit() else 0
        gate_names = expand_axis(gpuv6_params["gate_names"])
        build_ident = {}  # § 5.15.2 : nom / UUID / CC du build, lies aux pilotes
        # INVENTAIRE PRE-EXECUTION (§ 5.14.4) : le runner liste (ctest -N)
        # AVANT de courir ; le fichier grave doit porter EXACTEMENT les noms
        # du plan — un 17e test decouvert apres coup a deja depense.
        inv_path = os.path.join(out, "gpuv6_inventaire.txt")
        if not os.path.exists(inv_path):
            bad.append("gpuv6_inventaire.txt: ABSENT (inventaire pre-execution non grave)")
        else:
            inv_names = sorted(re.findall(r"Test +#\d+: ([A-Za-z0-9_]+)", read_text(inv_path)))
            if inv_names != sorted(gate_names):
                bad.append(f"gpuv6_inventaire.txt: noms != plan ({len(inv_names)} vs {len(gate_names)})")
        for run in gpuv6_runs:
            name = run["name"]
            # § 5.14.4 : le transcript CTest agrege n'est pas une surface
            # produit (mhgp6_pilote_refus_n y ecrit REFUS en etant Passed).
            body, st, meas = check_common(out, name, commit, payload_sha, manifest_sha,
                                          profile.get("threads", "0"), bad,
                                          forbidden_free=(run["kind"] == "gates"))
            if body is None:
                continue
            if run["kind"] == "build":
                if not re.search(r"^nvcc=\S+$", body, re.M):
                    bad.append(f"{name}: provenance nvcc absente")
                mident = re.search(r"^(NVIDIA [^,]*), (GPU-[0-9a-f-]+), (\d+\.\d+), \d+", body, re.M)
                if not mident:
                    bad.append(f"{name}: identite device (nom, UUID, CC, driver) absente")
                else:
                    build_ident = {"name": mident.group(1), "uuid": mident.group(2),
                                   "cc": mident.group(3)}
            elif run["kind"] == "gates":
                # Les DEUX libelles de resume CTest sont admis (<= 4.3 avec
                # « , 0 tests failed », 4.4+ sans) — meme doctrine que le
                # lifecycle (7e346926) : 100% exige, total == inventaire.
                total = re.search(r"100% tests passed(?:, 0 tests failed)? out of (\d+)", body)
                if not total or total.group(1) != str(len(gate_names)):
                    bad.append(f"{name}: resume ctest != 100% sur {len(gate_names)} portes "
                               f"({total.group(0) if total else 'resume ctest absent'})")
                for nm in gate_names:
                    if not re.search(rf"Test +#\d+: {re.escape(nm)} \.+ +Passed", body):
                        bad.append(f"{name}: porte gpu {nm} absente ou non Passed")
            else:
                for field, want in (("family", run["family"]), ("n", run["n"]),
                                    ("kind", "pilote"), ("min_lots", gpuv6_params["min_lots"]),
                                    ("repeat", "4"), ("ordre", "cpu-device")):
                    fm = re.search(rf"^{field}=(\S+)$", st, re.M)
                    if not fm or fm.group(1) != want:
                        bad.append(f"{name}: {field}={fm.group(1) if fm else '?'} != {want} (annonce)")
                bs = re.findall(r"^binaire_sha256=([0-9a-f]{64})$", st, re.M)
                if len(bs) != 1:
                    bad.append(f"{name}: binaire_sha256 absent ou duplique")
                else:
                    pilote_bin_shas.add(bs[0])
                # § 5.14.3 : le juge fail-fast du runner a laisse un verdict
                # grave par pilote — exige et conforme.
                juge_path = os.path.join(out, name + ".juge.txt")
                if not os.path.exists(juge_path):
                    bad.append(f"{name}: verdict du juge embarque absent ({name}.juge.txt)")
                elif "pilote juge conforme" not in read_text(juge_path):
                    bad.append(f"{name}: verdict du juge embarque non conforme")
                cmd = re.search(r"^commande=(.*)$", st, re.M)
                cmd_full = cmd.group(1) if cmd else ""
                for tok in (f"--family={run['family']}", f"--n={run['n']}", "--repeat=4",
                            "--ordre=cpu-device", f"--min-lots={gpuv6_params['min_lots']}"):
                    if tok not in cmd_full:
                        bad.append(f"{name}: commande gravee sans {tok}")
                for marker in ("--- gpu_avant ---", "--- gpu_apres ---"):
                    idx = body.find(marker)
                    nxt = body[idx + len(marker):].strip().splitlines()
                    snap = re.match(r"^(GPU-[0-9a-f-]+), \d+, \d+, \d+", nxt[0]) if (idx >= 0 and nxt) else None
                    if not snap:
                        bad.append(f"{name}: instantane nvidia-smi {marker} absent ou mal forme")
                    elif build_ident and snap.group(1) != build_ident["uuid"]:
                        bad.append(f"{name}: UUID {marker} ({snap.group(1)}) != UUID du build "
                                   f"({build_ident['uuid']}) — device change en cours de session")
                if juge_mod is None:
                    bad.append(f"{name}: juge du pilote introuvable "
                               f"(morsehgp3D_v6/tests/pilote_juge.py) — fail-closed")
                else:
                    # § 5.15.2 : identite attendue liee au plan (famille, n,
                    # graine 3, fils du profil) — le juge la verifie dans
                    # l'en-tete, puis nom/SM sont lies au build.
                    fils_att = int(profile.get("threads", "0")) if profile.get("threads", "0").isdigit() else None
                    verdict = juge_mod.juger(body, "cpu-device", repeat=4, min_lots=plan_min_lots,
                                             famille=run["family"], n=int(run["n"]),
                                             graine=3, fils=fils_att)
                    if verdict is not None:
                        bad.append(f"{name}: records du pilote non conformes ({verdict})")
                    tete = juge_mod.entete(body)
                    if isinstance(tete, dict) and build_ident:
                        if tete["device"] != build_ident["name"]:
                            bad.append(f"{name}: device de l'en-tete ({tete['device']!r}) != nom du build "
                                       f"({build_ident['name']!r})")
                        if tete["sm"] != build_ident["cc"]:
                            bad.append(f"{name}: sm de l'en-tete ({tete['sm']}) != compute capability du build "
                                       f"({build_ident['cc']})")
                pilote_bodies[name] = (run, body, meas)
    # IDENTITE DES BINAIRES par phase (§ 5.14.4) : un seul hash par phase —
    # un binaire change en cours de phase invalide la comparaison.
    for phase_name, shas, nruns in (("matrice", matrice_bin_shas, len(matrice_runs)),
                                    ("attribution", attrib_bin_shas, len(attrib_runs)),
                                    ("pilote", pilote_bin_shas,
                                     len([r for r in gpuv6_runs if r["kind"] == "pilote"]))):
        if nruns > 0 and len(shas) != 1:
            bad.append(f"{phase_name}: binaire_sha256 non unique sur la phase ({len(shas)} valeurs)")

    # PHASE GPU — contrats v5 herites de validate_v5_campaign.py. CONTROLE
    # HISTORIQUE NON AUTORITAIRE (cinquieme tour) : la v5 ne mesure ni un GPU
    # v6 ni l'exactitude de la v6 — chaque run le grave (engine/lineage/
    # authority) et le resume le rappelle.
    gpu_threads = gpu_params["threads"] if gpu_params else "0"
    gpu_cpu_bodies = {}     # (family, n) -> body du contrat CPU
    gpu_rows = []           # (run, meas, kernel_ms)
    for run in gpu_runs:
        name = run["name"]
        want_code = "4" if run["kind"] == "mutant" else "0"
        body, st, meas = check_common(out, name, commit, payload_sha, manifest_sha,
                                      gpu_threads, bad, want_code=want_code)
        if body is None:
            continue
        for field, want in (("engine", "v5"), ("lineage", "historical_baseline"),
                            ("authority", "non_authoritative")):
            fm = re.search(rf"^{field}=(\S+)$", st, re.M)
            if not fm or fm.group(1) != want:
                bad.append(f"{name}: {field}={fm.group(1) if fm else '?'} != {want} "
                           f"(lignee historique non gravee)")
        kernel_ms = None
        if run["kind"] == "witness":
            if not re.search(r"^nvcc=\S+$", body, re.M) or "release 12.9" not in body:
                bad.append(f"{name}: provenance nvcc absente ou pas 12.9")
            if not re.search(r"^NVIDIA .*, \d+\.\d+", body, re.M):
                bad.append(f"{name}: provenance nvidia-smi absente (fail-closed)")
            arith = re.search(r"^arith cas=(\d+) desaccords=(\d+)$", body, re.M)
            if not arith or int(arith.group(1)) < 1000 or int(arith.group(2)) != 0:
                bad.append(f"{name}: lot arithmetique absent, sous le plancher ou en desaccord")
            for fam in ("uniform", "eight_clusters"):
                m = re.search(rf"^scan famille={fam} ancres=\d+ seeds=(\d+) sites=\d+ morts=\d+ desaccords=(\d+) kernel_ms=", body, re.M)
                if not m or int(m.group(1)) < 1000 or int(m.group(2)) != 0:
                    bad.append(f"{name}: scan {fam} absent, sous le plancher (1000 seeds) ou en desaccord")
            if "device_witness OK" not in body:
                bad.append(f"{name}: temoin device non conforme")
        elif run["kind"] == "mutant":
            if "DESACCORD device/hote" not in body:
                bad.append(f"{name}: le mutant du temoin n'a pas ete tue sur le device")
        elif run["kind"] == "lane":
            want_soa = {("uniform", "1200", "1", "soa"): 200, ("eight_clusters", "1200", "4", "soa"): 200,
                        ("uniform", "8000", "8", "soa"): 100000, ("uniform", "300", "1", "soa"): 200}
            want_q3 = dict(want_soa)
            want_q3.update({("uniform", "1200", "1", "index"): 200, ("eight_clusters", "1200", "4", "index"): 200,
                            ("uniform", "300", "1", "index"): 200})
            for lane, cand_key, want, n_ok in (("q3_lane_device", "candidats_q3", want_q3, 7),
                                               ("q4_lane_device", "candidats_q4", want_q3, 7)):
                rx = re.compile(rf"^{lane} famille=(\S+) n=(\d+) fils=(\d+) (.*) desaccords_vecteur=(\d+) desaccords_compteurs=(\d+)$", re.M)
                seen = {}
                for f, n, t, mid, v, k in rx.findall(body):
                    kv = dict(re.findall(r"(\w+)=([0-9.]+)", mid))
                    wire = re.search(r"\bwire=(\w+)", mid)
                    lkey = (f, n, t, wire.group(1) if wire else "soa")
                    seen[lkey] = seen.get(lkey, 0) + 1
                    if lkey not in want:
                        bad.append(f"{name}: {lane} — triple inattendu {lkey}")
                        continue
                    if int(v) != 0 or int(k) != 0:
                        bad.append(f"{name}: {lane} {lkey} — desaccords")
                    if int(kv.get(cand_key, 0)) < want[lkey]:
                        bad.append(f"{name}: {lane} {lkey} — plancher de candidats {want[lkey]} non atteint")
                    if int(kv.get("seeds", 0)) < 1 or int(kv.get("tues", kv.get("coeur_tues", 0))) < 1 \
                       or int(kv.get("lancements", 0)) < 1:
                        bad.append(f"{name}: {lane} {lkey} — seeds, morts ou lancements vides")
                for lkey in want:
                    if seen.get(lkey, 0) != 1:
                        bad.append(f"{name}: {lane} {lkey} — {seen.get(lkey, 0)} occurrence(s), une attendue")
                if body.count(f"{lane} OK") != n_ok:
                    bad.append(f"{name}: {lane} — {n_ok} OK attendus, {body.count(lane + ' OK')} vus")
        else:
            # Contrats par famille : moteur v5, identite, digests obligatoires.
            check_pipeline_run(name, body, run["family"], run["n"], "3", "v5", gpu_threads, bad)
            for field, want in (("family", run["family"]), ("n", run["n"]), ("kind", run["kind"])):
                fm = re.search(rf"^{field}=(\S+)$", st, re.M)
                if not fm or fm.group(1) != want:
                    bad.append(f"{name}: {field}={fm.group(1) if fm else '?'} != {want} (annonce)")
            # ARGV CONTRACTUEL PAR BRAS (cinquieme tour : retirer
            # --gpu-wire=index du bras index restait vert).
            cmd = re.search(r"^commande=(.*)$", st, re.M)
            argv = cmd.group(1).split() if cmd else []
            base_args = {f"--family={run['family']}", f"--n={run['n']}", "--s=8", "--smax=11",
                         "--seed=3", f"--threads={gpu_threads}", "--digest"}
            if not base_args.issubset(argv):
                bad.append(f"{name}: commande gravee sans les arguments contractuels du bras")
            want_in = {"cpu": set(), "dev": {"--gpu"}, "ad": {"--gpu", "--gpu-min-sites=256"},
                       "idx": {"--gpu", "--gpu-wire=index"}}[run["kind"]]
            want_out = {"cpu": {"--gpu"}, "dev": {"--gpu-min-sites=256", "--gpu-wire=index"},
                        "ad": {"--gpu-wire=index"}, "idx": {"--gpu-min-sites=256"}}[run["kind"]]
            if not want_in.issubset(argv):
                bad.append(f"{name}: argument de route absent de la commande gravee ({sorted(want_in - set(argv))})")
            hit = want_out & set(argv)
            if hit:
                bad.append(f"{name}: argument d'une autre route dans la commande gravee ({sorted(hit)})")
            for dkey in ("digest_balls", "digest_all"):
                if len(re.findall(rf"^{dkey}=([0-9a-f]{{64}})$", body, re.M)) != 1:
                    bad.append(f"{name}: {dkey} absent ou duplique")
            if run["kind"] == "cpu":
                if re.search(r"^gpu=1", body, re.M):
                    bad.append(f"{name}: le contrat CPU annonce gpu=1")
                gpu_cpu_bodies[(run["family"], run["n"])] = body
            else:
                if not re.search(r"^backend=override_experimental", body, re.M):
                    bad.append(f"{name}: le run --gpu doit s'annoncer backend=override_experimental (non autoritaire)")
                g = re.search(r"^gpu=1 kernel_ms=([0-9.]+) lancements=(\d+) min_sites=(\d+) "
                              r"routage_q3=(\d+)/(\d+) ancres \(seeds (\d+)/(\d+)\) "
                              r"routage_q4=(\d+)/(\d+) ancres \(seeds (\d+)/(\d+)\)", body, re.M)
                if not g:
                    bad.append(f"{name}: ligne gpu= (lancements, min_sites, routage) absente ou mal formee")
                else:
                    kernel_ms = float(g.group(1))
                    launches, min_sites = int(g.group(2)), int(g.group(3))
                    s3d, s3h, s4d, s4h = int(g.group(6)), int(g.group(7)), int(g.group(10)), int(g.group(11))
                    if launches < 1:
                        bad.append(f"{name}: aucun lancement device")
                    if run["kind"] == "ad":
                        if min_sites != 256:
                            bad.append(f"{name}: adaptatif attendu a min_sites=256, vu {min_sites}")
                        if s3d < 1 or s3h < 1 or s4d < 1 or s4h < 1:
                            bad.append(f"{name}: adaptatif — les deux routes doivent avoir des seeds "
                                       f"(q3 {s3d}/{s3h}, q4 {s4d}/{s4h})")
                    elif min_sites != 1:
                        bad.append(f"{name}: tout-device attendu a min_sites=1, vu {min_sites}")
                cpu_body = gpu_cpu_bodies.get((run["family"], run["n"]), "")
                for dkey in ("digest_balls", "digest_all"):
                    d_gpu = re.search(rf"^{dkey}=([0-9a-f]{{64}})$", body, re.M)
                    d_cpu = re.search(rf"^{dkey}=([0-9a-f]{{64}})$", cpu_body, re.M)
                    if not d_gpu or not d_cpu or d_gpu.group(1) != d_cpu.group(1):
                        bad.append(f"{name}: {dkey} DIFFERENT du contrat CPU de la meme famille (ou absent)")
        gpu_rows.append((run, meas, kernel_ms))

    # PHASE FRONTIERE — issues TYPEES (cinquieme tour : une panne quelconque
    # n'est PAS une frontiere). Trois classes admises :
    #   0   -> succes : contrat pipeline v6 COMPLET (identite, compteurs,
    #          cardinalites, temps), aucun digest ;
    #   124 -> timeout de la borne annoncee (l'outil timeout du runner) ;
    #   != 0 avec motif STRUCTURE de capacite (resource_exhausted /
    #          bad_alloc) -> refus de capacite, la donnee de frontiere.
    # TOUT autre code (CLI 2, invariant 3, binaire absent 127, signal sans
    # motif — SIGKILL non prouve = observation censuree) invalide la phase.
    frontier_threads = frontier_params["threads"] if frontier_params else "0"
    frontier_rows = []
    for run in frontier_runs:
        name = run["name"]
        body, st, meas = check_common(out, name, commit, payload_sha, manifest_sha,
                                      frontier_threads, bad, code_free=True)
        if st is None:
            continue
        # CODE et DUREE : decimaux EXACTS, exactement une fois (sixieme tour :
        # code=abc n'est pas une issue, c'est une panne).
        cms = re.findall(r"^code=(\d+)$", st, re.M)
        if len(cms) != 1:
            bad.append(f"{name}: code non decimal ou duplique dans le statut")
            code = "?"
        else:
            code = cms[0]
        if len(re.findall(r"^duree_s=(\d+)$", st, re.M)) != 1:
            bad.append(f"{name}: duree_s non decimale ou dupliquee dans le statut")
        for field, want in (("family", run["family"]), ("n", run["n"]), ("seq", run["seq"])):
            fm = re.search(rf"^{field}=(\S+)$", st, re.M)
            if not fm or fm.group(1) != want:
                bad.append(f"{name}: {field}={fm.group(1) if fm else '?'} != {want} (annonce)")
        cmd = re.search(r"^commande=(.*)$", st, re.M)
        # LIAISON DU PLAFOND (septieme tour : les jetons decoratifs sont
        # morts) : occurrences UNIQUES de limit_kind/limit_kb, et la commande
        # gravee doit correspondre EXACTEMENT au wrapper ulimit du plan —
        # prefixe bash absolu, option -v, position du plafond, exec du
        # binaire, arguments contractuels UNIQUES et sans conflit.
        plan_ulimit = frontier_params.get("ulimit_kb", "0") if frontier_params else "0"
        lks = re.findall(r"^limit_kind=(\S+)$", st, re.M)
        lbs = re.findall(r"^limit_kb=(\d+)$", st, re.M)
        if len(lks) != 1 or len(lbs) != 1:
            bad.append(f"{name}: limit_kind/limit_kb absents ou dupliques "
                       f"({len(lks)}/{len(lbs)} occurrences)")
        want_kind, want_kb = ("rlimit_as", plan_ulimit) if plan_ulimit != "0" else ("none", "0")
        if lks != [want_kind] or lbs != [want_kb]:
            bad.append(f"{name}: attestation de plafond {lks}/{lbs} != {want_kind}/{want_kb} (plan)")
        args_re = (rf"--family={re.escape(run['family'])} --n={run['n']} "
                   rf"--s=8 --smax=11 --seed=3 --threads={frontier_threads}")
        if plan_ulimit != "0":
            cmd_re = (r"^/bin/bash -c ulimit -v \"\$1\" && shift && exec \"\$@\" _ "
                      + plan_ulimit + r" \S+ " + args_re + r"$")
        else:
            cmd_re = r"^\S+ " + args_re + r"$"
        cmd_full = cmd.group(1) if cmd else ""
        if not re.match(cmd_re, cmd_full):
            bad.append(f"{name}: commande gravee sans correspondance EXACTE au contrat de frontiere "
                       f"(wrapper ulimit du plan, arguments uniques, aucun jeton parasite)")
        # MOTIF FATAL : applique a TOUTES les classes — invariant brise,
        # sanitizer, panne d'infrastructure ; un motif de capacite present ne
        # les masque pas.
        fatal = FATAL_FRONT.search(body or "")
        if fatal:
            bad.append(f"{name}: motif FATAL ({fatal.group(0)}) — la phase frontiere est invalide")
        # QUATRE CLASSES FERMEES et rien d'autre.
        note = ""
        gtime_path = os.path.join(out, name + ".status.time")
        gt = read_text(gtime_path) if os.path.exists(gtime_path) else ""
        if code == "0":
            if body is not None:
                check_pipeline_run(name, body, run["family"], run["n"], "3", "v6",
                                   frontier_threads, bad)
                if ANY_DIGEST.search(body):
                    bad.append(f"{name}: digest imprime sur un run de frontiere")
                fb = FORBIDDEN.search(body)
                if fb:
                    bad.append(f"{name}: motif interdit sur un succes de frontiere ({fb.group(0)})")
        elif code == "2":
            # Grammaire FERMEE du refus (septieme tour : le suffixe _faux
            # passait) : exactement UNE ligne, et jamais un diagnostic
            # reserve a une autre classe.
            refus = re.findall(r"^REFUS resource_exhausted : .*$", body or "", re.M)
            if len(refus) != 1:
                bad.append(f"{name}: code 2 sans exactement une ligne "
                           f"« REFUS resource_exhausted : ... » ({len(refus)} vues)")
                note = "code 2 non type"
            elif re.search(r"std::bad_alloc|bad_alloc", body or ""):
                bad.append(f"{name}: code 2 avec un diagnostic bad_alloc — classes non exclusives")
                note = "code 2 contradictoire"
            else:
                note = "resource_exhausted (refus du pipeline)"
        elif code == "134":
            if not re.search(r"std::bad_alloc", body or ""):
                bad.append(f"{name}: code 134 sans diagnostic exact std::bad_alloc")
                note = "134 non type"
            elif re.search(r"^REFUS ", body or "", re.M):
                bad.append(f"{name}: code 134 avec une ligne REFUS — classes non exclusives")
                note = "134 contradictoire"
            elif not re.search(r"terminated by signal 6", gt):
                bad.append(f"{name}: code 134 sans preuve de signal 6 par le superviseur "
                           f"(« terminated by signal 6 » absent de la sortie GNU time — "
                           f"un simple exit(134) n'est pas un abort)")
                note = "134 sans preuve de signal"
            elif plan_ulimit == "0":
                bad.append(f"{name}: code 134 bad_alloc SANS RLIMIT_AS atteste — signal non attribue")
                note = "134 sans plafond atteste"
            else:
                note = f"bad_alloc sous rlimit_as={plan_ulimit}kB (signal 6 prouve)"
        elif code == "124":
            bad.append(f"{name}: sortie 124 NON ATTRIBUEE (aucun marqueur causal ne distingue le "
                       f"timeout du superviseur d'un exit(124) du binaire) — la phase frontiere est invalide")
            note = "124 non attribue"
        else:
            bad.append(f"{name}: code={code} HORS des trois classes fermees "
                       f"(0, 2 REFUS resource_exhausted, 134 bad_alloc prouve sous rlimit) — "
                       f"panne non typee, la phase frontiere est invalide")
            note = f"panne non typee (code={code})"
        frontier_rows.append((run, code, meas, note))

    # CONTROLE EXHAUSTIF des fichiers : chaque fichier de out/ doit etre un
    # artefact d'un run annonce (.txt/.status/.status.time) ou un auxiliaire
    # connu — quelle que soit son extension (audit GCP v6, P1).
    known = {r["name"] for r in conf_runs + bench_runs + queue_runs
             + sweep_runs + gpu_runs + frontier_runs
             + matrice_runs + attrib_runs + gpuv6_runs}
    allowed = set(AUX)
    for name in known:
        allowed.update((f"{name}.txt", f"{name}.status", f"{name}.status.time"))
    for r in gpuv6_runs:
        if r["kind"] == "pilote":
            allowed.add(f"{r['name']}.juge.txt")
    for f in sorted(os.listdir(out)):
        full = os.path.join(out, f)
        if os.path.islink(full):
            bad.append(f"{f}: lien symbolique refuse")
        elif os.path.isdir(full):
            bad.append(f"{f}: repertoire inattendu dans out/")
        elif f not in allowed:
            bad.append(f"{f}: fichier inattendu")
    # MANIFESTE DISTANT : le runner grave les sha256 de tous ses artefacts ;
    # apres rapatriement, chaque fichier est recoupe (corruption scp tuee).
    remote_manifest = os.path.join(out, "MANIFESTE_DISTANT.txt")
    if not os.path.exists(remote_manifest):
        bad.append("MANIFESTE_DISTANT.txt: ABSENT (artefacts distants non hashes)")
    else:
        listed = {}
        for line in read_text(remote_manifest).splitlines():
            m = re.match(r"^([0-9a-f]{64})  (\S+)$", line)
            if not m:
                bad.append(f"MANIFESTE_DISTANT.txt: ligne hors grammaire ({line[:60]})")
                continue
            if m.group(2) in listed:
                bad.append(f"MANIFESTE_DISTANT.txt: chemin duplique {m.group(2)}")
            listed[m.group(2)] = m.group(1)
        expected_files = {f for f in os.listdir(out)
                          if f not in ("MANIFESTE_DISTANT.txt",) and os.path.isfile(os.path.join(out, f))}
        if set(listed) != expected_files:
            bad.append("MANIFESTE_DISTANT.txt: liste != artefacts rapatries")
        else:
            for name, want in sorted(listed.items()):
                if sha256_file(os.path.join(out, name)) != want:
                    bad.append(f"{name}: hash different du manifeste distant (corruption de rapatriement)")

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
    resume_dir = os.path.dirname(os.path.abspath(out))
    tmp = os.path.join(resume_dir, "bench_resume.txt.tmp")
    with open(tmp, "w", encoding="utf-8") as fh:
        fh.write("\n".join(lines) + "\n")
    os.replace(tmp, os.path.join(resume_dir, "bench_resume.txt"))

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
    tmp = os.path.join(resume_dir, "queue_resume.txt.tmp")
    with open(tmp, "w", encoding="utf-8") as fh:
        fh.write("\n".join(lines) + "\n")
    os.replace(tmp, os.path.join(resume_dir, "queue_resume.txt"))

    lines = [
        "# sweep_resume — murs par nombre de fils (dispersion, JAMAIS une conclusion de speedup).",
        "famille\tn\tfils\truns\tmur_median_ms\tmur_min_ms\tmur_max_ms\trss_max_kb",
    ]
    for key in sorted(sweep_measures, key=lambda k: (k[0], int(k[1]), int(k[2]))):
        vals = sweep_measures[key]
        murs = [v[0] for v in vals if v[0] is not None]
        rss = [v[2] for v in vals if v[2] is not None]
        lines.append("\t".join([key[0], key[1], key[2], str(len(vals)),
                                fmt(median(murs)), fmt(min(murs) if murs else None),
                                fmt(max(murs) if murs else None),
                                str(max(rss)) if rss else "NA"]))
    tmp = os.path.join(resume_dir, "sweep_resume.txt.tmp")
    with open(tmp, "w", encoding="utf-8") as fh:
        fh.write("\n".join(lines) + "\n")
    os.replace(tmp, os.path.join(resume_dir, "sweep_resume.txt"))

    lines = [
        "# gpu_resume — CONTROLE HISTORIQUE NON AUTORITAIRE (engine=v5, lineage=historical_baseline) :",
        "# murs des contrats CPU/GPU v5 et kernel_ms — ne mesure NI un GPU v6 NI l'exactitude v6 ;",
        "# faits seulement, pas de conclusion (un run par route ne suffit pas a attribuer un gain).",
        "famille\tn\tkind\tmur_ms\tduree_s\trss_kb\tkernel_ms",
    ]
    for run, meas, kernel_ms in gpu_rows:
        if run["kind"] in ("witness", "lane", "mutant"):
            continue
        lines.append("\t".join([run["family"], run["n"], run["kind"], fmt(meas[0]),
                                str(meas[1]) if meas[1] is not None else "NA",
                                str(meas[2]) if meas[2] is not None else "NA",
                                f"{kernel_ms:.1f}" if kernel_ms is not None else "NA"]))
    tmp = os.path.join(resume_dir, "gpu_resume.txt.tmp")
    with open(tmp, "w", encoding="utf-8") as fh:
        fh.write("\n".join(lines) + "\n")
    os.replace(tmp, os.path.join(resume_dir, "gpu_resume.txt"))

    lines = [
        "# matrice_resume — murs par point pre-enregistre (contrastes fils/inflight/join/",
        "# digest ; dispersion entre passages, JAMAIS une conclusion — la decision passe",
        "# par l'arbre pre-enregistre du § 5.10 apres audit).",
        "famille\tn\tfils\tinflight\tjoin\tdigest\tpassage\tmur_ms\tduree_s\trss_kb",
    ]
    for run, meas in matrice_rows:
        lines.append("\t".join([run["family"], run["n"], run["mat_threads"], run["inflight"],
                                run["join"], run["digest"], run["passage"], fmt(meas[0]),
                                str(meas[1]) if meas[1] is not None else "NA",
                                str(meas[2]) if meas[2] is not None else "NA"]))
    tmp = os.path.join(resume_dir, "matrice_resume.txt.tmp")
    with open(tmp, "w", encoding="utf-8") as fh:
        fh.write("\n".join(lines) + "\n")
    os.replace(tmp, os.path.join(resume_dir, "matrice_resume.txt"))

    lines = [
        "# gpuv6_resume — records RETENUS du pilote serie C (echauffement exclu) :",
        "# murs CPU/route device et etages par repetition ; faits seulement, la parite",
        "# est jugee par le meme juge que la porte stub (pilote_juge.juger).",
        "famille\tn\trepetition\tordre\tmur_cpu_ms\tmur_route_device_ms\tprefiltre_census_cpu_ms\troute_device_etage_ms\tkernels_ms\th2d_ms\td2h_ms\tlots",
    ]
    for name in sorted(pilote_bodies):
        run, body, meas = pilote_bodies[name]
        for ln in body.splitlines():
            if not ln.startswith("repetition=") or "retenue=OUI" not in ln:
                continue
            f = dict(kv.split("=", 1) for kv in ln.split() if "=" in kv)
            lines.append("\t".join([run["family"], run["n"], f.get("repetition", "?"),
                                    f.get("ordre", "?"), f.get("mur_cpu_ms", "NA"),
                                    f.get("mur_route_device_ms", "NA"),
                                    f.get("prefiltre_census_cpu_ms", "NA"),
                                    f.get("route_device_etage_ms", "NA"),
                                    f.get("kernels_ms", "NA"), f.get("h2d_ms", "NA"),
                                    f.get("d2h_ms", "NA"), f.get("lots", "NA")]))
    tmp = os.path.join(resume_dir, "gpuv6_resume.txt.tmp")
    with open(tmp, "w", encoding="utf-8") as fh:
        fh.write("\n".join(lines) + "\n")
    os.replace(tmp, os.path.join(resume_dir, "gpuv6_resume.txt"))

    lines = [
        "# frontier_resume — codes, murs et RSS de la frontiere d'echelle SOUS PLAFOND",
        "# VIRTUEL RLIMIT_AS (pas le mur RAM natif) ; un refus de capacite type est une donnee.",
        ("# limit_kind=rlimit_as limit_kb=" + frontier_params.get("ulimit_kb", "?")
         if frontier_params and frontier_params.get("ulimit_kb", "0") != "0"
         else "# limit_kind=none (aucun plafond impose par le plan)"),
        "famille\tn\tcode\tmur_ms\tduree_s\trss_kb\tnote",
    ]
    for run, code, meas, note in frontier_rows:
        lines.append("\t".join([run["family"], run["n"], code, fmt(meas[0]),
                                str(meas[1]) if meas[1] is not None else "NA",
                                str(meas[2]) if meas[2] is not None else "NA",
                                note or "-"]))
    tmp = os.path.join(resume_dir, "frontier_resume.txt.tmp")
    with open(tmp, "w", encoding="utf-8") as fh:
        fh.write("\n".join(lines) + "\n")
    os.replace(tmp, os.path.join(resume_dir, "frontier_resume.txt"))

    # PROFIL DE MESURE G4 (dixieme tour) : une instrumentation non standard
    # n'est pas un simple declassement — les RSS d'un faux instrument ne
    # doivent JAMAIS etre publies comme mesures d'une campagne payante.
    if canon_axes.get("PROFIL_NOM") == "g4_mesure_v1" \
            and not (len(TIME_BINS) == len(known) and set(TIME_BINS) == {"/usr/bin/time"}):
        bad.append("profil g4_mesure_v1 : instrumentation NON STANDARD (time_bin != /usr/bin/time, "
                   "vide ou incomplet) — mesures INVALIDES et NON RECEVABLES (les resumes ecrits ne sont pas des mesures)")
    if bad:
        print("campaign_status=partial_or_failed")
        for b in bad:
            print("  -", b)
        return 1
    axis_map = (("conf_specs", "CONF_SPECS"), ("bench_specs", "BENCH_SPECS"),
                ("queue_families", "QUEUE_FAMILIES"), ("queue_n", "QUEUE_N"),
                ("queue_seeds", "QUEUE_SEEDS"), ("run_timeout", "RUN_TIMEOUT"),
                ("threads", "THREADS_VM"), ("v5_gate_min", "V5_GATE_MIN"),
                ("v6_gate_min", "V6_GATE_MIN"),
                ("sweep_specs", "SWEEP_SPECS"), ("sweep_repeats", "SWEEP_REPEATS"),
                ("gpu_specs", "GPU_SPECS"), ("frontier_specs", "FRONTIER_SPECS"),
                ("frontier_timeout", "FRONTIER_TIMEOUT"),
                ("gpu_build_timeout", "GPU_BUILD_TIMEOUT"),
                ("frontier_ulimit_kb", "FRONTIER_ULIMIT_KB"))
    axes_equal = bool(canon_axes) and all(
        profile.get(pk, "").split() == canon_axes.get(ck, "").split() for pk, ck in axis_map)
    # INSTRUMENTATION EPINGLEE (huitieme tour, TOTALITE au dixieme) : GNU
    # time enveloppe le superviseur — un TIME_BIN de test invente RSS et
    # attestations. Un time_bin par run annonce, tous /usr/bin/time, sinon
    # le verdict est DECLASSE (jamais decision_complete).
    instrumentation_standard = (len(TIME_BINS) == len(known)
                                and set(TIME_BINS) == {"/usr/bin/time"})
    # L'IDENTITE du canon est dans sa grammaire (PROFIL_NOM) : un canon reduit
    # renomme ne peut pas porter une pretention decision_v1.
    canon_is_decision = canon_axes.get("PROFIL_NOM", "") == "decision_v1"
    if profile.get("profil") == profile.get("profil_canonique") == "decision_v1" and axes_equal \
            and canon_is_decision and instrumentation_standard:
        print(f"campaign_status=decision_complete profil=decision_v1 "
              f"({len(known)} runs valides, source_commit={commit[:12]}, "
              f"resumes bench_resume.txt / queue_resume.txt)")
        print("=== CAMPAGNE COMPLETE ===")
    else:
        if profile.get("profil") == "decision_v1" and not instrumentation_standard:
            reason = "instrumentation de test (TIME_BIN non epingle /usr/bin/time)"
        elif profile.get("profil") == "decision_v1" and not axes_equal:
            reason = "axes != canon"
        elif profile.get("profil") == "decision_v1" and not canon_is_decision:
            reason = "canon non decisionnel (PROFIL_NOM)"
        else:
            reason = "profil non decisionnel"
        print(f"campaign_status=verifie_non_decisionnel profil={profile.get('profil', '?')} "
              f"canonique={profile.get('profil_canonique', '?')} cause={reason} "
              f"({len(known)} runs valides — JAMAIS une decision)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
