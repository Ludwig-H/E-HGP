#!/usr/bin/env python3
"""Portable reader; no cloud API, compiler, process or credential access."""
import argparse
import hashlib
import io
import json
from pathlib import Path
import tarfile

ROOT = Path(__file__).resolve().parent
WORKER = '043197e4c73d92ff8845ddb5836c11fbb9ed9bdbd9a609edf7ce4a946d2c0fb3'
CONTROLLER = '177b25a0d72150dc331661fdf8da1ccde77ea17fb694d9c6af5b0929755160d8'
START = '73d76c674c71d997a803587a0b20186f668e7aa44f62d4c8b516e22e13469bc0'
STOP = 'ddcad77aa995ebb334fd3f341f7bb81ac94f749593fec98f885fb1c4b7956f3c'
SUPPORT = 'da967163bdb7247bc6aad4df0c294cda1071076a0127cd5bd9f59bc0e4788439'
ADAPTER = '994d9e6970797594efa2d333275594ef2085cdd166b345b0000135e94e395fb7'
PROBE = '21d0a5dd8e086506c91a8d3c0aec55c009bcbb901885085878f7cf3f5f450216'
GATE = '98e426f288d52d0331b3480f3a8233ac525814069893296794a46e1a19b30d29'
TAR = '15f0abbd41bc77d3c3c6e0bb0bce3bb373f5d41ea525b53d02e8879ea90d2c4b'
SOURCE_MANIFEST = '803f1fcfdd9369f1d13900a5793a446d455caf1b938fb2c36aa7f9c83d850f76'
PREFIX = 'morsehgp3D_v7/bench/'
PRIVATE = PREFIX + 'terminal_batch_private/'


def need(ok, reason):
    if not ok:
        raise ValueError(reason)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def unique(pairs):
    out = {}
    for key, value in pairs:
        need(key not in out, 'duplicate JSON key')
        out[key] = value
    return out


def decode(raw):
    return json.loads(raw, object_pairs_hook=unique)


