#!/usr/bin/env python3
"""Portable source/report reader and bounded actual-judge fakes; no geometry suite."""
import ast
import contextlib
import copy
import hashlib
import io
import json
from pathlib import Path
import re
import types

HERE = Path(__file__).resolve().parent
SRC = HERE/'sources/git/morsehgp3D_v11/reference'
REPORTS = HERE/'sources/reports/l0_consolidation_verif'
checks = 0


def check(ok, why):
    global checks
    checks += 1
    if not ok:
        raise ValueError(why)


def selected_function(tree, name, env):
    func = next(n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name == name)
    code = ast.fix_missing_locations(ast.Module(body=[copy.deepcopy(func)], type_ignores=[]))
    exec(compile(code, '<copied-source:' + name + '>', 'exec'), env)
    return env[name]


def call(fn, *args):
    stream = io.StringIO()
    with contextlib.redirect_stdout(stream):
        code = fn(*args)
    return code, stream.getvalue()


def assignments(tree):
    return {t.id: n.value for n in tree.body if isinstance(n, ast.Assign)
            for t in n.targets if isinstance(t, ast.Name)}


def main():
    src_tree = ast.parse((SRC/'test_supports.py').read_text())
    assigned = assignments(src_tree)
    def constant_dict(name):
        return {kw.arg: ast.literal_eval(kw.value) for kw in assigned[name].keywords}
    exact, floors = constant_dict('SUITE_EXACT'), constant_dict('FLOORS')
    digest = ast.literal_eval(assigned['SUITE_DIGEST'])
    env = dict(SUITE_EXACT=exact, FLOORS=floors, SUITE_DIGEST=digest)
    floor_fn = selected_function(src_tree, 'floors', env)
    check(floor_fn(exact, digest) == [], 'recorded complete counters accepted')
    for key in exact:
        changed = dict(exact)
        changed[key] -= 1
        check(bool(floor_fn(changed, digest)), 'exact counter omitted: ' + key)
    for key, minimum in floors.items():
        changed = dict(exact)
        changed[key] = minimum - 1
        check(any(key in row for row in floor_fn(changed, digest)), 'floor omitted: ' + key)
    check(bool(floor_fn(exact, '0'*64)), 'digest corruption accepted')
    env['SUITE_EXACT'] = {}
    check('compteurs exacts non graves' in floor_fn(exact, digest), 'empty exact table accepted')
    env['SUITE_EXACT'] = exact
    source_text = (SRC/'hgp11_ref/supports.py').read_text()
    mutant_tree = ast.parse((SRC/'ref_mutants.py').read_text())
    mut_env = dict(S8=' '*8)
    selected_function(mutant_tree, '_support_mutant', mut_env)
    expr = assignments(mutant_tree)['SUPPORT_MUTANTS']
    mutants = eval(compile(ast.Expression(body=expr), '<copied-source:SUPPORT_MUTANTS>', 'eval'), mut_env)
    check(len(mutants) == 13, 'current inventory13')
    for name, meta in mutants.items():
        check(source_text.count(meta['old']) == 1, 'stale/ambiguous mutation ' + name)
        check(bool(meta['cause']) and bool(meta['fixtures']) and not meta['equivalent'], 'vacuous mutation ' + name)
        log = (REPORTS/('mut_' + name + '.txt')).read_text()
        check(log.splitlines()[-1] == 'mutant_killed ' + name, 'missing exact cause verdict ' + name)
        check(meta['cause'] in log, 'cause absent from captured log ' + name)
    class StalePatch(Exception):
        pass
    intact, mutated = object(), object()
    control = dict(baseline=[], errors=[])
    loader = types.SimpleNamespace(SUPPORT_MUTANTS=mutants, StalePatch=StalePatch,
                                  load_supports=lambda *_: mutated)
    mut_call_env = dict(ref_mutants=loader, STAGE=intact, PACKAGE='unused',
                        judge_fixtures=lambda stage, _names: (control['baseline'] if stage is intact else control['errors'], 1),
                        DISAGREEMENT=1, FLOOR=3, OK=0, MUTANT_KILLED=4)
    run_mutant = selected_function(src_tree, 'run_mutant', mut_call_env)
    cause = mutants['regle_parent_inversee']['cause']
    for errors, code, marker in [([], 0, 'mutant_survives'),
                                (['wrong cause'], 3, 'tue sans sa cause'),
                                ([cause], 4, 'mutant_killed regle_parent_inversee')]:
        control['errors'] = errors
        actual, output = call(run_mutant, 'regle_parent_inversee')
        check(actual == code and marker in output, 'actual helper mutant branch')
    control['baseline'] = ['intact witness disagreement']
    actual, output = call(run_mutant, 'regle_parent_inversee')
    check(actual == 1 and 'temoin non conforme' in output, 'baseline failure must invalidate mutation')
    control['baseline'] = []
    def stale(*_args):
        raise StalePatch('pattern gone')
    loader.load_supports = stale
    actual, output = call(run_mutant, 'regle_parent_inversee')
    check(actual == 3 and 'inapplicable' in output, 'stale patch falsely killed')
    logs = [(REPORTS/f).read_bytes() for f in ('gate_py310.out', 'gate_py310_O.out', 'gate_py312.out', 'gate_py312_O.out')]
    check(all(row == logs[0] for row in logs), 'four Python outputs differ')
    text = logs[0].decode()
    counters = dict((k, int(v)) for k,v in re.findall(r'([a-z_]+)=(\d+)', text.splitlines()[0]))
    check(counters['faits'] == 50 and counters['ecarts'] == 0, 'facts/suite errors')
    for key, want in exact.items():
        check(counters.get(key) == want, 'captured counter mismatch ' + key)
    check('empreinte='+digest in text, 'captured digest mismatch')
    ctest = (REPORTS/'ctest_reference_LE_long.txt').read_text()
    passed = re.findall(r'Test\s+#\d+:\s+(\S+)\s+\.*\s+Passed\b', ctest)
    check(len(passed) == len(set(passed)) == 90, 'CTest total90')
    supports_gates = sorted(n for n in passed if n.startswith('mhgp11_reference_supports'))
    expected_gates = ['mhgp11_reference_supports', 'mhgp11_reference_supports_refusal']
    expected_gates += ['mhgp11_reference_supports_mutant_' + name for name in mutants]
    expected_gates += [n+'_opt' for n in list(expected_gates)]
    check(supports_gates == sorted(expected_gates), '30actual S1 gates missing/duplicated')
    for line in (REPORTS/'SHA256SUMS').read_text().splitlines():
        sha, rel = line.split(maxsplit=1)
        rel = rel.lstrip('*')
        check(hashlib.sha256((REPORTS/rel).read_bytes()).hexdigest() == sha, 'report closure ' + rel)
    source_count = 0
    for file in ('SOURCE_BEFORE.json', 'ADDITIONAL_BEFORE.json', 'REPORT_PAYLOADS_BEFORE.json', 'DELTA_BEFORE.json'):
        for row in json.loads((HERE/file).read_text())['sources']:
            check(hashlib.sha256((HERE/row['copy']).read_bytes()).hexdigest() == row['sha256'], 'source copy hash')
            source_count += 1
    doc = (HERE/'sources/git/morsehgp3D_v11/docs/SORTIES.md').read_text()
    check('MHGP11GX' in doc and 'version 2' in doc and 'Aucun `PointId`' in doc, 'versioned identity contract')
    check('Elle ne se recalcule pas depuis le seul `MHGP11FUL1`' in doc, 'legacy FUL1 caveat')
    check('incidences $(b,F)$' in doc, 'K-parts aggregate caveat')
    check('published_complete' in doc and 'ne qualifie aucune implémentation' in doc, 'source/native scopes')
    check(not any(isinstance(n, ast.Assert) for t in (src_tree, mutant_tree, ast.parse(source_text)) for n in ast.walk(t)), 'optimization-sensitive guard')
    return dict(schema='mhgp11.audit.l0_s1_review.v1', source_pin='5adf6a59f3d3b99fdf947e676e8548d4102ab48f', checks=checks,
                scope='copied-source AST/fakes and existing-report rehash only; no geometry suite/native/build/GCP/fit',
                report_gate=dict(facts=50, counters=exact, suite_digest=digest,
                                 python_outputs_sha256=hashlib.sha256(logs[0]).hexdigest(),
                                 ctest_total=90, supports_gates=len(supports_gates), mutant_gates=26),
                mutants={name:dict(cause=meta['cause'], fixtures=meta['fixtures']) for name,meta in sorted(mutants.items())},
                source_count=source_count,
                limitations=['Source D2 signature norm is adopted; no native signature emitter/reader qualified here.',
                             'Permanent S1 suite K<=5, maxshell12. Independent report highK is separate, not a permanent CMake gate.',
                             'No native shell24 refusal/profile/time/CPU-GPU claims transferred.'])


if __name__ == '__main__':
    print(json.dumps(main(), ensure_ascii=False, indent=2, sort_keys=True))
