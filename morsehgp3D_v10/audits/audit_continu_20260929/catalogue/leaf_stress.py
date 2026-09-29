"""Bounded observation of a pathological leaf parameter; not a correctness failure."""
import json,pathlib,subprocess,time
p=pathlib.Path(__file__).resolve().parent
rows=[]
for k,leaf in ((1,0),(1,2),(5,0),(5,2)):
    cmd=[str(p/'probe'),str(p/'cube_max.txt'),str(k),str(leaf),'1']
    t=time.monotonic()
    try:
        r=subprocess.run(cmd,capture_output=True,text=True,timeout=2)
        row={'argv':cmd,'status':'completed','returncode':r.returncode,'balls':len(r.stdout.splitlines())}
    except subprocess.TimeoutExpired:
        row={'argv':cmd,'status':'timeout_killed_and_joined','seconds_limit':2}
    row['wall_s']=time.monotonic()-t;rows.append(row)
(p/'leaf_stress.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps(rows))
