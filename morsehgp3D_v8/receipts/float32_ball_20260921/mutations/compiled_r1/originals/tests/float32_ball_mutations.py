#!/usr/bin/env python3
"""Three compiled, geometrically judged mutations of the binary32 ball core.

Only fresh source copies/builds are changed. The existing process-group
collector handles commands/cancellation; the independent Fraction oracle
judges three small fixtures. System headers are HASHED before/after builds,
not copied wholesale into the receipt. No timeout turns a crash into a kill.
"""
from __future__ import annotations

import argparse
import base64
import difflib
import os
from pathlib import Path
import signal
import sys

V8 = Path(__file__).resolve().parents[1]
ROOT = V8.parent
sys.path.insert(0, str(V8 / "bench"))
import float32_ball_gate as oracle
import float32_index_mutation_gate as utility
from run_p0_matrix import invoke, on_signal

require, stamp, digest, pins = utility.require, utility.stamp, utility.digest, utility.pins
write_bytes, write_json, load = utility.write_bytes, utility.write_json, utility.load
SCHEMA = "mhgp8_float32_ball_compiled_mutations_v1"
LOCAL = ("src/core/float32_predicates.hpp", "src/core/fixed_signed.hpp",
         "src/core/float32_ball.hpp", "src/core/float32_ball.cpp", "tests/float32_ball_probe.cpp")
HELPERS = ("tests/float32_ball_gate.py", "tests/float32_index_mutation_gate.py",
           "bench/run_p0_matrix.py", "tests/float32_ball_mutations.py")
SOURCES = (*LOCAL, *HELPERS)
FLAGS = ("-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-pthread",
         "-ffp-contract=off", "-fno-fast-math", "-frounding-math", "-O3", "-DNDEBUG")
ENVIRONMENT = {"PATH": os.defpath, "LANG": "C", "LC_ALL": "C", "TZ": "UTC"}
MUTATIONS = (
    dict(name="nonstrict_barycentric", source="src/core/float32_ball.cpp",
         before="if (x.sign() <= 0) return false;", after="if (x.sign() < 0) return false;", fixture=0,
         property="a circumcentre on a tetrahedron face must reject the support"),
    dict(name="unnormalized_power", source="src/core/float32_ball.cpp",
         before="return sign * exact.scale.sign();", after="return sign;", fixture=1,
         property="negative determinant must not reverse the interior power sign"),
    dict(name="normal_exponent_shift", source="src/core/fixed_signed.hpp",
         before="exponent == 0 ? 0 : exponent - 1;", after="exponent == 0 ? 0 : exponent;", fixture=2,
         property="normal and subnormal coordinates must share the same exact dyadic unit"),
)
CASES = ("baseline", *(m["name"] for m in MUTATIONS))
ARTIFACTS = ("pre_core.d", "pre_test.d", "core.d", "test.d", "core.o", "test.o", "probe")


def fixtures():
    def encoded(points):
        return tuple(tuple(oracle.bits(x) for x in p) for p in points)
    return (
        dict(label="centre_on_face", support=encoded(((5,0,0),(-3,4,0),(-3,-4,0),(0,0,5))), query=(0,0,0)),
        dict(label="negative_orientation_inside", support=encoded(((1,1,1),(1,-1,-1),(-1,1,-1),(-1,-1,1))), query=(0,0,0)),
        dict(label="normal_subnormal_near_right", support=((0,0,0),(0x00800000,0,0),(0x00400000,0x00400000,1)), query=(0,0,0)),
    )


def expected():
    result = [oracle.oracle(case) for case in fixtures()]
    require(result == [(False,None),(True,-1),(True,0)], "independent fixture expectations changed")
    return [dict(valid=valid, sign=sign) for valid,sign in result]


def mutation(name):
    return next((m for m in MUTATIONS if m["name"]==name), None)


def changed(data, name, source):
    m = mutation(name)
    if m is None or m["source"] != source:
        return data
    before, after = m["before"].encode(), m["after"].encode()
    require(data.count(before)==1, "mutation site is not unique: "+name)
    return data.replace(before, after, 1)


