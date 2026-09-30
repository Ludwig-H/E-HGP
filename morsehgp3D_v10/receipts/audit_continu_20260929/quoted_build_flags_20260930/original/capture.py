#!/usr/bin/env python3
import hashlib,json,pathlib,shlex,subprocess,time
ROOT=pathlib.Path("/workspaces/E-HGP/build/v10-integration-r2/src/morsehgp3D_v10")
HERE=pathlib.Path(__file__).resolve().parent
paths=[ROOT/"CMakeLists.txt",ROOT/"src/cloud/site_tree.cpp",ROOT/"src/tower/tower.cpp",ROOT/"tests/regression/fp_flags_configure.cmake"]
def pins():
 return {str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
def run(name,argv):
 start=time.monotonic()
 r=subprocess.run(argv,capture_output=True,text=True,timeout=10)
 return dict(name=name,argv=argv,returncode=r.returncode,seconds=time.monotonic()-start,stdout=r.stdout,stderr=r.stderr)
before=pins()
base=["cmake","-S",str(ROOT),"-DCMAKE_CXX_COMPILER=/usr/bin/clang++","-DBUILD_TESTING=OFF"]
control=run("unquoted_refused",base+["-B",str(HERE/"control"),"-DCMAKE_CXX_FLAGS=-freciprocal-math"])
quoted=run("quoted_configure",base+["-B",str(HERE/"quoted"),'-DCMAKE_CXX_FLAGS="-freciprocal-math"'])
cache=(HERE/"quoted/CMakeCache.txt").read_text() if quoted["returncode"]==0 else ""
lines=[x for x in cache.splitlines() if x.startswith("CMAKE_CXX_FLAGS:") or x.startswith("CMAKE_CXX_COMPILER:")]
entries=json.loads((HERE/"quoted/compile_commands.json").read_text()) if quoted["returncode"]==0 else []
site=[x for x in entries if x["file"].endswith("/cloud/site_tree.cpp")]
tokens=shlex.split(site[0]["command"]) if len(site)==1 else []
macro=run("clang_reciprocal_macros",["/usr/bin/clang++","-freciprocal-math","-dM","-E","-x","c++","/dev/null"])
selected=[x for x in macro["stdout"].splitlines() if any(t in x for t in ["FAST_MATH","ASSOCIATIVE_MATH","RECIPROCAL_MATH","FINITE_MATH","FLT_EVAL"])]
guard=run("site_tree_preprocessor",["/usr/bin/clang++","-freciprocal-math","-std=c++20","-I"+str(ROOT/"src"),"-E","-o","/dev/null",str(ROOT/"src/cloud/site_tree.cpp")])
after=pins()
out=dict(schema="mhgp10_quote_configure_micro_v1",scope="One quoted flag fixture and its unquoted control. Two configurations and two preprocessor calls; no engine object compiled or native geometric execution.",
 calls=[control,quoted,macro,guard],cache_lines=lines,site_compile_command=site[0]["command"] if len(site)==1 else None,site_command_tokens=tokens,selected_math_macros=selected,source_before=before,source_after=after,source_stable=before==after)
expected=control["returncode"]!=0 and "mhgp10_fp_flags_interdits" in control["stderr"] and quoted["returncode"]==0 and 'CMAKE_CXX_FLAGS:STRING="-freciprocal-math"' in lines and "-freciprocal-math" in tokens and macro["returncode"]==0 and guard["returncode"]==0 and not any(x.startswith("#define "+n+" ") for x in macro["stdout"].splitlines() for n in ["__FAST_MATH__","__ASSOCIATIVE_MATH__","__RECIPROCAL_MATH__"]) and before==after
out["EXPECTED_GAP"]=expected
print(json.dumps(out,indent=2))
raise SystemExit(0 if expected else 1)
