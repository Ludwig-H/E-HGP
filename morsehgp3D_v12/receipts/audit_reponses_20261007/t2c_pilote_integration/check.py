#!/usr/bin/env python3
"""Contrelecture du pilote integre, source reconstruite ; aucune sonde HGP."""
import argparse
import contextlib
import hashlib
import importlib.util
import inspect
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent


def require(ok, msg):
    if not ok:
        raise RuntimeError(msg)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def helper():
    path = HERE.parent / "t2c_pilote_proposition/check.py"
    spec = importlib.util.spec_from_file_location("published_check", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def catalogue_witness(module, fixed, root):
    # Champs portes de la branche catalogue du lecteur D6 publie (pas son exigence --digest).
    fields = json.loads((HERE / "capture.json").read_text())["catalogue_schema"]
    require(set(fields["ledger"]) == fixed.CAT_LEDGER and set(fields["diagnostics"]) == fixed.CAT_DIAG,
            "champs differents du producteur C++ capture")
    data = []
    for p in range(3):
        data.append(dict(phase="catalogue", **{"pass": p}, status="ok", reason="none", coord_bits=21,
                         kmax=5, leaf=24, threads=3, sites=8, wall_ns=100, balls=1, incidences=2, levels=1,
                         ledger=dict.fromkeys(fields["ledger"], 0), diagnostics=dict.fromkeys(fields["diagnostics"], 0)))
    data.append(dict(phase="exit", status="ok", reason="none"))
    exe = root / "fake_catalogue.py"
    exe.write_text("#!" + sys.executable + "\nimport pathlib,sys\n"
                   "sys.stdout.write((pathlib.Path(__file__).parent/'cat_wire').read_text())\n")
    exe.chmod(0o700)
    outcomes = []
    for name in ("forme_nominale", "autre_configuration"):
        if name == "autre_configuration":
            for row in data[:3]:
                row["kmax"], row["threads"], row["coord_bits"] = 10, 1, 24
        (root / "cat_wire").write_text("".join(json.dumps(row) + "\n" for row in data))
        before = module.prise_catalogue(str(exe), [], 3, 3, str(root / "cat_before.jsonl"))
        after = fixed.prise_catalogue(str(exe), [], 3, 3, str(root / "cat_after.jsonl"))
        require(before["valide"] is True and after["valide"] is (name == "forme_nominale"), "temoin catalogue")
        outcomes.append(dict(cas=name, actif_admis=before["valide"], proposition_admise=after["valide"],
                             motif=after.get("admission")))
    return outcomes


def replay(logs):
    published = helper()
    pins = json.loads((HERE / "capture.json").read_text())
    with tempfile.TemporaryDirectory(prefix="t2c_integration_") as name:
        root = Path(name)
        published.prepared(root)
        for extra in (["--check"], []):
            subprocess.run(["git", "apply", *extra, str(HERE / "integration.patch")], cwd=root, check=True,
                           stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        source = (root / published.REL).read_bytes()
        require(sha(source) == pins["prototype_sha256"], "autre prototype")
        module = published.load(source, "integration")
        old, capture = published.fixtures()
        trial = published.trials(module, root, old, capture)
        nominal = published.native_formats(module, root)
        # Seule adaptation du temoin publie : huit -> dix tours, nouveau minimum.
        campaign_source = inspect.getsource(published.campaigns)
        require(campaign_source.count('"tours_demandes": 8') == 1 and campaign_source.count('range(8)') == 1,
                "temoin de campagne modifie")
        campaign_source = campaign_source.replace('"tours_demandes": 8', '"tours_demandes": 10').replace('range(8)', 'range(10)')
        namespace = dict(published.__dict__)
        exec(compile(campaign_source, "campaign_10.py", "exec"), namespace)
        campaign = namespace["campaigns"](module, root, old, capture)
        with contextlib.redirect_stdout(io.StringIO()) as auto_output:
            auto = module.etape_auto_test()
        require(auto == 0, "auto-test invalide")
        gates = []
        original_auto, original_report = module.etape_auto_test, module.etape_rapport
        for outcome in (0, 1):
            events = []
            module.etape_auto_test = lambda: (events.append("auto-test") or outcome)
            module.etape_rapport = lambda *_: (events.append("rapport") or {"verdicts": {}, "refus": []})
            with contextlib.redirect_stdout(io.StringIO()):
                status = module.main(["pilote", "rapport", "--sortie", str(root / "sortie"),
                                      "--travail", str(root), "--processus", "10"])
            require(events == (["auto-test", "rapport"] if outcome == 0 else ["auto-test"]), "garde rapport")
            gates.append(dict(auto_test=outcome, code=status, events=events))
        module.etape_auto_test, module.etape_rapport = original_auto, original_report
        with contextlib.redirect_stdout(io.StringIO()):
            default = module.main(["pilote", "rapport", "--sortie", str(root / "sortie"), "--travail", str(root)])
        require(default == 2, "defaut CLI inattendu")
        for extra in (["--check"], []):
            subprocess.run(["git", "apply", *extra, str(HERE / "catalogue_metadata_proposed.patch")],
                           cwd=root, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        fixed = published.load((root / published.REL).read_bytes(), "catalogue_proposition")
        c_witness = catalogue_witness(module, fixed, root)
        fixed.etape_auto_test = lambda: 0
        fixed.etape_rapport = lambda *_: {"verdicts": {}, "refus": []}
        with contextlib.redirect_stdout(io.StringIO()):
            fixed_default = fixed.main(["pilote", "rapport", "--sortie", str(root / "sortie"), "--travail", str(root)])
        require(fixed_default == 0, "defaut CLI propose invalide")
        manifests = json.loads((HERE.parent / "t2c_pilote_essai_local/manifest.json").read_text())
        raw = (logs / "rapport_t2c.json").read_bytes()
        require(sha(raw) == manifests["rapport_sha256"], "rapport local modifie")
        for path, digest, size in manifests["journaux"]:
            data = (logs / path).read_bytes()
            require(sha(data) == digest and len(data) == size, "journal local modifie")
        report = json.loads(raw)
        checked = 0
        for turns in report["campagne_k5"]["trames"].values():
            for turn in turns:
                for arm, take in turn.items():
                    got = module.lire_prise(str(logs / take["journal"]), take["code"], 5, 3, 3, module.schema_bras(arm))
                    require(got["valide"], "prise reelle refusee")
                    checked += 1
        local = module.juger(report, str(logs))
        for path, digest, _ in manifests["journaux"]:
            require(sha((logs / path).read_bytes()) == digest, "journal modifie pendant lecture")
        require((logs / "rapport_t2c.json").read_bytes() == raw, "rapport modifie pendant lecture")
    return dict(prises=len(trial), admises=sum(t["valide"] for t in trial),
                formats=nominal, campagnes=campaign, auto_test=auto_output.getvalue().strip(),
                garde_rapport=gates, cli_sans_processus=default, minimum_tours=module.REGLE_T2C["processus_min"],
                catalogue_informatif=c_witness, cli_sans_processus_propose=fixed_default,
                local=dict(prises_admises=checked, verdicts=local["verdicts"], refus=local["refus"]))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--journaux", type=Path, required=True)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    result = replay(args.journaux)
    if args.check:
        require(result == json.loads((HERE / "results.json").read_text()), "resultats differents")
        print("t2c_integration_ok: schema G et garde rapport ; aucun moteur")
    else:
        print(json.dumps(result, sort_keys=True, indent=1))
