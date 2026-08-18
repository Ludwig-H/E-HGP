#!/usr/bin/env python3
# Validateur LOCAL des sessions SCALE_THREADS (audits 9223888 / b3a6eb4,
# durci par 66886c0) : seule autorite du statut. Exigences :
#   - LIAISON NOM -> EXPERIENCE (66886c0 § 1) : pour chaque run, le
#     validateur RECONSTRUIT l'argv contractuel depuis le nom (famille,
#     n, s=8, smax=11, seed=3, fils) et exige (1) args_sha256 == SHA-256
#     de cette serialisation NUL, (2) la ligne d'identite du probe
#     (famille/n/s/smax/seed) CONFORME au nom, (3) threads_requested
#     conforme au suffixe (tmax == nproc du statut) ;
#   - WORKERS MESURES (66886c0 § 2) : la ligne `execution ...` publie
#     des comptes alimentes au point de creation des std::thread —
#     gen/prefiltre/census/expansion == threads_requested (les taches
#     sont abondantes aux tailles de campagne), fold == min(threads, 10) ;
#   - SCHEMA COMPLET (66886c0 § 3) : exactement UNE occurrence de
#     digest_balls, digest_forest_K1..K10, digest_all et de
#     cardinalites K=1..10 — ni doublon ni ordre manquant ;
#   - APPARIEMENT : egalite stricte digests + cardinalites au sein d'une
#     famille (t1/t8/tmax a n=32000) ; la phase n64000 est etiquetee
#     honnetement digest_recorded_unpaired (66886c0 § 4) ;
#   - aucun statut not_run_budget ; pins ; motifs interdits.
# Usage : validate_v4_scale_threads.py OUT COMMIT PAYLOAD_SHA MANIFEST_SHA
#         REMOTE_RC SCP_RC PHASE
import hashlib
import os
import re
import sys

KMAX = 10


def expected(phase):
    if phase == "n32000":
        return {
            "uniform": [("thr_uniform_n32000_smax11_t1", "1"),
                        ("thr_uniform_n32000_smax11_t8", "8"),
                        ("thr_uniform_n32000_smax11_tmax", "max")],
            "eight_clusters": [("thr_eight_clusters_n32000_smax11_t8", "8"),
                               ("thr_eight_clusters_n32000_smax11_tmax",
                                "max")],
        }
    if phase == "n64000":
        return {fam: [(f"thr_{fam}_n64000_smax11_tmax", "max")]
                for fam in ("uniform", "terrain", "eight_clusters",
                            "scanline_overlap_multiecho")}
    if phase == "court1h":
        # Directive <= 1 h : paires t8/tmax a n=32000, sans t1.
        return {
            "uniform": [("thr_uniform_n32000_smax11_t8", "8"),
                        ("thr_uniform_n32000_smax11_tmax", "max")],
            "eight_clusters": [("thr_eight_clusters_n32000_smax11_t8", "8"),
                               ("thr_eight_clusters_n32000_smax11_tmax",
                                "max")],
        }
    raise SystemExit(f"phase inconnue : {phase}")


def phase_n(phase):
    return {"n32000": 32000, "n64000": 64000, "court1h": 32000}[phase]


def contract_argv(family, n, threads):
    # L'argv EXACT que le runner passe au probe — toute derive du runner
    # ou du validateur casse le hash, dans les deux sens.
    return [f"--family={family}", f"--n={n}", "--s=8", "--smax=11",
            "--seed=3", "--min-balls=10000", "--min-fusions=1000",
            f"--threads={threads}", "--digest"]


def argv_sha256(argv):
    h = hashlib.sha256()
    for a in argv:
        h.update(a.encode())
        h.update(b"\0")
    return h.hexdigest()


