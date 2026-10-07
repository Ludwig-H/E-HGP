#!/usr/bin/env python3
"""Relecture locale, journaux externes epingles ; aucune execution HGP."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile

HERE = Path(__file__).resolve().parent
PROPOSAL = HERE.parent / "t2c_pilote_proposition"


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def replay(root):
    pins = json.loads((HERE / "manifest.json").read_text())
    raw = (root / "rapport_t2c.json").read_bytes()
    require(sha(raw) == pins["rapport_sha256"], "autre rapport")
    checker = PROPOSAL / "check.py"
    require(sha(checker.read_bytes()) == pins["checker_sha256"], "autre lecteur de proposition")
    for path, digest, size in pins["journaux"]:
        data = (root / path).read_bytes()
        require(sha(data) == digest and len(data) == size, "autre journal : " + path)
    spec = importlib.util.spec_from_file_location("proposition_check", checker)
    helper = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(helper)
    report = json.loads(raw)
    camp, cohorts = report["campagne_k5"], {}
    with tempfile.TemporaryDirectory(prefix="t2c_local_read_") as temporary:
        tmp = Path(temporary)
        module = helper.prepared(tmp)
        require(sha((tmp / helper.REL).read_bytes()) == pins["reader_sha256"], "autre pilote propose")
        for frame, turns in camp["trames"].items():
            sites, digests, count = set(), set(), 0
            for turn in turns:
                for arm, take in turn.items():
                    got = module.lire_prise(str(root / take["journal"]), take["code"], 5,
                                           camp["fils"], camp["passes"], module.schema_bras(arm))
                    for key in ("murs_ns", "empreinte", "g_ns", "diagnostics", "profil"):
                        require(json.dumps(got[key], sort_keys=True) == json.dumps(take[key], sort_keys=True),
                                "resume divergent : " + key)
                    sites.add(got["sites"])
                    digests.add(got["empreinte"])
                    count += 1
            cohorts[frame] = dict(admises=count, sites=sorted(sites), digests_distincts=len(digests))
        judged = module.juger(report, str(root))
    require((root / "rapport_t2c.json").read_bytes() == raw, "rapport modifie")
    for path, digest, _ in pins["journaux"]:
        require(sha((root / path).read_bytes()) == digest, "journal modifie")
    return dict(config=dict(K=5, W=camp["fils"], P=camp["passes"], tours=camp["tours_demandes"]),
                cohorte=cohorts, refus_lecture=0, verdicts=judged["verdicts"], refus=judged["refus"],
                ratios={f: {k: d[k]["moyenne_geometrique"] for k in ("lot_t2c", "A/A")}
                        for f, d in judged["cas"].items()}, stable=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--journaux", required=True, type=Path, help="dossier contenant rapport_t2c.json et journaux/")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    result = replay(args.journaux)
    if args.check:
        require(result == json.loads((HERE / "results.json").read_text()), "resultats differents")
        print("t2c_local_ok: 30 prises admises, 4 leviers refuses pour quotas ; aucun moteur")
    else:
        print(json.dumps(result, sort_keys=True, indent=1))
