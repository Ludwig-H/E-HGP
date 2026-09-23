#!/usr/bin/env python3
"""Selftest hors ligne du protocole G4 de la tour FULL v9 ; aucune commande GCP.

Port explicite de q34_spatial_selftest_v8.py (SHA 70e9bf1a17ef32c3...,
commit 70de84f2). Comme en v8, la classe Commands du controleur est
remplacee : un faux gcloud repond a before_start, OS Login, start/stop gardes
et recertifications, et un faux ssh/scp pointe vers un repertoire local qui
tient lieu de VM. Les scripts SSH du controleur (sha256sum, mkdir, tar) y
sont rejoues tels quels par bash, et le vrai worker s'y execute en processus
avec de faux outils (cmake, g++, sonde) et un faux invite (metadonnees,
arret programme, 48 vCPU, horloge de demarrage). GNU time, uptime, lscpu et
nproc sont les vrais outils locaux.

Ces tests qualifient les predicats, le cycle de vie et la fermeture du
protocole, pas une mesure G4 : aucun compilateur reel, aucune VM, aucun
appel cloud. Ils passent sous python3 -B et python3 -B -O : aucune
instruction assert, uniquement need().
"""
import atexit
from contextlib import ExitStack, contextmanager, redirect_stdout
from copy import deepcopy
from datetime import datetime, timezone
import hashlib
import io
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import struct
import subprocess
import sys
import tarfile
import tempfile
import time
from types import SimpleNamespace
import unittest
from unittest.mock import patch
import tower_session_v9 as session
import tower_worker_v9 as worker
import tower_snapshot_v9 as snapshot

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
need = worker.need
NEVER_RUN_GCLOUD = Path('/never-run/gcloud')

FAKE_PROBE = r'''
import hashlib
import json
import pathlib
import sys
import time

TIMES = ('read', 'prepare', 'gen_index', 'q2', 'q34', 'merge', 'tower_index', 'census', 'tower', 'chain_total', 'digest')


def fnv_u32le(raw):
    h = 14695981039346656037
    prime, mask = 1099511628211, (1 << 64) - 1
    for byte in (len(raw) // 12).to_bytes(8, 'little'):
        h = ((h ^ byte) * prime) & mask
    for index in range(0, len(raw), 4):
        for byte in raw[index:index + 4] + bytes(4):
            h = ((h ^ byte) * prime) & mask
    return '%016x' % h


def probe_value(n, fnv, k, s, workers, static, status='complete_relative', salt='', levers=None, schema=None):
    effective = min(k, n)
    complete = status == 'complete_relative'
    orders = [dict(K=q, nodes=2 * n * q, births=n * q, merges=n * q - 1, parents=2 * n * q - 1, contributions=n * q)
              for q in range(1, effective + 1)] if complete else []
    digest = hashlib.sha256((fnv + ':' + str(k) + ':' + str(s) + ':' + salt).encode()).hexdigest()[:16]
    levers = dict(levers) if levers is not None else {name: True for name in schema['levers']}
    # A ledger consistent with the generator and catalogue below, and with
    # each lever (the reader checks these identities exactly).
    ledger = {name: 1 for name in schema['ledger']}
    # Two covers: one keeps both lanes open, the other has both proved dead.
    ledger.update(expanded_pairs=5, cover_builds=2, witness_rejected_pairs=3, cover_sites=10, q3_edges=1, q4_edges=1,
                  both_edges=1)
    if levers['q34_dead_lanes']:
        ledger.update(dead_loads=2, dead_form_sites=6, dead_q3_open=1, dead_q4_open=1, dead_q3_proved=1,
                      dead_q4_proved=1, dead_cells=3)
    else:
        ledger.update({name: 0 for name in schema['ledger'] if name.startswith('dead_')})
    if not levers['q34_witness_cache']:
        ledger.update({name: 0 for name in schema['ledger'] if name.startswith('witness_cache_')})
    if not levers['q3_leaf_census']:
        ledger.update(q3_leaf_censuses=0, q3_leaf_point_tests=0)
    if levers['q34_dead_core']:
        # One edge closed by the diametral core, two sent on to the covers.
        ledger.update(expanded_pairs=6, core_closed_edges=1, core_builds=3, dead_core_loads=3, core_sites=12,
                      dead_core_form_sites=6, dead_core_q3_open=ledger['dead_q3_proved'] + ledger['dead_q3_open'],
                      dead_core_q4_open=ledger['dead_q4_proved'] + ledger['dead_q4_open'], dead_core_q3_proved=1,
                      dead_core_q4_proved=1, core_cover_node_visits=4, core_cover_bound_tests=3,
                      core_cover_point_tests=1, dead_core_cells=3)
    else:
        ledger.update({name: 0 for name in schema['ledger'] if name.startswith(('core_', 'dead_core_'))})
    # Hidden index traversals: consistent identities and 2n-1 bounds.
    ledger.update(q34_input_rectangles=7, witness_rect_queries=7, witness_rect_node_visits=20,
                  witness_pair_queries=ledger['expanded_pairs'] - ledger['witness_cache_rejected_pairs'],
                  witness_pair_node_visits=9, q3_edge_queries=1, q3_seed_node_visits=5, q3_seed_point_tests=3,
                  q3_seed_bound_tests=2, q4_geometry_preparations=1, q4_domain_node_visits=4,
                  q4_cover_decomposition_node_visits=4, q4_seed_node_visits=3, q4_seed_cell_queries=1,
                  q4_sweep_active_sites=6)
    return dict(schema='mhgp9_tower_probe_v12', status=status,
                reason='complete_relative_to_cross_checked_catalogue' if complete else 'selftest_explicit_refusal',
                input=dict(format='u32le', grid='1mm', sites=n, hash=fnv),
                options=dict(K=k, K_effective=effective, s=s, workers=workers, tower_static_threads=static,
                             run_tower=True,
                             levers=levers),
                times_ms=dict({key: 0.125 for key in TIMES}, chain_total=1.5), chain_cpu_s=0.25,
                generator=dict(q2_front_rectangles=3, q2_candidate_pairs=2, q2_accepted_pairs=1,
                               q34_expanded_pairs=ledger['expanded_pairs'], q34_cover_builds=ledger['cover_builds'],
                               q3_emitted=2,
                               q4_emitted=1),
                ledger=ledger,
                catalogue=dict(q2_presentations=1, q3_presentations=2, q4_presentations=1, unique_keys=4, balls=4,
                               extra_shell_balls=0, shell_over_12=0, max_shell=4, max_interior=3, census_nodes=9,
                               census_leaf_tests=5, bytes=64, by_qmin=[1, 2, 1],
                               by_shell=[0, 0, 1, 2, 1] + [0] * 12),
                tower_work=dict(records=4, extra_records=0, representatives=5, anchor_hits=1, key_lookups=4,
                                intruder_queries=2, intruder_nodes=7, meb_calls=4, meb_power_tests=9, births=3,
                                merges=2, contributions=3, grouped_lots=1, resolver_cache_hits=2,
                                meb_accounting=('anchor_meb_first_maximal_pair_then_double_welzl_proposal_exact_'
                                                'boundary_canonical_v3' if levers['tower_meb_proposal'] else
                                                'anchor_meb_first_maximal_pair_then_lexicographic_supports_'
                                                'extremes_first_v2'),
                                meb_pair_distances=6, meb_materializations=4, meb_supports_by_size=[0, 4, 3, 1],
                                meb_proposals=3 if levers['tower_meb_proposal'] else 0,
                                meb_verified_proposals=2 if levers['tower_meb_proposal'] else 0,
                                meb_boundary_canonicalizations=1 if levers['tower_meb_proposal'] else 0,
                                meb_proposal_fallbacks=1 if levers['tower_meb_proposal'] else 0),
                orders=orders, tower_digest=digest if complete else '0' * 16, peak_rss_kb=2048)


def main():
    config = json.loads(pathlib.Path(CONFIG).read_text())
    path, k, workers = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
    levers = {}
    options = {}
    for argument in sys.argv[4:]:
        if argument.startswith('--lever='):
            name, _, value = argument[len('--lever='):].partition('=')
            levers[name] = value == '1'
        elif '=' in argument:
            key, _, value = argument[2:].partition('=')
            options[key] = value
        else:
            options[argument] = None
    if set(options) != {'s', 'static', 'grid'} or options['grid'] != '1mm' or sorted(levers) != sorted(config['schema']['levers']):
        print('argument refusal: selftest', file=sys.stderr)
        return 2
    raw = pathlib.Path(path).read_bytes()
    if pathlib.Path(path).name == 'preflight.u32le':
        value = probe_value(len(raw) // 12, fnv_u32le(raw), k, int(options['s']), workers, int(options['static']),
                            'complete_relative', '', levers, config['schema'])
        if config.get('fail_preflight'):
            value['tower_work']['selftest_unknown'] = 1
        if config.get('vacuous_preflight'):
            value['generator'].update(q3_emitted=0, q4_emitted=0)
            value['catalogue'].update(q3_presentations=0, q4_presentations=0)
        print(json.dumps(value, separators=(',', ':')))
        return 0
    scene = pathlib.Path(path).name[len('scene_'):-len('.u32le')]
    if hashlib.sha256(raw).hexdigest() != config['sha256'][scene]:
        print('input refusal: selftest', file=sys.stderr)
        return 2
    for rule in config.get('sleep', []):
        if rule['workers'] == workers and rule.get('k', k) == k and rule.get('scene', scene) == scene:
            time.sleep(rule['seconds'])
    status = 'complete_relative'
    for rule in config.get('refuse', []):
        if rule['scene'] == scene and rule['k'] == k:
            status = 'unsupported_degeneracy'
    salt = str(workers) if config.get('salt_by_workers') else ''
    value = probe_value(len(raw) // 12, config['fnv'][scene], k, int(options['s']), workers, int(options['static']),
                        status, salt, levers, config['schema'])
    for rule in config.get('malform', []):
        if rule['scene'] == scene and rule['k'] == k:
            value['tower_work']['meb_accounting'] = 'selftest_unpinned_accounting'
    print(json.dumps(value, separators=(',', ':')))
    return 0 if status == 'complete_relative' else 3


if __name__ == '__main__':
    raise SystemExit(main())
'''

