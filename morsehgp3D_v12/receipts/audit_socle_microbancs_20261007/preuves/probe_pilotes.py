#!/usr/bin/env python3
"""Injection légère des frontières externes des pilotes épinglés, sans CUDA.

Les faux temps, binaires, noms et octets sont synthétiques. Les fonctions main,
judge, m3, m4, vider et synthese restent celles du dépôt au pin demandé.
"""
import contextlib
import copy
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import types

PIN = "95247cf4baf2ebd0856c1ac75670f643d24daa6e"
ROOT = Path(__file__).resolve().parents[4]
M2 = "morsehgp3D_v12/microbancs/mes_m2_feuille/scripts/g4_leaf_bench.py"
M34 = "morsehgp3D_v12/microbancs/mes_m3_m4_tour/pilote.py"


def need(value, message):
    if not value:
        raise ValueError(message)


def source(path):
    pinned = subprocess.check_output(["git", "show", f"{PIN}:{path}"], cwd=ROOT)
    need((ROOT/path).read_bytes() == pinned, "worktree source differs from audit pin: " + path)
    return pinned


def module(name, path):
    result = types.ModuleType(name)
    result.__file__ = str(ROOT / path)
    exec(compile(source(path), f"{PIN}:{path}", "exec"), result.__dict__)
    return result


def form(name, *, identity=True, ms=None):
    return dict(form=name, ms=[100.0 if name == "witness" else 10.0]*15 if ms is None else ms,
                identity=identity, unresolved=0)


def payload(dump, *, mismatched=False):
    return dict(bench="mhgp12_leaf_bench", device="SYNTHETIC_NO_GPU", reps=15, warmup=3,
                cases=[dict(dump="unrelated.bin" if mismatched else str(dump),
                            kmax=10 if mismatched else 5,
                            leaf_size=16 if mismatched else int(Path(dump).stem.rsplit("_l",1)[1]),
                            leaves=1, reference_records=1, reference_population=1,
                            forms=[form("witness"), form("j3")])], identity=True)


def probe_m2(kind):
    m = module("m2_" + kind, M2)
    with tempfile.TemporaryDirectory(prefix="audit-pilote-synth-") as tmp:
        tmp = Path(tmp)
        dumpdir, bdir, out = tmp/"dumps", tmp/"build", tmp/"out"
        dumpdir.mkdir(); bdir.mkdir(); out.mkdir()
        leaf = 16 if kind == "no_deciding_case" else 24
        dump = dumpdir / f"synthetic_k5_l{leaf}.bin"
        dump.write_bytes(b"SYNTHETIC_INPUT_NOT_A_REAL_DUMP")
        for name in ("mhgp12_leaf_identity", "mhgp12_mes_s", "mhgp12_arena_selftest", "mhgp12_leaf_bench"):
            (bdir/name).write_bytes(b"SYNTHETIC_BINARY_NOT_EXECUTED")
        if kind == "stale_unrelated_json":
            (out/"runs").mkdir()
            for i in range(5):
                (out/"runs"/f"{dump.stem}_p{i}.json").write_text(json.dumps(payload(dump, mismatched=True)))
        calls = []
        def run(self, name, cmd, timeout, **kwargs):
            calls.append(name)
            code, stdout = 0, ""
            if name == "git_head":
                stdout = PIN
            elif name == "identity_host" and kind == "host_identity_failure":
                code, stdout = 1, json.dumps(dict(identity=False, mismatched_emissions=1))
            elif name == "arena_selftest" and kind == "arena_failure_control":
                code = 1
            elif name.startswith("bench_") and name != "bench_discarded":
                target = Path(cmd[cmd.index("--json")+1])
                if kind == "bench_execution_failure_control":
                    code = 2
                elif kind != "stale_unrelated_json":
                    target.write_text(json.dumps(payload(dump)))
            self.steps.append(dict(step=name, code=code, seconds=0.0))
            return code, stdout, ""
        m.Session.run = run
        m.environment = lambda *args: dict(gpu_apps="", mock="external operations replaced; no GPU")
        m.find_nvcc = lambda *_: "/nonexistent/synthetic-cuda/bin/nvcc"
        m.build_feuille = lambda *args: bdir
        old_argv = sys.argv
        sys.argv = [M2, "--out", str(out), "--repo", str(ROOT), "--dumps", str(dumpdir),
                    "--frames", "synthetic", "--configs", f"5:{leaf}", "--forms", "witness,j3",
                    "--processes", "5", "--skip-sanitizer"]
        try:
            with contextlib.redirect_stdout(io.StringIO()):
                code = m.main()
        finally:
            sys.argv = old_argv
        report = json.loads((out/"report.json").read_text())
        return dict(probe=kind, driver_exit_code=code, host_identity_code=report["identity_host"]["code"],
                    verdict=report["verdicts"]["j3"], choice=report["choice"], refusals=report["refusals"],
                    bench_process_calls=sum(name.startswith("bench_") and name != "bench_discarded" for name in calls),
                    fresh_bench_json_writes=0 if kind in ("stale_unrelated_json", "bench_execution_failure_control") else 5,
                    stale_payload_metadata=dict(dump="unrelated.bin",kmax=10,leaf_size=16)
                      if kind=="stale_unrelated_json" else None,
                    cli_deciding_configs="5:24,10:24", configured_case=f"synthetic_k5_l{leaf}")


def probe_m2_nan():
    m = module("m2_nan", M2)
    run = dict(forms={"witness":form("witness"),"j3":form("j3",ms=[float("nan")])},witness_identity=True)
    verdicts,choice=m.judge({"synthetic_k5_l24":dict(runs=[copy.deepcopy(run) for _ in range(5)])},
                           ["witness","j3"],5)
    v=verdicts["j3"]
    return dict(probe="nonfinite_timings",verdict=v["verdict"],choice=choice,
                ratio_is_nan=str(v["ratio_gm_all_cases"])=="nan",reason_refused=v["refused"])


