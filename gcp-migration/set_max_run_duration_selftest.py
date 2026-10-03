#!/usr/bin/env python3
"""Exercise the scheduling guard with an isolated fake gcloud; no cloud or native product calls."""
import copy
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
SCRIPT = HERE / 'set_max_run_duration_and_verify.sh'
PROJECT = 'devpod-gpu-exploration'
TARGETS = (('europe-west4-a', 'ehgp-blackwell-spot'),
           ('europe-west4-ai1a', 'ehgp-blackwell-spot-ai1a'),
           ('us-central1-c', 'ehgp-v7-3b1d496aed430749ea7e049f'))
FIELDS = {'status': 'TERMINATED', 'scheduling.instanceTerminationAction': 'STOP',
          'scheduling.automaticRestart': 'false', 'scheduling.maxRunDuration.seconds': '3600',
          'labels.project': 'e-hgp', 'machineType.basename()': 'g4-standard-48',
          'scheduling.onHostMaintenance': 'TERMINATE', 'scheduling.provisioningModel': 'SPOT',
          'lastStartTimestamp': '2026-10-03T04:30:00.000000Z'}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def fake_gcloud():
    path = Path(os.environ['E_HGP_SCHEDULING_FAKE_STATE'])
    state = json.loads(path.read_text())
    args = sys.argv[2:]
    state['calls'].append(args)
    path.write_text(json.dumps(state))
    config = state['config']
    zone, instance = config['target']
    if args[:2] == ['config', 'get-value']:
        require(len(args) == 3 and args[2] in ('project', 'account'), 'unexpected config read')
        print(config.get('active_project', PROJECT) if args[2] == 'project'
              else config.get('account', 'test@example.invalid'))
        return 0
    if args[:3] == ['compute', 'instances', 'list']:
        require(args == ['compute', 'instances', 'list', '--project='+PROJECT, '--zones='+zone,
                         '--filter=name='+instance, '--limit=1', '--format=value(name)'], 'foreign inventory')
        print(config.get('listed', instance))
        return 0
    require(args[:2] == ['compute', 'instances'] and args[3] == instance, 'foreign instance')
    require(args[4:6] == ['--project='+PROJECT, '--zone='+zone], 'foreign project/zone')
    if args[2] == 'set-scheduling':
        expected = ['--max-run-duration='+str(config['seconds'])+'s', '--instance-termination-action=STOP', '--quiet']
        require(args[6:] == expected, 'unexpected mutation arguments')
        state['mutated'] = True
        path.write_text(json.dumps(state))
        return config.get('mutation_code', 0)
    require(args[2] == 'describe' and len(args) == 7, 'unapproved cloud operation')
    require(args[6].startswith('--format=value(') and args[6].endswith(')'), 'unexpected describe format')
    field = args[6][15:-1]
    require(field in FIELDS, 'unexpected describe field')
    phase = 'post' if state['mutated'] else 'pre'
    if config.get('read_failure') == phase+':'+field:
        return 1
    value = copy.deepcopy(FIELDS)
    if state['mutated']:
        value['scheduling.maxRunDuration.seconds'] = str(config['seconds'])
    value.update(config.get(phase, {}))
    print(value[field])
    return 0


def run_case(root, number, name, *, target=TARGETS[2], seconds=4200, arguments=None,
             changes=None, environment=None, expected='pre', no_calls=False, defaults=False):
    config = dict(target=list(target), seconds=seconds)
    config.update(changes or {})
    statefile = root / ('case_%03d.json' % number)
    statefile.write_text(json.dumps(dict(config=config, calls=[], mutated=False)))
    env = {key: value for key, value in os.environ.items()
           if not key.startswith(('GCP_', 'CLOUDSDK_', 'E_HGP_SCHEDULING_'))}
    env.update(PATH=str(root/'bin'), E_HGP_SCHEDULING_FAKE_STATE=str(statefile),
               PYTHONDONTWRITEBYTECODE='1')
    if not defaults:
        env.update(GCP_PROJECT_ID=PROJECT, GCP_ZONE=target[0], GCP_INSTANCE_NAME=target[1])
    env.update(environment or {})
    argv = ['/bin/bash', str(SCRIPT)] + (arguments if arguments is not None else
                                       ['--yes', '--max-run-duration-seconds', str(seconds)])
    result = subprocess.run(argv, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=15, check=False)
    state = json.loads(statefile.read_text())
    mutations = [call for call in state['calls'] if call[:3] == ['compute', 'instances', 'set-scheduling']]
    stdout, stderr = result.stdout.decode(), result.stderr.decode()
    require('Traceback' not in stdout+stderr, name+': fake protocol error')
    require(len(mutations) == (0 if expected == 'pre' else 1), name+': mutation count')
    if no_calls:
        require(not state['calls'], name+': cloud read before validation')
    if expected == 'ok':
        require(result.returncode == 0 and '[OK]' in stdout and
                '[GARDE pre-mutation]' in stdout and '[GARDE post-mutation]' in stdout and not stderr,
                name+': success not certified')
    else:
        require(result.returncode == 1 and '[ERREUR]' in stderr and '[OK]' not in stdout,
                name+': missing refusal')
        require(('[RECONFIGURATION NON CERTIFIÉE]' in stderr) == (expected == 'post'),
                name+': uncertain mutation not preserved')
    return dict(case=name, expected=expected, exit_code=result.returncode, argv=argv,
                calls=state['calls'], stdout=stdout, stderr=stderr)


