#!/usr/bin/env python3
"""Replay fixed statistical checks from closed private captures, no geometry.

The 18-scene sample was fixed before inspecting this campaign's scores.
ARI is recomputed with exact rational contingency arithmetic, NMI from counts.
Matching reconstructs point sets independently but shares SciPy's Hungarian
solver with evaluation.py. No EOM refit, point-tree rebuild, or native process.
"""
import argparse
import json, pathlib, hashlib, math, sys, statistics
from collections import Counter, defaultdict
from fractions import Fraction
import numpy as np
from scipy.optimize import linear_sum_assignment

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument("--run",type=pathlib.Path,required=True)
parser.add_argument("--checks",type=pathlib.Path,required=True)
parser.add_argument("--output",type=pathlib.Path,required=True)
args=parser.parse_args()
if args.output.exists():
    raise SystemExit("fresh output file required; audits must not be overwritten")
base=pathlib.Path(__file__).resolve().parents[1]
capture=args.run.resolve()
receipt_path=capture/"receipt.json"
receipt=json.loads(receipt_path.read_text())
if receipt["status"]!="completed" or receipt["failures"] or len(receipt["rows"])!=1920:
    raise SystemExit("capture not completed")
manifest=json.loads(pathlib.Path(receipt["manifest_path"]).read_text())
bycase={c["id"]:c for c in manifest["cases"]}
rows=receipt["rows"]
errors=[]
checks=Counter()
def check(v, text):
    checks["comparisons"]+=1
    if not v: errors.append(text)
def same(a,b):
    if a is None or b is None: return a is b
    if isinstance(a,(int,float)) and isinstance(b,(int,float)): return math.isclose(a,b,rel_tol=2e-12,abs_tol=2e-12)
    return a==b
expected_grid={(cid,k,m,z,method) for cid in bycase for k in (5,10) for m in (10,20,50,100)
               for z in (1,2) for method in (("hgp_first_coverage","hdbscan_common","hdbscan_standard")
                                            if z==1 else ("hgp_first_coverage","hdbscan_common"))}
actual_grid=[tuple(row[key] for key in ("case","k","min_cluster_size","exp_z","method")) for row in rows]
check(len(actual_grid)==len(set(actual_grid))==1920 and set(actual_grid)==expected_grid,"full prescribed unique grid")
command_grid=[(c["case"],c["k"]) for c in receipt["commands"]]
check(len(command_grid)==len(set(command_grid))==96 and set(command_grid)=={(cid,k) for cid in bycase for k in (5,10)},"full native command grid")
for row in rows:
    check(all(row[key]==bycase[row["case"]][key] for key in ("n","communities","regime","separation","seed")),"row metadata matches manifest")
def ari(truth,labels):
    n=len(truth)
    nij=Counter(zip(truth,labels)); ai=Counter(truth); bj=Counter(labels)
    choose=lambda x:x*(x-1)//2
    total=choose(n)
    if not total: return 1.0
    index=sum(choose(v) for v in nij.values())
    a=sum(choose(v) for v in ai.values()); b=sum(choose(v) for v in bj.values())
    expected=Fraction(a*b,total); maximum=Fraction(a+b,2)
    return 1.0 if maximum==expected else float((index-expected)/(maximum-expected))
def nmi(truth,labels):
    n=len(truth); aa=Counter(truth); bb=Counter(labels); both=Counter(zip(truth,labels))
    h=lambda c:-math.fsum((s/n)*math.log(s/n) for s in c.values())
    hx,hy=h(aa),h(bb)
    if not hx and not hy:return 1.0
    information=math.fsum((v/n)*math.log(Fraction(v*n,aa[a]*bb[b])) for (a,b),v in both.items())
    return information/((hx+hy)/2)
sample=[]
for i,g in enumerate((2,4,8,16)):
    for j,delta in enumerate((8,4,2)):
        sample.append(f"spherical_g{g}_d{delta}_s{(i+j)%3+1}")
for i,regime in enumerate(("anisotropic","unbalanced")):
    for j,delta in enumerate((8,4,2)):
        sample.append(f"{regime}_g8_d{delta}_s{(i+j)%2+1}")