def safe(name):
    path = Path(name)
    need(type(name) is str and not path.is_absolute() and '..' not in path.parts and str(path) == name,
         'safe relative path')
    return path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--extract', type=Path, help='new directory; reconstruct logical bytes and replay source tree')
    args = parser.parse_args()
    physical = decode((ROOT / 'manifest.json').read_bytes())
    need(physical['schema'] == 'mhgp7-terminal-batch-worker-packet-v1', 'packet schema')
    found = {p.relative_to(ROOT).as_posix() for p in ROOT.rglob('*') if p.is_file()}
    need(found == set(physical['files']) | {'manifest.json'}, 'closed physical inventory')
    for name, pin in physical['files'].items():
        path = ROOT / safe(name)
        need(path.is_file() and not path.is_symlink() and path.resolve().is_relative_to(ROOT), 'physical regular file')
        raw = path.read_bytes()
        need(len(raw) == pin['bytes'] and sha(raw) == pin['sha256'] and not raw.startswith(b'\x7fELF'), 'physical bytes')
    mapping = decode((ROOT / 'storage_map.json').read_bytes())
    logical = decode((ROOT / 'logical_manifest.json').read_bytes())
    need(set(mapping) == set(logical) and set(mapping.values()) <= set(physical['files']), 'logical map')
    data = {}
    for name, pin in logical.items():
        safe(name)
        raw = (ROOT / safe(mapping[name])).read_bytes()
        need(len(raw) == pin['bytes'] and sha(raw) == pin['sha256'], 'logical bytes')
        need(b'\n-----BEGIN OPENSSH PRIVATE KEY-----\n' not in raw, 'no credential payload')
        data[name] = raw

    def get(name):
        need(name in data, 'required logical file: ' + name)
        return data[name]

    def obj(name):
        return decode(get(name))

    receipt = obj('snapshot/receipt.json')
    need(receipt['status'] == 'passed' and receipt['files'] == 59 and receipt['GCP_used'] is False
         and receipt['inherited_device_results'] is False and receipt['no_ELF'] and receipt['no_vendor'], 'snapshot scope')
    for name, pin in (('worker.py', WORKER), ('source_snapshot.tar.gz', TAR), ('source_manifest.json', SOURCE_MANIFEST)):
        need(sha(get('snapshot/' + name)) == pin, 'frozen worker/archive/manifest')
    need(receipt['worker_sha256'] == WORKER and receipt['snapshot_sha256'] == TAR
         and receipt['source_manifest_sha256'] == SOURCE_MANIFEST, 'snapshot receipt pins')
    manifest = obj('snapshot/source_manifest.json')
    need(len(manifest) == 59, '59 source files')
    source_names = {name.removeprefix('snapshot/snapshot/') for name in data if name.startswith('snapshot/snapshot/')}
    need(source_names == set(manifest), 'exact source closure')
    for name, pin in manifest.items():
        safe(name)
        need(name.startswith(PREFIX) and sha(get('snapshot/snapshot/' + name)) == pin, 'source file pin')
    for name, pin in ((PREFIX + 'session_support/full_probe_worker_v7.py', SUPPORT),
                      (PREFIX + 'nvcc_strict_host.py', ADAPTER),
                      (PRIVATE + 'whole_gate.cu', GATE),
                      (PRIVATE + 'prototype/source/morsehgp3D_v7/bench/full_ball_tower_probe.cpp', PROBE)):
        need(manifest[name] == pin, 'fixed executable/source authority')
    need(PREFIX + 'full_gabriel_lazy_probe.cpp' in manifest, 'controller historical source admitted, never selected')
    with tarfile.open(fileobj=io.BytesIO(get('snapshot/source_snapshot.tar.gz')), mode='r:gz') as archive:
        entries = archive.getmembers()
        need(len(entries) == len(manifest) and {m.name for m in entries} == set(manifest), 'exact tar inventory')
        for member in entries:
            safe(member.name)
            need(member.isfile() and member.mode == 0o600 and member.uid == member.gid == member.mtime == 0,
                 'source archive metadata')
            stream = archive.extractfile(member)
            need(stream is not None and stream.read() == get('snapshot/snapshot/' + member.name), 'tar/source bytes')
    qualification = obj('snapshot/gate_qualification.receipt.json')
    need(qualification['status'] == 'passed' and qualification['stable'] is True
         and qualification['source_gate_sha256'] == GATE and qualification['mode'] == 'san'
         and qualification['device_executed'] is False and qualification['cuda_ELF_invoked'] is False,
         'host SAN source prerequisite only')
    need(sha(get('snapshot/gate_qualification.receipt.json')) == receipt['gate_qualification_receipt_sha256'],
         'qualification receipt hash')
    qualified = obj('snapshot/gate_qualified_sources.json')
    qbefore, qafter = obj('origins/gate_sources_before.json'), obj('origins/gate_sources_after.json')
    need(qbefore == qafter and all(qbefore.get(name) == pin for name, pin in qualified.items()), 'common qualified sources')
    inputs = receipt['source_inputs']
    for name, pin in manifest.items():
        if name.startswith(PRIVATE):
            need(qualified.get(inputs[name]) == pin, 'no silent helper substitution')
    for name, pin in (('full_probe_session_v7.py', CONTROLLER), ('start_and_verify.sh', START), ('stop_and_verify.sh', STOP)):
        need(sha(get('origins/' + name)) == pin, 'unchanged controller/guard source')
    need(START.encode() in get('origins/full_probe_session_v7.py')
         and STOP.encode() in get('origins/full_probe_session_v7.py'), 'controller fixed guard pins')
    test_pins = {'pure_r1': '6bfedb84d265677feb1a25a3003185192c4e6ec5f47426631ef2088375263081',
                 'pure_r2': '02155533416c6e49f6e1987878b2224010f896229573d1acd30913a0c67e8fbc'}
    for capture, test_pin in test_pins.items():
        prefix = 'tests/' + capture + '/'
        record = obj(prefix + 'receipt.json')
        need(record['status'] == 'passed' and record['source_stable'] is True and record['error'] is None
             and record['compiled'] is False and record['GCP_used'] is False and record['device_executed'] is False,
             'pure Python capture only')
        before, after = obj(prefix + 'sources_before.json'), obj(prefix + 'sources_after.json')
        need(before == after and len(before) == 5, 'five source pins stable')
        for path, pin in before.items():
            need(sha(get(prefix + Path(path).name)) == pin, 'captured exact test sources')
        need(sha(get(prefix + 'terminal_batch_worker_v7.py')) == WORKER
             and sha(get(prefix + 'selftest_terminal_batch_worker_v7.py')) == test_pin, 'revision identity')
        need([command['name'] for command in record['commands']] == ['normal', 'optimized'], 'two pure test commands')
        for command in record['commands']:
            name = command['name']
            need(command == obj(prefix + name + '.command.json') and command['exit_code'] == 0, 'exact command receipt')
            flags = ['python3', '-B'] + (['-O'] if name == 'optimized' else [])
            need(command['argv'][:-1] == flags and command['argv'][-1].endswith('/gcp-migration/selftest_terminal_batch_worker_v7.py'),
                 'normal/-O exact interpreter mode')
            for stream in ('stdout', 'stderr'):
                need(sha(get(prefix + name + '.' + stream)) == command[stream + '_sha256'], 'stream pin')
            need(not get(prefix + name + '.stderr'), 'empty pure test stderr')
            summary = obj(prefix + name + '.stdout')
            need(summary == dict(status='passed', checks=29, rejections=231, synthetic_only=True,
                                 subprocess_invoked=False, device_executed=False, GCP_used=False), 'fixed pure nonvacuity')
    original = get('tests/pure_r1/selftest_terminal_batch_worker_v7.py')
    hardened = get('tests/pure_r2/selftest_terminal_batch_worker_v7.py')
    addition = b"    w.need(checks == 29 and rejections == 231, 'fixed pure selftest nonvacuity')\n"
    need(hardened.count(addition) == 1 and hardened.replace(addition, b'', 1) == original, 'r2 only explicit nonvacuity guard')
    if args.extract:
        destination = args.extract.resolve()
        need(not destination.exists() and not destination.is_relative_to(ROOT), 'fresh extraction outside packet')
        destination.mkdir(parents=True)
        for name, raw in data.items():
            target = destination / 'logical' / safe(name)
            target.parent.mkdir(parents=True, exist_ok=True)
            with target.open('xb') as stream:
                stream.write(raw)
        replay = destination / 'replay'
        replay_files = {
            'gcp-migration/terminal_batch_worker_v7.py': get('snapshot/worker.py'),
            'gcp-migration/selftest_terminal_batch_worker_v7.py': hardened,
            'gcp-migration/full_probe_worker_v7.py': get('snapshot/snapshot/' + PREFIX + 'session_support/full_probe_worker_v7.py'),
            PREFIX + 'nvcc_strict_host.py': get('snapshot/snapshot/' + PREFIX + 'nvcc_strict_host.py')}
        for name, raw in replay_files.items():
            target = replay / name
            target.parent.mkdir(parents=True, exist_ok=True)
            with target.open('xb') as stream:
                stream.write(raw)
        plan = dict(scope='pure Python replays; never start a cloud session',
            commands=[['python3', '-B', *flags, str(replay / 'gcp-migration/selftest_terminal_batch_worker_v7.py')]
                      for flags in ([], ['-O'])], source_snapshot_tar=str(destination / 'logical/snapshot/source_snapshot.tar.gz'),
            source_manifest=str(destination / 'logical/snapshot/source_manifest.json'),
            worker=str(destination / 'logical/snapshot/worker.py'), GCP_used=False)
        with (destination / 'command_plan.json').open('x') as stream:
            json.dump(plan, stream, indent=2, sort_keys=True)
            stream.write('\n')
        need(all((destination / 'logical' / name).read_bytes() == raw for name, raw in data.items()), 'extraction exact bytes')
    print(json.dumps(dict(status='passed', logical_files=len(data), snapshot_sources=59, pure_captures=2,
                          pure_commands=4, checks_each=29, rejections_each=231, unchanged_controller_guards=True,
                          extracted=bool(args.extract), device_executed=False, GCP_used=False), sort_keys=True))


if __name__ == '__main__':
    main()
