#!/usr/bin/env python3
"""Contre-jugement strict du reçu M2 public, avec ses petites archives brutes locales.

N'ouvre aucun nuage ni dump .bin. Le seul git archive reconstruit des sources v11.
--published-only rejoue les contrôles publics et indique explicitement la limite.
"""
import argparse
import hashlib
import io
import itertools
import json
import math
from pathlib import Path
import random
import re
import shlex
import statistics
import subprocess
import tarfile

PIN="4147c546000b198b5239646063bfb1e3ed6d28fc"
ROOT=Path(__file__).resolve().parents[4]
RECEIPT="morsehgp3D_v12/receipts/g4_t0a_20261007"
M2=RECEIPT+"/resultats/cmd/004_m2_publier/files/m2"
SOURCE="morsehgp3D_v12/microbancs/mes_m2_feuille/"
HASHES={}
FRAMES=("ng00","ng01","ng02")
CONFIGS=((5,16),(5,24),(10,24))
FORMS=("witness","j3","j3_r168","j3_r128","coherent","coherent_r168","coherent_r128")
CASES=sorted(f"{f}_k{k}_l{leaf}" for f in FRAMES for k,leaf in CONFIGS)


def need(ok,reason):
    if not ok:
        raise ValueError(reason)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def pinned(path,commit=PIN):
    data=subprocess.check_output(["git","show",f"{commit}:{path}"],cwd=ROOT)
    if commit==PIN:
        need((ROOT/path).read_bytes()==data,"worktree differs from pin: "+path)
        HASHES[path]=sha(data)
    return data


def load(path):
    return json.loads(pinned(path))


def clean(text):
    return re.sub(r"/home/[A-Za-z0-9_.-]+","$HOME",text)


def geom(values):
    return math.exp(sum(math.log(x) for x in values)/len(values))