sample_set=set(sample)
artifacts={}
truth_cache={}
for cid in sample:
    truth_cache[cid]=json.loads(pathlib.Path(bycase[cid]["labels_json"]).read_text())
for row in rows:
    cid=row["case"]
    if cid not in sample_set:continue
    truth=truth_cache[cid]
    k,m,z,method=(row[x] for x in ("k","min_cluster_size","exp_z","method"))
    directory=capture/f"{cid}_k{k}"
    if method=="hdbscan_standard":
        path=directory/f"hdbscan_m{m}.json"
        payload=json.loads(path.read_text()); labels=payload["standard_labels_z1"]
    else:
        path=directory/f"{method}_m{m}_z{z}.json"
        payload=json.loads(path.read_text()); labels=payload["selection"]["labels"]
        check(payload["stats"]["point_exits"]==len(truth),"point exit accounting "+str(path))
        check(payload["stats"]["points_removed_from_input"]==0,"points removed "+str(path))
    artifacts[str(path.relative_to(capture))]=hashlib.sha256(path.read_bytes()).hexdigest()
    checks["replayed_rows"]+=1
    prefix=f"{cid}/K{k}/m{m}/z{z}/{method}:"
    check(len(labels)==len(truth)==1200,prefix+" lengths")
    grouped=[("cluster",v) if v>=0 else ("noise",i) for i,v in enumerate(labels)]
    retained=[i for i,v in enumerate(labels) if v>=0]
    metric=dict(ari_all=ari(truth,labels),nmi_all=nmi(truth,labels),
        clusters=len(set(v for v in labels if v>=0)),coverage=len(retained)/len(labels),
        noise_count=len(labels)-len(retained),ari_true_inliers=ari(truth,labels),
        ari_inliers_noise_singletons=ari(truth,grouped),
        ari_classified=ari([truth[i] for i in retained],[labels[i] for i in retained]) if len(retained)>=2 else None,
        noise_precision=None,noise_recall=None,noise_f1=None)
    for key,value in metric.items():check(same(value,row["metrics"][key]),prefix+key)
    actual=sorted(set(truth)); predicted=sorted(set(v for v in labels if v>=0))
    actualsets=[{p for p,t in enumerate(truth) if t==v} for v in actual]
    predictedsets=[{p for p,l in enumerate(labels) if l==v} for v in predicted]
    matrix=np.array([[len(a & b) for b in predictedsets] for a in actualsets],dtype=np.int64).reshape(len(actual),len(predicted))
    matching={}
    if actual and predicted:
        ix,jx=linear_sum_assignment(matrix,maximize=True)
        matching={int(i):int(j) for i,j in zip(ix,jx) if matrix[i,j]>0}
    exact=sum(a in predictedsets for a in actualsets)
    eligible=sum(len(a)>=m for a in actualsets)
    extra=row["extra"]
    for key,value in dict(truth_classes=len(actual),predicted_clusters=len(predicted),
        cluster_count_error=len(predicted)-len(actual),cluster_count_absolute_error=abs(len(predicted)-len(actual)),
        exact_classes=exact,exact_class_fraction=exact/len(actual),eligible_classes=eligible,
        classes_below_min_cluster_size=len(actual)-eligible).items():
        check(same(value,extra[key]),prefix+key)
    vals=[]
    for i,a in enumerate(actualsets):
        j=matching.get(i); b=predictedsets[j] if j is not None else set(); tp=len(a & b)
        item=dict(truth_label=actual[i],size=len(a),matched_label=predicted[j] if j is not None else None,
            matched_size=len(b),intersection=tp,precision=tp/len(b) if b else 0.0,
            recall=tp/len(a),f1=2*tp/(len(a)+len(b)),exact=a in predictedsets,eligible=len(a)>=m)
        for key,value in item.items():check(same(value,extra["per_class"][i][key]),prefix+"class"+str(i)+":"+key)
        vals.append(item)
    total=sum(v["intersection"] for v in vals)
    for key,value in dict(matched_correct_points=total,
        matched_macro_precision=statistics.mean(v["precision"] for v in vals),
        matched_macro_recall=statistics.mean(v["recall"] for v in vals),
        matched_macro_f1=statistics.mean(v["f1"] for v in vals),
        matched_micro_precision=total/len(retained) if retained else 0.0,
        matched_micro_recall=total/len(truth),matched_micro_f1=2*total/(len(truth)+len(retained))).items():
        check(same(value,extra[key]),prefix+key)

