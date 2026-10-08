#!/usr/bin/env python3
"""Lecture de métadonnées archivées ; ne lance aucune sonde ni compilation."""
import argparse
import hashlib
import json
from pathlib import Path
import re


def need(ok, message):
    if not ok:
        raise ValueError(message)


def identities(text):
    cases = [(f"lidar_ng0{i}", 5) for i in range(3)]
    cases += [(f"uniform_u18_n{n}", 5) for n in (8000, 16000, 32000)]
    cases += [(f"lidar_ng0{i}", 10) for i in range(3)]
    expected = [(name, k, side) for name, k in cases for side in ("avant", "apres")]
    runs, current, finished = [], None, False
    for line in text.splitlines():
        start = re.fullmatch(r"=== (\w+) K(\d+) (avant|apres) (\d\d:\d\d:\d\d)", line)
        if start:
            need(current is None and not finished, "processus chevauchés")
            current = {"key": (start[1], int(start[2]), start[3]), "rows": [], "transit": None}
        elif re.fullmatch(r"=== fin \d\d:\d\d:\d\d", line):
            need(current is None, "fin pendant processus")
            finished = True
        elif line.startswith("code "):
            need(current is not None and line == "code 0", "code absent/non nul")
            runs.append(current)
            current = None
        elif line.startswith("{"):
            need(current is not None, "JSON hors processus")
            row = json.loads(line)
            if set(row) == {"transit_octets"}:
                need(current["transit"] is None, "transit répété")
                current["transit"] = row["transit_octets"]
            else:
                current["rows"].append(row)
        else:
            need(not line.strip(), "ligne inconnue")
    need(finished and current is None, "journal incomplet")
    need([r["key"] for r in runs] == expected, "cohorte différente")
    out = []
    sigkeys = ("boules", "niveaux", "catalogue_sha256", "niveaux_sha256", "table_sha256")
    for i, (name, k) in enumerate(cases):
        pair = runs[2*i:2*i+2]
        signatures = []
        for run in pair:
            paths = ["cpu", "appareil_pool", "appareil_comptes_cuda"]
            if run["key"][2] == "apres":
                paths.append("appareil_transit")
            need([x["voie"] for x in run["rows"]] == paths, "voies différentes")
            for row in run["rows"]:
                need(type(row["table_ecarts"]) is int and row["table_ecarts"] == 0, "table différente")
                for key in ("boules", "niveaux", "pic_hote", "pic_appareil"):
                    need(type(row[key]) is int and row[key] >= 0, "compte invalide")
                for key in sigkeys[2:]:
                    need(re.fullmatch(r"[0-9a-f]{64}", row[key]) is not None, "empreinte invalide")
                signatures.append(tuple(row[key] for key in sigkeys))
        need(len(set(signatures)) == 1, "identité différente")
        out.append({"name": name, "k": k, "balls": signatures[0][0], "levels": signatures[0][1],
                    "rows_equal": len(signatures),
                    "model_host_peak": [r["rows"][2]["pic_hote"] for r in pair],
                    "model_device_peak": [r["rows"][2]["pic_appareil"] for r in pair],
                    "model_transit_bytes": [r["transit"] for r in pair]})
    return {"processes_closed_zero": len(runs), "pairs": out}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--evidence", type=Path, required=True)
    args = parser.parse_args()
    capture = json.loads(Path(__file__).with_name("capture.json").read_text())
    for rel, expected in capture["files"].items():
        raw = (args.evidence / rel).read_bytes()
        need(len(raw) == expected["bytes"] and hashlib.sha256(raw).hexdigest() == expected["sha256"], "pin différent : " + rel)
    result = identities((args.evidence / "essais/identite_902.log").read_text())
    build = (args.evidence / "build_902.log").read_text()
    need(re.search(r"^b902 code 0 \d\d:\d\d:\d\d$", build, re.M), "build candidat non clos")
    need(re.search(r"^bbase902 code 0 \d\d:\d\d:\d\d$", build, re.M), "build base non clos")
    cache = (args.evidence / "b902/CMakeCache.txt").read_text()
    for setting in ("CMAKE_BUILD_TYPE:STRING=Release", "MHGP12_COORD_BITS:STRING=21", "MHGP12_ENABLE_CUDA:BOOL=OFF"):
        need(setting in cache, "configuration différente : " + setting)
    need(result == capture["result"], "résultat différent")
    print(json.dumps(result, ensure_ascii=False, sort_keys=True, separators=(",", ":")))


if __name__ == "__main__":
    main()
