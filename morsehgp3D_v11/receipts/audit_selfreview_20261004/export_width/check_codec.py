#!/usr/bin/env python3
"""Fixed WIP AST exact decoder and stdlib limb model; no native execution."""
import ast
import hashlib
import json
from fractions import Fraction
from pathlib import Path
from types import SimpleNamespace

ROOT = Path(__file__).resolve().parent
source = ROOT / "source/morsehgp3D_v11/bench/points_hierarchy.py"
module = ast.parse(source.read_text())
cls = next(n for n in module.body if isinstance(n, ast.ClassDef) and n.name == "Levels")
method = next(n for n in cls.body if isinstance(n, ast.FunctionDef) and n.name == "exact")
ns = {}
exec(compile(ast.Module(body=[method], type_ignores=[]), str(source), "exec"), ns)
exact = ns["exact"]
checks = 0

def need(ok, message):
    global checks
    checks += 1
    if not ok:
        raise RuntimeError(message)

def encode(value, words):
    need(0 <= value < 1 << (64 * words), "model refused integer")
    return [(value >> (64 * j)) & ((1 << 64) - 1) for j in range(words)]

profiles = []
for bits in (18, 21, 24):
    limit = (1 << bits) - 1
    numerator, denominator = 12 * limit ** 8, 16 * limit ** 6
    budget_n, budget_d = 8 * bits + 12, 6 * bits + 8
    words = max(budget_n, budget_d) // 64 + 1
    need(words == (4 if bits == 24 else 3), "profile width")
    need(numerator.bit_length() <= budget_n and denominator.bit_length() <= budget_d, "level budget")
    limbs = encode(numerator, words) + encode(denominator, words)
    state = SimpleNamespace(words=words, limbs=[limbs], cache={})
    need(exact(state, 0) == (numerator, denominator), "unreduced tetra level")
    need(exact(state, 0) == (numerator, denominator), "cache")
    need(Fraction(numerator, denominator) == Fraction(3 * limit * limit, 4), "tetra radius")
    if bits == 24:
        need(numerator.bit_length() == 196 and denominator.bit_length() == 148, "P2 witness")
        need(numerator >= 1 << 192, "old writer would refuse")
    profiles.append({"bits": bits, "words": words, "version": 2 if words == 4 else 1,
                     "numerator_bits": numerator.bit_length(), "denominator_bits": denominator.bit_length()})
for words in (3, 4):
    values = [0, 1, (1 << 63) - 1, 1 << 63, (1 << 64) - 1, 1 << 64,
              1 << 128, (1 << (64 * words)) - 1]
    for numerator in values:
        for denominator in values[1:]:
            limbs = encode(numerator, words) + encode(denominator, words)
            state = SimpleNamespace(words=words, limbs=[limbs], cache={})
            need(exact(state, 0) == (numerator, denominator), "word boundary")
print(json.dumps({"status": "PASS", "checks": checks, "profile_cases": profiles,
                  "scope": "AST Levels.exact from fixed WIP plus stdlib encoding model only; no exporter, read_export, numpy pipeline or native qualification",
                  "native_runs": 0, "fits": 0, "gcp_actions": 0}, sort_keys=True))
