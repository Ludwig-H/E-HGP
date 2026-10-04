#!/usr/bin/env python3
"""Portable stdlib proofs on copied design + exact SHA judge, with no process launched."""
import ast
import contextlib
import hashlib
import io
import json
from pathlib import Path
import struct
import sys
import types

HERE = Path(__file__).resolve().parent
SOURCES = HERE / 'sources'
CHECKS = 0


def require(value, what):
    global CHECKS
    CHECKS += 1
    if not value:
        raise ValueError(what)


def sha(data):
    return hashlib.sha256(data).hexdigest()


for name in ('ACTOR_BEFORE.json', 'ADDITIONAL_BEFORE.json'):
    inventory = json.loads((HERE / name).read_text())
    for record in inventory['sources']:
        require(sha((HERE / record['copy']).read_bytes()) == record['sha256'], 'copied source hash: '+record['copy'])

spec = (SOURCES / 'SPECIFICATION_FINALE.md').read_text()
revised = (SOURCES / 'CRITIQUE_ET_PLAN_REVISE.md').read_text()
require('ne change que la colonne `SITES.point_id`' in spec, 'original spec explicitly excepts PointId column')
require('labels i64[n_points]' in spec, 'existing flat labels format')
require('**dans l\'ordre du fichier d\'entrée**' in spec, 'flat labels declared in input order')
require('octets identiques sous permutation et réétiquetage' in revised, 'revised assertion in captured domain')
require('`point_id` en `u32`' in revised, 'revised format stores raw PointId')


def morton(point):
    return sum(((coordinate >> bit) & 1) << (3*bit+axis)
               for axis, coordinate in enumerate(point) for bit in range(21))


def sites_columns(rows):
    ordered = sorted(rows, key=lambda row: morton(row[:3]))
    return [b''.join(struct.pack('<I', row[column]) for row in ordered) for column in range(4)]


def raw_inputs(rows):
    return (b''.join(struct.pack('<III', *row[:3]) for row in rows),
            b''.join(struct.pack('<I', row[3]) for row in rows))


rows = [(0, 0, 0, 10), (2, 0, 0, 20), (0, 2, 0, 40)]
permutation = [2, 0, 1]
permuted = [rows[i] for i in permutation]
rename = {10: 0xFFFFFFFF, 20: 50, 40: 100}
relabeled = [(*row[:3], rename[row[3]]) for row in rows]
require(len({row[:3] for row in rows}) == len(rows), 'unit multiplicity domain')
require(all(0 <= value < 2**21 for row in rows for value in row[:3]), 'u21 coordinate domain')
require(len(set(rename.values())) == len(rename), 'injective relabeling')
require(all(0 <= value <= 0xFFFFFFFF for value in rename.values()), 'external PointId u32 domain')
base_cols, perm_cols, relabel_cols = [sites_columns(value) for value in (rows, permuted, relabeled)]
require(base_cols == perm_cols, 'canonical SITES payload same after row permutation')
require(base_cols[:3] == relabel_cols[:3], 'geometry columns same after relabeling')
require(base_cols[3] != relabel_cols[3], 'raw PointId column changes after relabeling')
require(b''.join(base_cols) != b''.join(relabel_cols), 'whole SITES bytes necessarily change')
base_inputs, perm_inputs, relabel_inputs = [raw_inputs(value) for value in (rows, permuted, relabeled)]
require(sha(base_inputs[0]) != sha(perm_inputs[0]), 'raw XYZ input hash changes after row permutation')
require(sha(base_inputs[1]) != sha(perm_inputs[1]), 'raw ID input hash changes after row permutation')
require(sha(base_inputs[1]) != sha(relabel_inputs[1]), 'raw ID input hash changes after relabeling')
labels = [10, 10, -1]
permuted_labels = [labels[i] for i in permutation]
require(permuted_labels != labels, 'input-order flat label vector changes after permutation')
require([permuted_labels[permutation.index(i)] for i in range(3)] == labels,
        'inverse permutation restores identical flat partition')
new_label = min(rename[10], rename[20])
require(new_label != rename[min(10, 20)], 'minimum ID cluster label is not equivariant under arbitrary injection')

