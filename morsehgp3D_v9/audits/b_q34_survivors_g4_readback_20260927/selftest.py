#!/usr/bin/env python3
"""Synthetic publication tests, not measured timings or GPU qualification."""
import copy
import json
from pathlib import Path
from unittest.mock import patch
import readback as r


def classification_tests():
    """Mock only the evidence validator; never claim this fixture ran CUDA."""
    host = Path('/synthetic/no-session')
    manifest = {'synthetic/source': 'not-a-real-pin'}
    generation = '2026-09-27T10:00:00Z'
    complete = {'status': 'completed'}
    evidence = dict(gate={'synthetic': True}, high_gate={'synthetic': True},
                    measures={'synthetic': True}, ignored='not exported')
    failed = dict(status='failed', semantic_replay=False,
                  GPU_execution='unqualified_or_unknown')
    with patch.object(r.session, 'validate_received', return_value=evidence) as validator:
        result = r.classify(host, manifest, generation, complete, complete)
        r.need(validator.call_count == 1 and
               validator.call_args.args == (host, manifest, generation),
               'successful classification requires bound semantic validation')
        r.need(result == dict(status='completed', semantic_replay=True,
               GPU_execution='qualified_S2', gate=evidence['gate'],
               high_gate=evidence['high_gate'], measures=evidence['measures']),
               'simulated success exports only validated evidence')

    refused = 0
    closed_failures = ('worker_failed', 'failed', 'capture_failed', 'capture_incomplete')
    for status in closed_failures:
        with patch.object(r.session, 'validate_received', return_value=evidence) as validator:
            result = r.classify(host, manifest, generation, {'status': status}, complete)
            r.need(result == failed and validator.call_count == 0,
                   'failed host cannot be promoted: '+status)
            refused += 1
    for receipt in ({'status': 'failed'}, {'status': 'running'}, {}, None):
        with patch.object(r.session, 'validate_received', return_value=evidence) as validator:
            result = r.classify(host, manifest, generation, complete, receipt)
            r.need(result == failed and validator.call_count == 0,
                   'missing or incomplete worker cannot be promoted')
            refused += 1
    for error_type in (ValueError, KeyError, OSError, TypeError):
        error = error_type('synthetic validation rejection')
        with patch.object(r.session, 'validate_received', side_effect=error) as validator:
            result = r.classify(host, manifest, generation, complete, complete)
            r.need(validator.call_count == 1 and result == dict(failed,
                   validation_error=error_type.__name__+': '+str(error)),
                   'validator failure retained without qualification: '+error_type.__name__)
            refused += 1
    return 1, refused


def stop_certificate_tests():
    generation = '2026-09-27T10:00:00Z'
    target = r.c.TARGET
    after = dict(name=target['instance'], status='TERMINATED',
        zone='https://www.googleapis.com/compute/v1/projects/'+target['project']+'/zones/'+target['zone'],
        selfLink='https://www.googleapis.com/compute/v1/projects/'+target['project']+
                 '/zones/'+target['zone']+'/instances/'+target['instance'],
        lastStartTimestamp=generation, lastStopTimestamp='2026-09-27T10:00:30Z')
    for stop, elapsed in (('2026-09-27T10:00:30Z', 30),
                          (generation, 0), ('2026-09-27T12:00:30+02:00', 30)):
        value = r.stop_certificate(dict(after, lastStopTimestamp=stop), generation)
        r.need(value == dict(generation=generation, stopped=stop,
               vm_elapsed_seconds=elapsed, billed_cost_usd=None,
               reason='Elapsed allocation only; no billing export or verified hourly price'),
               'same target/generation aware chronology, not a billing estimate')

    mutations = (
        ('name', 'other-instance', 'exact stopped generation/target'),
        ('status', 'RUNNING', 'exact stopped generation/target'),
        ('zone', 'other-zone', 'exact stopped generation/target'),
        ('selfLink', after['selfLink'].replace('/projects/'+target['project']+'/',
                                             '/projects/other-project/'), 'exact stopped generation/target'),
        ('selfLink', after['selfLink'].replace('/zones/'+target['zone']+'/',
                                             '/zones/other-zone/'), 'exact stopped generation/target'),
        ('selfLink', after['selfLink']+'-other', 'exact stopped generation/target'),
        ('lastStartTimestamp', '2026-09-27T09:59:00Z', 'exact stopped generation/target'),
        ('lastStopTimestamp', '2026-09-27T09:59:59Z', 'stop chronology'),
        ('lastStopTimestamp', '2026-09-27T10:00:30', 'stop chronology'),
    )
    refused = 0
    for field, value, reason in mutations:
        try:
            r.stop_certificate(dict(after, **{field: value}), generation)
        except ValueError as error:
            r.need(str(error) == reason, 'causal stop refusal: '+field)
            refused += 1
        else:
            raise ValueError('stop certificate mutation survived: '+field)
    naive = '2026-09-27T10:00:00'
    try:
        r.stop_certificate(dict(after, lastStartTimestamp=naive), naive)
    except ValueError as error:
        r.need(str(error) == 'stop chronology', 'timezone-free generation refused')
        refused += 1
    else:
        raise ValueError('timezone-free generation survived')
    return 3, refused


