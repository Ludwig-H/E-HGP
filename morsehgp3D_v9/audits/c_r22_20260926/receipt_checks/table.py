#!/usr/bin/env python3
"""Counter-read of R22 receipt: per-case table, arm classification from argv, raw-output consistency."""
import hashlib, json, os, re

R = "/workspaces/E-HGP/build/v9-audit-c-publish/morsehgp3D_v9/receipts/g4_tower_r22_20260926"
OUT = os.path.dirname(os.path.abspath(__file__))
S = json.load(open(os.path.join(R, "SUMMARY.json")))
P = json.load(open(os.path.join(R, "PACKAGE.json")))

GPU_LEVERS = ["q34_batch_filter", "q34_gpu_filter", "q34_batch_certificates", "q34_gpu_certificates",
              "q34_batch_q3", "q34_gpu_q3", "q34_batch_q4", "q2_during_device", "device_session"]
TOWER_LEVERS = ["tower_pipelined_tail", "tower_hash_grouping", "tower_persistent_pool"]
NEW = ["tower_sealed_catalogue", "q2_early_census", "q34_lanes_pinned"]
ref_levers = None


def arm(lv):
    gpu = [lv[k] for k in GPU_LEVERS]
    if not any(gpu):
        a = "engine"
    elif all(gpu):
        a = "gpu"
    else:
        a = "gpu_partial(" + ",".join(k for k in GPU_LEVERS if not lv[k]) + ")"
    if not all(lv[k] for k in TOWER_LEVERS):
        a += "+tower_off"
    off = [k for k in NEW if not lv[k]]
    on = [k for k in NEW if lv[k]]
    if a.startswith("engine"):
        a += "[" + ",".join("+" + k for k in on) + "]"
    else:
        if len(off) == 3:
            a = "gpu_r21" if a == "gpu" else a + "-all3"
        elif off:
            a += "-" + ",".join(off)
    return a