def case_numbers(name):
    frame,k,leaf=name.split("_")
    return int(k[1:]),int(leaf[1:])


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--published-only",action="store_true")
    args=ap.parse_args()
    s=load(RECEIPT+"/receipt.json")
    report=load(M2+"/report.json")
    # Every listed public artifact exists, is versioned at the audit pin and matches.
    manifest=pinned(RECEIPT+"/SHA256SUMS").decode().splitlines()
    for line in manifest:
        expected,name=line.split(None,1)
        need(sha(pinned(RECEIPT+"/"+name))==expected,"public receipt checksum: "+name)
    need(len(manifest)==73,"public manifest count changed")
    need(report["args"]["frames"]==",".join(FRAMES) and
         report["args"]["configs"]=="5:16,5:24,10:24" and
         report["args"]["decide"]=="5:24,10:24" and
         report["args"]["forms"]==",".join(FORMS),"unexpected campaign plan")
    need(report["args"]["processes"]==5 and report["args"]["reps"]==15 and
         report["args"]["warmup"]==3,"process/repetition/warmup plan")
    need(not report["args"]["skip_identity"] and not report["args"]["skip_sanitizer"],"proof skipped")
    need(report["gpu_isolation"] and report["environment"]["gpu_apps"]=="","GPU isolation record")
    need(report["refusals"]==[] and report["choice"]=="j3_r168","recorded verdict context")
    steps={row["step"]:row for row in report["steps"]}
    need(len(steps)==len(report["steps"])==72 and
         all(row["code"]==(128 if name=="git_head" else 0) for name,row in steps.items()),
         "missing/failed/duplicate execution step")

    source_checks=[]
    for name,expected in report["hashes"].items():
        if name.startswith("src/"):
            path=SOURCE+name[4:]
            need(sha(pinned(path))==expected,"measured M2 source differs: "+name)
            need(sha(pinned(path,"26b53648c"))==expected,"M2 source not at declared snapshot head: "+name)
            source_checks.append(dict(path=path,sha256=expected))
        elif name.startswith("v11/"):
            path="morsehgp3D_v11/"+name[4:]
            need(sha(pinned(path,"ac081a06f"))==expected,"v11 reference source mismatch: "+name)
            source_checks.append(dict(path=path,sha256=expected))
        else:
            need(re.fullmatch(r"[0-9a-f]{64}",expected),"binary hash missing")
    archive_paths=["CMakeLists.txt","cmake","src","bench","tests","reference","tools","cli"]
    source_archive=subprocess.check_output(["git","archive","--format=tar.gz","ac081a06f"]+
                                          ["morsehgp3D_v11/"+x for x in archive_paths],cwd=ROOT)
    declared_archive=next(x for x in s["data_files"] if x["name"]=="v11_src_ac081a06f.tar.gz")
    need(sha(source_archive)==declared_archive["sha256"] and len(source_archive)==declared_archive["size"],
         "frozen v11 source archive mismatch")
    source_receipt=load(RECEIPT+"/resultats/cmd/000_source_v11/files/source_v11.json")
    need(source_receipt["archive_sha256"]==declared_archive["sha256"] and source_receipt["files"]==644 and
         source_receipt["configure"][0]==source_receipt["build"][0]==0,"source extraction/build")
    old_local=pinned("morsehgp3D_v12/receipts/mes_m2_local_20261007/RAPPORT.md").decode()
    data_manifest={x["name"]:x["sha256"] for x in s["data_files"]}
    for name,expected in report["data"].items():
        need(data_manifest[name]==expected,"input hash disagrees with session manifest: "+name)
    for name,expected in report["dump_hashes"].items():
        need(f"| {name[:-4]} | `{expected}` |" in old_local,"dump differs from local reference: "+name)

    need(sorted(report["bench"])==CASES and sorted(report["dumps_made"])==CASES,"case set incomplete")
    host=report["identity_host"]
    need(host["code"]==0 and len(host["results"])==18,"host identity gate")
    host_rows={}
    for row in host["results"]:
        name=Path(row["dump"]).stem
        need((name,row["form"]) not in host_rows,"duplicate host proof")
        need(row["identity"] and row["unresolved"]==row["mismatched_counts"]==row["mismatched_emissions"]==0,
             "host identity false/incomplete")
        need(row["resolved"]==row["leaves"],"host partial resolution")
        host_rows[name,row["form"]]=row
    need(set(host_rows)==set(itertools.product(CASES,("j3","coherent"))),"host proof coverage")
    arena=report["arena_selftest"]
    need(arena["code"]==0 and len(arena["results"])==1 and arena["results"][0]["identity"] and
         arena["results"][0]["mutants_vivants"]==0,"arena verification gate")

    raw_runs={};binary_medians={};raw_hashes=set();timings=0
    for name in CASES:
        k,leaf=case_numbers(name)
        d=report["dumps_made"][name]
        need(d["code"]==0 and d["summary"]["coord_bits"]==21 and
             (d["summary"]["kmax"],d["summary"]["leaf_size"])==(k,leaf),"dump case header")
        summary=d["summary"]
        need(summary["v11_device_unresolved"]==summary["v11_device_mismatch"]==0,"reference dump discrepancy")
        for form in ("j3","coherent"):
            row=host_rows[name,form]
            need(row["leaves"]==summary["dumped_leaves"] and row["emissions"]==summary["records"] and
                 row["population"]==summary["population"],"host proof/input size disagreement")
        block=report["bench"][name]
        need(not block["failures"] and sorted(x["process"] for x in block["runs"])==list(range(5)),"process grid")
        by_process={row["process"]:row for row in block["runs"]}
        raw_runs[name]=[]
        for process in range(5):
            path=M2+f"/runs/{name}_p{process}.json"
            raw=load(path);raw_hashes.add(HASHES[path])
            need(raw["bench"]=="mhgp12_leaf_bench" and raw["identity"] and raw["reps"]==15 and
                 raw["warmup"]==3 and raw["cc"]=="12.0" and len(raw["cases"])==1,"raw result header")
            c=raw["cases"][0]
            need(Path(c["dump"]).stem==name and (c["kmax"],c["leaf_size"])==(k,leaf),"raw result/command case")
            need(c["leaves"]==summary["dumped_leaves"] and c["reference_records"]==summary["records"] and
                 c["reference_population"]==summary["population"],"raw result/input cardinalities")
            forms={f["form"]:f for f in c["forms"]}
            need(len(c["forms"])==7 and set(forms)==set(FORMS),"forms incomplete or duplicated")
            need(forms==by_process[process]["forms"] and by_process[process]["code"]==0 and
                 by_process[process]["witness_identity"],"report differs from per-process raw result")
            for form,value in forms.items():
                need(value["identity"] and not value["overflow"] and value["unresolved"]==0 and
                     value["mismatched_counts"]==value["mismatched_emissions"]==0,"GPU identity mismatch")
                need(value["records"]==summary["records"] and value["population"]==summary["population"],"GPU arena size")
                need(value["totals"]==forms["witness"]["totals"],"logical counters disagree")
                need(len(value["ms"])==15 and all(type(t) in (int,float) and math.isfinite(t) and t>0
                                                 for t in value["ms"]),"invalid/missing timings")
                need(abs(statistics.median(value["ms"])-value["median_ms"])<=0.0011,"reported median")
                timings+=len(value["ms"])
                binary_medians[name,process,form]=statistics.median(value["ms"])
            raw_runs[name].append(raw)
    need(len(raw_hashes)==45 and timings==4725,"duplicate/missing raw process payload")

    rng=random.Random(20261007);statistics_out={}
    for form in FORMS[1:]:
        cases={}
        for name in CASES:
            ratios=[binary_medians[name,p,form]/binary_medians[name,p,"witness"] for p in range(5)]
            logs=[math.log(x) for x in ratios]
            draws=sorted(sum(logs[rng.randrange(5)] for _ in range(5))/5 for _ in range(10000))
            ci=[math.exp(draws[250]),math.exp(draws[9749])]
            # Independent exact distribution of the nonparametric bootstrap (5^5 samples).
            exact=sorted(sum(sample)/5 for sample in itertools.product(logs,repeat=5))
            exact_hi=math.exp(exact[math.ceil(.975*len(exact))-1])
            reference=report["verdicts"][form]["cases"][name]
            gm=geom(ratios)
            need(abs(gm-reference["ratio_gm"])<1e-12 and
                 all(abs(a-b)<1e-12 for a,b in zip(ci,reference["ci95"])),"statistic replay differs")
            deciding=case_numbers(name)[1]==24
            need(reference["deciding"]==deciding and reference["processes"]==5 and reference["identity"],"case decision metadata")
            cases[name]=dict(ratio_gm=gm,ci95=ci,exact_bootstrap_hi=exact_hi,deciding=deciding,
                             median_ms=statistics.median(binary_medians[name,p,form] for p in range(5)),
                             witness_median_ms=statistics.median(binary_medians[name,p,"witness"] for p in range(5)))
        deciding=[x for x in cases.values() if x["deciding"]]
        verdict="adopte" if all(x["ci95"][1]<=1/3 for x in deciding) else "rejete"
        overall=geom([x["ratio_gm"] for x in deciding])
        need(verdict==report["verdicts"][form]["verdict"] and
             abs(overall-report["verdicts"][form]["ratio_gm_all_cases"])<1e-12,"variant verdict")
        statistics_out[form]=dict(verdict=verdict,ratio_gm=overall,
                                  worst_hi=max(x["ci95"][1] for x in deciding),cases=cases,
                                  exact_bootstrap_same_verdict=(all(x["exact_bootstrap_hi"]<=1/3 for x in deciding)
                                                               ==(verdict=="adopte")))
    choice=min((f for f in statistics_out if statistics_out[f]["verdict"]=="adopte"),
               key=lambda f:statistics_out[f]["ratio_gm"])
    need(choice=="j3_r168","strict choice changed")
    need(all(v["exact_bootstrap_same_verdict"] for v in statistics_out.values()),
         "exact bootstrap disagrees with published adoption")
    for tool in ("memcheck","racecheck","synccheck"):
        san=load(M2+f"/sanitizer_{tool}.json")
        need(report["sanitizer"][tool]["code"]==0 and san["identity"] and san["reps"]==1 and san["warmup"]==0,
             "sanitizer execution failed")
        need(len(san["cases"])==1 and san["cases"][0]["leaves"]==3000 and
             Path(san["cases"][0]["dump"]).stem=="ng00_k5_l24" and
             {f["form"] for f in san["cases"][0]["forms"]}==set(FORMS[1:]),"sanitizer scope")
        need(all(f["identity"] and not f["overflow"] and f["unresolved"]==0 for f in san["cases"][0]["forms"]),
             "sanitizer result identity")

    raw_checks=dict(available=False,reason="published-only requested")
    if not args.published_only:
        session=Path(s["receipt_path"]).parent
        need(session.is_dir(),"original local session unavailable; use --published-only for limited replay")
        result_archive=(session/"results/results.tar.gz").read_bytes()
        package_archive=(session/"package/package.tar.gz").read_bytes()
        need(sha(result_archive)==s["results_sha256"],"raw results archive hash")
        need(sha(package_archive)==s["package_sha256"],"raw source package hash")
        plan_bytes=(session/"package/plan.json").read_bytes()
        need(sha(plan_bytes)==s["plan_sha256"] and sha((session/"package/plan.sh").read_bytes())==s["worker_plan_sha256"],
             "plan JSON/shell fingerprint")
        plan=json.loads(plan_bytes)
        command=next(c for c in plan["commands"] if c["name"]=="m2")
        need(command["argv"]==["python3","{src}/"+SOURCE+"scripts/g4_leaf_bench.py","--out","{build}/m2",
                                "--data","{data}","--repo","{build}/v11src","--jobs","44"],"M2 planned command")
        with tarfile.open(fileobj=io.BytesIO(package_archive),mode="r:gz") as package:
            members={m.name:m for m in package.getmembers() if m.isfile()}
            m2files=[row for row in s["source"]["manifest"] if row["path"].startswith(SOURCE)]
            for row in m2files:
                data=package.extractfile(members[row["path"]]).read()
                need(sha(data)==row["sha256"] and data==pinned(row["path"]),"measured package/source discrepancy")
            need(not any(n.startswith(SOURCE) and any(x in n.split("/") for x in ("runs","results","build","dumps"))
                         for n in members),"pre-existing M2 outputs in source package")
        with tarfile.open(fileobj=io.BytesIO(result_archive),mode="r:gz") as archive:
            members={m.name:m for m in archive.getmembers() if m.isfile()}
            prefix="results/cmd/004_m2_publier/files/m2/"
            def text_file(relative):
                return clean(archive.extractfile(members[prefix+relative]).read().decode())
            public_copies=0
            for path in sorted((ROOT/M2).rglob("*.json")):
                relative=path.relative_to(ROOT/M2).as_posix()
                need(text_file(relative).encode()==pinned(M2+"/"+relative),"published/raw mismatch: "+relative)
                public_copies+=1
            for name in CASES:
                dumped=[json.loads(line) for line in text_file(f"logs/dump_{name}.log").splitlines()
                        if line.startswith("{")]
                need(dumped==[report["dumps_made"][name]["summary"]],"dump raw summary differs")
            for stage,key in (("identity_host","identity_host"),("arena_selftest","arena_selftest")):
                rows=[json.loads(line) for line in text_file(f"logs/{stage}.log").splitlines()
                      if line.startswith("{")]
                need(rows==report[key]["results"],"host raw proof differs from report")
            for name in CASES:
                for p in range(5):
                    lines=text_file(f"logs/bench_{name}_p{p}.log").splitlines()
                    argv=shlex.split(lines[0][2:])
                    need(argv==[report["args"]["out"]+"/work/b_feuille/mhgp12_leaf_bench","--dump",
                                 report["args"]["out"]+f"/dumps/{name}.bin","--forms",",".join(FORMS),
                                 "--reps","15","--warmup","3","--json",
                                 report["args"]["out"]+f"/runs/{name}_p{p}.json"],"actual command does not match raw case")
                    summaries=re.findall(r"(witness|j3(?:_r\d+)?|coherent(?:_r\d+)?) ([0-9.]+) ms \(identite\)","\n".join(lines[1:]))
                    need(len(summaries)==7 and {x[0] for x in summaries}==set(FORMS),"process stderr identity summary")
                    for form,value in summaries:
                        need(abs(float(value)-binary_medians[name,p,form])<=0.0011,"process stderr timing differs from result")
            log_count=sum(name.startswith(prefix+"logs/") for name in members)
            for tool in ("memcheck","racecheck","synccheck"):
                text=text_file(f"logs/sanitizer_{tool}.log")
                marker="0 errors, 0 warnings" if tool=="racecheck" else "ERROR SUMMARY: 0 errors"
                need(marker in text and "--leaves 3000" in text and "--error-exitcode 9" in text,"raw sanitizer proof")
            raw_checks=dict(available=True,results_archive_sha256=sha(result_archive),
                            source_package_sha256=sha(package_archive),plan_sha256=sha(plan_bytes),
                            published_json_copies_equal_after_home_redaction=public_copies,
                            source_m2_files_matched=len(m2files),process_commands_verified=45,
                            process_stderr_medians_reconciled=315,m2_log_files=log_count,
                            dump_summaries_reconciled=9,host_raw_identity_rows_reconciled=18,
                            archived_run_mtimes=sorted({members[n].mtime for n in members if n.startswith(prefix+"runs/")}),
                            freshness_limit="publication uses copyfile; mtimes are publication times, not run creation times")
    result=dict(schema="ehgp.v12.audit_actual_m2_campaign.v1",pin=PIN,
                source_snapshot_head=s["source"]["head_commit"],source_manifest_sha256=s["source"]["manifest_sha256"],
                public_receipt_files_verified=len(manifest),source_hashes_checked=source_checks,
                binaries_sha256={k:v for k,v in report["hashes"].items() if not k.startswith(("src/","v11/"))},
                v11_archive_reproduced=declared_archive,repo_head_is_none=report["repo_head"] is None,
                case_count=9,deciding_cases=6,processes=45,forms_per_process=7,timings=timings,
                distinct_run_hashes=len(raw_hashes),host_identity_rows=18,
                leaves_per_host_form=sum(host_rows[name,"j3"]["leaves"] for name in CASES),
                gpu_identity_records=315,unresolved=0,source_and_dump_checks=True,
                sanitizer_scope=dict(case="ng00_k5_l24",leaves=3000,forms=6,tools=3),
                statistics=statistics_out,choice=choice,raw_evidence=raw_checks,
                generic_cst0018_fixed=False,gcp_used_by_audit=False,
                key_input_sha256={p:HASHES[p] for p in (RECEIPT+"/receipt.json",RECEIPT+"/SHA256SUMS",M2+"/report.json",
                                                       SOURCE+"scripts/g4_leaf_bench.py")})
    print(json.dumps(result,ensure_ascii=False,indent=2,sort_keys=True,allow_nan=False))


if __name__=="__main__":
    main()
