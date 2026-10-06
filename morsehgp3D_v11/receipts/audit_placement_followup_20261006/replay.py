#!/usr/bin/env python3
"""Bounded source/AST comparison with native CLI mask guards; never builds or runs native code."""
import ast
import difflib
import hashlib
import itertools
import json
from pathlib import Path
import re
import subprocess

HERE = Path(__file__).resolve().parent

def need(ok, message):
    if not ok:
        raise ValueError(message)

manifest = json.loads((HERE / 'sources.json').read_text())
sources = {}
for path, digest in manifest['files'].items():
    raw = subprocess.check_output(['git', 'show', manifest['source_commit'] + ':morsehgp3D_v11/' + path])
    need(hashlib.sha256(raw).hexdigest() == digest, 'source hash: ' + path)
    sources[path] = raw.decode()
old = sources['bench/full_campaign.py']
new = old.replace("0 <= value <= 131071, 'optimization mode outside 0..131071'",
                  "0 <= value <= 524287, 'optimization mode outside 0..524287'")
needle = "    need(not (value & 32768 and value & 65536), 'one leaf batch executor')\n"
new = new.replace(needle, needle +
    "    need(not value & 131072 or value & (32768 | 65536), 'replay overflow requires a leaf batch')\n" +
    "    need(not value & 262144 or value & 8192, 'pipeline placement requires concurrent orders')\n")
patch = ''.join(difflib.unified_diff(old.splitlines(True), new.splitlines(True),
    fromfile='a/morsehgp3D_v11/bench/full_campaign.py', tofile='b/morsehgp3D_v11/bench/full_campaign.py'))
need(patch == (HERE / 'proposal.patch').read_text(), 'exact proposal')

def load_function(text):
    functions = [n for n in ast.parse(text).body if isinstance(n, ast.FunctionDef) and n.name == 'optimization']
    need(len(functions) == 1, 'one optimization function')
    namespace = {'need': need}
    exec(compile(ast.Module(body=functions, type_ignores=[]), 'optimization_source', 'exec'), namespace)
    return namespace['optimization']

original, corrected = load_function(old), load_function(new)

def accepted(function, value):
    try:
        function(value)
        return True
    except ValueError:
        return False

native = sources['bench/full_probe.cpp']
limits = re.findall(r'optimizations > (\d+)\)\) return 2;', native)
need(limits == ['524287'], 'native mask maximum')
conditions = re.findall(r'if \((\(optimizations & [^\n]+)\) return 2;', native)
need(len(conditions) == 7, 'seven native prerequisite guards')
conditions = [compile(s.replace('&&', 'and').replace('||', 'or'), 'native_mask_guard', 'eval') for s in conditions]

def native_accepts(value):
    return type(value) is int and 0 <= value <= int(limits[0]) and not any(
        eval(s, {'__builtins__': {}}, {'optimizations': value}) for s in conditions)

bits = (8, 64, 128, 2048, 8192, 16384, 32768, 65536, 131072, 262144)
checked, disagreements = 0, 0
for selection in itertools.product((0, 1), repeat=len(bits)):
    value = sum(bit * flag for bit, flag in zip(bits, selection))
    want = native_accepts(value)
    need(accepted(corrected, value) == want, 'corrected/native mismatch: ' + str(value))
    disagreements += accepted(original, value) != want
    checked += 1
for value in (-1, 524288, True, 1.0):
    need(not accepted(corrected, value), 'invalid type/domain accepted')
valid = [180219, 212987, 278523, 344059, 475131]
for value in valid:
    need(native_accepts(value) and not accepted(original, value) and accepted(corrected, value), 'valid witness ' + str(value))
io = sources['tests/tower/full_bench_io.py']
need("'opt_large':'524288'" in io and "'opt_replay_requires_batch':'131072'" in io and
     "'opt_placement_requires_concurrent':'262144'" in io, 'updated native IO source')
print(json.dumps({'source_commit': manifest['source_commit'], 'mask_combinations': checked,
    'original_disagreements': disagreements, 'corrected_disagreements': 0, 'valid_rejected_modes': valid,
    'native_executions': 0, 'cloud_actions': 0, 'scope': 'AST of Python optimization; mask guard expressions read from C++; no native execution'},
    sort_keys=True))