# Every reported numeric aggregate independently grouped from the private rows.
sys.path.insert(0,str(base))
import report
flat=[report.flatten(r) for r in rows]
published=report.aggregates(flat)
def metric_value(row,name):
    if name=="dendrogram_purity":return row[name]
    if name in row["metrics"]:return row["metrics"][name]
    if name in row["extra"]:return row["extra"][name]
    if name in row.get("condensation_stats",{}):return row["condensation_stats"][name]
    if name.endswith("_macro_f1"):
        return row.get(name[:-len("_macro_f1")],{}).get("macro_best_f1")
    return None
agkeys=("regime","communities","separation","k","min_cluster_size","exp_z","method")
groups=defaultdict(list)
for row in rows:groups[tuple(row[k] for k in agkeys)].append(row)
for agg in published:
    group=groups[tuple(agg[k] for k in agkeys)]
    check(agg["repetitions"]==len(group),"aggregate repetitions")
    for key,value in agg.items():
        if not key.endswith("_mean"):continue
        name=key[:-5]; values=[metric_value(row,name) for row in group]
        mean=statistics.mean(values) if values[0] is not None else None
        sd=statistics.stdev(values) if values[0] is not None else None
        check(same(value,mean),"aggregate mean "+str(tuple(agg[k] for k in agkeys))+name)
        check(same(agg[name+"_sd"],sd),"aggregate sd "+str(tuple(agg[k] for k in agkeys))+name)
check(len(published)==720,"aggregate 720 cells")
check(checks["replayed_rows"]==720,"720 replayed rows")
# Validate the closure authorities read-only.
check(receipt["sources_before"]==receipt["sources_after"],"source closure map")
for path,digest in receipt["sources_before"].items():
    check(hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest()==digest,"current source "+path)
check(hashlib.sha256(pathlib.Path(receipt["manifest_path"]).read_bytes()).hexdigest()==receipt["manifest_sha256"],"manifest digest")
check(hashlib.sha256(pathlib.Path(receipt["native_binary"]).read_bytes()).hexdigest()==receipt["native_binary_sha256"],"native binary digest")
for path,digest in receipt["input_hashes"].items():
    check(hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest()==digest,"current input "+path)
for command in receipt["commands"]:
    directory=capture/(command["case"]+"_k"+str(command["k"]))
    check(command["returncode"]==0,"command exit")
    for suffix,file in (("stdout","native.json"),("stderr","native.stderr")):
        check(hashlib.sha256((directory/file).read_bytes()).hexdigest()==command[suffix+"_sha256"],"command stream "+str(directory))

# Explicit binding not performed by the frozen report formatter itself.
qroot=args.checks.resolve()
qualification=json.loads((qroot/"receipt.json").read_text())
digest=lambda path:hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest()
common=set(qualification["sources_after"]) & set(receipt["sources_before"])
check(qualification["status"]=="passed","qualification status")
check(qualification["sources_before"]==qualification["sources_after"],"qualification source closure")
for path in common:
    check(qualification["sources_after"][path]==receipt["sources_before"][path]==digest(path),"qualification shared source "+path)
for path,expected in qualification["old_dependencies_expected"].items():
    check(qualification["sources_before"][path]==qualification["sources_after"][path]==digest(path)==expected,"qualification old dependency "+path)
check(qualification["native_binary"]==receipt["native_binary"],"qualification binary path")
check(qualification["native_binary_sha256"]==qualification["native_binary_sha256_after"]==receipt["native_binary_sha256"]==receipt["native_binary_sha256_after"]==digest(receipt["native_binary"]),"qualification binary digest")
check(qualification["native_build_receipt_sha256"]==qualification["native_build_receipt_sha256_after"]==digest(qualification["native_build_receipt"]),"qualification build receipt")
check(qualification["sklearn_before"]==qualification["sklearn_after"]==receipt["sklearn"],"qualification sklearn identity")
for path,expected in qualification["sklearn_after"]["sha256"].items():
    check(digest(path)==expected,"qualification sklearn current hash "+path)
