#!/usr/bin/env python3
"""Session G4 SPOT gardee pour la tour FULL v9 (reference_cpu). Inerte sans --execute.

Port explicite de q34_spatial_session_v8.py (SHA d830651eb34f2b97...,
commit 70de84f2) ; aucun fichier v7/v8 n'est modifie. Comme
en v8, seules les primitives epinglees de full_probe_session_v7.py (SHA
177b25a0...) sont reutilisees : lecture de generation, collecte de
processus, extraction de capture, fermeture. Son run_session, sa validation
de paquet v7 et son producteur FULL ne sont jamais appeles.

Conserve a l'identique : cible unique (projet, zone, instance G4 existante),
validation TERMINATED/SPOT/STOP/3600 s/g4-standard-48/label project=e-hgp,
inscription OS Login de la seule cle publique ephemere (70 min), demarrage
par start_and_verify.sh (double coupe-circuit), recertification avant
televersement, worker et recuperation, capture tar hachee, et ARRET CIBLE
CERTIFIE par stop_and_verify.sh --expected-last-start-timestamp dans le
finally, meme apres echec, interruption ou recuperation impossible.

Change pour la v9 : paquet construit depuis un commit (tower_snapshot_v9.py),
arret invite 40 min au lieu de 30 (justification dans tower_worker_v9.py,
constante GUEST_SHUTDOWN_MINUTES), budget utile 1500 s et plafond par cas,
reception adaptee au schema mhgp9_tower_probe_v3. GPU jamais execute ;
public_status=not_claimed.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import secrets
import shlex
import shutil
import signal
import sys
import tarfile
import time
import types
import tower_worker_v9 as payload

HERE = Path(__file__).resolve().parent
TARGET = dict(payload.TARGET)
LEGACY_SHA = '177b25a0d72150dc331661fdf8da1ccde77ea17fb694d9c6af5b0929755160d8'
raw = (HERE / 'full_probe_session_v7.py').read_bytes()
if hashlib.sha256(raw).hexdigest() != LEGACY_SHA:
    raise ValueError('pinned lifecycle helper changed')
legacy = types.ModuleType('mhgp9_pinned_lifecycle_primitives')
legacy.__file__ = str(HERE / 'full_probe_session_v7.py')
exec(compile(raw, legacy.__file__, 'exec'), legacy.__dict__)
GUARDS = dict(legacy.GUARDS)
need, sha, save, unique, fields, epoch, target = (
    legacy.need, legacy.sha, legacy.save, legacy.unique, legacy.fields, legacy.epoch, legacy.target)
generation_from_records, closure_generation = legacy.generation_from_records, legacy.closure_generation
Commands, extract_capture = legacy.Commands, legacy.extract_capture
FIXED_COMMANDS = ('compiler', 'cmake_version', 'time_version', 'lscpu', 'nproc', 'configure', 'build', 'preflight')
CASE_COMMAND = re.compile(r'(uptime_before|probe|uptime_after)_(0|[1-9][0-9]*)')


def validate_target(value, status, generation=None):
    need(type(value) is dict and value.get('name') == TARGET['instance'] and
         value.get('zone', '').rsplit('/', 1)[-1] == TARGET['zone'] and
         value.get('selfLink', '').endswith('/projects/' + TARGET['project'] + '/zones/' +
             TARGET['zone'] + '/instances/' + TARGET['instance']) and
         value.get('status') == status and value.get('labels', {}).get('project') == 'e-hgp' and
         value.get('machineType', '').rsplit('/', 1)[-1] == 'g4-standard-48', 'fixed G4 target/status')
    scheduling = value.get('scheduling', {})
    duration = scheduling.get('maxRunDuration', {})
    need(scheduling.get('provisioningModel') == 'SPOT' and scheduling.get('instanceTerminationAction') == 'STOP' and
         scheduling.get('onHostMaintenance') == 'TERMINATE' and scheduling.get('automaticRestart') is False and
         str(duration.get('seconds')) == payload.MAX_RUN_SECONDS and duration.get('nanos', 0) == 0,
         'fixed SPOT/STOP/3600s')
    if generation is not None:
        need(value.get('lastStartTimestamp') == generation, 'current target/generation mismatch')


def guard_deadline(mark, schedule, generation, now):
    """Port explicite de full_probe_session_v7.guard_deadline (SHA 177b25a0...).

    La primitive epinglee fige guest_shutdown_minutes == '30' ; seule cette
    constante devient 40 (cf. tower_worker_v9.GUEST_SHUTDOWN_MINUTES). Toutes
    les autres conditions sont reprises mot pour mot ; la duree GCE reste
    exactement 3600 s et l'arret invite ne peut tomber qu'avant l'echeance
    sure GCE moins 300 s."""
    target(mark)
    need(mark.get('schema') == 'e-hgp.guard-mark.v1' and mark.get('mark') == 'double_guard_verified' and
         mark.get('generation') == generation and mark.get('guest_shutdown_minutes') == payload.GUEST_SHUTDOWN_MINUTES and
         mark.get('max_run_seconds') == payload.MAX_RUN_SECONDS, 'double guard identity / v9 guard durations')
    duration = int(mark['max_run_seconds'])
    need(2700 <= duration <= 28800 and epoch(generation) <= epoch(mark['date_utc']) <= now + 5, 'guard duration/chronology')
    need(schedule.get('MODE') == 'poweroff' and re.fullmatch('[0-9]{1,18}', schedule.get('USEC', '')), 'guest shutdown')
    deadline = int(schedule['USEC']) // 1000000
    need(now + 300 < deadline <= epoch(generation) + duration - 300, 'session deadline')
    return deadline


def validate_snapshot(path, manifest):
    payload.validate_manifest(manifest)
    observed, names = {}, set()
    with tarfile.open(path, 'r:*') as archive:
        for member in archive.getmembers():
            name = member.name.rstrip('/') if member.isdir() else member.name
            parts = PurePosixPath(name)
            allowed_dir = member.isdir() and (name in (payload.SOURCE_ROOT, 'data', 'gcp-migration') or
                                               name.startswith(payload.SOURCE_ROOT + '/') or name.startswith('data/'))
            need(name and len(name) <= 4096 and str(parts) == name and not parts.is_absolute() and
                 all(p not in ('.', '..') for p in parts.parts) and name not in names and
                 (allowed_dir or (member.isfile() and payload.safe_name(name))), 'unsafe v9 archive member')
            names.add(name)
            if member.isfile():
                need(name in manifest, 'unmanifested payload')
                observed[name] = hashlib.sha256(archive.extractfile(member).read()).hexdigest()
        need(observed == manifest, 'snapshot differs from manifest')
        read = lambda name: archive.extractfile(name).read()
        payload.validate_sources(read)
        cases = payload.validate_plan(payload.strict_json(read(payload.PLAN)), manifest)
        payload.validate_data(read)
        provenance = payload.validate_provenance(payload.strict_json(read(payload.PROVENANCE)), manifest)
    return cases, provenance


def validate_protocol_runtime(manifest):
    need(all(sha(HERE.parent / name) == manifest.get(name) for name in payload.PROTOCOL_NAMES),
         'transported protocol differs from executing controller/worker')


def validate_received(output, manifest, worker_pin, expected_cases, generation, provenance, verified_guard):
    """Relit le recu du worker et ses fichiers bruts ; rend completed ou partial.

    `generation` et `provenance` (obligatoires) lient le recu au contexte hote
    de la session (cible fixe, generation demarree, provenance du paquet
    valide) ; la preuve de garde archivee est rejugee champ par champ."""
    need(type(generation) is str and generation and type(provenance) is dict and provenance and
         type(verified_guard) is dict and set(verified_guard) == {'mark', 'schedule'},
         'reception needs the session generation, provenance and host-verified guard')
    value = payload.strict_json((output / 'receipt.json').read_bytes())
    need(type(value) is dict and value.get('target') == payload.TARGET and
         value.get('generation') == generation and
         value.get('provenance') == provenance, 'worker receipt identity (target/generation/provenance)')
    evidence = payload.strict_json((output / 'guard_evidence.json').read_bytes())
    need(type(evidence) is dict and set(evidence) == {'mark', 'schedule', 'metadata'} and
         type(evidence['metadata']) is dict and
         all(evidence['metadata'].get(key) == item for key, item in payload.TARGET.items()) and
         evidence['metadata'].get('machine') == 'g4-standard-48' and type(evidence['mark']) is dict and
         evidence['mark'].get('max_run_seconds') == payload.MAX_RUN_SECONDS and
         evidence['mark'].get('guest_shutdown_minutes') == payload.GUEST_SHUTDOWN_MINUTES and
         evidence['mark'].get('generation') == generation, 'guard evidence identity')
    # Marque : cible, schema et chronologie ; calendrier invite : arret
    # (poweroff) avant l'echeance sure GCE, comme guard_deadline a l'aller.
    mark, schedule = evidence['mark'], evidence['schedule']
    need(all(mark.get(key) == item for key, item in payload.TARGET.items()) and
         mark.get('schema') == 'e-hgp.guard-mark.v1' and mark.get('mark') == 'double_guard_verified' and
         type(mark.get('date_utc')) is str and epoch(generation) <= epoch(mark['date_utc']), 'guard mark target/chronology')
    need(type(schedule) is dict and schedule.get('MODE') == 'poweroff' and
         type(schedule.get('USEC')) is str and re.fullmatch('[0-9]{1,18}', schedule['USEC']) and
         epoch(generation) < int(schedule['USEC']) // 1000000 <=
         epoch(generation) + int(payload.MAX_RUN_SECONDS) - 300 and
         epoch(mark['date_utc']) < int(schedule['USEC']) // 1000000, 'guest shutdown schedule')
    # Exact binding: the archived mark and schedule are the ones the host
    # verified before upload; a schedule changed between the two reads is
    # refused rather than rewritten.
    need(mark == verified_guard['mark'] and schedule == verified_guard['schedule'],
         'archived guard evidence differs from the host-verified guard')
    need(type(value) is dict and value.get('status') in ('completed', 'partial') and
         value.get('sources_stable') is True and value.get('compiled_dependencies_stable') is True and
         value.get('binary_stable') is True and value.get('worker_sha256') == worker_pin and
         value.get('backend') == 'reference_cpu' and value.get('GPU_executed') is False and
         value.get('FULL_executed') is True and value.get('contract_certified') is False and
         value.get('public_status') == 'not_claimed', 'worker completion/scope')
    need(value.get('useful_budget_seconds') == payload.USEFUL_BUDGET_SECONDS and
         value.get('case_cap_seconds') == payload.CASE_CAP_SECONDS, 'worker budgets')
    dependencies_path = output / 'compiled_dependencies.json'
    need(sha(dependencies_path) == value.get('compiled_dependency_manifest_sha256'), 'compiled dependencies manifest pin')
    dependencies = payload.strict_json(dependencies_path.read_bytes())
    need(type(dependencies) is dict and dependencies and all(type(name) is str and Path(name).is_absolute() and
         type(pin) is str and re.fullmatch('[0-9a-f]{64}', pin) for name, pin in dependencies.items()),
         'compiled dependency inventory')
    for name in ('sources_before.json', 'sources_after.json'):
        need(payload.strict_json((output / name).read_bytes()) == manifest, 'worker payload closure')
    rows = {}
    for path in sorted(output.glob('*.command.json')):
        row = payload.strict_json(path.read_bytes())
        stem = path.name[:-len('.command.json')]
        need(type(row) is dict and type(row.get('argv')) is list and row['argv'], 'worker invocation')
        need(sha(output / (stem + '.stdout')) == row.get('stdout_sha256') and
             sha(output / (stem + '.stderr')) == row.get('stderr_sha256'), 'worker raw log hash')
        rows[stem] = row
    canonical = lambda items: sorted(json.dumps(item, sort_keys=True) for item in items)
    need(type(value.get('commands')) is list and canonical(rows.values()) == canonical(value['commands']),
         'worker command list/raw receipts mismatch')
    need(set(FIXED_COMMANDS) <= set(rows), 'environment/build commands absent')
    for stem, row in rows.items():
        match = CASE_COMMAND.fullmatch(stem)
        need(stem in FIXED_COMMANDS or (match is not None and int(match.group(2)) < len(expected_cases)),
             'unexpected worker command: ' + stem)
        if match is None or match.group(1) != 'probe':
            need(row.get('exit_code') == 0 and row.get('group_closed') is True and
                 not row.get('residual_or_interrupted_group_killed'), 'worker command closure: ' + stem)
    configure, build = rows['configure']['argv'], rows['build']['argv']
    need(configure[0] == build[0] == rows['cmake_version']['argv'][0] and configure[1] == '-S' and
         configure[2].endswith('/' + payload.SOURCE_ROOT) and configure[3] == '-B' and
         configure[5:] == ['-DCMAKE_BUILD_TYPE=Release', '-DBOOST_ROOT=' + payload.BOOST_ROOT,
                           '-DCMAKE_CXX_COMPILER=' + rows['compiler']['argv'][0]] and
         build[1:] == ['--build', configure[4], '--target', payload.PROBE_TARGET, '--parallel', payload.BUILD_PARALLEL],
         'strict CMake configure/build invocation (no -Wno-error)')
    cases = payload.validate_plan(dict(schema=value.get('plan_schema'), cases=value.get('cases')), manifest)
    need(cases == expected_cases, 'worker case plan differs from transported plan')
    # Preflight natif rejuge a la reception : memes octets de nuage, meme
    # invocation, sortie complete sous le meme validateur que les cas.
    pre_raw = (output / payload.PREFLIGHT_FILE).read_bytes()
    need(pre_raw == payload.preflight_cloud(), 'preflight cloud bytes')
    pre_case = payload.preflight_case(cases, pre_raw)
    pre_argv = rows['preflight']['argv']
    need(pre_argv[:2] == [payload.TIME, '-v'] and pre_argv[2] == configure[4] + '/' + payload.PROBE_TARGET and
         pre_argv[3].endswith('/' + payload.PREFLIGHT_FILE) and pre_argv[4:] == payload.expected_probe_tail(pre_case),
         'exact preflight invocation')
    pre_value = payload.strict_json((output / 'preflight.stdout').read_bytes())
    need(payload.validate_probe(pre_value, pre_case, 0, inputs=payload.preflight_inputs(pre_raw)) == 'complete_relative'
         and value.get('preflight') == dict(sites=pre_case['n'], tower_digest=pre_value['tower_digest']),
         'preflight recomputation')
    payload.validate_external_wall(pre_value, rows['preflight'].get('elapsed_seconds'))
    payload.validate_preflight_work(pre_value, pre_case['levers'])
    payload.validate_gnu_time((output / 'preflight.stderr').read_text(errors='replace'), 0)
    outcomes = value.get('case_outcomes')
    need(type(outcomes) is list and len(outcomes) == len(cases), 'case outcome list')
    values, exhausted = {}, False
    for index, (case, entry) in enumerate(zip(cases, outcomes)):
        need(type(entry) is dict and entry.get('index') == index and entry.get('outcome') in payload.OUTCOMES,
             'case outcome record')
        outcome, name = entry['outcome'], 'probe_' + str(index)
        need(not exhausted or outcome == 'skipped_budget', 'cases after budget exhaustion must be skipped')
        if outcome == 'skipped_budget':
            need(name not in rows and 'uptime_before_' + str(index) not in rows, 'skipped case has no command')
            exhausted = True
            continue
        need(name in rows and 'uptime_before_' + str(index) in rows, 'launched case commands')
        row = rows[name]
        argv = row['argv']
        need(argv[:2] == [payload.TIME, '-v'] and len(argv) == 4 + len(payload.expected_probe_tail(case)) and
             argv[2] == configure[4] + '/' + payload.PROBE_TARGET and argv[3].endswith('/' + case['file']) and
             argv[4:] == payload.expected_probe_tail(case), 'exact GNU time / probe invocation')
        need(entry.get('exit_code') == row.get('exit_code'), 'case exit code record')
        if outcome in ('killed_case_cap', 'killed_budget'):
            need(row.get('session_deadline_reached') is True and row.get('residual_or_interrupted_group_killed') is True
                 and row.get('group_closed') is True, 'killed case record (group certified closed)')
            if outcome == 'killed_case_cap':
                need(row.get('elapsed_seconds', 0) >= payload.CASE_CAP_SECONDS - 1.0, 'case killed before its cap')
            else:
                exhausted = True
            continue
        need(outcome in ('complete_relative', 'explicit_refusal'), 'probe failure in an accepted receipt')
        need(row.get('group_closed') is True and not row.get('residual_or_interrupted_group_killed') and
             not row.get('session_deadline_reached'), 'probe closure')
        probe = payload.strict_json((output / (name + '.stdout')).read_bytes())
        need(payload.validate_probe(probe, case, row['exit_code']) == outcome, 'probe outcome recomputation')
        payload.validate_external_wall(probe, row.get('elapsed_seconds'))
        rss = payload.validate_gnu_time((output / (name + '.stderr')).read_text(errors='replace'), row['exit_code'])
        need(entry.get('probe_status') == probe['status'] and entry.get('tower_digest') == probe['tower_digest'] and
             entry.get('elapsed_seconds') == row.get('elapsed_seconds') and
             entry.get('chain_total_ms') == probe['times_ms']['chain_total'] and entry.get('gnu_time_max_rss_kb') == rss,
             'case summary differs from raw probe output / GNU time / command record')
        summary = payload.strict_json((output / (name + '.summary.json')).read_bytes())
        need(summary == dict(case=case, input_file_sha256=manifest[case['file']], **entry), 'case summary file')
        values[index] = probe
    completed = [i for i, entry in enumerate(outcomes) if entry['outcome'] == 'complete_relative']
    need(value.get('completed_case_indices') == completed, 'completed case indices')
    # Un plan sans aucune tour achevee n'est jamais un succes de reception.
    need(completed, 'no complete tower in the campaign')
    comparisons = payload.compare_cases(cases, outcomes, values)
    need(value.get('cross_worker_comparisons') == comparisons and all(item['equal'] for item in comparisons),
         'cross-worker object comparison')
    need(any(entry['outcome'] != 'skipped_budget' for entry in outcomes), 'no executed case')
    status = 'completed' if len(completed) == len(cases) else 'partial'
    need(value['status'] == status, 'worker status recomputation')
    return status


def run_session(args):
    session = args.session_dir.absolute()
    key = args.ssh_key.absolute()
    need(session.is_dir() and not session.is_symlink() and session.stat().st_mode & 0o777 == 0o700, 'existing private session 0700')
    need(key.is_file() and not key.is_symlink() and key.stat().st_mode & 0o777 == 0o600 and
         Path(str(key) + '.pub').is_file() and not Path(str(key) + '.pub').is_symlink(), 'existing private/public key files')
    need(sha(__file__) == args.expected_controller_sha256, 'controller pin')
    need(args.worker_sha256 == sha(payload.__file__), 'worker must match this v9 adapter')
    for name, pin in GUARDS.items():
        need(sha(HERE / name) == pin, 'guard pin ' + name)
    inputs = [(args.snapshot, args.snapshot_sha256, 'snapshot.tar.gz'),
              (args.manifest, args.manifest_sha256, 'source_manifest.json'), (args.worker, args.worker_sha256, 'worker.py')]
    for path, pin, _ in inputs:
        need(path.is_file() and not path.is_symlink() and re.fullmatch('[0-9a-f]{64}', pin) and sha(path) == pin, 'input pin')
    manifest = json.loads(args.manifest.read_text(), object_pairs_hook=unique)
    expected_cases, provenance = validate_snapshot(args.snapshot, manifest)
    validate_protocol_runtime(manifest)
    useful, cap = payload.USEFUL_BUDGET_SECONDS, payload.CASE_CAP_SECONDS
    host = session / 'tower_v9_host'
    host.mkdir(mode=0o700, exist_ok=False)
    marks = host / 'guardmarks'
    marks.mkdir(mode=0o700)
    for path, pin, name in inputs:
        shutil.copyfile(path, host / name)
        need(sha(host / name) == pin, 'copied input pin')
    for name in GUARDS:
        shutil.copyfile(HERE / name, host / name)
        need(sha(host / name) == GUARDS[name], 'copied guard pin')
        (host / name).chmod(0o700)
    env = dict(os.environ, GCP_PROJECT_ID=TARGET['project'], GCP_ZONE=TARGET['zone'],
               GCP_INSTANCE_NAME=TARGET['instance'], GCP_SSH_KEY_FILE=str(key),
               PATH=str(args.gcloud.parent) + os.pathsep + os.environ.get('PATH', ''))
    commands = Commands(host, env)
    state = dict(status='failed', target=TARGET, public_status='not_claimed', targeted_shutdown_certified=False,
                 backend='reference_cpu', GPU_executed=False, FULL_executed=False, useful_budget_seconds=useful,
                 case_cap_seconds=cap, guest_shutdown_minutes=int(payload.GUEST_SHUTDOWN_MINUTES),
                 max_run_seconds=int(payload.MAX_RUN_SECONDS), provenance=provenance,
                 snapshot_sha256=args.snapshot_sha256, manifest_sha256=args.manifest_sha256,
                 worker_sha256=args.worker_sha256, controller_sha256=args.expected_controller_sha256)
    common = None
    remote = None
    worker_attempted = False
    generation = None
    deadline = monotonic_deadline = None
    previous_handlers = {}

    def interrupted(signum, _frame):
        raise InterruptedError('host signal ' + str(signum))

    def read_generation():
        handoff = json.loads((host / 'handoff.json').read_text(), object_pairs_hook=unique) if (host / 'handoff.json').exists() else None
        lifecycle = fields((host / 'lifecycle.txt').read_text()) if (host / 'lifecycle.txt').exists() else None
        return generation_from_records(handoff, lifecycle)

    def ssh(name, script, timeout=90):
        return commands.run(name, [args.gcloud, 'compute', 'ssh', TARGET['instance'], *common,
                                  '--ssh-flag=-n', '--ssh-flag=-o BatchMode=yes', '--ssh-flag=-o ConnectTimeout=15',
                                  '--command=' + script], timeout=timeout)

    def recertify(name, timeout=60):
        rc, raw, _ = commands.run(name, [args.gcloud, 'compute', 'instances', 'describe', TARGET['instance'],
                                '--project=' + TARGET['project'], '--zone=' + TARGET['zone'], '--format=json'], timeout=timeout)
        need(rc == 0, 'target unreadable')
        value = json.loads(raw, object_pairs_hook=unique)
        validate_target(value, 'RUNNING', generation)

    def remaining(reserve, cap_seconds):
        need(deadline is not None and monotonic_deadline is not None, 'no certified session deadline')
        value = min(deadline-reserve-time.time(), monotonic_deadline-reserve-time.monotonic(), cap_seconds)
        need(value > 0, 'session window ended; stop without further recovery')
        return value

    try:
        for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
            previous_handlers[sig] = signal.signal(sig, interrupted)
        rc, raw, _ = commands.run('before_start', [args.gcloud, 'compute', 'instances', 'describe',
            TARGET['instance'], '--project=' + TARGET['project'], '--zone=' + TARGET['zone'], '--format=json'])
        need(rc == 0, 'pre-start target unreadable')
        validate_target(json.loads(raw, object_pairs_hook=unique), 'TERMINATED')
        # Only this public key is registered; never export/read the private key.
        rc, _, _ = commands.run('oslogin_add', [args.gcloud, 'compute', 'os-login', 'ssh-keys', 'add',
            '--project=' + TARGET['project'], '--key-file=' + str(key) + '.pub', '--ttl=70m', '--format=json', '--quiet'])
        need(rc == 0, 'OS Login registration failed')
        rc, start_output, _ = commands.run('guarded_start', [host / 'start_and_verify.sh', '--yes',
            '--guest-shutdown-minutes', payload.GUEST_SHUTDOWN_MINUTES, '--handoff-file', host / 'handoff.json',
            '--lifecycle-state-file', host / 'lifecycle.txt', '--guard-mark-dir', marks], timeout=None, guard=True)
        generation = read_generation()
        need(rc == 0 and generation is not None, 'start not certified')
        expiration = re.findall(r'expiration fixe=(\d{4}-\d\d-\d\dT\d\d:\d\d:\d\d\.\d{6}Z)', start_output)
        need(len(expiration) == 1 and epoch(expiration[0]) > time.time(), 'exact SSH expiration not certified')
        common = ['--project=' + TARGET['project'], '--zone=' + TARGET['zone'], '--quiet',
                  '--ssh-key-file=' + str(key), '--ssh-key-expiration=' + expiration[0]]
        mark_path = marks / 'double_guard_verified'
        mark = fields(mark_path.read_text())
        target(mark)
        need(mark.get('mark') == 'double_guard_verified' and mark.get('generation') == generation, 'double guard before upload')
        recertify('before_upload')
        rc, schedule_raw, _ = ssh('guest_schedule', 'sudo -n cat /run/systemd/shutdown/scheduled')
        need(rc == 0, 'guest schedule unreadable')
        schedule = fields(schedule_raw)
        deadline = guard_deadline(mark, schedule, generation, time.time())
        monotonic_deadline = time.monotonic() + deadline-time.time()
        # The exact mark and guest schedule verified here: the archived guard
        # evidence must equal them at reception (never a merely plausible one).
        verified_guard = dict(mark=mark, schedule=schedule)
        state.update(generation=generation, session_deadline_epoch=deadline, ssh_expiration=expiration[0],
                     verified_guard=verified_guard)
        nonce = secrets.token_hex(8)
        prefix = '/tmp/ehgp-tower-v9-' + nonce + '.'
        rc, raw, _ = ssh('remote_mkdir', 'umask 077; mktemp -d ' + shlex.quote(prefix + 'XXXXXXXXXX'))
        need(rc == 0 and re.fullmatch(re.escape(prefix) + r'[A-Za-z0-9]{10}\n?', raw), 'fresh remote path')
        remote = raw.strip()
        state['remote_directory'] = remote
        rc, _, _ = commands.run('upload', [args.gcloud, 'compute', 'scp', *common, host / 'snapshot.tar.gz',
            host / 'source_manifest.json', host / 'worker.py', mark_path, TARGET['instance'] + ':' + remote + '/'], timeout=180)
        need(rc == 0, 'upload failed')
        pins = [(args.snapshot_sha256, 'snapshot.tar.gz'), (args.manifest_sha256, 'source_manifest.json'),
                (args.worker_sha256, 'worker.py'), (sha(mark_path), 'double_guard_verified')]
        checks = ' && '.join('test "$(sha256sum ' + shlex.quote(remote + '/' + name) +
                            ' | cut -d " " -f 1)" = ' + shlex.quote(pin) for pin, name in pins)
        # The tar was fully inspected locally and its transported bytes are
        # rehashed before extraction into a fresh, private source directory.
        rc, _, _ = ssh('unpack', checks + ' && mkdir -m 700 ' + shlex.quote(remote + '/source') +
            ' && tar --no-same-owner --no-same-permissions -xzf ' + shlex.quote(remote + '/snapshot.tar.gz') +
            ' -C ' + shlex.quote(remote + '/source'))
        need(rc == 0, 'remote pins/extraction failed')
        recertify('before_worker')
        argv = ['python3', remote + '/worker.py', '--source-root', remote + '/source',
                '--source-manifest', remote + '/source_manifest.json', '--source-manifest-sha256', args.manifest_sha256,
                '--guard-mark', remote + '/double_guard_verified', '--guard-mark-sha256', sha(mark_path),
                '--generation', generation, '--session-deadline-epoch', str(deadline), '--closing-margin-seconds', '300',
                '--output', remote + '/output']
        for key_name, value in TARGET.items():
            argv += ['--' + key_name, value]
        argv += ['--execute', '--useful-budget-seconds', str(useful), '--case-cap-seconds', str(cap)]
        worker_attempted = True
        # 60 s beyond the useful budget for worker closure; the worker itself
        # stops at the session deadline minus 300 s, i.e. before this timeout.
        rc, _, _ = ssh('worker', 'exec ' + shlex.join(argv), timeout=remaining(240, useful + 60))
        state['worker_exit_code'] = rc
        state['status'] = 'worker_returned' if rc == 0 else 'worker_failed'
    except BaseException as error:
        state.update(status='failed', error=type(error).__name__ + ': ' + str(error))
    finally:
        # Repeated signals cannot interrupt recovery or the targeted stop.
        for sig in previous_handlers:
            signal.signal(sig, signal.SIG_IGN)
        try:
            if worker_attempted and remote is not None and common is not None:
                try:
                    recertify('before_retrieve', timeout=remaining(60, 60))
                    script = ('tar -czf ' + shlex.quote(remote + '/capture.tar.gz') + ' --exclude=output/build -C ' +
                              shlex.quote(remote) + ' output; capture_code=$?; sha256sum ' +
                              shlex.quote(remote + '/capture.tar.gz') + '; exit "$capture_code"')
                    pack_rc, raw, _ = ssh('pack_capture', script, timeout=remaining(60, 90))
                    matches = re.findall(r'^([0-9a-f]{64})  ' + re.escape(remote + '/capture.tar.gz') + r'$', raw, re.M)
                    need(len(matches) == 1, 'capture tar hash absent')
                    state['capture_pack_exit_code'] = pack_rc
                    rc, _, _ = commands.run('download', [args.gcloud, 'compute', 'scp', *common,
                        TARGET['instance'] + ':' + remote + '/capture.tar.gz', host / 'capture.tar.gz'], timeout=remaining(60, 180))
                    need(rc == 0 and sha(host / 'capture.tar.gz') == matches[0], 'capture download/hash')
                    extract_capture(host / 'capture.tar.gz', host / 'received')
                    receipt_path = host / 'received/output/receipt.json'
                    state.update(capture_sha256=matches[0], capture_received=True,
                                 worker_receipt_present=receipt_path.is_file())
                    if state['worker_receipt_present']:
                        try:
                            received = payload.strict_json(receipt_path.read_bytes())
                            state['worker_status'] = received.get('status') if type(received) is dict else None
                        except ValueError:
                            state['worker_status'] = None
                    if pack_rc != 0 or not state['worker_receipt_present']:
                        state['status'] = 'capture_incomplete'
                    elif state['status'] == 'worker_returned':
                        state['status'] = validate_received(host / 'received/output', manifest, args.worker_sha256,
                                                            expected_cases, generation, provenance, verified_guard)
                        state['FULL_executed'] = True
                except BaseException as error:
                    state.update(status='capture_failed', capture_error=type(error).__name__ + ': ' + str(error))
        finally:
            # Never infer a generation from a new GCE read. Unknown/ambiguous
            # lifecycle requires human recovery, not an unversioned stop.
            try:
                try:
                    closing_generation = closure_generation(generation, read_generation())
                except (OSError, UnicodeError, json.JSONDecodeError):
                    closing_generation = closure_generation(generation, None, unreadable=True)
                if closing_generation is not None:
                    rc, _, _ = commands.run('guarded_stop', [host / 'stop_and_verify.sh', '--yes',
                        '--expected-last-start-timestamp', closing_generation], timeout=None, critical=True, guard=True)
                    need(rc == 0, 'targeted stop not certified')
                    state['targeted_shutdown_certified'] = True
                    state['generation'] = closing_generation
                else:
                    state['no_start_lifecycle_created'] = True
            except BaseException as error:
                state.update(status='shutdown_uncertified', shutdown_error=type(error).__name__ + ': ' + str(error))
            state['commands'] = commands.rows
            try:
                save(host / 'receipt.json', state)
            except OSError as error:
                print('Host receipt write failed: ' + str(error), file=sys.stderr)
            for sig, handler in previous_handlers.items():
                signal.signal(sig, handler)
    print(json.dumps(state, sort_keys=True), flush=True)
    if state['status'] == 'shutdown_uncertified':
        return 74
    return 0 if state['status'] in ('completed', 'partial') else 1


def require_committed_protocol(snapshot, snapshot_sha256):
    """Une vraie session n'emporte que le protocole v9 committe au commit du paquet."""
    need(snapshot.is_file() and not snapshot.is_symlink() and type(snapshot_sha256) is str and
         sha(snapshot) == snapshot_sha256, 'snapshot pin')
    try:
        with tarfile.open(snapshot, 'r:*') as archive:
            files = {member.name: archive.extractfile(member).read() for member in archive.getmembers()
                     if member.isfile()}
        provenance = payload.strict_json(files[payload.PROVENANCE])
    except (KeyError, tarfile.TarError, OSError) as error:
        raise ValueError('snapshot provenance unreadable: ' + type(error).__name__) from error
    need(type(provenance) is dict and provenance.get('protocol_source') == 'commit',
         'real session refused: v9 protocol files are not committed at the snapshot commit')
    # Recertification Git a la reception (contre-audit B) : le paquet doit se
    # reconstruire octet pour octet depuis les objets du commit qu'il declare,
    # plan transporte compris ; une archive qui s'attribue un commit echoue.
    import tower_snapshot_v9 as snapshot_builder
    rebuilt, rebuilt_provenance = snapshot_builder.collect(provenance.get('commit', ''), plan_raw=files[payload.PLAN])
    need(rebuilt_provenance == provenance and
         {name: hashlib.sha256(raw).hexdigest() for name, raw in rebuilt.items()} ==
         {name: hashlib.sha256(raw).hexdigest() for name, raw in files.items()},
         'snapshot is not reproducible from the Git objects of its declared commit')


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('snapshot', 'manifest', 'worker', 'session-dir', 'ssh-key'):
        parser.add_argument('--' + name, type=Path)
    for name in ('snapshot-sha256', 'manifest-sha256', 'worker-sha256', 'expected-controller-sha256'):
        parser.add_argument('--' + name)
    parser.add_argument('--gcloud', type=Path, default=Path('/home/codespace/google-cloud-sdk/bin/gcloud'))
    parser.add_argument('--execute', action='store_true')
    args = parser.parse_args(argv)
    if not args.execute:
        print(json.dumps(dict(status='inert', target=TARGET, guest_minutes=int(payload.GUEST_SHUTDOWN_MINUTES),
                              gce_seconds=int(payload.MAX_RUN_SECONDS), useful_seconds=payload.USEFUL_BUDGET_SECONDS,
                              case_cap_seconds=payload.CASE_CAP_SECONDS, closing_margin_seconds=300,
                              backend='reference_cpu', scope=payload.SCOPE, GCP_used=False,
                              GPU_executed=False, FULL_executed=False), sort_keys=True))
        return 0
    try:
        need(all(getattr(args, name.replace('-', '_')) is not None for name in (
            'snapshot', 'manifest', 'worker', 'session-dir', 'ssh-key', 'snapshot-sha256',
            'manifest-sha256', 'worker-sha256', 'expected-controller-sha256')), 'missing execution argument')
        require_committed_protocol(args.snapshot, args.snapshot_sha256)
    except ValueError as error:
        print(json.dumps(dict(status='refused', reason=str(error), GCP_used=False), sort_keys=True), flush=True)
        return 2
    return run_session(args)


if __name__ == '__main__':
    raise SystemExit(main())
