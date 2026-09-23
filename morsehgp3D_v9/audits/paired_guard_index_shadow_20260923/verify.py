#!/usr/bin/env python3
"""Static reader for the pinned indexed-palette shadow, without /tmp inputs."""

from collections import defaultdict
from hashlib import sha256
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
OLD = HERE.parent / "paired_guards_precore_20260923"


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def expected():
    rows = []
    lines = ["seed a b F mask q3_4 q4_4 closed_4 q3_8 q4_8 closed_8 q3_16 q4_16 closed_16"]
    for filename in ("RESULT.json", "RESULT_SEED2.json"):
        payload = json.loads((OLD / filename).read_text())
        for row in payload["rows"]:
            record = [payload["sample_seed"], row["a"], row["b"], row["F"], row["mask"]]
            for budget in (4, 8, 16):
                result = row["results"][str(budget)]
                record.extend((result["q3_pairs"], result["q4_pairs"], int(result["closed"])))
            rows.append(record)
            lines.append(" ".join(map(str, record)))
    require((HERE / "SAMPLES.tsv").read_text() == "\n".join(lines)+"\n", "sample table changed")
    require(len(rows)==120, "120 samples expected")
    return rows


def budget_receipt(cap, rows):
    lines=(HERE / f"BUDGET{cap}.stdout").read_text().splitlines()
    require(len(lines)==481,"budget output length")
    require(lines[0].startswith("META "),"budget META")
    totals=defaultdict(lambda: defaultdict(int))
    for i,row in enumerate(rows):
        seed,a,b,F,mask,*reference=row
        chunk=lines[1+i*4:1+(i+1)*4]
        require(chunk[0].startswith(f"ORACLE {seed} {a} {b} {F} "),"budget oracle identity")
        for j,B in enumerate((4,8,16)):
            fields=chunk[1+j].split()
            require(fields[0]=="BUDGET","budget tag")
            v=list(map(int,fields[1:]))
            require(len(v)==17 and v[:6]==[seed,a,b,F,mask,B],"budget row identity")
            candidates,hit,q3,q4,closed=v[6:11]
            require(0<=candidates<=4*B and 0<=hit<=4 and closed in (0,1),"budget row bounds")
            require(v[13]<=4*cap and v[16]<=v[13],"budget visit cap")
            if cap==256:
                require(hit==0 and [q3,q4,closed]==reference[j*3:j*3+3],"256 must be exact")
            if cap==64 and closed:
                require(reference[j*3+2]==1,"budget proof lacks independent published closure")
            out=totals[str(B)]
            for key,value in zip(("candidates","interrupted_quadrants","q3_pairs","q4_pairs",
                                  "closed","query_ns","matching_ns","node_pops",
                                  "box_tests","leaf_tests","site_tests"),v[6:]):
                out[key]+=value
            out["F_closed"]+=F*closed
            out["edges_with_interruption"]+=int(hit>0)
    return {k:dict(v) for k,v in sorted(totals.items())}