problems = []
rows = []
all_lever_sets = {}
for row in S["rows"]:
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
    infile_arg = [a for a in argv if a.endswith(".u32le")][0]
    infile = infile_arg.split("/")[-1]
    kk = int(argv[argv.index(infile_arg) + 1])
    workers = int(argv[argv.index(infile_arg) + 2])
    lv_argv = {a.split("=")[1]: a.split("=")[2] == "1" for a in argv if a.startswith("--lever=")}
    lv_out = out["options"]["levers"]
    if lv_argv != lv_out or lv_out != row["levers"] or lv_out != pc["levers"]:
        problems.append(f"{case}: lever mismatch argv/out/row/package")
    all_lever_sets[idx] = lv_argv
    if pc["file"].split("/")[-1] != infile or pc["k"] != kk or row["K"] != kk:
        problems.append(f"{case}: file/K mismatch pkg={pc['file']},{pc['k']} argv={infile},{kk} row={row['K']}")
    if workers != 48 or "--s=8" not in argv or "--static=48" not in argv or "--grid=1mm" not in argv:
        problems.append(f"{case}: argv params {workers} {argv[3:9]}")
    t = out["times_ms"]
    checks = {
        "chain_s": round(t["chain_total"] / 1000, 3),
        "tower_s": round(t["tower"] / 1000, 3),
        "q2_s": round(t["q2"] / 1000, 3),
        "q34_s": round(t["q34"] / 1000, 3),
        "census_s": round(t["census"] / 1000, 3),
        "digest_s": round(t["digest"] / 1000, 3),
        "cpu_s": out["chain_cpu_s"],
        "balls": out["catalogue"]["balls"],
        "euler_status": out["catalogue"]["euler"]["status"],
        "outcome": out["status"],
        "sites": out["input"]["sites"],
        "rss_kb": out["peak_rss_kb"],
        "wall_s": round(cmd["elapsed_seconds"], 3),
    }
    for k, v in checks.items():
        rv = row.get(k)
        if rv is None:
            problems.append(f"{case}: row lacks {k}")
            continue
        if isinstance(v, float):
            if abs(rv - v) > 0.0015:
                problems.append(f"{case}: row.{k}={rv} stdout={v}")
        elif rv != v:
            problems.append(f"{case}: row.{k}={rv} stdout={v}")
    for k in ("q34_batch", "tower_phases_ms", "tower_detail", "device_session", "q34_occupancy"):
        if k in row and row.get(k) != out.get(k):
            problems.append(f"{case}: {k} differs row vs stdout")
    for k in ("tower_digest", "catalogue_digest", "presentation_digest"):
        if k in row and row[k] != out[k]:
            problems.append(f"{case}: row.{k}")
    if summ.get("tower_digest") != out["tower_digest"] or summ.get("outcome") != out["status"]:
        problems.append(f"{case}: per-case summary.json differs")
    if abs(summ.get("chain_total_ms", -1) - t["chain_total"]) > 1e-6:
        problems.append(f"{case}: summary chain_total")
    m = re.search(rb"Maximum resident set size \(kbytes\): (\d+)", err_raw)
    if m and int(m.group(1)) != row["gnu_time_max_rss_kb"]:
        problems.append(f"{case}: gnu time rss {m.group(1)} vs {row['gnu_time_max_rss_kb']}")
    qb = out.get("q34_batch") or {}
    td = out.get("tower_detail") or {}
    ds = out.get("device_session") or {}
    rows.append(dict(
        idx=idx, case=case, frame=row["frame"], file=infile, K=kk, arm=arm(lv_out), rep=row["repeat"], pkg_rep=pc["repeat"],
        outcome=row["outcome"], reason=out["reason"], euler=row["euler_status"], ekmax=out["catalogue"]["euler"].get("checkable_max_k"),
        eby=out["catalogue"]["euler"].get("by_k"),
        chain=t["chain_total"], tower=t["tower"], q2=t["q2"], q34=t["q34"], census=t["census"],
        merge=t["merge"], gen_index=t["gen_index"], tower_index=t["tower_index"], read=t["read"], prepare=t["prepare"],
        q2_wait=t["q2_wait"], q2_census=t.get("q2_census"), q2_census_index=t.get("q2_census_index"), q2_census_wait=t.get("q2_census_wait"),
        digest_ms=t["digest"], catdig_ms=t["catalogue_digest"],
        digest=out["tower_digest"], cat=out["catalogue_digest"],
        pres=out.get("presentation_digest"), balls=out["catalogue"]["balls"], sites=out["input"]["sites"],
        ihash=out["input"]["hash"], rss=out["peak_rss_kb"], gnu_rss=row["gnu_time_max_rss_kb"], ds=ds,
        used=qb.get("used"), deferred=qb.get("deferred"), lanes_deferred=qb.get("lanes_deferred"),
        judged=qb.get("judged_edges"), lanes_judged=qb.get("lanes_judged"),
        front=qb.get("front_ms"), filt=qb.get("filter_ms"), cert=qb.get("certificate_ms"), lanes=qb.get("lanes_ms"),
        lanes_tr=qb.get("lanes_transfer_ms"), lanes_dl=qb.get("lanes_download_ms"), lanes_copy=qb.get("lanes_download_copy_ms"),
        lanes_pinned=qb.get("lanes_pinned"), pinned_alloc=qb.get("lanes_pinned_allocations"), pinned_bytes=qb.get("lanes_pinned_bytes"),
        validate=(out.get("tower_phases_ms") or {}).get("validate"), vparts=(out.get("tower_phases_ms") or {}).get("validate_parts"),
        tp=out.get("tower_phases_ms"), td=td,
        sealed=td.get("sealed_catalogues"), sampled=td.get("seal_sampled_balls"), decl=td.get("declared_support_checks"),
        early_keys=out["catalogue"].get("early_census_keys"), q2_acc=out["generator"]["q2_accepted_pairs"],
        q2_pres=out["catalogue"]["q2_presentations"], regsup=out["catalogue"].get("regular_supports"),
        wall=cmd["elapsed_seconds"], started=cmd["started_utc"], ended=cmd.get("ended_utc"),
        schema=out["schema"], opts=out["options"],
    ))

rows.sort(key=lambda r: r["idx"])
json.dump(rows, open(os.path.join(OUT, "rows.json"), "w"), indent=1)
print("problems:", len(problems))
for p in problems:
    print("  ", p)
print("schemas:", {r["schema"] for r in rows}, "outcomes:", {r["outcome"] for r in rows}, "reasons:", {r["reason"] for r in rows})
hdr = "idx file K arm rep/pkg outcome euler(k) chain tower q34 census validate digest cat pres balls rss_GiB defer/ldefer sealed/sampled/decl"
print(hdr)
for r in rows:
    print(f"{r['idx']:2d} {r['file'][6:-6]:4s} {r['K']:2d} {r['arm']:50s} {r['rep']}/{r['pkg_rep']} {r['euler']}({r['ekmax']}) "
          f"{r['chain']:8.1f} {r['tower']:7.1f} {r['q34']:7.1f} {r['census']:6.1f} {r['validate']} {r['digest']} {r['cat']} {r['pres']} "
          f"{r['balls']:9d} {r['rss']/1048576:5.2f} {r['deferred']}/{r['lanes_deferred']} {r['sealed']}/{r['sampled']}/{r['decl']} {r['started'][11:19]}")
