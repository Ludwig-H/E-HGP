#!/usr/bin/env python3
"""Small pure orchestration/reuse tests; no new worker, fit or geometry."""
from copy import deepcopy
import json

import post_audit_parallel as reader


COUNTS = dict(positive=0, refusals=0)


def accepted(function, *args):
    result = function(*args)
    COUNTS['positive'] += 1
    return result


def refused(function, *args):
    try:
        function(*args)
    except (ValueError, KeyError):
        COUNTS['refusals'] += 1
        return
    raise RuntimeError('corrupted proof accepted')


def command(case, pid):
    return dict(case=case, k=5, pid=pid, argv=['python', '-B', 'runner', '--worker-spec', case],
                returncode=0, elapsed_seconds=1.5, stdout_sha256='a'*64, stderr_sha256='b'*64)


def start(row):
    return dict(event='start', case=row['case'], k=row['k'], pid=row['pid'], argv=row['argv'])


def joined(row):
    return dict(event='joined', **row)


def test_ledger():
    a, b, c = command('a', 101), command('b', 102), command('c', 103)
    commands = [a, b, c]; units = {reader.key(row) for row in commands}
    events = [start(a), start(b), joined(a), start(c), joined(b), joined(c)]
    reader.need(accepted(reader.validate_ledger, events, commands, units, 2) == 2, 'actual peak concurrency')
    accepted(reader.validate_ledger, [], [], set(), 1)  # all units reused
    accepted(reader.validate_ledger, [start(a), joined(a), start(b), joined(b)], [a, b], {('a', 5), ('b', 5)}, 1)
    refused(reader.validate_ledger, events, commands, units, 1)
    refused(reader.validate_ledger, events[:-1], commands, units, 2)
    refused(reader.validate_ledger, [joined(a), *events], commands, units, 2)
    refused(reader.validate_ledger, [start(a), *events], commands, units, 2)
    refused(reader.validate_ledger, [*events, joined(a)], commands, units, 2)
    refused(reader.validate_ledger, events, commands + [a], units, 2)
    refused(reader.validate_ledger, events, commands, units | {('d', 5)}, 2)
    refused(reader.validate_ledger, events, commands, units, True)
    for field, value in (('pid', 999), ('argv', ['wrong']), ('event', 'cleanup')):
        bad = deepcopy(events); bad[0][field] = value
        refused(reader.validate_ledger, bad, commands, units, 2)
    failed = deepcopy(a); failed['returncode'] = 1
    refused(reader.validate_ledger, [start(failed), joined(failed)], [failed], {('a', 5)}, 1)
    interrupted = dict(a, interrupted=True)
    refused(reader.validate_ledger, [start(interrupted), joined(interrupted)], [interrupted], {('a', 5)}, 1)
    collision = dict(b, pid=a['pid'])
    refused(reader.validate_ledger, [start(a), start(collision), joined(a), joined(collision)],
            [a, collision], {('a', 5), ('b', 5)}, 2)
    # A PID may legally be reused only after its previous process was joined.
    accepted(reader.validate_ledger, [start(a), joined(a), start(collision), joined(collision)],
             [a, collision], {('a', 5), ('b', 5)}, 1)


