#!/usr/bin/env python3
"""Validation LOCALE de la campagne v4 (audits « campagne transactionnelle »
et « pin source ») : seule cette validation decide de campaign_status —
jamais le lanceur, jamais la VM. `complete` exige l'ensemble exact des
fichiers, code=0 partout, la ligne de compteurs presente, aucun motif
interdit, le MEME pin source (commit + sha256 du tar) dans chaque statut,
et des codes de session (ssh distant, scp) nuls.

Usage : validate_v4_campaign.py OUT_DIR SOURCE_COMMIT SOURCE_TAR_SHA256 \
        REMOTE_CAMPAIGN_RC SCP_RC
Sortie : 0 si complete, 1 sinon (les preuves partielles restent sur place).
"""
import os
import re
import sys


def expected_names():
    names = ["lat_uniform_n8000_smax11_rep1", "lat_uniform_n8000_smax11_rep2",
             "lat_uniform_n16000_smax11", "lat_uniform_n32000_smax11"]
    for fam in ("uniform", "terrain", "eight_clusters",
                "scanline_overlap_multiecho"):
        for n in (8000, 16000, 32000):
            for smax in (11, 6):
                names.append(f"cov_{fam}_n{n}_smax{smax}")
    return names


def main():
    out, commit, tar_sha, remote_rc, scp_rc = sys.argv[1:6]
    forbidden = re.compile(r"REFUS|INVARIANT|PLANCHER|Killed|bad_alloc")
    counters = re.compile(r"boules_uniques=\d+.*evenements=\d+.*juge=off desaccords=NA")
    bad = []
    if remote_rc != "0":
        bad.append(f"session distante : remote_campaign_rc={remote_rc}")
    if scp_rc != "0":
        bad.append(f"rapatriement : scp_rc={scp_rc}")
    for name in expected_names():
        txt = os.path.join(out, name + ".txt")
        status = os.path.join(out, name + ".status")
        if not os.path.exists(status):
            bad.append(f"{name}: .status ABSENT")
            continue
        st = open(status).read()
        if "finished=1" not in st:
            bad.append(f"{name}: status incomplet")
        m = re.search(r"^code=(\d+)$", st, re.M)
        if not m or m.group(1) != "0":
            bad.append(f"{name}: code={m.group(1) if m else '?'}")
        for field, want in (("source_commit", commit),
                            ("source_tar_sha256", tar_sha)):
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
        if not counters.search(body):
            bad.append(f"{name}: ligne de compteurs absente ou juge ambigu")
    known = set(expected_names())
    for f in sorted(os.listdir(out)):
        if f.endswith(".txt") and f[:-4] not in known:
            bad.append(f"{f}: fichier inattendu")
    if bad:
        print("campaign_status=partial_or_failed")
        for b in bad:
            print("  -", b)
        return 1
    print(f"campaign_status=complete ({len(known)} runs valides, "
          f"source_commit={commit[:12]})")
    print("=== CAMPAGNE COMPLETE ===")
    return 0


if __name__ == "__main__":
    sys.exit(main())