def main():
    measures = {}
    fields = ('adapter', 'adapter_plus_native_release_noncontiguous', 'consume_outer',
              'open', 'arena', 'order_convert', 'native_output_release')
    for width in (4, 48):
        for first in ('baseline', 'candidate'):
            sequence = ('resident_af369c44', 'survivors_6af40d886')
            if first == 'candidate':
                sequence = tuple(reversed(sequence))
            rows = []
            for position in range(4):
                name = sequence[0 if position in (0, 3) else 1]
                duration = (1000 if position == 0 else 500) if position < 2 else (
                    20 if name == 'resident_af369c44' else 10)
                rows.append(dict(implementation=name, position=position,
                    first_cuda_call=position == 0, first_implementation_call=position < 2,
                    times_ms={key: duration for key in fields}))
            measures['w'+str(width)+'_'+first] = dict(runs=rows)
    value = r.comparisons(measures)
    r.need(set(value) == {'4', '48'}, 'two widths')
    for row in value.values():
        r.need(row['median_ratio_baseline_over_candidate'] == 2, 'no cold subtraction in repeated median')
        r.need(set(row['first_process_cuda_call_adapter_ms'].values()) == {1000}, 'separate first CUDA calls')
        r.need(all(x['samples'] == 2 for x in row['repeated_operator_medians_ms'].values()), 'two repetitions')
    refusals = 0
    for mutant in ('missing_repeat', 'missing_first', 'zero_candidate'):
        changed = copy.deepcopy(measures)
        rows = changed['w4_baseline']['runs']
        if mutant == 'missing_repeat':
            rows[2]['first_implementation_call'] = True
        elif mutant == 'missing_first':
            rows[0]['first_cuda_call'] = False
        else:
            for case in changed.values():
                for row in case['runs']:
                    if row['implementation'] == 'survivors_6af40d886':
                        row['times_ms']['adapter'] = 0
        try:
            r.comparisons(changed)
        except ValueError:
            refusals += 1
        else:
            raise ValueError('aggregate mutation survived: '+mutant)
    raw = b'account test@example.org\nssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAfake comment\n203.0.113.2 2001:db8::1\n'
    safe = r.redact(raw)
    r.need(all(token not in safe for token in (b'test@example.org', b'AAAAC3', b'203.0.113.2', b'2001:db8::1')),
           'identifiers removed')
    r.need(r.redact(safe) == safe and r.redact(b'03decc16c 2026-09-27 10:01:18 1.234567\n') ==
           b'03decc16c 2026-09-27 10:01:18 1.234567\n', 'redaction stable and ordinary measurements unchanged')
    try:
        r.redact(b'-----BEGIN PRIVATE KEY-----')
    except ValueError:
        refusals += 1
    else:
        raise ValueError('private key marker accepted')
    positive = 9
    for test in (classification_tests, stop_certificate_tests):
        accepted, rejected = test()
        positive += accepted
        refusals += rejected
    print(json.dumps(dict(status='passed', scope='pure_publication_helpers', positive=positive,
                         rejected=refusals, GCP_used=False, measured_data=False), sort_keys=True))


if __name__ == '__main__':
    main()
