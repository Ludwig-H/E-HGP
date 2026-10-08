#!/usr/bin/env python3
"""Sources reconstruites et fixtures publiees ; aucun moteur natif."""
import copy
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
import types
import argparse

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
PRIOR = Path("morsehgp3D_v12/receipts/audit_reponses_20261007")
PLAN = Path("morsehgp3D_v12/receipts/audit_d6_20261007/pilotage")


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def load(raw, name):
    module = types.ModuleType(name)
    module.__file__ = str(ROOT / "morsehgp3D_v12/microbancs/mes_d6_profils/pilote_d6.py")
    exec(compile(raw, name, "exec"), module.__dict__)
    return module


def git(pin, path):
    return subprocess.check_output(["git", "show", pin + ":" + str(path)], cwd=ROOT)


def apply(root, path):
    for extra in (["--check"], []):
        subprocess.run(["git", "apply", *extra, str(path)], cwd=root, check=True,
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE)


def replay():
    pins = json.loads((HERE / "capture.json").read_text())
    for path, expected in pins["dependencies"].items():
        require(sha((ROOT / path).read_bytes()) == expected, "dependance differente : " + path)
    require(sha(git(pins["delivered_commit"], pins["source"])) == pins["adapted_worktree_sha256"],
            "objet Git livre different de la copie rejouee")
    schema_pin = pins["shared_schema"]
    shared = git(schema_pin["commit"], schema_pin["path"])
    require(sha(shared) == schema_pin["sha256"], "schema livre different")
    schema = load(shared, "shared_schema")
    with tempfile.TemporaryDirectory(prefix="d6_compat_") as name:
        root = Path(name)
        target = root / pins["source"]
        target.parent.mkdir(parents=True)
        target.write_bytes(git(pins["base_pin"], pins["source"]))
        apply(root, ROOT / PRIOR / "d6_admission_proposition/proposition.patch")
        require(sha(target.read_bytes()) == pins["strict_proposal_sha256"], "proposition ancienne differente")
        old = load(target.read_bytes(), "strict_old")
        apply(root, HERE / "adaptation_developpeur.patch")
        raw = target.read_bytes()
        require(sha(raw) == pins["adapted_worktree_sha256"], "adaptation differente")
        active = load(raw, "adapted")
        require(active.LEDGER_KEYS == schema.LEDGER and active.CAT_DIAG_KEYS == schema.DIAGNOSTICS and
                active.DEVICE_KEYS == schema.DEVICE, "listes divergentes du schema livre")
        fixture = ROOT / PRIOR / "cuda_juge_proposition/cpu_format.jsonl"
        rows = [json.loads(line) for line in fixture.read_text().splitlines()]
        expected = dict(passes=3, coord_bits=21, kmax=5, threads=1, leaf=24)
        summarize = lambda m, r, e=expected: m.summarize(0, r, "catalogue", "catalogue_sha256", e)
        require(not summarize(old, rows)["ok"] and summarize(active, rows)["ok"], "compatibilite non reproduite")
        cases = {"native_cpu_t1b": True}
        mutations = [("device_path", lambda r: r.update(path="device")),
                     ("device_nonzero", lambda r: r["device"].update(allocations=1)),
                     ("missing_chain_counter", lambda r: r["diagnostics"].pop("chains_repaired")),
                     ("bool_device", lambda r: r["device"].update(allocations=False))]
        for name, mutate in mutations:
            changed = copy.deepcopy(rows)
            mutate(changed[0])
            cases[name] = summarize(active, changed)["ok"]
            require(not cases[name], "corruption admise : " + name)
        for bits in (24, 32):
            changed = copy.deepcopy(rows)
            for row in changed:
                if row["phase"] == "catalogue":
                    row["coord_bits"] = bits
            cases["synthetic_profile_" + str(bits)] = summarize(active, changed, dict(expected, coord_bits=bits))["ok"]
            require(cases["synthetic_profile_" + str(bits)], "forme de profil refusee")
        grows = [json.loads(line) for line in (ROOT / PRIOR / "t2c_pilote_proposition/before.jsonl").read_text().splitlines()]
        g = active.summarize(0, grows, "tour_g", "resolution_sha256", dict(expected, passes=2))
        require(g["ok"], "forme G actuelle refusee")
        plans = load((ROOT / PLAN / "check.py").read_bytes(), "plan_witnesses")
        specs = [dict(name="reference_present", largest=1),
                 dict(name="u21_absent_u24_present", largest=1 << 21),
                 dict(name="all_combinations_absent", largest=1 << 21, profiles="21"),
                 dict(name="duplicate_profile", largest=1, profiles="21,21,24"),
                 dict(name="duplicate_case", largest=1, cases="test,test")]
        before = [plans.witness(raw.decode(), spec) for spec in specs]
        apply(root, ROOT / PLAN / "proposition.patch")  # proposition deja publiee, aucun patch duplique
        fixed_raw = target.read_bytes()
        after = [plans.witness(fixed_raw.decode(), spec) for spec in specs]
        require([case["code"] for case in after] == [0, 2, 2, 2, 2], "admission du plan non corrigee")
    return dict(compatibility={"old_reader_native_cpu": False, "adapted_native_cpu": True,
                               "native_G_format": True, "shared_schema_lists_equal": True},
                catalogue_cases=cases, plan_patch_applies=True, after_plan_sha256=sha(fixed_raw),
                plan=[dict(case=spec["name"], before=a, after=b) for spec, a, b in zip(specs, before, after)],
                native_engine_run=False)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--repo-root", type=Path, default=ROOT)
    args = parser.parse_args()
    ROOT = args.repo_root.resolve()
    result = replay()
    if args.check:
        require(result == json.loads((HERE / "results.json").read_text()), "resultats differents")
        print("d6_compatibilite_ok: lecteur, forme CPU/G et cinq plans ; aucun moteur")
    else:
        print(json.dumps(result, sort_keys=True, indent=1))
