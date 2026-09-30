"""Closed conditional Qκ archive + one tiny Python-only scalar replay."""
import hashlib,json,re,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parent
FILES={"PROOF.md","check.py","normal.stdout","optimized.stdout","execution.json","README.md","verify.py"}
PINS={"PROOF.md":"11f7a874841c78dcb283c5fb4bd94de8154ed9f25c7ebe029afc1e79a57cdd34",
"check.py":"78b1d42c19b22044d10773579360fca8ea09bd6fab53623fae4e717f91e6a486",
"normal.stdout":"43332af4783cdb5112967ed35d531b8faff55caf174c2c27c0489ecfdad12f43",
"optimized.stdout":"43332af4783cdb5112967ed35d531b8faff55caf174c2c27c0489ecfdad12f43",
"execution.json":"e8e80a71a1e834caa6c13800fd135d4ccc2faf2a37741565f35efc50a58e2dbe"}
def need(ok,text):
    if not ok: raise RuntimeError(text)
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
manifest={}
for line in (ROOT/"SHA256SUMS").read_text().splitlines():
    m=re.fullmatch(r"([0-9a-f]{64})  ([A-Za-z0-9_.]+)",line)
    need(m is not None,"malformed manifest")
    digest,name=m.groups();need(name not in manifest,"duplicate manifest entry")
    manifest[name]=digest
need(set(manifest)==FILES,"wrong archive file inventory")
before={name:sha(ROOT/name) for name in FILES}
need(before==manifest,"archive SHA mismatch before scalar replay")
for name,digest in PINS.items():need(before[name]==digest,"original closed artifact changed:"+name)
sources={"PROOF.md":PINS["PROOF.md"],"check.py":PINS["check.py"]}
counts={"inequalities":12096,"paired_fixtures":2,"scalar_cases":1728}
for stream in ("normal.stdout","optimized.stdout"):
    r=json.loads((ROOT/stream).read_text())
    need(r["status"]=="SCALAR_PASS" and r["counts"]==counts,"archived scalar outcome changed")
    need(r["pins_before"]==r["pins_after"]==sources,"archived source pins changed")
    need(type(r["native_executions"]) is int and r["native_executions"]==0 and r["GCP_used"] is False,"scope changed")
e=json.loads((ROOT/"execution.json").read_text())
need(e["schema"]=="quadratic_cohort_scalar_proof_v1","wrong execution schema")
need(e["source_before"]==e["source_after"]==sources,"execution source pins changed")
need(type(e["native_executions"]) is int and e["native_executions"]==0,"native executions claimed")
need(type(e["engine_compilations"]) is int and e["engine_compilations"]==0 and e["GCP_used"] is False,"expanded execution scope")
need(e["outputs_identical"] is True and len(e["runs"])==2,"wrong historic run pair")
old="/tmp/mhgp10-quadratic-cohort-rule-20260930.jQmaep1M/check.py"
for mode,(stream,run) in enumerate(zip(("normal.stdout","optimized.stdout"),e["runs"])):
    need(run["argv"]==["timeout","15s","python3","-B"]+(["-O"] if mode else [])+[old],"historic argv changed")
    need(type(run["exit_code"]) is int and run["exit_code"]==0 and run["stream"]==stream,"historic outcome changed")
argv=[sys.executable,"-B"]+(["-O"] if sys.flags.optimize else [])+[str(ROOT/"check.py")]
p=subprocess.run(argv,cwd=ROOT,capture_output=True,text=True,timeout=10)
need(p.returncode==0 and p.stderr=="","portable scalar replay refused")
need(p.stdout==(ROOT/("optimized.stdout" if sys.flags.optimize else "normal.stdout")).read_text(),"portable scalar stdout differs")
after={name:sha(ROOT/name) for name in FILES}
need(before==after==manifest,"archive changed during replay")
print(json.dumps({"status":"ARCHIVE_AND_SCALAR_REPLAY_PASS","files":len(FILES),
"fresh_python_subprocesses":1,"native_executions":0,"engine_compilations":0,"GCP_used":False,
"scalar_cases":1728,"limits":"Conditional Qκ algebra only; geometry/maps, statistics, D bound and engine remain unqualified."},sort_keys=True))
