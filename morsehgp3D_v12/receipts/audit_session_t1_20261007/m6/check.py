#!/usr/bin/env python3
"""Relecture des prises M6 publiées et injection hôte du pilote, sans CUDA."""
import contextlib
import hashlib
import importlib.util
import io
import json
import math
from pathlib import Path
import statistics
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[4]
PIN = "4147c546000b198b5239646063bfb1e3ed6d28fc"
REL = "morsehgp3D_v12/microbancs/mes_m6_session/run_m6.py"
BASE = "morsehgp3D_v12/receipts/g4_t0a_20261007/resultats/cmd/002_m6/files/m6"


def need(ok, message):
    if not ok:
        raise RuntimeError(message)


def main():
    hashes = {}
    def read(rel):
        data = (ROOT/rel).read_bytes()
        need(data == subprocess.check_output(["git","-C",str(ROOT),"show",f"{PIN}:{rel}"]),
             "source/reçu différent du pin : "+rel)
        hashes[rel] = hashlib.sha256(data).hexdigest()
        return data
    read(REL)
    report = json.loads(read(BASE+"/m6_report.json"))
    need(report["compile"]["code"] == 0 and len(report["runs"]) == 9, "compilation ou prises manquantes")
    need({(r["mode"],r["process"]) for r in report["runs"]} ==
         {(m,i) for m in ("spin","yield","blocking") for i in range(3)}, "processus dupliqué/absent")
    metrics = ("context_open","first_launch","empty_kernel_launch_sync","graph_of_ten_sync","touch_256mib")
    summary = {}
    for mode in ("spin","yield","blocking"):
        selected = {key:[] for key in metrics}
        for i in range(3):
            rows = [json.loads(x) for x in read(BASE+f"/m6_{mode}_{i}.jsonl").splitlines()]
            run = next(r for r in report["runs"] if (r["mode"],r["process"]) == (mode,i))
            need(run["code"] == 0 and run["lines"] == len(rows) == 65, "prise incomplète")
            keys = [(x["name"],x.get("bytes"),x.get("host"),x.get("dir")) for x in rows]
            need(len(set(keys)) == 65, "mesure dupliquée")
            need(next(x for x in rows if x["name"] == "device")["sync"] == mode, "mode différent")
            for row in rows:
                need(row["mes"] == "M6", "schéma de ligne")
                if "p50_us" in row:
                    vals = [row[k] for k in ("p05_us","p50_us","p95_us","max_us")]
                    need(all(math.isfinite(x) and x >= 0 for x in vals) and vals == sorted(vals), "quantiles invalides")
                    need(row["n"] >= 10, "pas assez de répétitions")
                    need((row["name"]+"_first",row.get("bytes"),row.get("host"),row.get("dir")) in keys,
                         "premier usage absent")
                elif "us" in row:
                    need(row["n"] == 1 and math.isfinite(row["us"]) and row["us"] >= 0, "usage unique invalide")
            for key in metrics:
                row = next(x for x in rows if x["name"] == key)
                selected[key].append(row.get("p50_us",row.get("us")))
        summary[mode] = {k:{"process_values_us":v,"median_across_processes_us":statistics.median(v)}
                         for k,v in selected.items()}
    # main et son interprétation des codes restent inchangés ; seules les frontières externes sont remplacées.
    spec = importlib.util.spec_from_file_location("audit_m6_driver",ROOT/REL)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    module.find_nvcc = lambda explicit: "/synthetic/nvcc"
    captures = []
    def capture(command, timeout):
        captures.append(command)
        return 0,"",""
    module.capture = capture
    with tempfile.TemporaryDirectory(prefix="v12-audit-m6-driver-") as tmp:
        out = Path(tmp)/"out"
        with contextlib.redirect_stdout(io.StringIO()):
            code = module.main(["run_m6.py","--work",str(Path(tmp)/"work"),"--out",str(out)])
        emitted = json.loads((out/"m6_report.json").read_text())
        need(code == 0 and len(emitted["runs"]) == 9 and all(r["lines"] == 0 for r in emitted["runs"]),
             "comportement du pilote modifié")
        injection = {"exit":code,"runs":9,"all_outputs_empty":True,"verdict":"mes_m6_ok",
                     "scope":"external capture replaced; real main; no compiler or GPU launched"}
    for path, expected in hashes.items():
        need(hashlib.sha256((ROOT/path).read_bytes()).hexdigest() == expected, "dérive pendant contrôle")
    print(json.dumps({"pin":PIN,"sources_and_receipts":hashes,"validated_processes":9,
        "validated_rows":585,"summary":summary,"empty_driver_injection":injection,
        "scope":"published G4 values re-read; no new GCP/GPU execution"},indent=2,sort_keys=True))


if __name__ == "__main__":
    main()
