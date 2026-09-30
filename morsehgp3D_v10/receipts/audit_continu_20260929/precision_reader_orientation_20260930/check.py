"""Portable mathematical rejudge; never launches a native engine."""
import hashlib, json, subprocess, sys
from pathlib import Path
ROOT=Path(__file__).resolve().parent
PINS={"judge.py":"76a7140e49cdc80b0f8a105a4ac5fdc9328d0139b93b9c43b44679fc2a5b0e67",
"original.jsonl":"8b00ab7891e5d7bd7e44424b536241b40b62d5028b2cc462e1bcadd92a674de7",
"mutant_missing_orientation.jsonl":"ee93987d70ff84df4d9a7268275d628d6bbb905ebca74e23489b2f30d2533421",
"mutant_wrong_orientation.jsonl":"2d17c0ee22de5b84d3573361603ed515907ed7ffade0a15ff164cd626e965f9d"}
def need(ok,text):
    if not ok: raise RuntimeError(text)
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
before={name:sha(ROOT/name) for name in PINS}
need(before==PINS,"input pins mismatch before judge subprocess")
original=(ROOT/"original.jsonl").read_bytes().splitlines(keepends=True)
drop=(ROOT/"mutant_missing_orientation.jsonl").read_bytes().splitlines(keepends=True)
wrong=(ROOT/"mutant_wrong_orientation.jsonl").read_bytes().splitlines(keepends=True)
def orientation(line): return json.loads(line)["case"]=="orient_center_q4_regular_tetra"
need(sum(map(orientation,original))==2,"original orientation inventory changed")
need(drop==[x for x in original if not orientation(x)],"missing fixture changes other bytes")
need(len(wrong)==len(original),"wrong fixture count changed")
for base,line in zip(original,wrong):
    if not orientation(base):
        need(base==line,"wrong fixture changes nonorientation bytes")
    else:
        a=json.loads(base); b=json.loads(line)
        expected=dict(a,strictly_inside=0,orient_center=[0,0,0,0],orient_center_wide=[0,0,0,0])
        need(b==expected,"wrong fixture changes unrelated orientation fields")
runs=[]
for name in PINS:
    if name=="judge.py": continue
    argv=[sys.executable,"-B"]+(["-O"] if sys.flags.optimize else [])+[str(ROOT/"judge.py"),str(ROOT/name)]
    p=subprocess.run(argv,cwd=ROOT,capture_output=True,text=True,timeout=10)
    report=json.loads(p.stdout)
    need(p.returncode==0 and p.stderr=="","unexpected mathematical judge refusal/diagnostic")
    need(report["status"]=="EXPECTED_FAILURES_CONFIRMED" and report["checks"]==16,"unexpected archived verdict")
    vv=[v for v in report["verdicts"] if v["case"]=="orient_center_q4_regular_tetra"]
    if name=="original.jsonl":
        need(len(vv)==2 and all(v["correct"] is True and v["inside_observed"]==1 for v in vv),"positive control changed")
    elif name=="mutant_missing_orientation.jsonl":
        need(vv==[],"missing mutation no longer missing")
    else:
        need(len(vv)==2 and all(v["correct"] is False and v["inside_observed"]==0 for v in vv),"wrong mutation no longer causal")
    runs.append({"input":name,"argv":argv,"returncode":p.returncode,"stdout":p.stdout,"stderr":p.stderr})
after={name:sha(ROOT/name) for name in PINS}
need(before==after==PINS,"input pins changed after judge subprocess")
print(json.dumps({"status":"READER_ORIENTATION_GAP_CONFIRMED","pins_before":before,"pins_after":after,
"native_executions":0,"engine_compilations":0,"GCP_used":False,"optimize_flag":sys.flags.optimize,
"runs":runs,"limits":"Portable replay of mathematical judge only. Archived native stdout is not rerun; no native orientation/large-domain/FULL qualification."},sort_keys=True))