def m34_arguments(tmp):
    return types.SimpleNamespace(sortie=str(tmp/"out"),construction=str(tmp/"build"),cas="synthetic:10",
                                 processus=5,repetitions_k5=3,repetitions_k10=1,repetitions_m4=5,
                                 fils_contraction=1,donnees=str(tmp/"data"),fils=1,chrono_k5=3,chrono_k10=1,
                                 journal="tous",garder_ful1=True)


def probe_m34_empty():
    m=module("m34_empty",M34)
    with tempfile.TemporaryDirectory(prefix="audit-pilote-synth-") as tmp:
        tmp=Path(tmp); args=m34_arguments(tmp)
        (tmp/"build").mkdir(); (tmp/"out"/"synthetic_k10").mkdir(parents=True)
        scripts={"mhgp12_mes_m3":0,"mhgp12_mes_m4":0,"mhgp12_mes_m4_mutant_sans_contraction":1}
        for name,code in scripts.items():
            p=tmp/"build"/name
            p.write_text(f"#!/bin/sh\nexit {code}\n");p.chmod(0o700)
        report={}
        with contextlib.redirect_stdout(io.StringIO()):
            m3_code=m.m3(args,report);m4_code=m.m4(args,report)
        return dict(probe="zero_output_native_processes",m3_driver_code=m3_code,m4_driver_code=m4_code,
                    m3_processes=len(report["mes_m3"]["synthetic_k10"]["processus"]),
                    m3_routes_declared_deterministic=report["mes_m3"]["synthetic_k10"]["routes_identiques_entre_processus"],
                    m4_mutant_declared_killed=report["mes_m4"]["mutant_sans_contraction_synthetic_k10"]["tue"],
                    summaries=m.synthese(report),binary_hashes={name:hashlib.sha256((tmp/"build"/name).read_bytes()).hexdigest()
                                                              for name in scripts})


def probe_m34_resolution():
    m=module("m34_resolution",M34)
    with tempfile.TemporaryDirectory(prefix="audit-pilote-synth-") as tmp:
        tmp=Path(tmp);args=m34_arguments(tmp)
        (tmp/"build").mkdir();(tmp/"out").mkdir()
        (tmp/"build"/"mhgp12_vidage").write_bytes(b"SYNTHETIC_NOT_EXECUTED")
        calls=[]
        def play(command,*ignored,**kwargs):
            calls.append([str(c) for c in command])
            return 0,[dict(phase="resolution_un_fil",k=2,traces=1,
                           secondes=dict(v11=1.0,replique_v11=1.0,replique_v12=0.1*len(calls)),
                           rapport_v12_sur_replique_v11=0.1*len(calls))],0.0,""
        m.jouer=play
        report={}
        with contextlib.redirect_stdout(io.StringIO()):
            first=m.vider(args,report)
            first_calls=len(calls)
            first_summary=m.synthese(report)["resolution"]["synthetic_k10"]
            second=m.vider(args,report)
        second_summary=m.synthese(report)["resolution"]["synthetic_k10"]
        return dict(probe="resolution_replication_and_overwrite",processus_requested=args.processus,
                    vidage_calls_after_first=first_calls,vidage_calls_after_second=len(calls),
                    driver_codes=[first,second],ful1_present=False,bin_files_created=0,
                    resolution_ratio_after_first=first_summary["rapport_total_v12_sur_replique_v11"],
                    resolution_ratio_after_second=second_summary["rapport_total_v12_sur_replique_v11"],
                    retained_resolution_lines=len(report["vidages"]["synthetic_k10"]["lignes"]),
                    input_data_opened=False)


def main():
    m2=[probe_m2(kind) for kind in ("baseline_control","host_identity_failure","stale_unrelated_json",
                                  "no_deciding_case","arena_failure_control","bench_execution_failure_control")]
    expected=["adopte","adopte","adopte","adopte","refuse","refuse"]
    need([case["verdict"]["verdict"] for case in m2]==expected,"unexpected M2 outcome")
    nan=probe_m2_nan();empty=probe_m34_empty();resolution=probe_m34_resolution()
    need(nan["verdict"]=="adopte" and nan["ratio_is_nan"],"unexpected nonfinite-time outcome")
    need(empty["m3_driver_code"]==empty["m4_driver_code"]==0 and
         empty["m3_routes_declared_deterministic"] and empty["m4_mutant_declared_killed"] and
         empty["summaries"]==dict(mes_m3={"synthetic_k10":{}},mes_m4={},resolution={}),
         "unexpected empty-proof outcome")
    need(resolution["driver_codes"]==[0,0] and resolution["vidage_calls_after_first"]==1 and
         resolution["retained_resolution_lines"]==1 and resolution["resolution_ratio_after_first"]==0.1 and
         resolution["resolution_ratio_after_second"]==0.2,"unexpected replication/overwrite outcome")
    need(m2[1]["host_identity_code"]==1 and m2[2]["fresh_bench_json_writes"]==0 and
         m2[3]["verdict"]["ratio_gm_all_cases"] is None,"missing M2 failure marker")
    result=dict(schema="ehgp.v12.audit_pilotes.v1",pin=PIN,
                source_sha256={p:hashlib.sha256(source(p)).hexdigest() for p in (M2,M34)},
                m2=m2,m2_nonfinite=nan,m34_empty=empty,m34_resolution=resolution,
                scope="synthetic fault injections; no CUDA, no real cloud, no actual timing or algorithm verdict")
    print(json.dumps(result,ensure_ascii=False,indent=2,sort_keys=True,allow_nan=False))


if __name__=="__main__":
    main()
