"""Pinned control-flow audit only: AST-extracted functions and explicit non-geometric stubs.

No original modules are imported; no Scene, exporter, Gamma, sklearn, engine, or
filesystem output is executed. Snapshot differences are injected into empreintes.
"""
import argparse
import ast
import contextlib
from fractions import Fraction
import hashlib
import io
import json
import os
from pathlib import Path
import sys
import time
from types import SimpleNamespace

ROOT = Path(__file__).resolve().parent
PINS = {
    "source/run_target.py": "41e92016a967180812549e5c3851b79baf510110404e5b333f600973193d50cf",
    "source/valide_lib.py": "94a96b1f0fcdd5032e13323aa065e44bb16f5e27abd397f5c1e16fcbebd2e6ad",
}


def require(ok, message):
    if not ok:
        raise ValueError(message)


def selected(path, names, namespace):
    tree = ast.parse((ROOT / path).read_text())
    nodes = [n for n in tree.body if isinstance(n, (ast.FunctionDef, ast.ClassDef)) and n.name in names]
    require({n.name for n in nodes} == set(names), "AST function inventory")
    module = ast.Module(body=nodes, type_ignores=[])
    exec(compile(module, path, "exec"), namespace)
    return namespace


class Incoherence(RuntimeError):
    pass


class RegleErreur(RuntimeError):
    pass


class MemoryFile(io.StringIO):
    def close(self):
        pass


def source_change_case(mode):
    written = {}
    payloads = {"masses": {"findings": {}}, "anchor": {}, "geom": {}, "shell": {}}
    calls = []
    before = {"frozen_unit": "0" * 64}
    after = {"frozen_unit": ("1" if mode == "changed" else "0") * 64}
    snapshots = iter((before, after))

    def fingerprint():
        calls.append("empreintes")
        return next(snapshots)

    def mocked_open(path, how="r", **_kw):
        if how == "w":
            f = MemoryFile()
            written[path] = f
            return f
        return MemoryFile(json.dumps(payloads.get(path, {})))

    def stub_group(*args):
        J = args[0]
        group = len(J.lignes) + 1
        ok = not (mode == "test_failure" and group == 1)
        J.check("stub" + str(group), "explicit harness control", ok, True, ok, "stub, not geometry")

    namespace = {
        "argparse": argparse, "json": json, "time": time, "sys": sys,
        "hashlib": hashlib, "open": mocked_open,
        "os": SimpleNamespace(path=os.path, makedirs=lambda *_a, **_k: None),
        "RG": SimpleNamespace(
            empreintes=fingerprint, sha_fichier=lambda _p: "2" * 64,
            CADRE="AST control-flow stubs; no mathematical qualification", SCRATCH="virtual"),
        "RECU_MASSES": "masses", "RECU_ANCRAGE": "anchor", "RECU_TOWER": "tower",
        "RECU_GEOM": "geom", "RECU_SHELL": "shell", "REPONSE": "response", "EXEMPLE": "example",
        "Incoherence": Incoherence,
    }
    namespace.update({"v" + str(i): stub_group for i in range(1, 10)})
    selected("source/valide_lib.py", ("Juge", "main"), namespace)
    console = io.StringIO()
    with contextlib.redirect_stdout(console):
        rc = namespace["main"](["--out", "virtual/" + mode + ".json"])
    receipt = json.loads(written["virtual/" + mode + ".json"].getvalue())
    require(len(calls) == 2 and receipt["controles"] == 9, "two snapshots and nine positive/failure controls")
    expected = 1 if mode == "test_failure" else 0
    require(rc == expected and receipt["code"] == expected, "native-free main result")
    require(receipt["sources_stables"] is (mode != "changed"), "snapshot difference observed")
    require(receipt["echecs"] == (1 if mode == "test_failure" else 0), "failure branch exercised")
    return {
        "mode": mode, "returned_code": rc, "receipt_code": receipt["code"],
        "sources_stables": receipt["sources_stables"], "controls": receipt["controles"],
        "failures": receipt["echecs"], "before": receipt["sources_avant"],
        "after": receipt["sources_apres"], "snapshot_calls": len(calls),
        "scope": "real AST main; nine stub test groups; synthetic fingerprint sampler; no source mutated",
    }


class Date:
    def __float__(self):
        return 0.0

    def texte(self):
        return "stub r=0"