def plan(manifest):
    output, build = Path(manifest["output"]), Path(manifest["build"])
    compiler = manifest["compiler"]
    result = [dict(label="compiler_version", command=[compiler,"--version"], case=None, phase="version", stdin=None)]
    for name in CASES:
        source, target = output/"cases"/name, build/name
        flags = [*FLAGS,"-I",str(source/"src")]
        for phase in ("dependencies","compile"):
            for unit, filename in (("core","src/core/float32_ball.cpp"),("test","tests/float32_ball_probe.cpp")):
                command = [compiler,*flags]
                if phase=="dependencies":
                    command += ["-M",str(source/filename),"-MF",str(target/("pre_"+unit+".d")),"-MT",str(target/(unit+".o"))]
                else:
                    command += ["-c",str(source/filename),"-MD","-MF",str(target/(unit+".d")),"-o",str(target/(unit+".o"))]
                result.append(dict(label=name+"_"+phase+"_"+unit,command=command,case=name,phase=phase,stdin=None))
        result.append(dict(label=name+"_link",command=[compiler,*flags,str(target/"core.o"),str(target/"test.o"),"-o",str(target/"probe")],
                           case=name,phase="link",stdin=None))
        result.append(dict(label=name+"_geometry",command=[str(target/"probe")],case=name,phase="geometry",stdin=str(output/"fixtures.txt")))
    return result


def judgment(item, record):
    require(record["exit_code"]==0 and "error" not in record, "compile/crash/nonzero exit is not a geometric mutation kill: "+item["label"])
    if item["phase"] != "geometry":
        return None
    require(record["stderr"]=="", "native geometric query printed an error")
    rows = [oracle.strict_json(line) for line in record["stdout"].splitlines()]
    cases, truth = fixtures(), expected()
    fields = {"arity","filtered_valid","exact_valid","filtered_sign","exact_sign",*oracle.WORK_KINDS}
    require(len(rows)==len(cases), "native fixture response count differs")
    disagreements = []
    for number,(case,row,answer) in enumerate(zip(cases,rows,truth,strict=True)):
        require(type(row) is dict and set(row)==fields and type(row["arity"]) is int and row["arity"]==len(case["support"]), "native geometric row shape differs")
        for prefix in ("filtered","exact"):
            valid, sign = row[prefix+"_valid"], row[prefix+"_sign"]
            require(type(valid) is bool and (type(sign) is int and sign in (-1,0,1) if valid else sign is None), "invalid native geometric decision type")
            if (valid,sign)!=(answer["valid"],answer["sign"]):
                disagreements.append(dict(fixture=number,label=case["label"],mode=prefix,expected=answer,actual=dict(valid=valid,sign=sign)))
        for name in oracle.WORK_KINDS:
            require(type(row[name]) is dict and set(row[name])==set(oracle.FIELDS) and
                    all(type(v) is int and 0<=v<2**64 for v in row[name].values()), "native work inventory/type changed")
    if item["case"]=="baseline":
        require(not disagreements,"unmodified primitive disagrees with the rational oracle")
    else:
        m=mutation(item["case"])
        require(any(d["fixture"]==m["fixture"] and d["mode"]=="exact" for d in disagreements),
                "mutation survived its targeted rational geometry oracle: "+item["case"])
    return dict(expected=truth,disagreements=disagreements,geometric_kill=item["case"]!="baseline")


def dependencies(directory,prefix):
    return utility.dependencies(directory,prefix)


def execute(item, output):
    record=dict(**item,cwd=str(output),environment=ENVIRONMENT,started_utc=stamp(),exit_code=None,
                stdout="",stderr="",stdout_base64="",stderr_base64="")
    try:
        # The reused collector inherits fd0. Give it the exact archived input
        # without a shell, wrapper process or mutable global fixture generator.
        path=Path(item["stdin"]) if item["stdin"] else Path(os.devnull)
        with path.open("rb") as stream:
            saved=os.dup(0)
            try:
                os.dup2(stream.fileno(),0)
                invoke(item["command"],ENVIRONMENT,output,record,new_session=True)
            finally:
                os.dup2(saved,0);os.close(saved)
    except BaseException as error:
        record["error"]=f"{type(error).__name__}: {error}"
        raise
    finally:
        record["finished_utc"]=stamp()
        write_json(output/(item["label"]+".json"),record)
    return record


def inventory(output):
    return utility.inventory(output)


