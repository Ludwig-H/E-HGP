#!/usr/bin/env python3
"""Counter-read of R21 receipt: per-case table, arm classification, raw-output consistency."""
import hashlib, json, os, sys

R = "/workspaces/E-HGP/build/v9-audit-c-publish/morsehgp3D_v9/receipts/g4_tower_r21_20260925"
S = json.load(open(os.path.join(R, "SUMMARY.json")))
P = json.load(open(os.path.join(R, "PACKAGE.json")))

GPU_LEVERS = ["q34_batch_filter", "q34_gpu_filter", "q34_batch_certificates", "q34_gpu_certificates",
              "q34_batch_q3", "q34_gpu_q3", "q34_batch_q4", "q2_during_device"]
TOWER_LEVERS = ["tower_pipelined_tail", "tower_hash_grouping", "tower_persistent_pool"]


def arm(lv):
    gpu = [lv[k] for k in GPU_LEVERS]
    tow = [lv[k] for k in TOWER_LEVERS]
    ds = lv["device_session"]
    if not any(gpu):
        a = "engine"
    elif all(gpu):
        a = "gpu"
    else:
        a = "gpu_partial"
    if a == "gpu" and not ds:
        a = "gpu_cold"
    if not all(tow):
        a += ("+tower_witness" if not any(tow) else "+tower_partial")
    return a