def empty_target_cases():
    counts = {"Scene_stubs": 0, "compat_stubs": 0, "judge_stubs": 0}

    def stub_points(points):
        items = points.items() if isinstance(points, dict) else points
        return [(name, tuple(xyz)) for name, xyz in items]

    def stub_lire_cible(cible, _noms, _tol, _free):
        return {
            "intervalle": {"texte": "stub interval"}, "blocs": [["a", "b"]],
            "tolerance": [], "libres": [], "pourquoi": "stub target", "accept": cible["accept"],
        }

    class Scene:
        def __init__(self, points, K, **_kw):
            counts["Scene_stubs"] += 1
            self.noms = [p[0] for p in points]
            self.n = len(points)
            self.gamma = {"statut": "not_run"}
            self.controles = {"stub": 1}
            self.recoupes = {}

        def resume_full(self):
            return {"scope": "stub, no FULL built"}

        def regle(self, _r):
            return SimpleNamespace(dates=[Date() for _ in range(self.n)])

    def compat(_sc, _ci):
        counts["compat_stubs"] += 1
        return {"controles": 1, "compatible": True, "scope": "stub"}

    def juger(_h, ci, _sc):
        counts["judge_stubs"] += 1
        return {
            "passe": ci["accept"], "viol": None if ci["accept"] else {"reason": "stub rejects"},
            "observes": [], "scope": "stub, no partition evaluated",
        }

    namespace = {
        "ALIAS": {},  # The unit fixtures use only canonical keys; alias parsing is not under test.
        "RG": SimpleNamespace(normaliser_points=stub_points, Scene=Scene),
        "RegleErreur": RegleErreur, "exiger": require, "time": time,
        "lire_cible": stub_lire_cible, "compat_full": compat, "juger": juger,
        "format_blocs": lambda b: str(b),
    }
    selected("source/run_target.py", ("_cles", "normaliser", "juger_fixture"), namespace)
    fixture = {
        "name": "tiny_control", "K": 2,
        "points": {"a": [0, 0, 0], "b": [2, 0, 0]},
        "intent": "Control-flow proof only; no geometric success claimed.",
        "target": [{"accept": True}],
    }
    reports = []
    for name, variant_target in (
        ("positive_nonempty", [{"accept": True}]),
        ("negative_nonempty", [{"accept": False}]),
        ("empty_target", []),
    ):
        raw = dict(fixture)
        raw["variants"] = [{"name": name, "displacements": {"a": [1, 0, 0]}, "target": variant_target}]
        normalized = namespace["normaliser"](raw, "in_memory_fixture")
        count = {"jugements": 0, "compat": 0}
        res = namespace["juger_fixture"](normalized, ["stub_rule"], "non", 1, count)
        var = res["variantes"][1]
        passed = var["regles"]["stub_rule"]["passe"]
        expected = name != "negative_nonempty"
        require(passed is expected and res["passe"]["stub_rule"] is expected, "variant pass/fail")
        require(len(var["cibles"]) == len(variant_target), "exact target count")
        require(count["jugements"] == 1 + len(variant_target), "base plus variant judgment count")
        reports.append({
            "case": name, "normalized_variant_targets": len(normalized["variantes"][1]["cibles"]),
            "variant_targets": len(var["cibles"]), "variant_passed": passed,
            "overall_passed": res["passe"]["stub_rule"], "judgments_including_base": count["jugements"],
            "variant_verdicts": var["regles"]["stub_rule"]["cibles"],
        })
    raw = dict(fixture)
    raw["target"] = []
    try:
        namespace["normaliser"](raw, "in_memory_fixture")
    except ValueError:
        base_empty_refused = True
    else:
        base_empty_refused = False
    require(base_empty_refused, "empty BASE target is already refused")
    require(counts == {"Scene_stubs": 6, "compat_stubs": 5, "judge_stubs": 5}, "stub calls non-vacuous")
    return {"reports": reports, "base_empty_refused": base_empty_refused, "stub_calls": counts}


def main():
    for name, expected in PINS.items():
        require(hashlib.sha256((ROOT / name).read_bytes()).hexdigest() == expected, "source pin " + name)
    result = {
        "status": "OBSERVED_CONTROL_FLOW_GAPS", "source_pins": PINS,
        "source_change": [source_change_case(m) for m in ("stable", "changed", "test_failure")],
        "empty_variant": empty_target_cases(),
        "native_calls": 0, "Gamma_calls": 0, "sklearn_calls": 0,
        "shared_source_mutations": 0, "filesystem_outputs": 0,
        "scope": "AST-selected pinned functions and explicit unit stubs, not validation of geometric results",
    }
    for name, expected in PINS.items():
        require(hashlib.sha256((ROOT / name).read_bytes()).hexdigest() == expected, "source changed " + name)
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