check(len(qualification["commands"])==9,"qualification nine commands")
check(qualification["planned_commands"]==[dict(name=c["name"],argv=c["argv"]) for c in qualification["commands"]],"qualification planned argv")
for command in qualification["commands"]:
    check(command["status"]=="passed" and command["returncode"]==0,"qualification command status "+command["name"])
    for suffix in ("stdout","stderr"):
        check(digest(qroot/(command["name"]+"."+suffix))==command[suffix+"_sha256"],"qualification stream "+command["name"]+suffix)
qualification_binding=dict(common_sources=len(common),qualification_sha256=digest(qroot/"receipt.json"),
    benchmark_only=sorted(pathlib.Path(p).name for p in set(receipt["sources_before"])-set(qualification["sources_after"])),
    qualification_only=sorted(pathlib.Path(p).name for p in set(qualification["sources_after"])-set(receipt["sources_before"])),
    native_binary_sha256=receipt["native_binary_sha256"],
    eom_sha256=next(h for p,h in qualification["old_dependencies_expected"].items() if pathlib.Path(p).name=="eom.py"),
    commands=len(qualification["commands"]))

# Paired descriptive summaries, never selecting a per-scene best parameter.
summaries=[]
for regime in ("spherical","anisotropic","unbalanced"):
  for delta in (8,4,2):
    block=[r for r in rows if r["regime"]==regime and r["separation"]==delta and r["k"]==5 and r["min_cluster_size"]==20 and r["exp_z"]==1]
    entry=dict(regime=regime,separation=delta)
    for method in ("hgp_first_coverage","hdbscan_common","hdbscan_standard"):
        rr=[r for r in block if r["method"]==method]
        entry[method]={name:statistics.mean(r["metrics"][name] for r in rr) for name in ("ari_all","ari_inliers_noise_singletons","coverage","clusters")}
        entry[method]["matched_macro_f1"]=statistics.mean(r["extra"]["matched_macro_f1"] for r in rr)
        entry[method]["exact_class_fraction"]=statistics.mean(r["extra"]["exact_class_fraction"] for r in rr)
        entry[method]["dendrogram_purity"]=statistics.mean(r["dendrogram_purity"] for r in rr)
        if method!="hdbscan_standard":
            entry[method]["raw_branch_f1"]=statistics.mean(r["raw_recoverability"]["macro_best_f1"] for r in rr)
            entry[method]["condensed_branch_f1"]=statistics.mean(r["condensed_recoverability"]["macro_best_f1"] for r in rr)
    pairs={}
    for r in block:pairs.setdefault(r["case"],{})[r["method"]]=r
    differences=[pair["hgp_first_coverage"]["metrics"]["ari_inliers_noise_singletons"]-pair["hdbscan_common"]["metrics"]["ari_inliers_noise_singletons"] for pair in pairs.values()]
    entry["paired_ari_singletons"]=dict(n=len(differences),mean=statistics.mean(differences),sd=statistics.stdev(differences),wins=sum(v>1e-12 for v in differences),losses=sum(v < -1e-12 for v in differences),ties=sum(abs(v)<=1e-12 for v in differences))
    summaries.append(entry)
result=dict(schema="mhgp9_gaussian_metric_audit_v1",status="PASS" if not errors else "FAIL",errors=errors,checks=dict(checks),
    GCP_used=False,GPU_used=False,native_processes_launched=0,
    source_sha256=digest(__file__),qualification_binding=qualification_binding,
    receipt_sha256=hashlib.sha256(receipt_path.read_bytes()).hexdigest(),elapsed_seconds=receipt["elapsed_seconds"],
    sample_cases=sample,sample_artifacts=len(artifacts),
    sample_artifact_index_sha256=hashlib.sha256(json.dumps(artifacts,sort_keys=True).encode()).hexdigest(),
    aggregate_rows=len(published),summaries=summaries)
args.output.parent.mkdir(parents=True,exist_ok=True)
with args.output.open("x") as handle:
    handle.write(json.dumps(result,sort_keys=True,indent=2,allow_nan=False)+"\n")
print(json.dumps(dict(status=result["status"],errors=errors,checks=dict(checks),
    output=str(args.output.resolve()),receipt_sha256=result["receipt_sha256"]),sort_keys=True))
raise SystemExit(0 if not errors else 1)