def run(args):
    require(os.name=="posix","POSIX descriptor/process-group capture required")
    output,build,compiler=args.output.resolve(),args.build.resolve(),args.compiler.absolute()
    require(not output.exists() and not build.exists(),"capture and build must be fresh")
    require(not output.is_relative_to(build) and not build.is_relative_to(output),"capture and build must be disjoint")
    require(compiler.is_file() and os.access(compiler,os.X_OK),"compiler unavailable")
    original=pins([*(V8/name for name in SOURCES),compiler])
    output.mkdir(parents=True);build.mkdir(parents=True)
    manifest=dict(schema=SCHEMA,output=str(output),build=str(build),compiler=str(compiler),source_sha256=original,
        flags=list(FLAGS),environment=ENVIRONMENT,mutations=list(MUTATIONS),fixtures=fixtures(),expected=expected(),
        launch=[sys.executable,*sys.argv],started_utc=stamp(),scope="three_targeted_fraction_oracle_mutations_not_census_catalogue_FULL_or_GPU",
        system_headers_archived=False)
    manifest["plan"]=plan(manifest)
    write_json(output/"MANIFEST.json",manifest)
    state=dict(status="failed",manifest_sha256=digest(output/"MANIFEST.json"),commands=[],compiled={},judgments=[],killed=[])
    handlers={sig:signal.signal(sig,on_signal) for sig in (signal.SIGINT,signal.SIGTERM)}
    print(oracle.strict_json.__module__+": "+str(output),flush=True)
    try:
        write_bytes(output/"fixtures.txt",oracle.payload(fixtures()))
        for name in SOURCES:
            write_bytes(output/"originals"/name,(V8/name).read_bytes())
        for case in CASES:
            (build/case).mkdir()
            patch=[]
            for name in LOCAL:
                data=(output/"originals"/name).read_bytes();altered=changed(data,case,name)
                write_bytes(output/"cases"/case/name,altered)
                if altered!=data:
                    patch.extend(difflib.unified_diff(data.decode().splitlines(True),altered.decode().splitlines(True),fromfile=name,tofile=case+"/"+name))
            write_bytes(output/"cases"/case/"change.patch","".join(patch).encode())
        for item in manifest["plan"]:
            try:
                record=execute(item,output)
            finally:
                path=output/(item["label"]+".json")
                if path.exists():state["commands"].append(dict(path=path.name,sha256=digest(path)))
            result=judgment(item,record)
            case=item["case"]
            if item["label"]==str(case)+"_dependencies_test":
                dep=dependencies(build/case,"pre_")
                require({output/"cases"/case/name for name in LOCAL}<=dep,"compiler omitted a local source")
                state["compiled"][case]=dict(dependencies_before=pins(dep))
            if item["phase"]=="link":
                detail=state["compiled"][case]
                detail["dependencies_after"]=pins(dependencies(build/case,""))
                require(detail["dependencies_before"]==detail["dependencies_after"],"dependencies changed while compiling")
                detail["artifacts_before"]=pins(build/case/name for name in ARTIFACTS)
            if result is not None:
                state["judgments"].append(dict(case=case,**result))
                if result["geometric_kill"]:state["killed"].append(case)
        state["status"]="passed"
    except BaseException as error:
        state["error"]=f"{type(error).__name__}: {error}"
    finally:
        for sig in handlers:signal.signal(sig,signal.SIG_IGN)
        try:
            state["source_sha256_after"]=pins(original)
            require(state["source_sha256_after"]==original and digest(output/"MANIFEST.json")==state["manifest_sha256"],"original sources/compiler/manifest changed")
            for detail in state["compiled"].values():
                detail["dependencies_closed"]=pins(detail["dependencies_before"])
                require(detail["dependencies_closed"]==detail["dependencies_before"],"compiled dependencies changed at closure")
                if "artifacts_before" in detail:
                    detail["artifacts_closed"]=pins(detail["artifacts_before"])
                    require(detail["artifacts_closed"]==detail["artifacts_before"],"compiled artifacts changed at closure")
            state["available_build_artifacts"]=pins(p for p in build.rglob("*") if p.is_file())
            state["artifact_sha256"]=inventory(output)
        except BaseException as error:
            state.update(status="failed",closing_error=f"{type(error).__name__}: {error}")
        state["finished_utc"]=stamp();write_json(output/"COMPLETION.json",state)
        for sig,handler in handlers.items():signal.signal(sig,handler)
    require(state["status"]=="passed","mutation capture failed; evidence preserved: "+str(output))
    return read(output,True)


