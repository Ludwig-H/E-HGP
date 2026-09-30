"""Validate the closed archive without native or mathematical subprocess replay."""
import hashlib,json,re
from pathlib import Path
ROOT=Path(__file__).resolve().parent
FILES={"judge.py","original.jsonl","mutant_missing_orientation.jsonl","mutant_wrong_orientation.jsonl",
"check.py","README.md","provenance.json","normal.stdout","optimized.stdout","execution.json",
"pins.before.txt","pins.after.txt","verify.py"}
PINS={"judge.py":"76a7140e49cdc80b0f8a105a4ac5fdc9328d0139b93b9c43b44679fc2a5b0e67",
"original.jsonl":"8b00ab7891e5d7bd7e44424b536241b40b62d5028b2cc462e1bcadd92a674de7",
"mutant_missing_orientation.jsonl":"ee93987d70ff84df4d9a7268275d628d6bbb905ebca74e23489b2f30d2533421",
"mutant_wrong_orientation.jsonl":"2d17c0ee22de5b84d3573361603ed515907ed7ffade0a15ff164cd626e965f9d"}
def need(ok,text):
    if not ok: raise RuntimeError(text)
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
manifest={}
for line in (ROOT/"SHA256SUMS").read_text().splitlines():
    m=re.fullmatch(r"([0-9a-f]{64})  ([A-Za-z0-9_.]+)",line)
    need(m is not None,"malformed manifest")
    digest,name=m.groups();need(name not in manifest,"duplicate manifest entry")
    manifest[name]=digest
need(set(manifest)==FILES,"wrong archive manifest inventory")
for name,digest in manifest.items(): need(sha(ROOT/name)==digest,"archive pin mismatch:"+name)
for name,digest in PINS.items():need(manifest[name]==digest,"known input changed:"+name)
need(manifest["check.py"]=="0cfe67ad8642ad61b8b72461a164cb4f5ae53e46decc81b502422f3e30389632","controller changed")
e=json.loads((ROOT/"execution.json").read_text())
need(e["schema"]=="portable_orientation_reader_probe_v1","wrong execution schema")
need(type(e["native_executions"]) is int and e["native_executions"]==0,"native executions claimed")
need(type(e["engine_compilations"]) is int and e["engine_compilations"]==0 and e["GCP_used"] is False,"out-of-scope execution")
need(e["source_before"]==e["source_after"]==(ROOT/"pins.before.txt").read_text()==(ROOT/"pins.after.txt").read_text(),"source closure differs")
need(len(e["runs"])==2,"wrong runner capture count")
origin="/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v10/receipts/audit_continu_20260929/precision_reader_orientation_20260930"
need(e["working_directory"]==origin,"historical producer path changed")
names=("original.jsonl","mutant_missing_orientation.jsonl","mutant_wrong_orientation.jsonl")
for mode,stream in enumerate(("normal.stdout","optimized.stdout")):
    outer=e["runs"][mode]
    need(outer["argv"]==["timeout","20s","python3","-B"]+(["-O"] if mode else [])+["check.py"],"runner argv changed")
    need(type(outer["exit_code"]) is int and outer["exit_code"]==0 and outer["stream"]==stream,"runner outcome changed")
    r=json.loads((ROOT/stream).read_text())
    need(r["status"]=="READER_ORIENTATION_GAP_CONFIRMED" and r["optimize_flag"]==mode,"wrong report")
    need(type(r["native_executions"]) is int and r["native_executions"]==0 and r["engine_compilations"]==0 and r["GCP_used"] is False,"report scope changed")
    need(r["pins_before"]==r["pins_after"]==PINS,"report input pins changed")
    need(len(r["runs"])==3,"wrong mathematical replay count")
    for index,(name,run) in enumerate(zip(names,r["runs"])):
        need(run["input"]==name and type(run["returncode"]) is int and run["returncode"]==0 and run["stderr"]=="","mathematical replay outcome changed")
        need(run["argv"]==[e["python_executable"],"-B"]+(["-O"] if mode else [])+[origin+"/judge.py",origin+"/"+name],"judge argv changed")
        result=json.loads(run["stdout"])
        need(result["status"]=="EXPECTED_FAILURES_CONFIRMED" and result["checks"]==16,"judge conclusion changed")
        vv=[x for x in result["verdicts"] if x["case"]=="orient_center_q4_regular_tetra"]
        if index==0:need(len(vv)==2 and all(x["correct"] is True and x["inside_observed"]==1 for x in vv),"control no longer positive")
        elif index==1:need(vv==[],"missing-case mutation changed")
        else:need(len(vv)==2 and all(x["correct"] is False and x["inside_observed"]==0 for x in vv),"wrong-sign mutation changed")
print(json.dumps({"status":"ARCHIVE_PASS","files":len(FILES),"archived_mathematical_subprocesses":6,
"native_executions":0,"fresh_subprocesses":0,"limits":"Validates frozen reader counterexample captures, not native geometry."},sort_keys=True))
