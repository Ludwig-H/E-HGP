#!/usr/bin/env python3
"""Cible v11 explicite : tests Python seuls, appels cloud/worker entierement doubles, aucun natif."""
import argparse
import copy
import importlib.util
import json
from pathlib import Path
import shutil
import sys
import tempfile
from unittest.mock import patch

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('target_session', HERE / 'v11_session.py')
M = importlib.util.module_from_spec(spec)
spec.loader.exec_module(M)
TARGET = dict(project='devpod-gpu-exploration', zone='us-central1-c', instance='ehgp-v7-3b1d496aed430749ea7e049f')
GENERATION = '2026-10-02T12:00:00.000000Z'
CHECKS = 0


def check(ok, message):
    global CHECKS
    CHECKS += 1
    if not ok:
        raise ValueError(message)


def refuses(fn):
    try:
        fn()
    except M.Refusal:
        check(True, 'refus attendu')
    else:
        raise ValueError('refus absent')


def write(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value))


def description():
    return dict(name=TARGET['instance'], zone=TARGET['zone'],
                selfLink='https://example.invalid/projects/%s/zones/%s/instances/%s' %
                         (TARGET['project'], TARGET['zone'], TARGET['instance']),
                labels=dict(project='e-hgp'), machineType='g4-standard-48', status='TERMINATED',
                lastStartTimestamp=GENERATION,
                metadata=dict(items=[dict(key='enable-oslogin', value='TRUE')]),
                scheduling=dict(provisioningModel='SPOT', instanceTerminationAction='STOP',
                                onHostMaintenance='TERMINATE', automaticRestart=False,
                                maxRunDuration=dict(seconds='3600')))


def identity_checks(root):
    check(M.configured_target() == M.DEFAULT_TARGET, 'ancienne cible par defaut')
    check(M.configured_target(TARGET['zone'], TARGET['instance']) == TARGET, 'cible c')
    for zone, name in ((None, 'vm'), ('us-central1-c', None), ('us-central1-c;id', 'vm'),
                       ('us-central1', 'vm'), ('us-central1-c', '-vm'), ('us-central1-c', 'A'),
                       ('us-central1-c', 'vm;id'), ('us-central1-c', 'a'*64)):
        refuses(lambda: M.configured_target(zone, name))
    M.TARGET = dict(TARGET)
    cloud = description()
    M.check_target(cloud, 'TERMINATED', 3600)
    check(True, 'gardes positives')
    changes = [(('name',), 'foreign'), (('zone',), 'us-central1-b'), (('selfLink',), '/foreign'),
               (('labels', 'project'), 'foreign'), (('machineType',), 'n1-standard-4'),
               (('scheduling', 'provisioningModel'), 'STANDARD'),
               (('scheduling', 'instanceTerminationAction'), 'DELETE'),
               (('scheduling', 'onHostMaintenance'), 'MIGRATE'),
               (('scheduling', 'automaticRestart'), True), (('scheduling', 'maxRunDuration', 'seconds'), '7200'),
               (('status',), 'RUNNING'), (('metadata', 'items'), [])]
    for keys, value in changes:
        bad = copy.deepcopy(cloud)
        into = bad
        for key in keys[:-1]:
            into = into[key]
        into[keys[-1]] = value
        refuses(lambda: M.check_target(bad, 'TERMINATED', 3600))
    env = M.runner_env(Path('/fake/gcloud'), account='fake@example.invalid')
    check(env['GCP_ZONE'] == TARGET['zone'] and env['GCP_INSTANCE_NAME'] == TARGET['instance'] and
          env['CLOUDSDK_CORE_PROJECT'] == TARGET['project'], 'transport cible figee')
    command = M.recovery_command(root, GENERATION)
    check(TARGET['zone'] in command and TARGET['instance'] in command and GENERATION in command,
          'commande fermeture exacte')


