#!/usr/bin/env python3
"""Relecture de source et autotests Python publics uniquement ; aucune sonde."""
import ast
import hashlib
import json
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent
C = json.loads((HERE / "capture.json").read_text())


def need(ok, why):
    if not ok:
        raise RuntimeError(why)


def git(repo, *args):
    return subprocess.check_output(["git", "-C", str(repo), *args])


class Normal(ast.NodeTransformer):
    def visit_Name(self, node):
        if node.id == "REGLE_T2D_A6B":
            node.id = "REGLE_T2D_A6"
        return node

    def visit_Constant(self, node):
        if isinstance(node.value, str):
            node.value = node.value.replace("t2d_a6b", "t2d_a6").replace("T2-d-A6b", "T2-d-A6")
        return node

    def visit_FunctionDef(self, node):
        if (node.body and isinstance(node.body[0], ast.Expr)
                and isinstance(node.body[0].value, ast.Constant)
                and isinstance(node.body[0].value.value, str)):
            node.body = node.body[1:]
        return self.generic_visit(node)


def functions(source):
    return {n.name: ast.dump(Normal().visit(n), include_attributes=False)
            for n in ast.parse(source).body if isinstance(n, ast.FunctionDef)}


def rule(source):
    for n in ast.parse(source).body:
        if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id == "REGLE_T2D_A6B"
                                            for t in n.targets):
            # Extract literal fields only; the symbolic site threshold is checked separately.
            return {k.value: ast.literal_eval(v) for k, v in zip(n.value.keys, n.value.values)
                    if isinstance(v, ast.Constant)}
    raise RuntimeError("regle absente")


def run(repo, snapshot):
    def hashes():
        for path, expected in C["snapshot_sha256"].items():
            need(hashlib.sha256((snapshot / path).read_bytes()).hexdigest() == expected, path)
    hashes()
    old = git(repo, "show", C["previous_pin"] + ":" + C["previous_pilot"]).decode()
    new = (snapshot / C["pilot"]).read_text()
    fa, fb = functions(old), functions(new)
    need(set(fa) == set(fb) and len(fa) == 31, "inventaire fonctions")
    different = sorted(k for k in fa if fa[k] != fb[k])
    need(different == ["etape_auto_test", "tableaux", "verifier_cohorte"], "delta AST inattendu")
    need(fa["verifier_cohorte"] == fb["verifier_cohorte"].replace("protocole A6b", "protocole A6"),
         "cohorte autrement modifiee")
    expected_rule = {"base": "47feedc96", "k": 5, "bootstrap": 10000, "graine": 20261008,
                     "borne_grandes": 0.95, "borne_ng": 1.01, "fenetre_aa": 0.015,
                     "tours_min": 5, "passes_min": 6}
    r = rule(new)
    need(all(r.get(k) == v for k, v in expected_rule.items()), "regle")
    delta = git(repo, "diff", "--name-only", C["reference_pin"], C["capture_head"], "--", "morsehgp3D_v12/src/")
    need(not delta, "socles src differents")
    for pin in (C["reference_pin"], C["capture_head"]):
        probe = git(repo, "show", pin + ":morsehgp3D_v12/bench/full_probe.cpp").decode()
        need("cache = u64{8} << 30" in probe, "cache par defaut")
    public = []
    for flags in ([], ["-O"]):
        p = subprocess.run([sys.executable, "-B", "-S", *flags, str(snapshot / C["pilot"]), "auto-test"],
                           capture_output=True, text=True, timeout=30)
        need(p.returncode == 0 and not p.stderr and p.stdout == C["expected_public_stdout"], "autotest")
        public.append({"flags": flags, "code": p.returncode, "stdout": p.stdout, "stderr": p.stderr})
    hashes()
    return {"sources_before_after": True, "functions": 31, "normalized_identical": 28,
            "normalized_differences": different, "cohort_change": "message_only",
            "src_reference_equal_capture_head": True, "cache_default_both_bytes": 8 << 30,
            "rule_literal_fields": expected_rule, "public_selftests": public,
            "native_run": False, "campaign_admitted": False}


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit("usage: check.py GIT_REPO SNAPSHOT")
    print(json.dumps(run(Path(sys.argv[1]), Path(sys.argv[2])), ensure_ascii=False, indent=2, sort_keys=True))
