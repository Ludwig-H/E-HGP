#!/usr/bin/env python3
"""Archive nine local gates in a NEW directory; preserve failures and hashes.

One inherited native --gate plus four Python suites in normal/-O modes.
The optional report formatter is not executed or pinned by this qualification.
No build, dataset generation, main benchmark, cloud call, or package install.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import time
import traceback

HERE = Path(__file__).resolve().parent
OLD = HERE.parent/"b_point_hierarchy_k_20260927"
EXPECTED_NATIVE = "897a715a5b40b7fa298aaf66696def3a5cf636c98118db69da2684277cd11dd8"
OLD_PINS = {
    "eom.py":"c7121c857010fd89b6798ec634a0f55c4abd14c1c6bc6305c551c06e4625dead",
    "projection.py":"a6a88955e6ea94f2d002a825df58db94cb602376d096164eee55792bcf014242",
    "benchmark.py":"8b5c80ddcf27896b7e25b5965caa50352a206de392318111a5623c37ce06736f",
    "datasets.py":"7ff3d93677f51eabe9c1bea0f56a62bbcd53ce531c3a49bdfafeab29ab14f53c",
    "native_export.cpp":"a5a62d458a628edbd4ca43cb633200344342c0287873aac15aacdf28d4fc6cf0",
    "build_native.py":"073f7bc97bf93b0d6c690fdb4e1dcfc91af845c04a78432afd20e57c782e31fb",
}
TESTS = ("test_gaussian_data","test_condensed","test_evaluation","test_pipeline")
SOURCE_NAMES = ("gaussian_data.py","test_gaussian_data.py","condensed.py","test_condensed.py",
                "evaluation.py","test_evaluation.py","run.py","qualification.py","test_pipeline.py",
                "DATASETS.md","CONDENSATION.md","README.md")


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def save(path,data):
    path.write_text(json.dumps(data,sort_keys=True,indent=2,allow_nan=False)+"\n")


def stop_group(process):
    try:
        os.killpg(process.pid,signal.SIGTERM)
    except ProcessLookupError:
        pass
    try:
        process.wait(timeout=5)
    except subprocess.TimeoutExpired:
        try:
            os.killpg(process.pid,signal.SIGKILL)
        except ProcessLookupError:
            pass
        process.wait(timeout=5)


def qualification(native,output):
    native,output = native.resolve(),output.resolve()
    output.mkdir(parents=True,exist_ok=False)
    receipt = dict(schema="mhgp9_gaussian_qualification_v1",status="running",commands=[],
        native_binary=str(native),GCP_used=False,GPU_used=False,engine_modified=False,
        python=sys.version,python_executable=sys.executable,
        commands_expected=9,scope="small_native_smoke_and_local_suites_not_48_scene_benchmark",
        excluded_nonruntime_formatter="report.py")
    save(output/"receipt.json",receipt)
    paths = [HERE/name for name in SOURCE_NAMES]+[OLD/name for name in OLD_PINS]
    try:
        receipt["sources_before"] = {str(p):sha(p) for p in paths}
        receipt["old_dependencies_expected"] = {str(OLD/name):value for name,value in OLD_PINS.items()}
        if any(sha(OLD/name)!=value for name,value in OLD_PINS.items()):
            raise ValueError("frozen previous-lot dependency changed")
        receipt["native_binary_sha256"] = sha(native)
        if receipt["native_binary_sha256"] != EXPECTED_NATIVE:
            raise ValueError("native binary differs from the explicitly inherited build")
        sys.path.insert(0,str(OLD))
        from eom import sklearn_provenance
        receipt["sklearn_before"] = sklearn_provenance()
        receipt["native_build_receipt"] = str(native.parent/"receipt.json")
        receipt["native_build_receipt_sha256"] = sha(native.parent/"receipt.json")
        commands = [("native_gate",[str(native),"--gate"])]
        for optimized in (False,True):
            for stem in TESTS:
                argv = [sys.executable,"-B"]+(["-O"] if optimized else [])+[str(HERE/(stem+".py"))]
                if stem == "test_pipeline":
                    argv.extend(["--native",str(native)])
                commands.append((stem+("_optimized" if optimized else "_normal"),argv))
        receipt["planned_commands"] = [dict(name=name,argv=argv) for name,argv in commands]
        save(output/"receipt.json",receipt)
        for name,argv in commands:
            stdout,stderr = output/(name+".stdout"),output/(name+".stderr")
            command = dict(name=name,argv=argv,cwd=str(HERE),status="running")
            receipt["commands"].append(command)
            started = time.monotonic()
            process = None
            try:
                with stdout.open("xb") as out,stderr.open("xb") as err:
                    process = subprocess.Popen(argv,cwd=HERE,stdout=out,stderr=err,start_new_session=True)
                    command["pid"] = process.pid
                    save(output/"receipt.json",receipt)
                    code = process.wait(timeout=180)
                command.update(returncode=code,status="passed" if code==0 else "failed")
            except BaseException as error:
                if process is not None:
                    stop_group(process)
                    command["returncode"] = process.returncode
                command.update(status="failed",error=repr(error))
                raise
            finally:
                command["seconds"] = time.monotonic()-started
                for field,path in (("stdout_sha256",stdout),("stderr_sha256",stderr)):
                    if path.exists():
                        command[field] = sha(path)
                save(output/"receipt.json",receipt)
            print(name,command["returncode"],flush=True)
        receipt["sources_after"] = {str(p):sha(p) for p in paths}
        receipt["native_binary_sha256_after"] = sha(native)
        receipt["native_build_receipt_sha256_after"] = sha(native.parent/"receipt.json")
        receipt["sklearn_after"] = sklearn_provenance()
        receipt["differing_normal_optimized_stdout"] = [stem for stem in TESTS if
            (output/(stem+"_normal.stdout")).read_bytes() != (output/(stem+"_optimized.stdout")).read_bytes()]
        stable = (receipt["sources_before"]==receipt["sources_after"] and
            receipt["native_binary_sha256"]==receipt["native_binary_sha256_after"] and
            receipt["native_build_receipt_sha256"]==receipt["native_build_receipt_sha256_after"] and
            receipt["sklearn_before"]==receipt["sklearn_after"])
        receipt["status"] = "passed" if (stable and len(receipt["commands"])==9 and
            all(c["status"]=="passed" for c in receipt["commands"]) and
            not receipt["differing_normal_optimized_stdout"]) else "failed"
    except BaseException as error:
        receipt.update(status="failed",error=repr(error),traceback=traceback.format_exc())
        raise
    finally:
        save(output/"receipt.json",receipt)
    return 0 if receipt["status"]=="passed" else 1


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native",type=Path,required=True)
    parser.add_argument("--output",type=Path,required=True)
    args = parser.parse_args()
    sys.exit(qualification(args.native,args.output))