def trace_checks(root):
    session = root / 'traces'
    write(session / 'preflight.json', dict(target=TARGET))
    write(session / 'launch.json', dict(target=TARGET))
    write(session / 'receipt.json', dict(target=TARGET))
    write(session / 'host/handoff.json', dict(TARGET, schema='e-hgp.start-handoff.v3', last_start_timestamp=GENERATION))
    check(M.recorded_target(session) == TARGET, 'reprise deduite')
    check(M.recorded_target(session, TARGET) == TARGET, 'reprise cible confirme')
    refuses(lambda: M.recorded_target(session, M.DEFAULT_TARGET))
    write(session / 'receipt.json', dict(target=M.DEFAULT_TARGET))
    refuses(lambda: M.recorded_target(session))
    (session / 'receipt.json').unlink()
    (session / 'preflight.json').write_text('{')
    warnings = []
    check(M.recorded_target(session, warnings=warnings) == TARGET and len(warnings) == 1,
          'preflight tronque : handoff conserve autorite')
    (session / 'preflight.json').unlink()
    write(session / 'launch.json', dict(status='launched'))
    check(M.recorded_target(session) == TARGET, 'ancien launch sans cible')
    (session / 'host/handoff.json').unlink()
    refuses(lambda: M.recorded_target(session))
    check(M.recorded_target(session, allow_absent=True) == M.DEFAULT_TARGET, 'ancien lanceur sans debut de start')
    for wrong in (dict(TARGET, project='foreign'), dict(TARGET, zone=None), dict(TARGET, instance='vm;id')):
        write(session / 'preflight.json', dict(target=wrong))
        refuses(lambda: M.recorded_target(session))


class FakeRunner:
    def __init__(self, *args, **kwargs):
        self.rows = []

    def run(self, name, argv, **kwargs):
        self.rows.append(dict(argv=[str(v) for v in argv]))
        if name == 'config_project':
            value = TARGET['project']
        elif name == 'config_account':
            value = 'fake@example.invalid'
        elif name == 'describe_before':
            value = json.dumps(description())
        else:
            raise ValueError('appel cloud inattendu '+name)
        return 0, value, ''


def cli_checks(root):
    outputs = []
    required = ['--commit', '9'*40, '--plan', 'fake.json', '--data', str(root), '--session-dir', str(root/'cli'),
                '--max-run-seconds', '3600']
    with patch.object(M, 'preflight', return_value={}) as preflight, \
            patch.object(M, 'planned_steps', return_value=[]), patch.object(M.CONSOLE, 'emit', side_effect=outputs.append):
        check(M.main(required + ['--zone', TARGET['zone'], '--instance', TARGET['instance']]) == 0 and
              outputs[-1]['target'] == TARGET, 'CLI cible explicite')
        check(M.main(required) == 0 and outputs[-1]['target'] == M.DEFAULT_TARGET, 'CLI defaut restaure')
        before = preflight.call_count
        check(M.main(required + ['--zone', TARGET['zone']]) == 2 and preflight.call_count == before,
              'CLI paire incomplete avant preflight')
    session = root / 'launched'
    run_dir = root / 'launch_run'; run_dir.mkdir()
    M.TARGET = dict(TARGET)
    argv = required + ['--zone', TARGET['zone'], '--instance', TARGET['instance'], '--execute']
    with patch.object(M.subprocess, 'Popen') as popen, patch.object(M, 'make_run_directory', return_value=run_dir), \
            patch.object(M.CONSOLE, 'emit', side_effect=outputs.append), patch.object(M, 'log'):
        popen.return_value.pid = 123
        check(M.launch(argparse.Namespace(wait=False), {}, dict(session=session), argv) == 0, 'lanceur mocke')
        child = popen.call_args.args[0]
        check(child[child.index('--zone')+1] == TARGET['zone'] and
              child[child.index('--instance')+1] == TARGET['instance'] and '--child' in child,
              'cible exacte transmise a enfant')
        check(json.loads((session/'launch.json').read_text())['target'] == TARGET, 'cible gravee avant reprise')


