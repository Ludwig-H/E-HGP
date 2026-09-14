#!/usr/bin/env python3
"""Synthetic startup/closure guards; these runs do not measure the census."""
import copy
import importlib.util
import io
import json
from pathlib import Path
import subprocess
from unittest.mock import patch

BASE = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location('sibling_measure', BASE / 'measure.py')
RUNNER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(RUNNER)


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def run():
    reference = BASE.parent / 'q2_front_20260914/campaign_pilot/MEASURES.jsonl'
    old = json.loads(reference.read_text().splitlines()[0])
    result = copy.deepcopy(old['result'])
    result.update(sibling_mode='baseline', sibling_work=dict(
        child_entries=0, population_eligible=0, bound_tests=0,
        rejected_children=0, rejected_pairs=0))
    stdout = json.dumps(result)
    key = ('single_000000', 8000, 8, 'samples', 'shared', 'baseline')
    command = ['synthetic_not_executed']
    cases = [
        ('success', subprocess.CompletedProcess(command, 0, stdout, ''), None, 'completed'),
        ('nonzero', subprocess.CompletedProcess(command, 2, '', 'failure'), None, 'failed'),
        ('stderr', subprocess.CompletedProcess(command, 0, stdout, 'unexpected'), None, 'failed'),
        ('invalid_json', subprocess.CompletedProcess(command, 0, '{bad', ''), None, 'failed'),
        ('nan_json', subprocess.CompletedProcess(command, 0, '{"timing":NaN}', ''), None, 'failed'),
        ('infinite_exponent', subprocess.CompletedProcess(command, 0, '{"timing":1e999}', ''), None, 'failed'),
        ('timeout', None, subprocess.TimeoutExpired(command, 180, b'partial', b'timeout'), 'failed'),
        ('startup_failure', None, OSError('synthetic startup failure'), 'failed'),
        ('interrupt', None, KeyboardInterrupt('synthetic interrupt'), 'interrupted'),
    ]
    checked = []
    for name, value, error, expected in cases:
        stream = io.StringIO()
        raised = None
        with patch.object(RUNNER.subprocess, 'run', return_value=value, side_effect=error):
            try:
                RUNNER.attempt(key, command, 0, stream)
            except BaseException as failure:
                raised = type(failure).__name__
        rows = [json.loads(line) for line in stream.getvalue().splitlines()]
        require(len(rows) == 1 and rows[0]['status'] == expected,
                'lost or wrong attempted row: ' + name)
        require((raised is None) == (expected == 'completed'), 'lost failure: ' + name)
        require(rows[0]['command'] == command and rows[0]['finished_utc'], 'incomplete attempt')
        if name == 'timeout':
            require(rows[0]['returncode'] == 124 and rows[0]['stdout'] == 'partial',
                    'lost timeout evidence')
        checked.append(dict(case=name, status=expected, raised=raised, rows=len(rows)))
    row = dict(old, key=key, result=result, stdout=stdout)
    mutants = []
    for name in ('wrong_mode', 'baseline_credit', 'bad_eligibility'):
        mutant = copy.deepcopy(row)
        if name == 'wrong_mode':
            mutant['result']['sibling_mode'] = 'sibling'
        elif name == 'baseline_credit':
            mutant['result']['sibling_work'].update(child_entries=1, population_eligible=1,
                bound_tests=1, rejected_children=1, rejected_pairs=1)
        else:
            mutant['result']['sibling_work']['bound_tests'] = 1
        mutant['stdout'] = json.dumps(mutant['result'])
        try:
            RUNNER.check(mutant)
        except RuntimeError:
            mutants.append(name)
        else:
            raise RuntimeError('surviving ledger mutant: ' + name)
    return dict(status='passed', scope='mocked_runner_no_census_execution',
                runner_sha256=RUNNER.sha(BASE / 'measure.py'),
                reference_rows_sha256=RUNNER.sha(reference), cases=checked, rejected_mutants=mutants)


if __name__ == '__main__':
    print(json.dumps(run(), sort_keys=True))