def exercise():
    records = []
    with tempfile.TemporaryDirectory(prefix='ehgp-scheduling-guard-') as folder:
        root = Path(folder)
        (root/'bin').mkdir()
        # PATH contains no real gcloud and no fallback directory, even if a case or fake fails.
        for program in ('timeout', 'sed', 'true'):
            executable = shutil.which(program)
            require(executable is not None, 'required local shell utility '+program)
            (root/'bin'/program).symlink_to(executable)
        fake = root/'bin/gcloud'
        fake.write_text('#!'+sys.executable+'\nimport runpy,sys\nsys.argv=['+repr(str(Path(__file__).resolve()))+
                        ', "--fake-gcloud", *sys.argv[1:]]\nrunpy.run_path(sys.argv[0], run_name="__main__")\n')
        fake.chmod(0o700)

        def case(name, **kwargs):
            records.append(run_case(root, len(records), name, **kwargs))

        for index, target in enumerate(TARGETS):
            case('allowed_'+str(index), target=target, expected='ok', defaults=index == 0)
        for seconds in (30, 28800):
            case('duration_boundary_'+str(seconds), seconds=seconds, expected='ok')
        for zone, _ in TARGETS:
            for _, name in TARGETS:
                if (zone, name) not in TARGETS:
                    case('cross_target_'+zone+'_'+name, target=(zone, name), no_calls=True)
        for target in (('us-central1-b', TARGETS[2][1]), (TARGETS[2][0], TARGETS[2][1]+'x'),
                       ('foreign-zone', 'foreign-instance')):
            case('foreign_target_'+str(target), target=target, no_calls=True)
        case('foreign_project', environment={'GCP_PROJECT_ID': 'foreign'}, no_calls=True)
        for value in ('0', '29', '28801', '999999', '030', '+30', '-30', '30.0', '30s', '', '1;true'):
            case('duration_invalid_'+value, seconds=value, no_calls=True)
        invalid = ([], ['--yes'], ['--max-run-duration-seconds', '4200'],
                   ['--yes', '--max-run-duration-seconds'], ['--yes', '--yes', '--max-run-duration-seconds', '4200'],
                   ['--yes', '--max-run-duration-seconds', '4200', '--max-run-duration-seconds', '4200'],
                   ['--yes', '--max-run-duration-seconds', '4200', '--foreign'])
        for index, args in enumerate(invalid):
            case('argument_invalid_'+str(index), arguments=args, no_calls=True)
        for key, values in {'active_project': ('foreign', ''), 'account': ('', '(unset)'),
                            'listed': ('', 'foreign', TARGETS[2][1]+'\nforeign')}.items():
            for value in values:
                case('config_'+key+'_'+value, changes={key: value})
        bad_fields = {'status': ('RUNNING', 'STOPPING'), 'scheduling.instanceTerminationAction': ('DELETE',),
                      'scheduling.automaticRestart': ('true',), 'scheduling.maxRunDuration.seconds':
                      ('', '0', '29', '28801', '03600', '999999'), 'labels.project': ('foreign',),
                      'machineType.basename()': ('g4-standard-96',), 'scheduling.onHostMaintenance': ('MIGRATE',),
                      'scheduling.provisioningModel': ('STANDARD',), 'lastStartTimestamp': ('foreign\nline', 'bad\r')}
        for field, values in bad_fields.items():
            for value in values:
                case('pre_'+field+'_'+repr(value), changes={'pre': {field: value}})
        for field in FIELDS:
            case('pre_read_failure_'+field, changes={'read_failure': 'pre:'+field})
        for field, value in {'status': 'RUNNING', 'scheduling.instanceTerminationAction': 'DELETE',
                              'scheduling.automaticRestart': 'true', 'scheduling.maxRunDuration.seconds': '3600',
                              'labels.project': 'foreign', 'machineType.basename()': 'g4-standard-96',
                              'scheduling.onHostMaintenance': 'MIGRATE', 'scheduling.provisioningModel': 'STANDARD',
                              'lastStartTimestamp': '2026-10-03T05:00:00.000000Z'}.items():
            case('post_'+field, changes={'post': {field: value}}, expected='post')
        for field in ('status', 'scheduling.maxRunDuration.seconds', 'lastStartTimestamp'):
            case('post_read_failure_'+field, changes={'read_failure': 'post:'+field}, expected='post')
        for code in (1, 124):
            case('mutation_error_'+str(code), changes={'mutation_code': code}, expected='post')
    return records


def main():
    if len(sys.argv) > 1 and sys.argv[1] == '--fake-gcloud':
        return fake_gcloud()
    require(len(sys.argv) in (1, 3) and (len(sys.argv) == 1 or sys.argv[1] == '--report'), 'arguments')
    records = exercise()
    summary = dict(schema='ehgp.scheduling_guard_selftest.v1', cases=len(records),
                   successes=sum(r['expected'] == 'ok' for r in records),
                   refusals_before_mutation=sum(r['expected'] == 'pre' for r in records),
                   uncertified_mutations=sum(r['expected'] == 'post' for r in records), cloud_calls=0, native_product_calls=0)
    if len(sys.argv) == 3:
        Path(sys.argv[2]).write_text(json.dumps(dict(summary=summary, cases=records), indent=2, sort_keys=True)+'\n')
    print(json.dumps(summary, sort_keys=True))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