def test_reuse():
    shared = {'/private/serial.py': '1'*64, '/private/model.py': '2'*64}
    receipt = dict(sources_before={**shared, str(reader.RUNNER): '3'*64}, input_hashes={'/private/input': '4'*64})
    for field in reader.FIXED_FIELDS:
        receipt[field] = '5'*64 if field.endswith('_sha256') else '/private/' + field
    old = dict(schema=reader.arithmetic.SCHEMA, status='failed', error='KeyboardInterrupt()',
               plan=deepcopy(reader.PLAN), sources_before=shared, input_hashes={'/private/input': '4'*64},
               **{field: receipt[field] for field in reader.FIXED_FIELDS})
    proof = dict(resume_verification_currentpins=deepcopy(shared), missing_original_failure_source_closure=True)
    original = deepcopy(old)
    accepted(reader.validate_reuse_header, old, proof, receipt)
    reader.need(old == original and 'sources_after' not in old, 'original failure not silently closed')
    closed = dict(old, sources_after=deepcopy(shared))
    accepted(reader.validate_reuse_header, closed, dict(proof, missing_original_failure_source_closure=False), receipt)
    for field, value in (('status', 'completed'), ('error', 'ValueError()'), ('schema', reader.SCHEMA),
                         ('native_binary_sha256', '6'*64), ('manifest_sha256', '7'*64),
                         ('sources_before', {}), ('input_hashes', {'/private/input': '8'*64})):
        bad = deepcopy(old); bad[field] = value
        refused(reader.validate_reuse_header, bad, proof, receipt)
    refused(reader.validate_reuse_header, old, dict(proof, missing_original_failure_source_closure=False), receipt)
    refused(reader.validate_reuse_header, old, dict(proof, resume_verification_currentpins={}), receipt)
    refused(reader.validate_reuse_header, dict(closed, sources_after={}),
            dict(proof, missing_original_failure_source_closure=False), receipt)
    tag = dict(receipt='/private/original/receipt.json', receipt_sha256='9'*64, directory='/private/original/unit')
    row = dict(case='a', k=5, reused_from=deepcopy(tag)); saved = deepcopy(row)
    reader.need(accepted(reader.untag, row, tag) == dict(case='a', k=5) and row == saved, 'nonmutating comparison projection')
    refused(reader.untag, dict(row, reused_from=dict(tag, receipt_sha256='0'*64)), tag)
    refused(reader.untag, dict(case='a', k=5), tag)


def test_pins():
    pins = {}
    accepted(reader.pin, pins, '/private/a', '1'*64)
    accepted(reader.pin, pins, '/private/a', '1'*64)
    refused(reader.pin, pins, '/private/a', '2'*64)
    reader.need(pins == {'/private/a': '1'*64}, 'conflict leaves original pin unchanged')
    reader.need(reader.sha(reader.HERE / 'post_audit.py') == reader.ARITHMETIC_SHA, 'stable arithmetic source pin')


def test_cleanup():
    rows = [command('a', 101), command('b', 102)]
    cleanup = [dict(pgid=row['pid'], signals=[], status='closed',
                    residual_group_before_cleanup=False, returncode=0) for row in rows]
    receipt = dict(worker_commands=rows, group_cleanup=cleanup)
    accepted(reader.validate_cleanup, receipt)
    accepted(reader.validate_cleanup, dict(worker_commands=[]))
    refused(reader.validate_cleanup, dict(receipt, cleanup_errors=[dict(pgid=101, error='survived')]))
    refused(reader.validate_cleanup, dict(receipt, group_cleanup=cleanup[:1]))
    refused(reader.validate_cleanup, dict(receipt, group_cleanup=list(reversed(cleanup))))
    for field, value in (('signals', [9]), ('residual_group_before_cleanup', True),
                         ('status', 'running'), ('returncode', -2), ('pgid', 999)):
        bad = deepcopy(receipt); bad['group_cleanup'][0][field] = value
        refused(reader.validate_cleanup, bad)


def test_input_inventory():
    manifest = {case: dict(labels_json='/private/' + case + '.labels', points='/private/' + case + '.points',
                          prepared_sha256=dict(labels_json='a'*64, points='b'*64)) for case in reader.PLAN['cases']}
    expected = {row[name]: digest for row in manifest.values() for name, digest in row['prepared_sha256'].items()}
    accepted(reader.validate_inputs, dict(input_hashes=expected), manifest)
    label = manifest[reader.PLAN['cases'][0]]['labels_json']
    missing = dict(expected); del missing[label]
    refused(reader.validate_inputs, dict(input_hashes=missing), manifest)
    refused(reader.validate_inputs, dict(input_hashes=dict(expected, **{label: 'c'*64})), manifest)
    refused(reader.validate_inputs, dict(input_hashes=dict(expected, extra='d'*64)), manifest)
    bad = deepcopy(manifest); del bad[reader.PLAN['cases'][0]]['prepared_sha256']['labels_json']
    refused(reader.validate_inputs, dict(input_hashes=expected), bad)


if __name__ == '__main__':
    test_ledger(); test_reuse(); test_pins(); test_cleanup(); test_input_inventory()
    print(json.dumps(dict(status='passed', **COUNTS, scope='pure ledger, reuse and pin guards; not a campaign replay'), sort_keys=True))
