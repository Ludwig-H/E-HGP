"""Recompute object comparisons from collected case JSON, no geometry or labels."""
import argparse
import csv
from fractions import Fraction
import hashlib
import json
from pathlib import Path


def collect(directories):
    rows, cases = [], []
    for directory in directories:
        for path in sorted(directory.glob("*.json")):
            data = json.loads(path.read_text())
            if data.get("schema") != "ehgp.audit.full_points.case.v1":
                continue
            if data.get("status") != "ok":
                raise ValueError("incomplete case: " + str(path))
            cases.append(dict(name=data["name"], sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                              sites=data["sites"], path=str(path)))
            for k, order in data["projection"]["orders"].items():
                reference = data["hdbscan"][k]["best_iou"]
                for name, value in order.items():
                    methods = (("qualified_m" + m, result) for m, result in value.items()) \
                        if name == "qualified" else ((name, value),)
                    for method, result in methods:
                        for target, best in result["best_iou"].items():
                            delta = Fraction(best["iou_exact"]) - Fraction(reference[target]["iou_exact"])
                            rows.append(dict(case=data["name"], k=int(k), method=method, target=int(target),
                                             target_size=best["target_size"], iou=best["iou"],
                                             hdbscan_iou=reference[target]["iou"], delta=float(delta),
                                             comparison="win" if delta > 0 else "loss" if delta < 0 else "tie"))
    return cases, rows


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--campaign", type=Path, action="append", required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--require-all", action="store_true")
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    cases, rows = collect(args.campaign)
    names = [case["name"] for case in cases]
    if len(names) != len(set(names)):
        raise ValueError("duplicate case names")
    if args.require_all:
        config = json.loads(Path(__file__).with_name("campaign.json").read_text())
        expected = {spec["family"] + "_" + str(spec["seed"]) for spec in config["synthetic"]}
        expected |= {"zoltan_" + name for name in ("01_velos_en_rang", "02_velos_contre_facade",
                       "03_pieton_contre_facade", "04_velos_en_rang_avec_sol", "05_temoin_voitures_en_file")}
        if set(names) != expected:
            raise ValueError("incomplete campaign: " + repr(sorted(expected - set(names))))
    if not rows:
        raise ValueError("no completed cases")
    summary = []
    keys = sorted({("zoltan" if row["case"].startswith("zoltan_") else "synthetic", row["k"], row["method"])
                   for row in rows})
    for scope, k, method in keys:
        group = [row for row in rows if ("zoltan" if row["case"].startswith("zoltan_") else "synthetic") == scope
                 and row["k"] == k and row["method"] == method]
        summary.append(dict(scope=scope, k=k, method=method, objects=len(group),
                            mean_iou=sum(row["iou"] for row in group)/len(group),
                            hdbscan_mean_iou=sum(row["hdbscan_iou"] for row in group)/len(group),
                            wins=sum(row["comparison"] == "win" for row in group),
                            losses=sum(row["comparison"] == "loss" for row in group),
                            ties=sum(row["comparison"] == "tie" for row in group)))
    with (args.out / "objects.csv").open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)
    (args.out / "comparison.json").write_text(json.dumps(dict(schema="ehgp.audit.full_points.comparison.v1",
        cases=cases, rows=len(rows), summary=summary,
        scope="best closed-plateau hierarchy node IoU; selected development examples; no flat partition"),
        indent=2, sort_keys=True, allow_nan=False) + "\n")


if __name__ == "__main__":
    main()
