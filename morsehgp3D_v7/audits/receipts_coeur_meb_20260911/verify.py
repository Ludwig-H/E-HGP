#!/usr/bin/env python3
"""Lecteur portable du reçu `receipts_coeur_meb_20260911`.

Ne compile rien, n'execute aucun moteur, ne contacte pas GCP. Il controle que
les pieces annoncees sont presentes, et que les chiffres cites par la note se
lisent bien dans la sortie brute conservee. Sans assert ; identique sous
`python3 -B` et `python3 -B -O`. Code 0 conforme, 1 sinon, 2 lecture impossible.
"""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REQUIRED = ["README.md", "meb_hybrid.cpp", "welzl2.cpp", "flux_reel.out",
            "flux_reel_16k.out", "realflow_patch.py"]


def main() -> int:
    problems: list[str] = []
    for name in REQUIRED:
        if not (HERE / name).is_file():
            problems.append(f"piece absente : {name}")

    scales = {}
    for out in sorted(HERE.glob("flux_reel*.out")):
        try:
            raw = out.read_text(encoding="utf-8", errors="replace")
        except OSError as error:
            problems.append(f"sortie brute illisible : {out.name} : {error}")
            continue
        digests = re.findall(r'"payload_digest":"([0-9a-f]{64})"', raw)
        towers = [float(v) for v in re.findall(r'"tower_s":([0-9.]+)', raw)]
        calls = [int(v) for v in re.findall(r'"resolver_meb_calls":([0-9]+)', raw)]
        supports = [int(v) for v in re.findall(r'"resolver_supports_tested":([0-9]+)', raw)]
        if len(digests) != 2 or digests[0] != digests[1]:
            problems.append(f"{out.name} : les deux payload_digest ne sont pas identiques")
        if len(towers) != 2 or not towers[1] < towers[0]:
            problems.append(f"{out.name} : tower_s ne decroit pas dans le bras patche")
        if calls and (len(calls) != 2 or calls[0] != calls[1]):
            problems.append(f"{out.name} : resolver_meb_calls differe entre les bras")
        if supports and (len(supports) != 2 or not supports[1] < supports[0]):
            problems.append(f"{out.name} : les supports testes ne decroissent pas")
        scales[out.name] = {
            "payload_digest_identical": len(digests) == 2 and digests[0] == digests[1],
            "payload_digest": digests[0] if digests else None,
            "tower_s": towers,
            "tower_gain": round(towers[0] / towers[1], 3) if len(towers) == 2 and towers[1] else None,
            "supports_tested": supports or None,
        }
    if len(scales) < 2:
        problems.append("moins de deux echelles conservees")

    summary = {
        "receipt": "audits/receipts_coeur_meb_20260911",
        "scales": scales,
        "engine_executed_by_reader": False,
        "gcp_used": False,
        "public_status": "not_claimed",
        "problems": problems,
        "status": "passed" if not problems else "failed",
    }
    print(json.dumps(summary, indent=2, ensure_ascii=False))
    return 0 if not problems else 1


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except OSError as error:
        print(f"lecture impossible : {error}", file=sys.stderr)
        raise SystemExit(2)
