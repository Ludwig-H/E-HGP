#!/usr/bin/env python3
"""Validation LOCALE de la campagne v5 : seule cette validation decide de
campaign_status — jamais le lanceur, jamais la VM. `complete` exige l'ensemble
exact des fichiers, code=0 partout, la ligne de compteurs v5 presente, la
conformite v4 (`balls=egal all=egal`) sur chaque run de phase 1, un digest_all
sur chaque run de phase 2, aucun motif interdit, le MEME pin (commit, sha256 du
payload, manifeste du protocole) dans chaque statut, et des codes de session
(ssh distant, scp) nuls.

Usage : validate_v5_campaign.py OUT_DIR SOURCE_COMMIT SOURCE_PAYLOAD_SHA256 \\
        PROTOCOL_MANIFEST_SHA256 REMOTE_CAMPAIGN_RC SCP_RC
Sortie : 0 si complete, 1 sinon (les preuves partielles restent sur place).
"""
import os
import re
import sys

FAMILIES = ("uniform", "terrain", "eight_clusters", "scanline_single_pass")


def expected_names():
    names = ["gpu_witness", "gpu_lane"]
    for n in (8000, 16000, 32000):
        for fam in FAMILIES:
            names.append(f"conf_{fam}_n{n}")
    for fam in FAMILIES:
        names.append(f"contrat_{fam}_n50000")
    for fam in FAMILIES:
        names.append(f"contrat_gpu_{fam}_n50000")
    return names


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
        if name not in ("gpu_witness", "gpu_lane") and not counters.search(body):
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
        if name.startswith("contrat_gpu_"):
            if not re.search(r"^gpu=1 kernel_ms=", body, re.M):
                bad.append(f"{name}: le run n'annonce pas gpu=1")
            cpu_txt = os.path.join(out, name.replace("contrat_gpu_", "contrat_") + ".txt")
            d_gpu = re.search(r"^digest_all=([0-9a-f]{64})$", body, re.M)
            d_cpu = None
            if os.path.exists(cpu_txt):
                d_cpu = re.search(r"^digest_all=([0-9a-f]{64})$", open(cpu_txt, encoding="utf-8", errors="replace").read(), re.M)
            if not d_gpu or not d_cpu or d_gpu.group(1) != d_cpu.group(1):
                bad.append(f"{name}: digest_all DIFFERENT du contrat CPU de la meme famille (ou absent)")
        if name == "gpu_lane":
            # Triples EXACTS famille/taille/fils, planchers (100000 candidats a
            # 8 k, seeds et morts non vides, lancements > 0), desaccords = 0,
            # et une occurrence unique de chaque triple.
            want = {("uniform", "1200", "1"): 200, ("eight_clusters", "1200", "4"): 200, ("uniform", "8000", "8"): 100000}
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
                if body.count(f"{lane} OK") != 3:
                    bad.append(f"{name}: {lane} — trois OK attendus, {body.count(lane + ' OK')} vus")
        if name.startswith("conf_") and not conformity.search(body):
            bad.append(f"{name}: conformite v4 non etablie")
        if name.startswith("contrat_") and not digest.search(body):
            bad.append(f"{name}: digest_all absent")
    known = set(expected_names())
    for f in sorted(os.listdir(out)):
        if f.endswith(".txt") and f[:-4] not in known:
            bad.append(f"{f}: fichier inattendu")
    if bad:
        print("campaign_status=partial_or_failed")
        for b in bad:
            print("  -", b)
        return 1
    print(f"campaign_status=complete ({len(known)} runs valides, source_commit={commit[:12]})")
    print("=== CAMPAGNE COMPLETE ===")
    return 0


if __name__ == "__main__":
    sys.exit(main())