def main():
    out, commit, payload_sha, manifest_sha, remote_rc, scp_rc, phase = \
        sys.argv[1:8]
    groups = expected(phase)
    n_of_phase = phase_n(phase)
    forbidden = re.compile(r"REFUS|INVARIANT|PLANCHER|Killed|bad_alloc")
    counters = re.compile(
        r"boules_uniques=\d+.*evenements=\d+.*juge=off desaccords=NA")
    bad = []
    if remote_rc != "0":
        bad.append(f"session distante : remote_rc={remote_rc}")
    if scp_rc != "0":
        bad.append(f"rapatriement : scp_rc={scp_rc}")
    signatures = {}
    for fam, names in groups.items():
        for name, tlabel in names:
            status_p = os.path.join(out, name + ".status")
            txt_p = os.path.join(out, name + ".txt")
            if not os.path.exists(status_p):
                bad.append(f"{name}: .status ABSENT")
                continue
            st = open(status_p).read()
            if "not_run_budget=1" in st:
                bad.append(f"{name}: non lance (budget) — campagne partielle")
                continue
            if "finished=1" not in st:
                bad.append(f"{name}: status incomplet")
            m = re.search(r"^code=(\d+)$", st, re.M)
            if not m or m.group(1) != "0":
                bad.append(f"{name}: code={m.group(1) if m else '?'}")
            for field, want in (("source_commit", commit),
                                ("source_payload_sha256", payload_sha),
                                ("protocol_manifest_sha256", manifest_sha)):
                fm = re.search(rf"^{field}=(\S+)$", st, re.M)
                if not fm or fm.group(1) != want:
                    bad.append(f"{name}: {field} absent ou different du pin")
            treq = re.search(r"^threads_requested=(\d+)$", st, re.M)
            nproc = re.search(r"^nproc=(\d+)$", st, re.M)
            if not treq or not nproc or int(nproc.group(1)) <= 0:
                bad.append(f"{name}: threads_requested/nproc absents")
                continue
            treq_v, nproc_v = int(treq.group(1)), int(nproc.group(1))
            want_t = nproc_v if tlabel == "max" else int(tlabel)
            if treq_v != want_t:
                bad.append(f"{name}: threads_requested={treq_v} incoherent "
                           f"avec le nom (attendu {want_t})")
            if not re.search(r"^cpu_set=\S+$", st, re.M):
                bad.append(f"{name}: cpu_set absent")
            # LIAISON NOM -> ARGV (66886c0 § 1) : hash recalcule, jamais
            # seulement present.
            want_sha = argv_sha256(contract_argv(fam, n_of_phase, want_t))
            got_sha = re.search(r"^args_sha256=([0-9a-f]{64})$", st, re.M)
            if not got_sha or got_sha.group(1) != want_sha:
                bad.append(f"{name}: args_sha256 absent ou DIFFERENT de "
                           f"l'argv contractuel du nom")
            if not os.path.exists(txt_p):
                bad.append(f"{name}: .txt ABSENT")
                continue
            body = open(txt_p, errors="replace").read()
            fb = forbidden.search(body)
            if fb:
                bad.append(f"{name}: motif interdit ({fb.group(0)})")
            if not counters.search(body):
                bad.append(f"{name}: ligne de compteurs absente")
            # LIAISON NOM -> IDENTITE IMPRIMEE (66886c0 § 1).
            ident = re.search(
                r"^famille=(\S+) n=(\d+) s=(\d+) smax=(\d+) seed=(\d+)",
                body, re.M)
            if not ident:
                bad.append(f"{name}: ligne d'identite absente")
            elif (ident.group(1) != fam or int(ident.group(2)) != n_of_phase
                  or ident.group(3) != "8" or ident.group(4) != "11"
                  or ident.group(5) != "3"):
                bad.append(f"{name}: identite imprimee "
                           f"({'/'.join(ident.groups())}) != nom du run")
            # WORKERS MESURES (66886c0 § 2, durci par 7d921ff : la
            # generation est recue PAR LANE et l'aval par la valeur
            # RETOURNEE de parallel_ranges) + AFFINITE CPU EFFECTIVE
            # (c9c3a48 : publiee par le processus mesure via
            # sched_getaffinity — cpu_set seul est une intention
            # auto-declaree).
            ex = re.search(
                r"^execution threads_requested=(\d+) gen_workers_q2=(\d+) "
                r"gen_workers_q3=(\d+) gen_workers_q4=(\d+) "
                r"gen_workers_max=(\d+) prefilter_workers=(\d+) "
                r"census_workers=(\d+) expansion_workers=(\d+) "
                r"fold_workers_max=(\d+) affinity_cpus_effective=(\d+) "
                r"affinity_mask=(\S+)$",
                body, re.M)
            if not ex:
                bad.append(f"{name}: ligne execution (workers mesures + "
                           f"affinite) absente")
            else:
                (tr, gq2, gq3, gq4, _gmax, pre, cen, exp, fld,
                 aff) = (int(g) for g in ex.groups()[:10])
                aff_mask = ex.group(11)
                if tr != want_t:
                    bad.append(f"{name}: threads_requested imprime {tr} != "
                               f"{want_t}")
                for label, v in (("gen_workers_q2", gq2),
                                 ("gen_workers_q3", gq3),
                                 ("gen_workers_q4", gq4),
                                 ("prefilter_workers", pre),
                                 ("census_workers", cen),
                                 ("expansion_workers", exp)):
                    if v != want_t:
                        bad.append(f"{name}: {label}={v} != fils demandes "
                                   f"{want_t} (taches abondantes a n="
                                   f"{n_of_phase})")
                if fld != min(want_t, KMAX):
                    bad.append(f"{name}: fold_workers_max={fld} != "
                               f"min({want_t}, {KMAX})")
                cpu_set = re.search(r"^cpu_set=(\S+)$", st, re.M)
                if aff != nproc_v:
                    bad.append(f"{name}: affinite effective {aff} != nproc "
                               f"{nproc_v} — les fils seraient assis sur "
                               f"moins de chaises que declare")
                if cpu_set and aff_mask != cpu_set.group(1):
                    bad.append(f"{name}: masque d'affinite effectif "
                               f"{aff_mask} != cpu_set contractuel "
                               f"{cpu_set.group(1)}")
            # SCHEMA COMPLET (66886c0 § 3) : une occurrence exacte de
            # chaque ligne attendue, ni plus ni moins.
            digest_lines = re.findall(
                r"^digest_(balls|forest_K(\d+)|all)=([0-9a-f]{64})$", body,
                re.M)
            seen = {}
            for kind, knum, hexv in digest_lines:
                key = kind if not knum else f"forest_K{knum}"
                seen[key] = seen.get(key, 0) + 1
            want_keys = (["balls"] +
                         [f"forest_K{k}" for k in range(1, KMAX + 1)] +
                         ["all"])
            for k in want_keys:
                if seen.get(k, 0) != 1:
                    bad.append(f"{name}: digest {k} present "
                               f"{seen.get(k, 0)} fois (attendu 1)")
            for k in seen:
                if k not in want_keys:
                    bad.append(f"{name}: digest inattendu {k}")
            card_ks = re.findall(r"^cardinalites K=(\d+) ", body, re.M)
            for k in range(1, KMAX + 1):
                c = card_ks.count(str(k))
                if c != 1:
                    bad.append(f"{name}: cardinalites K={k} presente {c} "
                               f"fois (attendu 1)")
            digests = tuple(sorted(
                f"{kind if not knum else 'forest_K' + knum}={hexv}"
                for kind, knum, hexv in digest_lines))
            cards = tuple(sorted(
                re.findall(r"^(cardinalites K=\d+ .*)$", body, re.M)))
            signatures[name] = (digests, cards)
    # Appariement au sein d'une famille.
    for fam, names in groups.items():
        sigs = [(n, signatures[n]) for n, _ in names if n in signatures]
        for i in range(1, len(sigs)):
            if sigs[i][1] != sigs[0][1]:
                bad.append(f"{fam}: OBJETS DIFFERENTS entre {sigs[0][0]} et "
                           f"{sigs[i][0]} (digests ou cardinalites)")
    known = {n for names in groups.values() for n, _ in names}
    for f in sorted(os.listdir(out)) if os.path.isdir(out) else []:
        if f.endswith(".txt") and f[:-4] not in known:
            bad.append(f"{f}: fichier inattendu")
    if bad:
        print("campaign_status=partial_or_failed")
        for b in bad:
            print("  -", b)
        return 1
    # Etiquetage honnete (66886c0 § 4) : seule n32000 prouve
    # l'equivalence inter-fils ; n64000 grave une empreinte non appariee.
    claim = {"n32000": "thread_equivalence_checked",
             "court1h": "thread_equivalence_checked_t8_tmax",
             "n64000": "digest_recorded_unpaired"}[phase]
    print(f"campaign_status=complete (phase {phase}, {len(known)} runs, "
          f"{claim}, source_commit={commit[:12]})")
    print("=== CAMPAGNE SCALE_THREADS COMPLETE ===")
    return 0


if __name__ == "__main__":
    sys.exit(main())