# Extract actual reader and actual Gate definitions. Never import a product module; never call its run().
reader_path = SOURCES / 'f98aeed67__morsehgp3D_v11__tests__io__sha256_oracle.py'
gate_path = SOURCES / 'f98aeed67__morsehgp3D_v11__tests__support__mhgp11_gate.py'
gate_tree = ast.parse(gate_path.read_text())
gate_nodes = [node for node in gate_tree.body
              if isinstance(node, ast.ClassDef) and node.name in ('Gate', 'Completed')
              or isinstance(node, ast.Assign) and any(isinstance(target, ast.Name)
                                                     and target.id in ('OK', 'DISAGREEMENT', 'REFUSAL', 'FLOOR', 'MUTANT_KILLED')
                                                     for target in node.targets)]
gate_scope = {'sys': sys}
exec(compile(ast.Module(body=gate_nodes, type_ignores=[]), str(gate_path), 'exec'), gate_scope)
reader_tree = ast.parse(reader_path.read_text())
reader_nodes = [node for node in reader_tree.body
                if isinstance(node, ast.FunctionDef) and node.name in ('message', 'main')
                or isinstance(node, ast.Assign) and any(isinstance(target, ast.Name)
                                                       and target.id in ('CHUNKS', 'LENGTHS', 'FLOOR') for target in node.targets)]
reader_scope = {'sys': sys, 'hashlib': hashlib}
exec(compile(ast.Module(body=reader_nodes, type_ignores=[]), str(reader_path), 'exec'), reader_scope)
require(len(reader_scope['LENGTHS']) == 207, 'actual reader has 207 message lengths')
require(len(reader_scope['CHUNKS']) == 6, 'actual reader has 6 chunk sizes')
require(reader_scope['FLOOR'] == 1242, 'actual reader comparison count')
original_message = reader_scope['message']
cache = {}


def message_cached(length, seed):
    key = length, seed
    if key not in cache:
        cache[key] = original_message(length, seed)
    return cache[key]


reader_scope['message'] = message_cached
reader_results = []
old_argv = sys.argv
try:
    for mode in ('correct', 'empty', 'missing_last', 'wrong_digest', 'no_final_newline', 'refusal', 'signal', 'timeout'):
        fake_runs = []

        def fake_run(argv, timeout, stdin):
            require(argv == ['not_a_native_program'], 'fake transport only, never a process')
            require(timeout == 240, 'reader timeout argument preserved')
            fake_runs.append(1)
            request_lines = stdin.splitlines()
            require(len(request_lines) == 1242, 'actual reader sends every declared case')
            digests = []
            for line in request_lines:
                chunk, raw = line.split(' ')
                data = bytes.fromhex(raw) if raw != '-' else b''
                digests.append(hashlib.sha256(data).hexdigest())
            if mode == 'empty':
                digests = []
            elif mode == 'missing_last':
                digests.pop()
            elif mode == 'wrong_digest':
                digests[0] = '0' * 64
            text = '\n'.join(digests) + ('\n' if digests and mode != 'no_final_newline' else '')
            code = 2 if mode == 'refusal' else None if mode in ('signal', 'timeout') else 0
            return gate_scope['Completed'](code, 15 if mode == 'signal' else 0, mode == 'timeout', text, '')

        reader_scope['mhgp11_gate'] = types.SimpleNamespace(**{key: value for key, value in gate_scope.items()
                                                             if key != 'sys'}, run=fake_run)
        sys.argv = ['copied_sha256_oracle.py', 'not_a_native_program']
        log = io.StringIO()
        with contextlib.redirect_stdout(log):
            code = reader_scope['main']()
        require(len(fake_runs) == 1, 'one replaced reader transport call')
        require(code == (0 if mode == 'correct' else 1), mode + ': exact judge result')
        reader_results.append({'case': mode, 'exit_code': code, 'stdout': log.getvalue()})
finally:
    sys.argv = old_argv

print(json.dumps({'schema': 'audit.supports_followup.evidence.v1', 'checks': CHECKS,
                  'scope': 'design identity counterexamples + exact Python SHA reader on fake transport; no native/build/GCP/fit',
                  'identity': {'SITES_permutation_equal': True, 'SITES_relabel_equal': False,
                               'geometry_relabel_equal': True, 'flat_permutation_equal': False,
                               'raw_input_hashes_permutation_equal': False,
                               'raw_ID_hash_relabel_equal': False, 'new_min_label': new_label,
                               'renamed_old_min_label': rename[10], 'permutation': permutation},
                  'reader': {'source_commit': 'f98aeed67d4030dd78e11d5faf7d8556c4d17aaf',
                             'requests_per_run': 1242, 'correct_comparison_floor': 1244,
                             'native_calls': 0, 'results': reader_results}}, indent=2, sort_keys=True))