def preflight_checks(root):
    sessions = root / 'sessions'; sessions.mkdir()
    data = root / 'data'; data.mkdir()
    (data / 'point.u32le').write_bytes(bytes(4))
    plan = root / 'plan.json'
    write(plan, dict(schema=M.PLAN_SCHEMA, default_build=False, python_packages='none', commands=[
        dict(name='check', timeout_seconds=60, argv=['python3', '{src}/morsehgp3D_v11/check.py'])]))
    package = root / 'frozen.tar.gz'; package.write_bytes(b'unchanged product package')
    commit = '9'*40
    args = argparse.Namespace(max_run_seconds=3600, sessions_root=str(sessions), session_dir=str(sessions/'s1'),
                              snapshot=None, commit=commit, no_fetch=True, data=str(data), plan=str(plan),
                              gcloud=shutil.which('true'))
    protocol = {name: (M.REPO / name).read_bytes() for name in (M.CONTROLLER, M.WORKER, M.START_GUARD, M.STOP_GUARD)}
    with patch.object(M, 'resolve_commit', return_value=commit), \
            patch.object(M, 'commit_file', side_effect=lambda sha, name: protocol[name]), \
            patch.object(M, 'tracked_tree', return_value={'morsehgp3D_v11/check.py'}), \
            patch.object(M, 'build_package', return_value=(package, M.sha_file(package), package.stat().st_size)) as packed, \
            patch.object(M, 'Runner', FakeRunner):
        report = {}
        context = M.preflight(args, report, root, False)
        check(context['commit'] == commit and context['package_sha'] == M.sha_file(package), 'source produit conservee')
        check(packed.call_args.args[0] == commit, 'archive du seul commit demande')
        check(report['protocol_sha256'] == {name: M.sha_bytes(raw) for name, raw in protocol.items()}, 'protocole epingle')
        protocol[M.CONTROLLER] += b'\n# mutation\n'
        refuses(lambda: M.preflight(args, {}, root, False))
        protocol[M.CONTROLLER] = (M.REPO / M.CONTROLLER).read_bytes()
        protocol[M.START_GUARD] += b'\n# mutation\n'
        refuses(lambda: M.preflight(args, {}, root, False))


def recovery_checks(root):
    session = root / 'recover'; host = session / 'host'; host.mkdir(parents=True)
    run_dir = root / 'run'; run_dir.mkdir()
    for name in M.GUARD_PINS:
        shutil.copyfile(M.REPO / name, host / Path(name).name)
    write(session / 'preflight.json', dict(target=TARGET, gcloud_account='fake@example.invalid'))
    write(host / 'handoff.json', dict(TARGET, schema='e-hgp.start-handoff.v3', last_start_timestamp=GENERATION))
    args = argparse.Namespace(session_dir=str(session), gcloud='/fake/gcloud', recover_wait=0, zone=None, instance=None)
    outputs, calls = [], []

    def close(state, runner, frozen_host, gcloud, pre_start, **kwargs):
        calls.append(dict(M.TARGET))
        state.update(targeted_shutdown_certified=True, closure='targeted_stopped')

    with patch.object(M, 'Runner', FakeRunner), patch.object(M, 'close_by_generation', side_effect=close), \
            patch.object(M, 'make_run_directory', return_value=run_dir), patch.object(M, 'run_directory', return_value=run_dir), \
            patch.object(M, 'session_processes', return_value=([], [])), patch.object(M.SIGNALS, 'install'), \
            patch.object(M.CONSOLE, 'emit', side_effect=outputs.append):
        M.TARGET = dict(M.DEFAULT_TARGET)
        check(M.recover(args) == 0 and calls == [TARGET], 'reprise c depuis defaut b')
        check(M.TARGET == TARGET, 'cible de reprise propagee')
        before = len(calls)
        args.zone, args.instance = M.DEFAULT_TARGET['zone'], M.DEFAULT_TARGET['instance']
        check(M.recover(args) == 74 and len(calls) == before, 'CLI reprise contradictoire refuse avant cloud')
        args.zone = args.instance = None
        held = M.take_lock(session.parent / M.ROOT_LOCK_NAME)
        try:
            check(held is not None and M.recover(args) == 76 and len(calls) == before, 'flock v10 commun tenu')
            check(outputs[-1]['status'] == 'session_alive' and outputs[-1]['recovery_receipt'], 'refus verrou journalise')
        finally:
            if held is not None and held >= 0:
                M.os.close(held)
        check(M.recover(args) == 0 and calls[-1] == TARGET, 'reprise apres liberation verrou')


def main():
    with tempfile.TemporaryDirectory(prefix='ehgp-v11-target-') as directory:
        root = Path(directory)
        identity_checks(root)
        trace_checks(root)
        cli_checks(root)
        preflight_checks(root)
        recovery_checks(root)
    print('v11_target_verdict conforme checks%d native0 cloud0' % CHECKS)


if __name__ == '__main__':
    main()
