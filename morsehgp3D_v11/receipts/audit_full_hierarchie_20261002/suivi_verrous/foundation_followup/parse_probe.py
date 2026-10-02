"""Bounded parser-only checks. No compiler, cmake or ctest is invoked."""
import contextlib
import importlib.util
import io
import json
import os
from pathlib import Path
import sys
import tempfile

ROOT = Path(__file__).resolve().parent
SRC = ROOT.parent / 'snapshot/morsehgp3D_v11'
sys.path.insert(0, str(SRC / 'tests/support'))
import mhgp11_gate as gate
spec = importlib.util.spec_from_file_location('reviewed_mutants', SRC / 'tests/mutants/run_mutants.py')
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)
name = 'mhgp11_probe'
records = []
for label, status, code, verdict in [
    ('passed', 'run', 0, ''),
    ('skipped', 'notrun', 0, ''),
    ('missing_report', None, 8, ''),
    ('real_failure_shape', 'fail', 8, 'code'),
    ('launch_failure_shape', 'fail', 8, 'lancement_impossible'),
    ('no_judge_shape', 'fail', 8, ''),
    ('contradictory_failed_report_zero_code', 'fail', 0, 'code'),
    ('contradictory_pass_report_bad_verdict', 'run', 0, 'code'),
]:
    with tempfile.TemporaryDirectory(prefix='parse_', dir=ROOT) as build:
        def simulated_run(argv, **kwargs):
            if status is not None:
                path = Path(argv[argv.index('--output-junit') + 1])
                path.write_text('<testsuite><testcase name="%s" status="%s"/></testsuite>' % (name, status))
            return gate.Completed(code, 0, False, ('run_expect_verdict '+verdict+'\n') if verdict else '', '')
        original = gate.run
        gate.run = simulated_run
        try:
            issue = gate.run_ctest_gate('not_executed', build, name)
        finally:
            gate.run = original
        cause = runner.kill_cause(issue) if issue.status == 'echec' else None
        records.append(dict(case=label, simulated=True, ctest_code=code, junit_status=status,
                            verdict=verdict, parsed_status=issue.status, recognized_kill_cause=cause))

stream = io.StringIO()
with contextlib.redirect_stdout(stream):
    list_code = runner.run(['--source', str(SRC), '--manifest', str(SRC/'tests/mutants/core.json'), '--list'])
lines = stream.getvalue().splitlines()
manifest = runner.load_manifest(SRC/'tests/mutants/core.json')
construction = [line for line in lines if ' construction:' in line]
if list_code != 0 or len(lines) != len(manifest['mutants']) or not construction:
    raise RuntimeError('list regression')
result = dict(scope='parser only with explicitly simulated CTest/JUnit; --list on pinned actual core manifest; no native execution',
              mode='optimized' if sys.flags.optimize else 'normal', records=records,
              list=dict(code=list_code, lines=len(lines), construction_lines=construction))
(ROOT/(result['mode']+'.json')).write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n')
print(json.dumps(dict(mode=result['mode'], parser_cases=len(records), list_code=list_code, list_lines=len(lines))))