problems = []
rows = []
for i, row in enumerate(S["rows"]):
    case = row["case"]
    idx = int(case.split("_")[1])
    cmd = json.load(open(os.path.join(R, "vm", case + ".command.json")))
    out_raw = open(os.path.join(R, "vm", case + ".stdout"), "rb").read()
    err_raw = open(os.path.join(R, "vm", case + ".stderr"), "rb").read()
    if hashlib.sha256(out_raw).hexdigest() != cmd.get("stdout_sha256"):
        problems.append(f"{case}: stdout sha256 != command.json")
    if hashlib.sha256(err_raw).hexdigest() != cmd.get("stderr_sha256"):
        problems.append(f"{case}: stderr sha256 != command.json")
    if cmd.get("exit_code") != 0:
        problems.append(f"{case}: exit_code {cmd.get('exit_code')}")
    out = json.loads(out_raw)
    summ = json.load(open(os.path.join(R, "vm", case + ".summary.json")))
    pc = P["cases"][idx]
    argv = cmd["argv"]
    infile = [a for a in argv if a.endswith(".u32le")][0].split("/")[-1]
    kk = int(argv[argv.index([a for a in argv if a.endswith('.u32le')][0]) + 1])
    lv_argv = {a.split("=")[1]: a.split("=")[2] == "1" for a in argv if a.startswith("--lever=")}
    lv_out = out["options"]["levers"]
    if lv_argv != lv_out or lv_out != row["levers"] or lv_out != pc["levers"]:
        problems.append(f"{case}: lever mismatch argv/out/row/package")
    if pc["file"].split("/")[-1] != infile or pc["k"] != kk or row["K"] != kk:
        problems.append(f"{case}: file/K mismatch pkg={pc['file']},{pc['k']} argv={infile},{kk} row={row['K']}")
    # consistency of SUMMARY row vs stdout
    t = out["times_ms"]
    checks = {
        "chain_s": round(t["chain_total"] / 1000, 3),
        "tower_s": round(t["tower"] / 1000, 3),
        "q2_s": round(t["q2"] / 1000, 3),
        "q34_s": round(t["q34"] / 1000, 3),
        "census_s": round(t["census"] / 1000, 3),
        "digest_s": round(t["digest"] / 1000, 3),
        "catalogue_digest_s": round(t["catalogue_digest"] / 1000, 3),
        "cpu_s": out["chain_cpu_s"],
        "balls": out["catalogue"]["balls"],
        "euler_status": out["catalogue"]["euler"]["status"],
        "digest": out["tower_digest"],
        "catalogue_digest": out["catalogue_digest"],
        "outcome": out["status"],
        "sites": out["input"]["sites"],
        "rss_kb": out["peak_rss_kb"],
    }
    for k, v in checks.items():
        rv = row.get(k)
        if isinstance(v, float):
            if abs(rv - v) > 0.0015:
                problems.append(f"{case}: row.{k}={rv} stdout={v}")
        elif rv != v:
            problems.append(f"{case}: row.{k}={rv} stdout={v}")
    if row.get("q34_batch") != out.get("q34_batch"):
        problems.append(f"{case}: q34_batch differs row vs stdout")
    if row.get("tower_phases_ms") != out.get("tower_phases_ms"):
        problems.append(f"{case}: tower_phases differs")
    if row.get("tower_detail") != out.get("tower_detail"):
        problems.append(f"{case}: tower_detail differs")
    if row.get("device_session") != out.get("device_session"):
        problems.append(f"{case}: device_session differs")
    if summ["tower_digest"] != out["tower_digest"] or summ["outcome"] != out["status"]:
        problems.append(f"{case}: per-case summary.json differs")
    if abs(summ["chain_total_ms"] - t["chain_total"]) > 1e-6:
        problems.append(f"{case}: summary chain_total")
    # GNU time rss in stderr
    import re
    m = re.search(rb"Maximum resident set size \(kbytes\): (\d+)", err_raw)
    if m and int(m.group(1)) != row["gnu_time_max_rss_kb"]:
        problems.append(f"{case}: gnu time rss {m.group(1)} vs {row['gnu_time_max_rss_kb']}")
    qb = out.get("q34_batch") or {}
    rows.append(dict(
        idx=idx, case=case, frame=row["frame"], file=infile, K=kk, arm=arm(lv_out), rep=row["repeat"],
        outcome=row["outcome"], euler=row["euler_status"], ekmax=out["catalogue"]["euler"].get("checkable_max_k"),
        chain=t["chain_total"], tower=t["tower"], q2=t["q2"], q34=t["q34"], census=t["census"],
        merge=t["merge"], gen_index=t["gen_index"], tower_index=t["tower_index"], read=t["read"], prepare=t["prepare"],
        q2_wait=t["q2_wait"],
        digest=out["tower_digest"], cat=out["catalogue_digest"],
        pres=out.get("presentation_digest"), balls=out["catalogue"]["balls"], sites=out["input"]["sites"],
        ihash=out["input"]["hash"], rss=out["peak_rss_kb"], ds=out.get("device_session"),
        used=qb.get("used"), deferred=qb.get("deferred"), lanes_deferred=qb.get("lanes_deferred"),
        judged=qb.get("judged_edges"), lanes_judged=qb.get("lanes_judged"),
        front=qb.get("front_ms"), filt=qb.get("filter_ms"), cert=qb.get("certificate_ms"), lanes=qb.get("lanes_ms"),
        lanes_tr=qb.get("lanes_transfer_ms"), prep_wait=qb.get("gpu_prepare_wait_ms"), prep=qb.get("gpu_prepare_ms"),
        wall=row["wall_s"], started=cmd["started_utc"], ended=cmd.get("ended_utc"),
    ))

json.dump(rows, open(os.path.join(os.path.dirname(__file__), "rows.json"), "w"), indent=1)
print("problems:", len(problems))
for p in problems:
    print("  ", p)
hdr = "idx file K arm rep outcome euler(k) chain tower q34 census digest cat pres balls rss_GiB defer/ldefer ds_ctx"
print(hdr)
for r in sorted(rows, key=lambda r: r["idx"]):
    ds = r["ds"]["context_ms"] if r["ds"] and r["ds"].get("opened") else "-"
    print(f"{r['idx']:2d} {r['file'][6:-6]:4s} {r['K']:2d} {r['arm']:24s} {r['rep']} {r['outcome']} {r['euler']}({r['ekmax']}) "
          f"{r['chain']:8.1f} {r['tower']:7.1f} {r['q34']:7.1f} {r['census']:6.1f} {r['digest']} {r['cat']} {r['pres']} "
          f"{r['balls']:9d} {r['rss']/1048576:5.2f} {r['deferred']}/{r['lanes_deferred']} {ds}")