FAKE_CMAKE = r'''
import json
import pathlib
import shutil
import sys

HERE = pathlib.Path(__file__).resolve().parent


def main():
    config = json.loads((HERE / 'config.json').read_text())
    args = sys.argv[1:]
    if args == ['--version']:
        print('cmake version 3.22.1 (selftest fake; never a real build)')
        return 0
    if len(args) == 7 and args[0] == '-S' and args[2] == '-B':
        source, build = pathlib.Path(args[1]), pathlib.Path(args[3])
        if (args[4:6] != ['-DCMAKE_BUILD_TYPE=Release', '-DBOOST_ROOT=/usr'] or
                not args[6].startswith('-DCMAKE_CXX_COMPILER=') or any('Wno-error' in item for item in args)):
            return 64
        if not (source / 'CMakeLists.txt').is_file() or config.get('fail_configure'):
            print('CMake Error: selftest configure failure', file=sys.stderr)
            return 1
        build.mkdir(parents=True)
        (build / 'source.txt').write_text(str(source))
        print('-- Build files have been written to: ' + str(build))
        return 0
    if len(args) == 6 and args[0] == '--build' and args[2:] == ['--target', 'mhgp9_tower_probe', '--parallel', '48']:
        build = pathlib.Path(args[1])
        source = pathlib.Path((build / 'source.txt').read_text())
        if config.get('fail_build'):
            print('error: selftest strict compilation failure (-Werror)', file=sys.stderr)
            return 2
        probe = build / 'mhgp9_tower_probe'
        shutil.copyfile(HERE / 'fake_probe.py', probe)
        probe.chmod(0o755)
        for target, relative in (('mhgp9_tower_probe', 'bench/tower_probe.cpp'),
                                 ('mhgp9_chain', 'src/chain/tower_chain.cpp')):
            depfile = build / 'CMakeFiles' / (target + '.dir') / (relative + '.o.d')
            depfile.parent.mkdir(parents=True, exist_ok=True)
            names = [source / relative, source / 'src/chain/tower_chain.hpp', HERE / 'system_header.hpp']
            depfile.write_text('CMakeFiles/' + target + '.dir/' + relative + '.o: \\\n ' +
                               ' \\\n '.join(map(str, names)) + '\n')
        print('[100%] Built target mhgp9_tower_probe')
        return 0
    return 64


if __name__ == '__main__':
    raise SystemExit(main())
'''

FAKE_GXX = r'''
import sys
if sys.argv[1:] != ['--version']:
    raise SystemExit(64)
print('g++ (selftest fake) 11.4.0')
'''


def refused(function, *args, **kwargs):
    try:
        function(*args, **kwargs)
    except ValueError:
        return True
    return False


def fnv_u32le_host(raw):
    namespace = {'__name__': 'mhgp9_fake_probe'}
    exec(compile(FAKE_PROBE, 'mhgp9_fake_probe', 'exec'), namespace)
    return namespace['fnv_u32le'](raw)


def fake_schema():
    # Formes exigees par le worker ; l'autorite du schema reste la porte CTest
    # qui juge la VRAIE sonde (probe_worker_contract), pas ce faux producteur.
    return dict(ledger=sorted(worker.LEDGER_KEYS), levers=list(worker.LEVER_NAMES))


def rewrite_guard(output, section, **fields):
    path = output / 'guard_evidence.json'
    value = json.loads(path.read_text())
    value[section].update(fields)
    path.write_text(json.dumps(value))


def probe_value(*args, **kwargs):
    namespace = {'__name__': 'mhgp9_fake_probe'}
    exec(compile(FAKE_PROBE, 'mhgp9_fake_probe', 'exec'), namespace)
    kwargs.setdefault('schema', fake_schema())
    return namespace['probe_value'](*args, **kwargs)


def rewrite_preflight_stderr(output):
    """Invalid GNU time report with every hash updated: only its parse refuses it."""
    (output / 'preflight.stderr').write_text('no GNU time report\n')
    digest = worker.sha(output / 'preflight.stderr')
    row = worker.strict_json((output / 'preflight.command.json').read_bytes())
    old = dict(row)
    row['stderr_sha256'] = digest
    (output / 'preflight.command.json').write_text(json.dumps(row, indent=1, sort_keys=True))
    receipt = worker.strict_json((output / 'receipt.json').read_bytes())
    receipt['commands'] = [row if item == old else item for item in receipt['commands']]
    (output / 'receipt.json').write_text(json.dumps(receipt, indent=1, sort_keys=True))


def target(status='TERMINATED', generation=None):
    result = dict(name=worker.TARGET['instance'], zone=worker.TARGET['zone'], status=status,
                  selfLink='https://compute.googleapis.com/compute/v1/projects/' + worker.TARGET['project'] +
                  '/zones/' + worker.TARGET['zone'] + '/instances/' + worker.TARGET['instance'],
                  labels={'project': 'e-hgp'}, machineType='g4-standard-48',
                  scheduling=dict(provisioningModel='SPOT', instanceTerminationAction='STOP',
                                  onHostMaintenance='TERMINATE', automaticRestart=False,
                                  maxRunDuration={'seconds': '3600'}))
    if generation is not None:
        result['lastStartTimestamp'] = generation
    return result


def head_blob(path):
    return subprocess.run(['git', '-C', str(ROOT), 'cat-file', 'blob', 'HEAD:' + path], check=True,
                          capture_output=True).stdout


def protocol_committed():
    full, _ = snapshot.resolve_commit('HEAD')
    entries = snapshot.tree_entries(full, sorted(worker.PROTOCOL_NAMES))
    if set(entries) != set(worker.PROTOCOL_NAMES):
        return False
    blobs = snapshot.read_blobs(entries.values())
    return all(blobs[entries[name]] == (ROOT / name).read_bytes() for name in worker.PROTOCOL_NAMES)


_PACKAGE = {}


def package():
    """Paquet construit une fois depuis HEAD (protocole pris a HEAD s'il y est committe)."""
    if not _PACKAGE:
        directory = Path(tempfile.mkdtemp(prefix='mhgp9-tower-selftest-'))
        atexit.register(shutil.rmtree, directory, True)
        committed = protocol_committed()
        record = snapshot.build('HEAD', directory / 'package', allow_uncommitted_protocol=not committed)
        manifest_path = directory / 'package/source_manifest.json'
        _PACKAGE.update(directory=directory, record=record, committed=committed,
                        archive=directory / 'package/snapshot.tar.gz', manifest_path=manifest_path,
                        manifest=worker.strict_json(manifest_path.read_bytes()))
    return _PACKAGE


def archive_files(path):
    with tarfile.open(path, 'r:*') as archive:
        return {member.name: archive.extractfile(member).read() for member in archive.getmembers() if member.isfile()}


def mutated_archive(destination, extras):
    source = package()['archive'].read_bytes()
    with tarfile.open(fileobj=io.BytesIO(source), mode='r:gz') as old, tarfile.open(destination, 'w:gz') as new:
        for member in old.getmembers():
            new.addfile(member, old.extractfile(member))
        for member, raw in extras:
            new.addfile(member, io.BytesIO(raw) if raw is not None else None)
    return destination


