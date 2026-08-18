#!/usr/bin/env python3
# Validateur LOCAL des sessions SCALE_THREADS (audits bloquants 9223888 /
# b3a6eb4) : seule autorite du statut. Au-dela des exigences historiques
# (statuts finis, code=0, pins, motifs interdits), il exige :
#   - les METADONNEES d'execution : threads_requested coherent avec le
#     suffixe du nom (t1/t8/tmax), nproc > 0, tmax == nproc, cpu_set et
#     args_sha256 presents, et la ligne `execution threads_effective=N`
#     du probe EGALE a threads_requested (le nom d'un run ne prouve pas
#     le nombre de fils reellement applique — audit § 2.1) ;
#   - la SIGNATURE CANONIQUE : lignes digest_balls / digest_forest_K* /
#     digest_all presentes, et EGALITE STRICTE de l'ensemble des lignes
#     digest + cardinalites entre les runs apparies d'une meme famille
#     (t1/t8/tmax a n=32000) — deux forets differentes a fils differents
#     ne peuvent plus etre declarees `complete` (audit § 2.2) ;
#   - aucun statut not_run_budget (un refus de budget est un resultat
#     honnete mais une campagne PARTIELLE).
# Usage : validate_v4_scale_threads.py OUT COMMIT PAYLOAD_SHA MANIFEST_SHA
#         REMOTE_RC SCP_RC PHASE
import os
import re
import sys


def expected(phase):
    if phase == "n32000":
        return {
            "uniform": ["thr_uniform_n32000_smax11_t1",
                        "thr_uniform_n32000_smax11_t8",
                        "thr_uniform_n32000_smax11_tmax"],
            "eight_clusters": ["thr_eight_clusters_n32000_smax11_t8",
                               "thr_eight_clusters_n32000_smax11_tmax"],
        }
    if phase == "n64000":
        return {fam: [f"thr_{fam}_n64000_smax11_tmax"]
                for fam in ("uniform", "terrain", "eight_clusters",
                            "scanline_overlap_multiecho")}
    raise SystemExit(f"phase inconnue : {phase}")


def main():
    out, commit, payload_sha, manifest_sha, remote_rc, scp_rc, phase = \
        sys.argv[1:8]
    groups = expected(phase)
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
        for name in names:
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
            suffix = name.rsplit("_", 1)[1]
            want_t = nproc_v if suffix == "tmax" else int(suffix[1:])
            if treq_v != want_t:
                bad.append(f"{name}: threads_requested={treq_v} incoherent "
                           f"avec le suffixe {suffix} (attendu {want_t})")
            if not re.search(r"^cpu_set=\S+$", st, re.M):
                bad.append(f"{name}: cpu_set absent")
            if not re.search(r"^args_sha256=[0-9a-f]{64}$", st, re.M):
                bad.append(f"{name}: args_sha256 absent")
            if not os.path.exists(txt_p):
                bad.append(f"{name}: .txt ABSENT")
                continue
            body = open(txt_p, errors="replace").read()
            fb = forbidden.search(body)
            if fb:
                bad.append(f"{name}: motif interdit ({fb.group(0)})")
            if not counters.search(body):
                bad.append(f"{name}: ligne de compteurs absente")
            te = re.search(r"^execution threads_effective=(\d+)$", body, re.M)
            if not te:
                bad.append(f"{name}: threads_effective absent de la sortie")
            elif int(te.group(1)) != treq_v:
                bad.append(f"{name}: threads_effective={te.group(1)} != "
                           f"threads_requested={treq_v}")
            digests = sorted(re.findall(
                r"^(digest_(?:balls|forest_K\d+|all)=[0-9a-f]{64})$", body,
                re.M))
            if len(digests) < 3:
                bad.append(f"{name}: lignes digest absentes ou incompletes")
            cards = sorted(re.findall(r"^(cardinalites K=\d+ .*)$", body, re.M))
            signatures[name] = (tuple(digests), tuple(cards))
    # Appariement : au sein d'une famille, TOUS les runs presents doivent
    # porter exactement les memes digests et les memes cardinalites.
    for fam, names in groups.items():
        sigs = [(n, signatures[n]) for n in names if n in signatures]
        for i in range(1, len(sigs)):
            if sigs[i][1] != sigs[0][1]:
                bad.append(f"{fam}: OBJETS DIFFERENTS entre {sigs[0][0]} et "
                           f"{sigs[i][0]} (digests ou cardinalites)")
    known = {n for names in groups.values() for n in names}
    for f in sorted(os.listdir(out)) if os.path.isdir(out) else []:
        if f.endswith(".txt") and f[:-4] not in known:
            bad.append(f"{f}: fichier inattendu")
    if bad:
        print("campaign_status=partial_or_failed")
        for b in bad:
            print("  -", b)
        return 1
    print(f"campaign_status=complete (phase {phase}, {len(known)} runs "
          f"apparies par digest, source_commit={commit[:12]})")
    print("=== CAMPAGNE SCALE_THREADS COMPLETE ===")
    return 0


if __name__ == "__main__":
    sys.exit(main())
