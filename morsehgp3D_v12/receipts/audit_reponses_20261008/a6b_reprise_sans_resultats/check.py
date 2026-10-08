#!/usr/bin/env python3
"""Reprise A6b : sources et arrêt local ; aucun moteur, contrôleur ou payload."""
from pathlib import Path
import argparse
import ast
import hashlib
import importlib.util
import json
import subprocess
import sys
import tarfile

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
PREFIX = 'morsehgp3D_v12/'


def need(ok, why):
    if not ok:
        raise ValueError(why)


def pin(raw):
    return dict(bytes=len(raw), sha256=hashlib.sha256(raw).hexdigest())


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--repo', required=True, type=Path)
    ap.add_argument('--session', required=True, type=Path)
    args = ap.parse_args()
    cap = json.loads((HERE / 'capture.json').read_text())
    protocol = json.loads((HERE / 'protocol.json').read_text())
    raw = {}
    for name, expected in cap['local_files'].items():
        raw[name] = (args.session / name).read_bytes()
        need(pin(raw[name]) == expected, 'closed primary: ' + name)
    for name, expected in cap['helpers'].items():
        need(pin((args.repo / PREFIX / name).read_bytes()) == expected,
             'reader or cohort: ' + name)
    recovery = json.loads(raw[cap['recovery_file']])
    need({k: recovery[k] for k in cap['closure']} == cap['closure'], 'closure')
    need(recovery['status'] == 'stopped' and
         recovery['closure'] == 'already_terminated' and
         recovery['targeted_shutdown_certified'] is True and
         recovery['oslogin_key_removed'] is True and
         not recovery['errors'] and not recovery['warnings'], 'certified recovery')
    need(recovery['observed_before_stop']['status'] == 'TERMINATED', 'observed stop')
    describe = json.loads(raw['host/logs/recover_1791478984_001_describe_before_stop_1.stdout'])
    need(describe['status'] == 'TERMINATED', 'description stop')
    commands = [{k: c[k] for k in ('name', 'exit_code', 'elapsed_seconds')}
                for c in recovery['host_commands']]
    need(commands == cap['commands'] and all(c['exit_code'] == 0 for c in commands),
         'recovery commands')
    for name in ('host/logs/recover_1791478984_001_describe_before_stop_1.json',
                 'host/logs/recover_1791478984_002_oslogin_remove.json'):
        item = json.loads(raw[name])
        need({k: item[k] for k in ('name', 'exit_code', 'elapsed_seconds')} in commands,
             'command primary')
    need(json.loads(raw['launch.json'])['started_utc'] == cap['launch_started_utc'],
         'launch date')
    pre = json.loads(raw['preflight.json'])
    plan = json.loads(raw['package/plan.json'])
    need(pre['commit'] == cap['source_git'] == protocol['source_git'], 'source pin')
    need(pin(raw['package/plan.json']) == protocol['plan'] and
         protocol['plan']['sha256'] == pre['plan_sha256'], 'plan pin')
    need([{k: c[k] for k in ('name', 'timeout_seconds')} for c in plan['commands']]
         == protocol['commands'], 'declared commands')
    argv = plan['commands'][1]['argv']
    opts = {k: argv[i + 1] for i, k in enumerate(argv[:-1]) if k.startswith('--')}
    need(all(opts.get(k) == v for k, v in protocol['command_parameters'].items()) and
         '--essai' not in argv, 'declared measurement configuration')
    common = load('a6b_source_reader', args.repo / PREFIX /
                  'receipts/audit_reponses_20261008/session_t1d_admission/check.py')
    before, before_bytes = common.source(args.repo,
        Path(pre['data_dir']) / 'v12_src_47feedc96.tar.gz', cap['before_git'])
    after, after_bytes = common.source(args.repo,
        args.session / 'package/package.tar.gz', cap['source_git'])
    need(dict(before=before, after=after) == protocol['source_packages'], 'source archives')
    need(after['archive']['sha256'] == pre['package_sha256'] and
         before['archive']['sha256'] == opts['--avant-sha256'], 'archive declarations')
    need(before_bytes[PREFIX + 'bench/full_probe.cpp'] ==
         after_bytes[PREFIX + 'bench/full_probe.cpp'], 'unchanged FULL emitter')
    need(pin(after_bytes[PREFIX + 'bench/full_probe.cpp']) == protocol['full_probe'],
         'FULL emitter pin')
    pilot = None
    with tarfile.open(args.session / 'package/package.tar.gz') as archive:
        for name, expected in protocol['source_pins'].items():
            item = archive.extractfile(PREFIX + name).read()
            need(pin(item) == expected and
                 item == common.git(args.repo, cap['source_git'], PREFIX + name),
                 'pilot source: ' + name)
            if name.endswith('/pilote_t2d_a6b.py'):
                pilot = item
    literals = {}
    for statement in ast.parse(pilot).body:
        if isinstance(statement, ast.Assign) and len(statement.targets) == 1:
            target = statement.targets[0]
            if isinstance(target, ast.Name) and target.id in ('SEUIL_GRANDES', 'REGLE_T2D_A6B'):
                value = statement.value
                if target.id == 'REGLE_T2D_A6B':
                    for i, item in enumerate(value.values):
                        if isinstance(item, ast.Name):
                            value.values[i] = ast.Constant(literals[item.id])
                literals[target.id] = ast.literal_eval(value)
    need(literals['REGLE_T2D_A6B'] == protocol['rule'], 'predeclared literal rule')
    old = json.loads((args.repo / PREFIX /
        'receipts/audit_reponses_20261008/session_m_provenance/capture.json').read_text())
    name = 'g4_kitti_v12set_xyz.tar'
    declared = [{k: d[k] for k in ('name', 'size', 'sha256')} for d in pre['data_files']]
    need(declared == protocol['data_declared'], 'input metadata')
    need(next(d for d in old['data_declared'] if d['name'] == name) ==
         next(d for d in declared if d['name'] == name), 'same declared 37-frame archive')
    cohort = json.loads((args.repo / PREFIX /
        'receipts/audit_reponses_20261008/session_t2da_admission/capture.json').read_text())['cohort']
    need([[c['name'], c['sites']] for c in cohort] == protocol['cohort']['cas'],
         'independent cohort declaration')
    delta = subprocess.check_output(['git', '-C', str(args.repo), 'diff', '--name-only',
        cap['before_git'], cap['source_git'], '--', PREFIX + 'src']).decode().splitlines()
    need(delta == protocol['native_delta'], 'product delta')
    for name, value in raw.items():
        need((args.session / name).read_bytes() == value, 'primary changed while reading')
    # Absence is a dated observation. Later recovered results need a separate admission.
    print(json.dumps(dict(sources_exact=True, targeted_shutdown_certified=True,
        observed_state='TERMINATED', campaign_admitted=False, timing_admitted=False,
        observation_utc=cap['observation_utc'],
        absent_then_present_now=[p for p in cap['absent_at_observation']
                                 if (args.session / p).exists()])))


if __name__ == '__main__':
    main()
