#!/usr/bin/env python3
"""Recalcule seulement les comptes des reçus versionnés ; aucun nuage n'est ouvert."""
import ast
import hashlib
import json
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[4]
PIN="95247cf4baf2ebd0856c1ac75670f643d24daa6e"
BASE=ROOT/"morsehgp3D_v12"


def digest(path):
    data=path.read_bytes()
    pinned=subprocess.check_output(["git","show",f"{PIN}:{path.relative_to(ROOT).as_posix()}"],cwd=ROOT)
    require(data==pinned,"worktree receipt/source differs from audit pin: "+path.name)
    return hashlib.sha256(data).hexdigest()


def require(condition, reason):
    if not condition:
        raise ValueError(reason)


def main():
    checked=[];missing=[]
    for name in ("mes_m2_local_20261007","mes_m3_m4_local_20261007"):
        folder=BASE/"receipts"/name
        digest(folder/"SHA256SUMS")
        for line in (folder/"SHA256SUMS").read_text().splitlines():
            expected,relative=line.split(None,1)
            entry=dict(receipt=name,path=relative,sha256=expected)
            if not (folder/relative).is_file():
                missing.append(entry)
                continue
            require(digest(folder/relative)==expected,"receipt hash mismatch: "+relative)
            checked.append(entry)
    m2=BASE/"receipts/mes_m2_local_20261007"
    source_checks=[]
    for line in (m2/"results/SHA256SUMS.sources").read_text().splitlines():
        expected,name=line.split(None,1)
        path=m2/name if name=="RAPPORT.md" else BASE/"microbancs/mes_m2_feuille"/name
        require(digest(path)==expected,"M2 published source manifest mismatch: "+name)
        source_checks.append(dict(path=path.relative_to(ROOT).as_posix(),sha256=expected))
    identities=[json.loads(line) for line in (m2/"results/identity_host_v2.jsonl").read_text().splitlines()]
    by_form={}
    for row in identities:
        acc=by_form.setdefault(row["form"],dict(cases=0,leaves=0,resolved=0,unresolved=0,
                                               mismatched_counts=0,mismatched_emissions=0,identities=True))
        acc["cases"]+=1
        for key in ("leaves","resolved","unresolved","mismatched_counts","mismatched_emissions"):
            acc[key]+=row[key]
        acc["identities"] &= row["identity"]
    m34=BASE/"receipts/mes_m3_m4_local_20261007/out/rapport_mes_m3_m4.json"
    report=json.loads(m34.read_text())
    m3_cases={};m4_cases={}
    for case,block in report["mes_m3"].items():
        if case.startswith("variante"):
            continue
        passages=block["processus"] if "processus" in block else [block]
        lines=[row for row in passages[0]["lignes"] if row.get("phase")=="ordre"]
        m3_cases[case]=dict(processes=len(passages),orders=len(lines),parts=sum(row["parties"] for row in lines),
                           identity=all(row["identite"]["identiques"] for row in lines),
                           code=block["code"])
    for case,block in report["mes_m4"].items():
        if case.startswith("mutant"):
            continue
        passages=block["processus"] if "processus" in block else [block]
        lines=[row for row in passages[0]["lignes"] if row.get("phase")=="ordre"]
        m4_cases[case]=dict(processes=len(passages),orders=len(lines),births=sum(row["naissances"] for row in lines),
                           t6_births=sum(row["naissances"] for row in lines if row["k"]>=2),
                           identity=all(row["identite"]["identiques"] for row in lines),
                           t6_differences=sum(row["lem_t6"]["ecarts"] for row in lines),code=block["code"])
    require(all(v["identity"] and v["code"]==0 for v in m3_cases.values()),"M3 receipt not identity/zero")
    require(all(v["identity"] and v["code"]==0 for v in m4_cases.values()),"M4 receipt not identity/zero")
    resolution={case:dict(processes_stored=1,orders=sum(row.get("phase")=="resolution_un_fil"
                                                      for row in block["lignes"]),
                         full_equals_expected=block["ful1_identique"],
                         dump_files=len(block["fichiers"]),
                         full_sha256=block["ful1_sha256"])
                for case,block in report["vidages"].items()}
    driver=BASE/"microbancs/mes_m3_m4_tour/pilote.py"
    digest(driver)
    expected=next(ast.literal_eval(node.value) for node in ast.parse(driver.read_text()).body
                  if isinstance(node,ast.Assign) and any(isinstance(t,ast.Name) and t.id=="EMPREINTES"
                                                         for t in node.targets))
    for name,case in resolution.items():
        frame,k=name.rsplit("_k",1)
        require(case["full_equals_expected"] and case["full_sha256"]==expected[(frame,int(k))],
                "recorded FULL hash differs from pinned reference: "+name)
    paths=["microbancs/mes_m2_feuille/scripts/g4_leaf_bench.py",
           "microbancs/mes_m2_feuille/cuda/leaf_bench.cu",
           "microbancs/mes_m2_feuille/host/leaf_identity.cpp",
           "microbancs/mes_m2_feuille/README.md",
           "microbancs/mes_m3_m4_tour/pilote.py",
           "microbancs/mes_m3_m4_tour/vidage/vidage_v11.cpp",
           "microbancs/mes_m3_m4_tour/CMakeLists.txt",
           "microbancs/mes_m3_m4_tour/README.md",
           "docs/MESURE.md","docs/PLAN.md"]
    sources={}
    for relative in paths:
        path="morsehgp3D_v12/"+relative
        pinned=subprocess.check_output(["git","show",f"{PIN}:{path}"],cwd=ROOT)
        require((ROOT/path).read_bytes()==pinned,"worktree source diverged: "+path)
        sources[path]=hashlib.sha256(pinned).hexdigest()
    result=dict(schema="ehgp.v12.audit_microbench_receipts.v1",pin=PIN,source_sha256=sources,
                receipt_files_verified=checked,receipt_files_missing=missing,
                m2_source_manifest_verified=source_checks,m2_host_identity=by_form,
                m3_cases=m3_cases,m3_parts=sum(v["parts"] for v in m3_cases.values()),m4_cases=m4_cases,
                m4_births_all_orders=sum(v["births"] for v in m4_cases.values()),
                m4_t6_births=sum(v["t6_births"] for v in m4_cases.values()),resolution_cases=resolution,
                m34_report_sha256=digest(m34),m34_top_level_keys=sorted(report),
                m34_construction_fields=sorted(report["construction"]),
                m34_recorded_v11_library_sha256=report["construction"]["libmhgp11_sha256"],
                limits="Verifies versioned receipt integrity and recomputes claims; raw clouds/dumps and original binaries not opened.")
    print(json.dumps(result,ensure_ascii=False,indent=2,sort_keys=True))


if __name__=="__main__":
    main()
