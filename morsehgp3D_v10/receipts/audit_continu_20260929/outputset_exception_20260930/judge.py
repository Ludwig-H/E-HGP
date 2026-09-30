"""Strict judge for this frozen helper observation, not a general CLI validator."""
import json
from pathlib import Path
import sys

FIELDS = {'kind', 'baseline_fds', 'control_ok', 'control_fds', 'control_size', 'caught_oom',
          'allocation_failures', 'oom_fds', 'oom_size', 'caught_writer', 'writer_fds', 'writer_size'}

def nonfinite(value):
    raise ValueError('nonfinite JSON: ' + value)

def judge(data):
    if type(data) is not dict or set(data) != FIELDS:
        raise ValueError('observation schema')
    if data['kind'] != 'outputset_exception_observation':
        raise ValueError('observation identity')
    for key in FIELDS - {'kind', 'control_ok', 'caught_oom', 'caught_writer'}:
        if type(data[key]) is not int:
            raise ValueError('integer field: ' + key)
    for key in ['control_ok', 'caught_oom', 'caught_writer']:
        if data[key] is not True:
            raise ValueError('missing positive control/fault: ' + key)
    base = data['baseline_fds']
    if base < 3 or data['control_fds'] != base or data['control_size'] != -1:
        raise ValueError('control must close and remove')
    if data['allocation_failures'] != 1 or data['oom_fds'] != base + 1 or data['oom_size'] != 0:
        raise ValueError('allocation leak/truncation not observed')
    if data['writer_fds'] != base + 2 or data['writer_size'] != -1:
        raise ValueError('writer leak/unlink not observed')
    return data

def parse(path):
    lines = Path(path).read_text().splitlines()
    if len(lines) != 1:
        raise ValueError('expected one observation')
    return judge(json.loads(lines[0], parse_constant=nonfinite))

def adverse_controls(data):
    mutations = {}
    for key in FIELDS:
        changed = dict(data)
        del changed[key]
        mutations['omit_' + key] = changed
    for key, value in [('control_ok', False), ('caught_oom', False), ('caught_writer', False),
                       ('allocation_failures', 0), ('oom_fds', data['baseline_fds']),
                       ('writer_fds', data['baseline_fds'] + 1), ('oom_size', 21),
                       ('control_size', 0), ('writer_size', 0), ('baseline_fds', True),
                       ('kind', 'status_ok')]:
        changed = dict(data)
        changed[key] = value
        mutations['replace_' + key] = changed
    # A no-op helper cannot pass: it produces neither positive reserve nor any fault/leak.
    noop = dict(data)
    noop.update(control_ok=False, caught_oom=False, caught_writer=False, allocation_failures=0,
                control_size=21, oom_fds=data['baseline_fds'], writer_fds=data['baseline_fds'])
    mutations['noop_helper'] = noop
    for name, changed in mutations.items():
        try:
            judge(changed)
        except ValueError:
            continue
        raise ValueError('surviving adverse control: ' + name)
    return len(mutations)

if __name__ == '__main__':
    try:
        if len(sys.argv) != 2:
            raise ValueError('usage: judge.py observation.stdout')
        observation = parse(sys.argv[1])
        print(json.dumps({'status': 'observed_faults', 'adverse_refusals': adverse_controls(observation)}, sort_keys=True))
    except (OSError, ValueError) as e:
        print('REFUS: ' + str(e), file=sys.stderr)
        sys.exit(2)