def read(output,check_live=False):
    output=output.resolve(strict=True)
    pin=digest(output/"COMPLETION.json")
    manifest,state=load(output/"MANIFEST.json"),load(output/"COMPLETION.json")
    require(manifest["schema"]==SCHEMA and manifest["mutations"]==list(MUTATIONS) and manifest["flags"]==list(FLAGS) and
            manifest["environment"]==ENVIRONMENT and manifest["system_headers_archived"] is False,"manifest configuration differs")
    require(manifest["plan"]==plan(manifest),"command plan differs")
    require(inventory(output)==state["artifact_sha256"] and digest(output/"MANIFEST.json")==state["manifest_sha256"],"capture closure differs")
    require(state["status"]=="passed" and state["killed"]==list(CASES[1:]) and state["source_sha256_after"]==manifest["source_sha256"],"capture is not closed three-kill PASS")
    require((output/"fixtures.txt").read_bytes()==oracle.payload(fixtures()) and manifest["expected"]==expected(),"fixtures/oracle truth differ")
    for name in SOURCES:
        require(digest(output/"originals"/name)==manifest["source_sha256"][str(V8/name)],"archived source mismatch")
    for name in HELPERS:
        require(digest(V8/name)==manifest["source_sha256"][str(V8/name)],"historical reader needs its pinned helper version")
    original_output,build=Path(manifest["output"]),Path(manifest["build"])
    for case in CASES:
        patch=[]
        for name in LOCAL:
            original=(output/"originals"/name).read_bytes();altered=changed(original,case,name)
            require((output/"cases"/case/name).read_bytes()==altered,"mutation copy differs")
            if altered!=original:
                patch.extend(difflib.unified_diff(original.decode().splitlines(True),altered.decode().splitlines(True),fromfile=name,tofile=case+"/"+name))
        require((output/"cases"/case/"change.patch").read_bytes()=="".join(patch).encode(),"mutation patch differs")
    require(len(state["commands"])==len(manifest["plan"]),"command count differs")
    judgments=[]
    for item,entry in zip(manifest["plan"],state["commands"],strict=True):
        require(entry["path"]==item["label"]+".json" and digest(output/entry["path"])==entry["sha256"],"command receipt hash/order differs")
        record=load(output/entry["path"])
        require(all(record[k]==v for k,v in item.items()) and record["cwd"]==str(original_output) and record["environment"]==ENVIRONMENT,"command invocation differs")
        for channel in ("stdout","stderr"):
            require(base64.b64decode(record[channel+"_base64"],validate=True).decode("utf-8",errors="replace")==record[channel],"raw/text output differs")
        result=judgment(item,record)
        if result is not None:judgments.append(dict(case=item["case"],**result))
    require(judgments==state["judgments"] and set(state["compiled"])==set(CASES),"geometric judgments/compiled cases differ")
    for case,detail in state["compiled"].items():
        require(detail["dependencies_before"]==detail["dependencies_after"]==detail["dependencies_closed"],"dependency closure differs")
        require(detail["artifacts_before"]==detail["artifacts_closed"],"artifact closure differs")
        dep=set(map(Path,detail["dependencies_before"]))
        require({original_output/"cases"/case/name for name in LOCAL}<=dep,"compiled source inventory incomplete")
        for name in LOCAL:
            require(detail["dependencies_before"][str(original_output/"cases"/case/name)]==digest(output/"cases"/case/name),"compiled source differs from archived mutation")
        require(set(detail["artifacts_before"])=={str(build/case/name) for name in ARTIFACTS},"compiled artifact inventory differs")
        if check_live:
            require(pins(dep)==detail["dependencies_closed"] and pins(detail["artifacts_closed"])==detail["artifacts_closed"],"live native dependencies/artifacts changed")
            require(dependencies(build/case,"pre_")==dependencies(build/case,"")==dep,"live dependency files disagree")
    if check_live:
        require(pins(manifest["source_sha256"])==manifest["source_sha256"] and
                pins(state["available_build_artifacts"])==state["available_build_artifacts"],"live sources/build changed")
    require(inventory(output)==state["artifact_sha256"] and digest(output/"COMPLETION.json")==pin,"capture changed during read")
    return dict(schema=SCHEMA,status="passed",path=str(output),check_live=check_live,killed=list(CASES[1:]),
        commands=len(manifest["plan"]),fixtures=len(fixtures()),manifest_sha256=digest(output/"MANIFEST.json"),completion_sha256=pin,
        reader_sha256=digest(Path(__file__)),native_reexecuted=False,scope=manifest["scope"])


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    if len(sys.argv)>1 and sys.argv[1]=="read":
        parser.add_argument("--path",type=Path,required=True);parser.add_argument("--check-live",action="store_true")
        args=parser.parse_args(sys.argv[2:]);result=read(args.path,args.check_live)
    else:
        parser.add_argument("--build",type=Path,required=True);parser.add_argument("--output",type=Path,required=True)
        parser.add_argument("--compiler",type=Path,default=Path("/usr/bin/g++"))
        result=run(parser.parse_args())
    import json
    print(json.dumps(result,sort_keys=True))


if __name__=="__main__":
    try:
        main()
    except BaseException as error:
        import json
        print(json.dumps(dict(schema=SCHEMA,status="failed",error=f"{type(error).__name__}: {error}")),file=sys.stderr)
        sys.exit(1)