def fake_tools(directory, **config):
    fakebin = directory / 'fakebin'
    fakebin.mkdir()
    base = dict(sha256={scene: data['sha256'] for scene, data in worker.INPUTS.items()},
                fnv={scene: data['fnv'] for scene, data in worker.INPUTS.items()}, schema=fake_schema())
    base.update(config)
    (fakebin / 'config.json').write_text(json.dumps(base))
    (fakebin / 'system_header.hpp').write_text('// selftest system header outside the snapshot\n')
    (directory / 'boost_cpp_int.hpp').write_text('// selftest stand-in for boost/multiprecision/cpp_int.hpp\n')
    shebang = '#!' + sys.executable + ' -B\n'
    probe = 'CONFIG = ' + repr(str(fakebin / 'config.json')) + '\n' + FAKE_PROBE
    for name, text in (('cmake', FAKE_CMAKE), ('g++', FAKE_GXX), ('fake_probe.py', probe)):
        (fakebin / name).write_text(shebang + text)
        (fakebin / name).chmod(0o755)
    return fakebin


@contextmanager
def guest(fakebin, generation, schedule_text):
    """Faux invite : metadonnees G4, arret programme, 48 vCPU, demarrage a la generation."""
    original_load = worker.load_helper

    def load(root):
        module = original_load(root)
        module.metadata = lambda: dict(worker.TARGET, machine='g4-standard-48')
        module.scheduled_text = lambda: schedule_text
        return module
    with ExitStack() as stack:
        stack.enter_context(patch.dict(os.environ, {'PATH': str(fakebin) + os.pathsep + os.environ.get('PATH', '')}))
        stack.enter_context(patch.object(worker, 'load_helper', load))
        stack.enter_context(patch.object(worker, 'BOOST_HEADER', str(fakebin.parent / 'boost_cpp_int.hpp')))
        stack.enter_context(patch.object(worker, 'available_cpus', lambda: list(range(48))))
        stack.enter_context(patch.object(worker, 'boot_epoch', lambda: session.epoch(generation)))
        yield


class FakeCloud:
    """Faux gcloud, ssh et scp : aucune commande cloud ; la VM est un repertoire local."""

    def __init__(self, directory, fakebin, before=None, fail_upload=False):
        now = time.time()
        self.fakebin, self.local, self.remote = fakebin, directory / 'remote_local', None
        self.generation = datetime.fromtimestamp(now - 30, timezone.utc).isoformat()
        self.schedule = 'MODE=poweroff\nUSEC=' + str((int(now) + 60 * int(worker.GUEST_SHUTDOWN_MINUTES)) * 1000000) + '\n'
        self.before = target() if before is None else before
        self.fail_upload = fail_upload
        self.calls, self.stops = [], []

    def commands_class(self):
        cloud = self

        class MockCommands:
            def __init__(self, host, _env):
                self.host, self.rows = host, []

            def run(self, name, argv, **_kwargs):
                return cloud.run(self.host, name, [str(item) for item in argv])
        return MockCommands

    def to_local(self, text):
        return text.replace(self.remote, str(self.local)) if self.remote else text

    def run(self, host, name, argv):
        self.calls.append((name, argv))
        if name in ('guarded_start', 'guarded_stop'):
            need(argv[0] == str(host / (name.split('_')[1] + '_and_verify.sh')), 'pinned guard copy invoked')
        else:
            need(argv[0] == str(NEVER_RUN_GCLOUD), 'only the fake gcloud path is ever named')
        if name == 'before_start':
            return 0, json.dumps(self.before), ''
        if name == 'oslogin_add':
            need('--ttl=70m' in argv and any(item.endswith('.pub') for item in argv), 'public key only, 70 min')
            return 0, '{}', ''
        if name == 'guarded_start':
            minutes = argv[argv.index('--guest-shutdown-minutes') + 1]
            handoff = dict(worker.TARGET, schema='e-hgp.start-handoff.v3', last_start_timestamp=self.generation)
            (host / 'handoff.json').write_text(json.dumps(handoff))
            mark = dict(worker.TARGET, schema='e-hgp.guard-mark.v1', mark='double_guard_verified',
                        generation=self.generation, max_run_seconds='3600', guest_shutdown_minutes=minutes,
                        date_utc=self.generation)
            (host / 'guardmarks/double_guard_verified').write_text(''.join(k + '=' + v + '\n' for k, v in mark.items()))
            expiration = datetime.fromtimestamp(time.time() + 4200, timezone.utc).strftime('%Y-%m-%dT%H:%M:%S.%fZ')
            return 0, '[GARDE SSH] expiration fixe=' + expiration + ', duree restante=4200s.\n', ''
        if name in ('before_upload', 'before_worker', 'before_retrieve'):
            return 0, json.dumps(target('RUNNING', self.generation)), ''
        if name == 'upload':
            if self.fail_upload:
                return 1, '', 'intentional failure after guarded start'
            need(argv[-1] == worker.TARGET['instance'] + ':' + self.remote + '/', 'upload destination')
            for item in argv[8:-1]:
                shutil.copyfile(item, self.local / Path(item).name)
            return 0, '', ''
        if name == 'download':
            need(argv[-2] == worker.TARGET['instance'] + ':' + self.remote + '/capture.tar.gz', 'download source')
            shutil.copyfile(self.local / 'capture.tar.gz', argv[-1])
            return 0, '', ''
        if name == 'guarded_stop':
            need(argv[-2:] == ['--expected-last-start-timestamp', self.generation], 'unversioned/wrong-generation stop')
            self.stops.append(argv[-1])
            return 0, '[SUCCES] TERMINATED\n', ''
        need(argv[1:3] == ['compute', 'ssh'] and argv[-1].startswith('--command='), 'unexpected command ' + name)
        script = argv[-1][len('--command='):]
        if name == 'guest_schedule':
            return 0, self.schedule, ''
        if name == 'remote_mkdir':
            self.remote = script.split('mktemp -d ', 1)[1][:-10] + 'ABCDEFGHIJ'
            self.local.mkdir(mode=0o700)
            return 0, self.remote + '\n', ''
        if name in ('unpack', 'pack_capture'):
            # Le script du controleur est rejoue tel quel sur le distant simule.
            done = subprocess.run(['bash', '-c', self.to_local(script)], capture_output=True, text=True, check=False)
            return done.returncode, done.stdout.replace(str(self.local), self.remote), done.stderr
        if name == 'worker':
            need(script.startswith('exec '), 'worker exec')
            argv_worker = [self.to_local(item) for item in shlex.split(script[len('exec '):])]
            need(argv_worker[:2] == ['python3', str(self.local / 'worker.py')] and
                 worker.sha(argv_worker[1]) == worker.sha(worker.__file__), 'uploaded worker is the pinned worker')
            stream = io.StringIO()
            with guest(self.fakebin, self.generation, self.schedule), redirect_stdout(stream):
                code = worker.main(argv_worker[2:])
            return code, stream.getvalue(), ''
        raise ValueError('unexpected fake cloud command: ' + name)


def session_args(directory, snapshot_path=None):
    private = directory / 'session'
    private.mkdir(mode=0o700)
    key = directory / 'key'
    key.write_text('fixture-key-never-used')
    key.chmod(0o600)
    Path(str(key) + '.pub').write_text('fixture-public-key')
    pkg = package()
    archive = pkg['archive'] if snapshot_path is None else snapshot_path
    return SimpleNamespace(session_dir=private, ssh_key=key, expected_controller_sha256=worker.sha(session.__file__),
                           worker=Path(worker.__file__), worker_sha256=worker.sha(worker.__file__), snapshot=archive,
                           snapshot_sha256=worker.sha(archive), manifest=pkg['manifest_path'],
                           manifest_sha256=worker.sha(pkg['manifest_path']), gcloud=NEVER_RUN_GCLOUD)


def run_scenario(directory, cloud=None, tools=None, patches=()):
    fakebin = fake_tools(directory, **(tools or {}))
    fake = FakeCloud(directory, fakebin, **(cloud or {}))
    args = session_args(directory)
    with ExitStack() as stack:
        stack.enter_context(patch.object(session, 'Commands', fake.commands_class()))
        for owner, name, value in patches:
            stack.enter_context(patch.object(owner, name, value))
        stack.enter_context(redirect_stdout(io.StringIO()))
        code = session.run_session(args)
    host = args.session_dir / 'tower_v9_host'
    return code, worker.strict_json((host / 'receipt.json').read_bytes()), fake, host


def names(fake):
    return [name for name, _ in fake.calls]


def expect_certified_stop(receipt, fake):
    need(receipt['targeted_shutdown_certified'] is True and receipt['generation'] == fake.generation and
         fake.stops == [fake.generation] and names(fake).count('guarded_stop') == 1 and
         names(fake)[-1] == 'guarded_stop', 'exactly one certified targeted stop, last')