def main():
    name = sys.argv[1] if len(sys.argv)>1 else "RESULT.stdout"
    lines = (HERE / name).read_text().splitlines()
    require(len(lines)==601, "one META, 120 ORACLE, 120 SHARED, 360 ROW lines")
    tag, *meta = lines[0].split()
    require(tag=="META" and len(meta)==8, "meta schema")
    meta=list(map(int,meta))
    require(meta[0]==123389 and meta[1]==2*meta[0]-1, "index shape")
    require(meta[6]>=meta[0] and 0<meta[7]<=54, "index build work")
    rows=expected()
    acc=defaultdict(lambda: defaultdict(int))
    for index,row in enumerate(rows):
        seed,a,b,F,mask,*reference=row
        block=lines[1+index*5:1+(index+1)*5]
        ora=block[0].split(); shared=block[1].split()
        require(ora[0]=="ORACLE" and shared[0]=="SHARED", "block order")
        oi=list(map(int,ora[1:])); si=list(map(int,shared[1:]))
        require(oi[:4]==[seed,a,b,F] and si[:4]==[seed,a,b,F], "edge identity")
        require(len(oi)==5 and len(si)==13, "oracle/shared schema")
        acc["oracle"]["ns"]+=oi[4]
        for label,data in (("shared",si),):
            d=acc[label]
            d["rows"]+=1;d["query_ns"]+=data[4]
            for field,value in zip(("node_pops","box_tests","box_rejects","leaf_tests",
                                    "site_tests","queue_pushes","frontier_cuts","queue_peak"),data[5:]):
                if field=="queue_peak":d[field]=max(d[field],value)
                else:d[field]+=value
        for j,budget in enumerate((4,8,16)):
            fields=block[2+j].split()
            require(fields[0]=="ROW", "row tag")
            vals=list(map(int,fields[1:]))
            require(len(vals)==20, "row schema")
            require(vals[:6]==[seed,a,b,F,mask,budget],"row identity")
            candidates,q3,q4,closed=vals[6:10]
            require([q3,q4,closed]==reference[j*3:j*3+3],"published matching differs")
            require(0<=candidates<=4*budget and closed in (0,1),"candidate bounds")
            d=acc[str(budget)];d["rows"]+=1;d["closed"]+=closed;d["F_closed"]+=F*closed
            d["query_ns"]+=vals[10];d["matching_ns"]+=vals[11]
            d["candidates"]+=candidates
            for field,value in zip(("node_pops","box_tests","box_rejects","leaf_tests",
                                    "site_tests","queue_pushes","frontier_cuts","queue_peak"),vals[12:]):
                if field=="queue_peak":d[field]=max(d[field],value)
                else:d[field]+=value
            split=acc[str(budget)+("_closed" if closed else "_open")]
            split["rows"]+=1;split["F"]+=F
            split["query_ns"]+=vals[10];split["matching_ns"]+=vals[11]
            split["node_pops"]+=vals[12];split["box_tests"]+=vals[13]
            split["site_tests"]+=vals[16]
    require(acc["shared"]["site_tests"]==acc["16"]["site_tests"],"shared versus separate core visits")
    require(acc["16"]["closed"]==67 and acc["16"]["F_closed"]==280728,"published potential")
    require(acc["shared"]["site_tests"]<120*meta[0],"index did not reduce point tests")
    trigger=json.loads((HERE / "TRIGGER_D.json").read_text())
    require(trigger["schema"]=="mhgp9_audit_precore_length_dispatch_v1", "dispatch schema")
    require(trigger["all_S2_survivors"]["all"]==3986433, "full S2 survivor count")
    require(trigger["threshold_D_ge_2_power"]["22"]["all"]==909278, "dispatch D22 count")
    require(trigger["stratified_120_sample"]["22"]["closable_B16"]==67, "dispatch sample")
    summary={
        "schema":"mhgp9_audit_indexed_paired_palette_v1",
        "source_archive_sha256":"208aabb30a764bad25ab1c99d74885bd405e84a13dcb8375622d66aa6203f70a",
        "input_points_sha256":"233cc4ea8cac6e0b1155ea845af57b32e5236764bf2e119557aeab5bac76c172",
        "input_raw_ids_sha256":"796e9814dd2dff7778ad8d1bb55f5113ab54206ff8b246e6432200f9a6833f7f",
        "index":{"cloud_sites":meta[0],"nodes":meta[1],"cloud_prepare_ns":meta[2],
                 "index_build_ns":meta[3],"cloud_retained_bytes":meta[4],
                 "index_retained_bytes":meta[5],"index_build_point_visits":meta[6],
                 "max_depth":meta[7]},
        "oracle_fullscan_site_tests":120*meta[0],
        "counts":{k:dict(v) for k,v in sorted(acc.items())},
        "budget64":budget_receipt(64,rows),
        "budget256":budget_receipt(256,rows),
        "dispatch_D":{"S2_survivors":trigger["all_S2_survivors"]["all"],
                      "D_ge_2_22":trigger["threshold_D_ge_2_power"]["22"]["all"],
                      "D_ge_2_24":trigger["threshold_D_ge_2_power"]["24"]["all"]},
        "note":"One host CPU run only; times are sidecar intervals, not product or tour time."
    }
    print(json.dumps(summary,indent=2,sort_keys=True))


if __name__=="__main__":
    main()