class Protocol(unittest.TestCase):
    def test_pins_and_guard_arithmetic(self):
        for name, pin in session.GUARDS.items():
            need(worker.sha(HERE / name) == pin, 'guard pin ' + name)
        need(worker.sha(HERE / 'full_probe_worker_v7.py') == worker.HELPER_SHA, 'worker helper pin')
        need(worker.sha(HERE / 'full_probe_session_v7.py') == session.LEGACY_SHA, 'session helper pin')
        need(session.TARGET == session.legacy.TARGET == worker.TARGET, 'unchanged fixed target')
        text = (HERE / 'start_and_verify.sh').read_text()
        value = {key: int(re.search(r'readonly ' + key + r'=(\d+)', text).group(1)) for key in (
            'TIMESTAMP_TOLERANCE_SECONDS', 'GUEST_SYSTEMD_TOLERANCE_SECONDS', 'GUEST_ARMING_BUDGET_SECONDS')}
        guest, duration = 60 * int(worker.GUEST_SHUTDOWN_MINUTES), int(worker.MAX_RUN_SECONDS)
        need(guest + sum(value.values()) <= duration and guest + 900 <= duration, 'guest guard fits maxRunDuration')
        # 1500 s utiles + 300 s de fermeture + 300 s de certification/transfert.
        need(guest >= worker.USEFUL_BUDGET_SECONDS + 300 + 300, 'useful budget fits before guest poweroff')
        need(60 * 30 < worker.USEFUL_BUDGET_SECONDS + 600, 'the v8 30-minute guard could not hold 1500 s')
        need(worker.MIN_CASE_START_SECONDS < worker.CASE_CAP_SECONDS <= worker.USEFUL_BUDGET_SECONDS, 'case cap')
        helper = worker.load_helper(ROOT)
        now = time.time()
        generation = datetime.fromtimestamp(now - 30, timezone.utc).isoformat()
        mark = dict(worker.TARGET, schema='e-hgp.guard-mark.v1', mark='double_guard_verified', generation=generation,
                    max_run_seconds='3600', guest_shutdown_minutes=worker.GUEST_SHUTDOWN_MINUTES, date_utc=generation)
        schedule = {'MODE': 'poweroff', 'USEC': str((int(now) + guest) * 1000000)}
        guards = helper.guard_values(mark, schedule, worker.TARGET, generation, int(now) + guest, 300, now)
        need(guards['work_deadline_epoch'] - now >= worker.USEFUL_BUDGET_SECONDS + 300, 'pinned v7 guard accepts 40 min')

    def test_inert_without_cloud(self):
        with patch('subprocess.Popen', side_effect=RuntimeError('no subprocess permitted')):
            for module in (session, worker):
                stream = io.StringIO()
                with redirect_stdout(stream):
                    need(module.main([]) == 0, 'inert exit code')
                value = json.loads(stream.getvalue())
                need(value['status'] == 'inert' and value['GPU_executed'] is False and
                     value['FULL_executed'] is False and value['target'] == worker.TARGET, 'inert report')
        need(value['useful_budget_seconds'] == 1500, 'worker default useful budget')

    def test_fnv_and_frames(self):
        def naive(raw):
            h = 14695981039346656037
            def word(item):
                nonlocal h
                for byte in item.to_bytes(8, 'little'):
                    h = ((h ^ byte) * 1099511628211) & ((1 << 64) - 1)
            word(len(raw) // 12)
            for (item,) in struct.iter_unpack('<I', raw):
                word(item)
            return '%016x' % h
        need(worker.INPUTS['00']['fnv'] == '5c785760053d17ce', 'C++ probe hash of scene 00 at d2700314')
        for data in worker.INPUTS.values():
            raw = head_blob(data['source'])
            need(hashlib.sha256(raw).hexdigest() == data['sha256'] and len(raw) == 12 * data['n'] and
                 worker.input_fnv(raw) == naive(raw) == data['fnv'], 'pinned frame ' + data['file'])
        need(worker.input_fnv(b'') == naive(b''), 'empty known answer')
        need(refused(worker.input_fnv, b'\0' * 11), 'incomplete record refused')
        need(refused(worker.input_fnv, struct.pack('<III', 1 << 18, 0, 0)), 'u18 domain refused')

    def test_target_mutations(self):
        value = target()
        session.validate_target(value, 'TERMINATED')
        for field, replacement in [('name', 'other'), ('status', 'RUNNING'), ('zone', 'other'),
                                   ('selfLink', 'wrong-project'), ('machineType', 'n2-standard-48'), ('labels', {})]:
            bad = deepcopy(value)
            bad[field] = replacement
            need(refused(session.validate_target, bad, 'TERMINATED'), 'target mutation ' + field)
        for field, replacement in [('provisioningModel', 'STANDARD'), ('instanceTerminationAction', 'DELETE'),
                                   ('onHostMaintenance', 'MIGRATE'), ('automaticRestart', True),
                                   ('maxRunDuration', {'seconds': '3601'}),
                                   ('maxRunDuration', {'seconds': '3600', 'nanos': 1})]:
            bad = deepcopy(value)
            bad['scheduling'][field] = replacement
            need(refused(session.validate_target, bad, 'TERMINATED'), 'scheduling mutation ' + field)
        need(refused(session.validate_target, target('RUNNING', 'first'), 'RUNNING', 'other'), 'generation mismatch')

    def test_guard_deadline_port(self):
        now = int(time.time())
        generation = datetime.fromtimestamp(now - 30, timezone.utc).isoformat()
        mark = dict(worker.TARGET, schema='e-hgp.guard-mark.v1', mark='double_guard_verified', generation=generation,
                    max_run_seconds='3600', guest_shutdown_minutes='40', date_utc=generation)
        schedule = {'MODE': 'poweroff', 'USEC': str((now + 2400) * 1000000)}
        need(session.guard_deadline(mark, schedule, generation, now) == now + 2400, 'accepted 40-minute guard')
        for field, value in [('max_run_seconds', '7200'), ('guest_shutdown_minutes', '30'),
                             ('guest_shutdown_minutes', '45'), ('generation', 'other'), ('project', 'other'),
                             ('mark', 'guest_guard_pending'), ('schema', 'x')]:
            need(refused(session.guard_deadline, dict(mark, **{field: value}), schedule, generation, now),
                 'guard mark mutation ' + field)
        for bad in ({'MODE': 'reboot', 'USEC': schedule['USEC']}, {'MODE': 'poweroff', 'USEC': str((now + 200) * 10**6)},
                    {'MODE': 'poweroff', 'USEC': str((now + 3400) * 10**6)}):
            need(refused(session.guard_deadline, mark, bad, generation, now), 'guest schedule mutation')
        need(refused(session.closure_generation, generation, generation.replace('+00:00', '+01:00')), 'ambiguity')

    def test_snapshot_from_commit(self):
        pkg = package()
        record, manifest = pkg['record'], pkg['manifest']
        head = subprocess.run(['git', '-C', str(ROOT), 'rev-parse', 'HEAD'], check=True, capture_output=True,
                              text=True).stdout.strip()
        need(record['status'] == 'prepared_not_executed' and record['commit'] == head and
             record['protocol_source'] == ('commit' if pkg['committed'] else 'worktree_uncommitted') and
             record['real_session_allowed'] is pkg['committed'], 'package record')
        cases, provenance = session.validate_snapshot(pkg['archive'], manifest)
        need(provenance['commit'] == head and [(c['scene'], c['k'], c['workers']) for c in cases] == [
            ('00', 5, 48), ('00', 10, 48), ('01', 5, 48), ('01', 10, 48), ('02', 5, 48), ('02', 10, 48),
            ('00', 5, 24), ('00', 5, 1)] and all(
                c['s'] == 8 and c['static_threads'] == (c['workers'] if c['workers'] > 1 else 0) and
                c['levers'] == {name: True for name in worker.LEVER_NAMES} and c['repeat'] == 0
                for c in cases),
               'default plan order and parameters')
        # Temoin independant : git archive du meme commit, jamais le worktree.
        exported = subprocess.run(['git', '-C', str(ROOT), 'archive', '--format=tar', 'HEAD', worker.SOURCE_ROOT],
                                  check=True, capture_output=True).stdout
        with tarfile.open(fileobj=io.BytesIO(exported)) as archive:
            committed = {m.name: hashlib.sha256(archive.extractfile(m).read()).hexdigest()
                         for m in archive.getmembers() if m.isfile() and worker.safe_name(m.name) and
                         not m.name.endswith('/.gitkeep')}
        need(committed == {k: v for k, v in manifest.items() if k.startswith(worker.SOURCE_ROOT + '/')},
             'v9 sources are exactly the committed objects')
        need(worker.PROBE_SOURCE in committed and not any('/docs/' in k or '/receipts/' in k for k in committed),
             'source perimeter')
        files = archive_files(pkg['archive'])
        for data in worker.INPUTS.values():
            need(files[data['file']] == head_blob(data['source']), 'frame copied from the commit')
        need(files[worker.HELPER] == head_blob(worker.HELPER), 'helper copied from the commit')
        again = snapshot.build('HEAD', pkg['directory'] / 'again', allow_uncommitted_protocol=not pkg['committed'])
        need(again['snapshot_sha256'] == record['snapshot_sha256'] and
             again['manifest_sha256'] == record['manifest_sha256'], 'deterministic snapshot')
        strict = pkg['directory'] / 'strict'
        if pkg['committed']:
            need(snapshot.build('HEAD', strict)['protocol_source'] == 'commit', 'committed protocol')
        else:
            need(refused(snapshot.build, 'HEAD', strict) and not strict.exists(), 'uncommitted protocol refused')
        need(refused(snapshot.resolve_commit, '--upload-pack=x'), 'option-like commit refused')

    def test_manifest_plan_and_data_refusals(self):
        pkg = package()
        manifest, files = pkg['manifest'], archive_files(pkg['archive'])
        plan = worker.strict_json(files[worker.PLAN])
        worker.validate_plan(plan, manifest)
        for key, value in [('k', 7), ('k', True), ('s', 9), ('s', 6), ('workers', 0), ('workers', 1025),
                           ('static_threads', -1), ('repeat', -1), ('n', 39884), ('n', True), ('scene', '03'),
                           ('levers', {}), ('levers', None),
                           ('levers', dict({name: True for name in worker.LEVER_NAMES}, extra=True)),
                           ('levers', dict({name: True for name in worker.LEVER_NAMES}, q34_dead_lanes=1)),
                           ('levers', dict({name: True for name in worker.LEVER_NAMES}, q34_dead_lanes=False)),
                           ('scene', '../00'), ('file', 'data/scene_01.u32le'), ('extra', 1)]:
            bad = deepcopy(plan)
            bad['cases'][0][key] = value
            need(refused(worker.validate_plan, bad, manifest), 'plan mutation ' + key)
        bad = deepcopy(plan)
        del bad['cases'][0]['repeat']
        need(refused(worker.validate_plan, bad, manifest), 'missing case field')
        bad = deepcopy(plan)
        bad['cases'].append(deepcopy(bad['cases'][0]))
        need(refused(worker.validate_plan, bad, manifest), 'duplicate case')
        bad['cases'][-1]['repeat'] = 1
        worker.validate_plan(bad, manifest)   # distinct repetition accepted
        # Ablation plans: ON first (preflight levers), then OFF, accepted;
        # OFF first is refused since the preflight would skip the ON path.
        off = dict(plan['cases'][0], levers=dict(plan['cases'][0]['levers'], q34_dead_core=False), repeat=7)
        worker.validate_plan(dict(plan, cases=[plan['cases'][0], off]), manifest)
        need(refused(worker.validate_plan, dict(plan, cases=[off, plan['cases'][0]]), manifest),
             'OFF-first plan would preflight without every lever')
        for bad in (dict(plan, schema='mhgp8_q34_spatial_plan_v1'), dict(plan, schema='mhgp9_tower_plan_v5'),
                    dict(plan, cases=[]),
                    dict(plan, cases=[dict(plan['cases'][0], repeat=i) for i in range(65)]), dict(plan, extra=1)):
            need(refused(worker.validate_plan, bad, manifest), 'plan envelope')
        for mutate in (lambda m: m.pop(worker.PROBE_SOURCE), lambda m: m.pop('data/scene_02.u32le'),
                       lambda m: m.pop(worker.PROVENANCE), lambda m: m.update({'morsehgp3D_v9/docs/PLAN_V9.md': '0' * 64}),
                       lambda m: m.update({'data/../x': '0' * 64}), lambda m: m.update({worker.HELPER: '0' * 64}),
                       lambda m: m.update({'data/scene_00.u32le': '1' * 64}),
                       lambda m: m.update({worker.CMAKE_LISTS: 'XYZ'})):
            bad = dict(manifest)
            mutate(bad)
            need(refused(worker.validate_manifest, bad), 'manifest mutation')
        read = files.__getitem__
        worker.validate_data(read)
        for name in ('data/scene_00.u32le', 'data/scene_01.u32le'):
            corrupt = dict(files)
            corrupt[name] = bytes([corrupt[name][0] ^ 1]) + corrupt[name][1:]
            need(refused(worker.validate_data, corrupt.__getitem__), 'corrupted frame')
            corrupt[name] = files[name][:-12]
            need(refused(worker.validate_data, corrupt.__getitem__), 'prefix frame')
        worker.validate_sources(read)
        for name, old, new in ((worker.CMAKE_LISTS, b'mhgp9_tower_probe bench/tower_probe.cpp', b'other bench/x.cpp'),
                               (worker.PROBE_SOURCE, b'"--static="', b'"--threads="')):
            changed = dict(files, **{name: files[name].replace(old, new)})
            need(refused(worker.validate_sources, changed.__getitem__), 'source interface ' + name)
        provenance = worker.strict_json(files[worker.PROVENANCE])
        worker.validate_provenance(provenance, manifest)
        for key, value in (('protocol_source', 'dirty'), ('commit', 'HEAD'), ('inputs', {}), ('helper', {}),
                           ('schema', 'x')):
            need(refused(worker.validate_provenance, dict(provenance, **{key: value}), manifest), 'provenance ' + key)

    def test_archive_refusals(self):
        pkg = package()
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            cases = []
            for name, kind in [('morsehgp3D_v9/src/extra.hpp', tarfile.REGTYPE), ('data/extra.bin', tarfile.REGTYPE),
                               ('data/../bad', tarfile.REGTYPE),
                               ('data/link', tarfile.SYMTYPE), ('/data/bad', tarfile.REGTYPE),
                               ('morsehgp3D_v8/x', tarfile.REGTYPE), ('morsehgp3D_v9/docs/PLAN_V9.md', tarfile.REGTYPE),
                               ('gcp-migration/other.py', tarfile.REGTYPE), (worker.PLAN, tarfile.REGTYPE),
                               ('morsehgp3D_v9/src/fifo', tarfile.FIFOTYPE)]:
                member = tarfile.TarInfo(name)
                member.type = kind
                raw = b'x' if kind == tarfile.REGTYPE else None
                member.size = len(raw) if raw else 0
                cases.append((name, mutated_archive(directory / (str(len(cases)) + '.tar.gz'), [(member, raw)])))
            for name, path in cases:
                need(refused(session.validate_snapshot, path, pkg['manifest']), 'archive member refused: ' + name)
            # Un fichier manifeste dont le contenu differe.
            files = archive_files(pkg['archive'])
            files[worker.PLAN] = worker.canonical_json(dict(worker.strict_json(files[worker.PLAN]), cases=[
                worker.strict_json(files[worker.PLAN])['cases'][0]]))
            snapshot.write_archive(directory / 'changed.tar.gz', files)
            need(refused(session.validate_snapshot, directory / 'changed.tar.gz', pkg['manifest']), 'changed payload')

    def test_probe_and_time_validation(self):
        plan_case = snapshot.default_plan()['cases'][0]
        data = worker.INPUTS['00']
        good = probe_value(data['n'], data['fnv'], 5, 8, 48, 48)
        need(worker.validate_probe(worker.strict_json(json.dumps(good)), plan_case, 0) == 'complete_relative', 'valid')
        worker.validate_external_wall(good, 0.0015)
        need(refused(worker.validate_external_wall, good, 0.0015 - worker.EXTERNAL_WALL_TOLERANCE_SECONDS - 0.001),
             'wall tolerance is not a free second')
        pre_raw = worker.preflight_cloud()
        need(fnv_u32le_host(pre_raw) == worker.input_fnv(pre_raw), 'fake FNV matches the worker FNV')
        need(refused(worker.validate_external_wall, good, -1.0) and
             refused(worker.validate_external_wall, dict(good, times_ms=dict(good['times_ms'], chain_total=9000.0)),
                     5.0), 'chain total bounded by the external wall')
        mutations = [('schema', lambda v: v.update(schema='mhgp9_tower_probe_v0')),
                     ('schema_v10', lambda v: v.update(schema='mhgp9_tower_probe_v10')),
                     ('schema_v11', lambda v: v.update(schema='mhgp9_tower_probe_v11')),
                     ('status', lambda v: v.update(status='complete')),
                     ('hash', lambda v: v['input'].update(hash='0' * 16)),
                     ('sites', lambda v: v['input'].update(sites=data['n'] - 1)),
                     ('format', lambda v: v['input'].update(format='u16le')),
                     ('grid', lambda v: v['input'].update(grid='unspecified')),
                     ('K', lambda v: v['options'].update(K=10)), ('workers', lambda v: v['options'].update(workers=24)),
                     ('static', lambda v: v['options'].update(tower_static_threads=1)),
                     ('saturate_mode', lambda v: v['options']['levers'].update(atlas_saturate_deep=False)),
                     ('leaf_mode', lambda v: v['options']['levers'].update(q3_leaf_census=False)),
                     ('leaf_mode_type', lambda v: v['options']['levers'].update(q3_leaf_census=1)),
                     ('leaf_mode_absent', lambda v: v['options']['levers'].pop('q3_leaf_census')),
                     ('dead_mode', lambda v: v['options']['levers'].update(q34_dead_lanes=False)),
                     ('cache_mode', lambda v: v['options']['levers'].update(q34_witness_cache=False)),
                     ('core_mode', lambda v: v['options']['levers'].update(q34_dead_core=False)),
                     ('meb_mode', lambda v: v['options']['levers'].update(tower_meb_proposal=False)),
                     ('meb_verified_beyond', lambda v: v['tower_work'].update(meb_verified_proposals=4)),
                     ('meb_fallbacks_beyond', lambda v: v['tower_work'].update(meb_proposal_fallbacks=4)),
                     ('meb_canonical_beyond', lambda v: v['tower_work'].update(meb_boundary_canonicalizations=3)),
                     ('meb_proposal_unaccounted', lambda v: v['tower_work'].update(meb_proposals=4)),
                     ('meb_reference_label', lambda v: v['tower_work'].update(
                         meb_accounting='anchor_meb_first_maximal_pair_then_lexicographic_supports_extremes_first_v2')),
                     ('lever_unknown', lambda v: v['options']['levers'].update(extra=True)),
                     ('meb_accounting', lambda v: v['tower_work'].update(meb_accounting='other')),
                     ('meb_accounting_absent', lambda v: v['tower_work'].pop('meb_accounting')),
                     ('meb_sizes_type', lambda v: v['tower_work'].update(meb_supports_by_size='4,3')),
                     ('meb_sizes_negative', lambda v: v['tower_work'].update(meb_supports_by_size=[0, -1])),
                     ('meb_sizes_empty', lambda v: v['tower_work'].update(meb_supports_by_size=[])),
                     ('meb_sizes_long', lambda v: v['tower_work'].update(meb_supports_by_size=[0] * 9)),
                     ('meb_sizes_bool', lambda v: v['tower_work'].update(meb_supports_by_size=[True])),
                     ('tower_work_text', lambda v: v['tower_work'].update(records='4')),
                     ('tower_work_list', lambda v: v['tower_work'].update(births=[3])),
                     ('stage_sum', lambda v: v['times_ms'].update(q34=5.0)),
                     ('meb_sizes_short', lambda v: v['tower_work'].update(meb_supports_by_size=[0])),
                     ('tower_work_unknown', lambda v: v['tower_work'].update(extra=1)),
                     ('tower_work_missing', lambda v: v['tower_work'].pop('records')),
                     ('generator_missing', lambda v: v['generator'].pop('q34_expanded_pairs')),
                     ('ledger_missing', lambda v: v['ledger'].pop('q3_seeds')),
                     ('ledger_unknown', lambda v: v['ledger'].update(extra=1)),
                     ('by_qmin_empty', lambda v: v['catalogue'].update(by_qmin=[])),
                     ('by_shell_empty', lambda v: v['catalogue'].update(by_shell=[])),
                     ('catalogue_unknown', lambda v: v['catalogue'].update(extra=1)),
                     ('dead_loads_zero', lambda v: v['ledger'].update(dead_loads=0)),
                     ('dead_form_sites_zero', lambda v: v['ledger'].update(dead_form_sites=0)),
                     ('cover_builds_zero', lambda v: v['ledger'].update(cover_builds=0)),
                     ('expanded_pairs_zero', lambda v: v['generator'].update(q34_expanded_pairs=0)),
                     ('q3_presentations_zero', lambda v: v['catalogue'].update(q3_presentations=0)),
                     ('dead_q3_open_shifted', lambda v: v['ledger'].update(dead_q3_open=2)),
                     ('core_closed_shifted', lambda v: v['ledger'].update(core_closed_edges=2)),
                     ('rect_queries_shifted', lambda v: v['ledger'].update(witness_rect_queries=8)),
                     ('q3_seed_visits_split', lambda v: v['ledger'].update(q3_seed_node_visits=6)),
                     ('q4_domain_beyond_bound', lambda v: v['ledger'].update(q4_domain_node_visits=10 ** 9)),
                     ('pair_queries_unaccounted', lambda v: v['ledger'].update(witness_pair_queries=0)),
                     ('core_loads_shifted', lambda v: v['ledger'].update(dead_core_loads=4)),
                     ('cache_queries_zero', lambda v: v['ledger'].update(witness_cache_queries=0)),
                     ('cache_node_tests_zero', lambda v: v['ledger'].update(witness_cache_node_tests=0)),
                     ('both_edges_beyond_q3', lambda v: v['ledger'].update(both_edges=2)),
                     ('dead_q3_proved_beyond_covers', lambda v: v['ledger'].update(dead_q3_proved=3)),
                     ('dead_cells_classes', lambda v: v['ledger'].update(dead_cells=2)),
                     ('core_cover_visits', lambda v: v['ledger'].update(core_cover_node_visits=5)),
                     ('core_closed_unproved', lambda v: v['ledger'].update(dead_core_q3_proved=0, dead_core_q4_proved=0)),
                     ('shell_over_12', lambda v: v['catalogue'].update(shell_over_12=1)),
                     ('max_shell_13', lambda v: v['catalogue'].update(max_shell=13)),
                     ('by_shell_2_to_16', lambda v: v['catalogue'].update(by_shell=[0, 0, 0, 2, 1] + [0] * 11 + [1])),
                     ('core_form_sites_shifted', lambda v: v['ledger'].update(dead_core_form_sites=8)),
                     ('core_q4_open_shifted', lambda v: v['ledger'].update(dead_core_q4_open=1)),
                     ('cache_rejections_excess', lambda v: v['ledger'].update(witness_cache_rejected_pairs=4)),
                     ('run_tower', lambda v: v['options'].update(run_tower=False)),
                     ('K_effective', lambda v: v['options'].update(K_effective=4)),
                     ('orders', lambda v: v['orders'].pop()), ('order_key', lambda v: v['orders'][0].update(extra=1)),
                     ('time', lambda v: v['times_ms'].update(q34=-1.0)),
                     ('time_key', lambda v: v['times_ms'].pop('q34')),
                     ('digest_time_absent', lambda v: v['times_ms'].pop('digest')),
                     ('digest_time_negative', lambda v: v['times_ms'].update(digest=-1.0)),
                     ('counter_bool', lambda v: v['generator'].update(q3_emitted=True)),
                     ('counter_negative', lambda v: v['catalogue'].update(balls=-1)),
                     ('shell_list', lambda v: v['catalogue'].update(by_shell=[1, -2])),
                     ('digest', lambda v: v.update(tower_digest='ABCDEF0123456789')),
                     ('digest_len', lambda v: v.update(tower_digest='0' * 15)), ('extra', lambda v: v.update(extra=1))]
        for label, mutate in mutations:
            bad = deepcopy(good)
            mutate(bad)
            need(refused(worker.validate_probe, bad, plan_case, 0), 'probe mutation ' + label)
        need(refused(worker.validate_probe, good, plan_case, 3), 'complete status with code 3')
        need(refused(worker.strict_json, '{"a": NaN}') and refused(worker.strict_json, '{"a": 1, "a": 2}'), 'strict JSON')
        refusal = probe_value(data['n'], data['fnv'], 5, 8, 48, 48, status='unsupported_degeneracy')
        need(worker.validate_probe(refusal, plan_case, 3) == 'explicit_refusal', 'explicit refusal')
        need(refused(worker.validate_probe, refusal, plan_case, 0), 'refusal with code 0')
        report = ('\tCommand being timed: "probe"\n\tElapsed (wall clock) time (h:mm:ss or m:ss): 0:01.00\n'
                  '\tMaximum resident set size (kbytes): 1024\n\tExit status: 0\n')
        need(worker.validate_gnu_time(report, 0) == 1024, 'GNU time report')
        need(refused(worker.validate_gnu_time, report, 3) and
             refused(worker.validate_gnu_time, report.replace('Maximum resident', 'Max'), 0), 'GNU time mutations')
        other = deepcopy(good)
        other['times_ms']['q34'] = 99.0
        other['options']['workers'] = 1
        other['tower_work']['meb_calls'] = 77
        outcomes = [dict(outcome='complete_relative'), dict(outcome='complete_relative')]
        cases = [plan_case, dict(plan_case, workers=1)]
        need(worker.compare_cases(cases, outcomes, {0: good, 1: other}) == [dict(reference=0, other=1, equal=True)],
             'costs are not compared')
        other['tower_digest'] = '0' * 16
        need(worker.compare_cases(cases, outcomes, {0: good, 1: other})[0]['equal'] is False, 'digest compared')

    def test_nominal_session_completed(self):
        with tempfile.TemporaryDirectory() as temporary:
            code, receipt, fake, host = run_scenario(Path(temporary))
            need(code == 0 and receipt['status'] == 'completed' and receipt['worker_status'] == 'completed' and
                 receipt['FULL_executed'] is True and receipt['GPU_executed'] is False and
                 receipt['public_status'] == 'not_claimed' and receipt['guest_shutdown_minutes'] == 40 and
                 receipt['useful_budget_seconds'] == 1500, 'nominal host receipt: ' + json.dumps(receipt)[:600])
            expect_certified_stop(receipt, fake)
            order = names(fake)
            need(order[:3] == ['before_start', 'oslogin_add', 'guarded_start'] and
                 order.index('before_upload') < order.index('upload') < order.index('unpack') <
                 order.index('before_worker') < order.index('worker') < order.index('before_retrieve') <
                 order.index('pack_capture') < order.index('download') < order.index('guarded_stop'),
                 'guarded lifecycle order')
            worker_call = dict(fake.calls)['worker'][-1]
            need('--useful-budget-seconds 1500' in worker_call and '--case-cap-seconds 600' in worker_call and
                 '--execute' in worker_call, 'worker budget arguments')
            output = host / 'received/output'
            value = worker.strict_json((output / 'receipt.json').read_bytes())
            need(value['completed_case_indices'] == list(range(8)) and value['cross_worker_comparisons'] == [
                dict(reference=0, other=6, equal=True), dict(reference=0, other=7, equal=True)] and
                 value['FULL_executed'] is True and value['provenance'] == receipt['provenance'], 'worker receipt')
            need(not (output / 'build').exists() and (output / 'configure.stdout').is_file(), 'capture excludes build')
            pkg = package()
            expected = session.validate_snapshot(pkg['archive'], pkg['manifest'])[0]
            pin = worker.sha(worker.__file__)
            bound = (receipt['generation'], receipt['provenance'], receipt['verified_guard'])
            need(session.validate_received(output, pkg['manifest'], pin, expected, *bound) == 'completed',
                 'revalidation bound to the session')
            need(refused(session.validate_received, output, pkg['manifest'], pin, expected, '', *bound[1:]) and
                 refused(session.validate_received, output, pkg['manifest'], pin, expected, bound[0], {}, bound[2]) and
                 refused(session.validate_received, output, pkg['manifest'], pin, expected, *bound[:2], {}) and
                 refused(session.validate_received, output, pkg['manifest'], pin, expected, *bound[:2],
                         dict(bound[2], schedule=dict(bound[2]['schedule'], USEC=str(
                             (session.epoch(receipt['generation']) + 600) * 1000000)))),
                 'reception without the session generation, provenance or host-verified guard')
            need(refused(session.validate_received, output, pkg['manifest'], pin, expected, 'another-generation',
                         receipt['provenance'], receipt['verified_guard']) and
                 refused(session.validate_received, output, pkg['manifest'], pin, expected, receipt['generation'],
                         dict(receipt['provenance'], commit='0' * 40), receipt['verified_guard']),
                 'receipt bound to generation and provenance')
            tampered = Path(temporary) / 'tampered'
            for label, mutate in (
                    ('probe stdout', lambda o: (o / 'probe_0.stdout').write_bytes(
                        (o / 'probe_0.stdout').read_bytes().replace(b'"chain_cpu_s":0.25', b'"chain_cpu_s":0.5'))),
                    ('missing command', lambda o: (o / 'uptime_after_3.command.json').unlink()),
                    ('status', lambda o: (o / 'receipt.json').write_bytes((o / 'receipt.json').read_bytes().replace(
                        b'"status": "completed"', b'"status": "partial"'))),
                    ('sources', lambda o: (o / 'sources_after.json').write_text('{}')),
                    ('guard evidence', lambda o: (o / 'guard_evidence.json').write_text('{}')),
                    ('preflight stderr', rewrite_preflight_stderr),
                    ('receipt target', lambda o: (o / 'receipt.json').write_bytes((o / 'receipt.json').read_bytes().replace(
                        worker.TARGET['instance'].encode(), b'another-instance'))),
                    ('guard mark instance', lambda o: rewrite_guard(o, 'mark', instance='another-instance')),
                    ('guard mark zone', lambda o: rewrite_guard(o, 'mark', zone='europe-west4-a')),
                    ('guard mark schema', lambda o: rewrite_guard(o, 'mark', schema='e-hgp.guard-mark.v0')),
                    ('guard mark before generation', lambda o: rewrite_guard(o, 'mark', date_utc='2000-01-01T00:00:00Z')),
                    ('guest schedule mode', lambda o: rewrite_guard(o, 'schedule', MODE='reboot')),
                    ('guest schedule text', lambda o: rewrite_guard(o, 'schedule', USEC='soon')),
                    ('guest schedule too late', lambda o: rewrite_guard(o, 'schedule', USEC=str(
                        (session.epoch(receipt['generation']) + 4000) * 1000000))),
                    ('guest schedule before start', lambda o: rewrite_guard(o, 'schedule', USEC=str(
                        (session.epoch(receipt['generation']) - 10) * 1000000))),
                    ('guard mark in the future', lambda o: rewrite_guard(o, 'mark', date_utc='2099-01-01T00:00:00Z')),
                    ('guest schedule plausible but not verified', lambda o: rewrite_guard(o, 'schedule', USEC=str(
                        (session.epoch(receipt['generation']) + 600) * 1000000)))):
                shutil.copytree(output, tampered)
                mutate(tampered)
                need(refused(session.validate_received, tampered, pkg['manifest'], pin, expected, *bound),
                     'tamper ' + label)
                shutil.rmtree(tampered)
            need(refused(session.validate_received, output, pkg['manifest'], '0' * 64, expected, *bound), 'worker pin')

    def test_partial_session_case_cap_and_refusal(self):
        with tempfile.TemporaryDirectory() as temporary:
            code, receipt, fake, host = run_scenario(
                Path(temporary), tools=dict(sleep=[dict(workers=1, seconds=60)], refuse=[dict(scene='02', k=10)]),
                patches=[(worker, 'CASE_CAP_SECONDS', 4)])
            need(code == 0 and receipt['status'] == 'partial', 'partial host receipt: ' + json.dumps(receipt)[:600])
            expect_certified_stop(receipt, fake)
            value = worker.strict_json((host / 'received/output/receipt.json').read_bytes())
            outcomes = [entry['outcome'] for entry in value['case_outcomes']]
            need(outcomes == ['complete_relative'] * 5 + ['explicit_refusal', 'complete_relative', 'killed_case_cap'],
                 'cap kill and explicit refusal: ' + repr(outcomes))
            killed = worker.strict_json((host / 'received/output/probe_7.command.json').read_bytes())
            need(killed['residual_or_interrupted_group_killed'] is True and 3.0 <= killed['elapsed_seconds'] < 30,
                 'probe group killed at its cap')
            # The killed case's summary is part of the receipt: removing or
            # altering it is refused (same cap as the session, never reread GCP).
            pkg = package()
            expected = session.validate_snapshot(pkg['archive'], pkg['manifest'])[0]
            pin = worker.sha(worker.__file__)
            bound = (receipt['generation'], receipt['provenance'], receipt['verified_guard'])
            output = host / 'received/output'
            with patch.object(worker, 'CASE_CAP_SECONDS', 4):
                need(session.validate_received(output, pkg['manifest'], pin, expected, *bound) == 'partial',
                     'partial revalidation')
                for label, mutate in (
                        ('killed summary removed', lambda o: (o / 'probe_7.summary.json').unlink()),
                        ('killed summary elapsed', lambda o: (o / 'probe_7.summary.json').write_text(
                            (o / 'probe_7.summary.json').read_text().replace('"elapsed_seconds": ',
                                                                             '"elapsed_seconds": 1')))):
                    tampered = Path(temporary) / 'tampered'
                    shutil.copytree(output, tampered)
                    mutate(tampered)
                    need(refused(session.validate_received, tampered, pkg['manifest'], pin, expected, *bound),
                         'tamper ' + label)
                    shutil.rmtree(tampered)

    def test_budget_exhaustion_skips_following_cases(self):
        with tempfile.TemporaryDirectory() as temporary:
            code, receipt, fake, host = run_scenario(
                Path(temporary), tools=dict(sleep=[dict(workers=48, k=10, scene='00', seconds=120)]),
                patches=[(worker, 'USEFUL_BUDGET_SECONDS', 15), (worker, 'MIN_CASE_START_SECONDS', 1)])
            need(code == 0 and receipt['status'] == 'partial', 'budget host receipt: ' + json.dumps(receipt)[:600])
            expect_certified_stop(receipt, fake)
            output = host / 'received/output'
            value = worker.strict_json((output / 'receipt.json').read_bytes())
            outcomes = [entry['outcome'] for entry in value['case_outcomes']]
            need(outcomes == ['complete_relative', 'killed_budget'] + ['skipped_budget'] * 6,
                 'budget exhaustion: ' + repr(outcomes))
            need(not any((output / ('probe_' + str(i) + '.command.json')).exists() for i in range(2, 8)),
                 'skipped cases never launched')

    def test_protocol_defect_stops_the_campaign(self):
        with tempfile.TemporaryDirectory() as temporary:
            code, receipt, fake, host = run_scenario(Path(temporary), tools=dict(malform=[dict(scene='00', k=10)]))
            need(code == 1 and receipt['status'] == 'worker_failed' and receipt['worker_status'] == 'probe_failed',
                 'protocol defect host receipt: ' + json.dumps(receipt)[:600])
            expect_certified_stop(receipt, fake)
            output = host / 'received/output'
            value = worker.strict_json((output / 'receipt.json').read_bytes())
            outcomes = [entry['outcome'] for entry in value['case_outcomes']]
            need(outcomes == ['complete_relative', 'probe_failed'] + ['skipped_protocol_defect'] * 6,
                 'protocol defect skips the following cases: ' + repr(outcomes))
            need('probe counters tower_work' in value['case_outcomes'][1]['reason'] and
                 not any((output / ('probe_' + str(i) + '.command.json')).exists() for i in range(2, 8)),
                 'skipped cases never launched after a protocol defect')

    def test_preflight_failure_runs_no_case(self):
        with tempfile.TemporaryDirectory() as temporary:
            code, receipt, fake, host = run_scenario(Path(temporary), tools=dict(fail_preflight=True))
            need(code == 1 and receipt['status'] == 'worker_failed' and receipt['worker_status'] == 'preflight_failed',
                 'preflight failure host receipt: ' + json.dumps(receipt)[:600])
            expect_certified_stop(receipt, fake)
            output = host / 'received/output'
            need((output / 'preflight.command.json').is_file() and
                 not (output / 'probe_0.command.json').exists(), 'no LiDAR case after a failed native preflight')

    def test_vacuous_preflight_runs_no_case(self):
        with tempfile.TemporaryDirectory() as temporary:
            code, receipt, fake, host = run_scenario(Path(temporary), tools=dict(vacuous_preflight=True))
            need(code == 1 and receipt['worker_status'] == 'preflight_failed',
                 'vacuous preflight host receipt: ' + json.dumps(receipt)[:600])
            expect_certified_stop(receipt, fake)
            need(not (host / 'received/output/probe_0.command.json').exists(), 'no case after a vacuous preflight')

    def test_zero_complete_campaign_is_refused(self):
        with tempfile.TemporaryDirectory() as temporary:
            refuse = [dict(scene=scene, k=k) for scene in ('00', '01', '02') for k in (5, 10)]
            code, receipt, fake, host = run_scenario(Path(temporary), tools=dict(refuse=refuse))
            need(code == 1 and receipt['status'] != 'completed' and receipt['status'] != 'partial',
                 'zero-complete campaign must not be a success: ' + json.dumps(receipt)[:600])
            expect_certified_stop(receipt, fake)

    def test_worker_build_failure_still_stops(self):
        with tempfile.TemporaryDirectory() as temporary:
            code, receipt, fake, host = run_scenario(Path(temporary), tools=dict(fail_build=True))
            need(code == 1 and receipt['status'] == 'worker_failed' and receipt['worker_exit_code'] == 1 and
                 receipt['worker_status'] == 'build_failed' and receipt['capture_received'] is True and
                 receipt['FULL_executed'] is False, 'build failure host receipt: ' + json.dumps(receipt)[:600])
            expect_certified_stop(receipt, fake)
            output = host / 'received/output'
            need(b'-Werror' in (output / 'build.stderr').read_bytes() and
                 not (output / 'probe_0.command.json').exists(), 'build logs kept; no case after a failed build')

    def test_cross_worker_mismatch_is_a_failure(self):
        with tempfile.TemporaryDirectory() as temporary:
            code, receipt, fake, _ = run_scenario(Path(temporary), tools=dict(salt_by_workers=True))
            need(code == 1 and receipt['status'] == 'worker_failed' and
                 receipt['worker_status'] == 'cross_worker_mismatch', 'mismatch host receipt')
            expect_certified_stop(receipt, fake)

    def test_upload_failure_stops_exact_generation(self):
        with tempfile.TemporaryDirectory() as temporary:
            code, receipt, fake, _ = run_scenario(Path(temporary), cloud=dict(fail_upload=True))
            need(code == 1 and receipt['status'] == 'failed' and 'upload failed' in receipt['error'] and
                 'worker' not in names(fake), 'upload failure host receipt')
            expect_certified_stop(receipt, fake)

    def test_misconfigured_target_never_starts(self):
        for field, value in (('maxRunDuration', {'seconds': '7200'}), ('provisioningModel', 'STANDARD')):
            bad = target()
            bad['scheduling'][field] = value
            with tempfile.TemporaryDirectory() as temporary:
                code, receipt, fake, _ = run_scenario(Path(temporary), cloud=dict(before=bad))
                need(code == 1 and receipt['status'] == 'failed' and names(fake) == ['before_start'] and
                     receipt['targeted_shutdown_certified'] is False and receipt['no_start_lifecycle_created'] is True,
                     'misconfigured target refused before OS Login/start: ' + field)

    def test_real_execute_refuses_uncommitted_protocol(self):
        pkg = package()
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            for source in ('worktree_uncommitted', 'commit'):
                files = archive_files(pkg['archive'])
                files[worker.PROVENANCE] = worker.canonical_json(dict(worker.strict_json(files[worker.PROVENANCE]),
                                                                      protocol_source=source))
                path = directory / (source + '.tar.gz')
                snapshot.write_archive(path, files)
                if source == 'commit':
                    # Recertification Git : accepte seulement si le paquet se
                    # reconstruit depuis le commit declare (protocole commite).
                    if pkg['committed']:
                        session.require_committed_protocol(path, worker.sha(path))
                    else:
                        need(refused(session.require_committed_protocol, path, worker.sha(path)),
                             'forged commit provenance of an uncommitted protocol')
                    need(refused(session.require_committed_protocol, path, '0' * 64), 'snapshot pin')
                    parent = subprocess.run(['git', '-C', str(ROOT), 'rev-parse', 'HEAD~1'], check=True,
                                            capture_output=True, text=True).stdout.strip()
                    forged = dict(files)
                    forged[worker.PROVENANCE] = worker.canonical_json(dict(
                        worker.strict_json(files[worker.PROVENANCE]), commit=parent))
                    forged_path = directory / 'forged_commit.tar.gz'
                    snapshot.write_archive(forged_path, forged)
                    need(refused(session.require_committed_protocol, forged_path, worker.sha(forged_path)),
                         'archive claiming another commit is refused')
                    continue
                args = session_args(directory, path)
                argv = ['--execute', '--snapshot', str(path), '--snapshot-sha256', worker.sha(path),
                        '--manifest', str(args.manifest), '--manifest-sha256', args.manifest_sha256,
                        '--worker', str(args.worker), '--worker-sha256', args.worker_sha256,
                        '--session-dir', str(args.session_dir), '--ssh-key', str(args.ssh_key),
                        '--expected-controller-sha256', args.expected_controller_sha256, '--gcloud', str(NEVER_RUN_GCLOUD)]
                stream = io.StringIO()
                with patch('subprocess.Popen', side_effect=RuntimeError('no subprocess permitted')), \
                        patch.object(session, 'Commands', side_effect=RuntimeError('no command permitted')), \
                        redirect_stdout(stream):
                    code = session.main(argv)
                need(code == 2 and json.loads(stream.getvalue())['status'] == 'refused' and
                     not (args.session_dir / 'tower_v9_host').exists(), 'real session refused before any command')


if __name__ == '__main__':
    unittest.main()
