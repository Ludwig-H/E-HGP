#!/usr/bin/env python3
"""Session G4 SPOT gardee pour la sonde S1 GPU v9 (filtre temoin exact q3/q4). Inerte sans --execute.

Le cycle de vie est celui de tower_session_v9.py, importe comme bibliotheque
et epingle dans le manifeste : validate_target, guard_deadline et les
primitives epinglees de full_probe_session_v7.py (SHA 177b25a0...) via lui.
run_session est la copie de celui de la tour, dans le meme ordre gardé :
cible TERMINATED/SPOT/STOP/3600 s verifiee, cle publique ephemere seule
(70 min), demarrage par start_and_verify.sh (double coupe-circuit),
recertification avant televersement, worker, capture tar hachee, et ARRET
CIBLE CERTIFIE par stop_and_verify.sh --expected-last-start-timestamp dans le
finally, meme apres echec, interruption ou recuperation impossible.

Differences : worker gpu_filter_worker_v9.py (bibliotheque tower_worker_v9.py
prise dans l'arbre source extrait et rehache, PYTHONPATH), reception propre
aux schemas mhgp9_gpu_filter_probe_v3 (v2 historique relisible), backend=cuda_g4, GPU_executed vrai
seulement apres une reception validee, FULL jamais execute ;
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

import gpu_filter_worker_v9 as payload
import tower_session_v9 as lifecycle

HERE = Path(__file__).resolve().parent
TARGET = dict(payload.TARGET)
GUARDS = dict(lifecycle.GUARDS)
need, sha, save, unique, fields, epoch, target = (
    lifecycle.need, lifecycle.sha, lifecycle.save, lifecycle.unique, lifecycle.fields, lifecycle.epoch,
    lifecycle.target)
generation_from_records, closure_generation = lifecycle.generation_from_records, lifecycle.closure_generation
Commands, extract_capture = lifecycle.Commands, lifecycle.extract_capture
validate_target, guard_deadline = lifecycle.validate_target, lifecycle.guard_deadline
FIXED_COMMANDS = ('compiler', 'cmake_version', 'nvcc_version', 'time_version', 'gpu_inventory', 'lscpu', 'nproc',
                  'configure', 'build', 'preflight', 'preflight_mutant')
CASE_COMMAND = re.compile(r'(uptime_before|probe|uptime_after)_(0|[1-9][0-9]*)')
RECEIVED_STATUSES = payload.RECEIVED_STATUSES


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
        payload.validate_sources(read, tile_cache=any(case.get('tile_cache', False) for case in cases))
        payload.base.validate_data(read)
        provenance = payload.base.validate_provenance(payload.strict_json(read(payload.PROVENANCE)), manifest)
    return cases, provenance


def validate_protocol_runtime(manifest):
    need(all(sha(HERE.parent / name) == manifest.get(name) for name in payload.PROTOCOL_NAMES),
         'transported protocol differs from executing controller/worker/libraries')


def _guard_evidence(output, generation, verified_guard):
    """Preuve de garde archivee : meme jugement que la reception de la tour."""
    evidence = payload.strict_json((output / 'guard_evidence.json').read_bytes())
    need(type(evidence) is dict and set(evidence) == {'mark', 'schedule', 'metadata'} and
         type(evidence['metadata']) is dict and
         all(evidence['metadata'].get(key) == item for key, item in payload.TARGET.items()) and
         evidence['metadata'].get('machine') == 'g4-standard-48' and type(evidence['mark']) is dict and
         evidence['mark'].get('max_run_seconds') == payload.MAX_RUN_SECONDS and
         evidence['mark'].get('guest_shutdown_minutes') == payload.GUEST_SHUTDOWN_MINUTES and
         evidence['mark'].get('generation') == generation, 'guard evidence identity')
    mark, schedule = evidence['mark'], evidence['schedule']
    need(all(mark.get(key) == item for key, item in payload.TARGET.items()) and
         mark.get('schema') == 'e-hgp.guard-mark.v1' and mark.get('mark') == 'double_guard_verified' and
         type(mark.get('date_utc')) is str and epoch(generation) <= epoch(mark['date_utc']), 'guard mark target/chronology')
    need(type(schedule) is dict and schedule.get('MODE') == 'poweroff' and
         type(schedule.get('USEC')) is str and re.fullmatch('[0-9]{1,18}', schedule['USEC']) and
         epoch(generation) < int(schedule['USEC']) // 1000000 <=
         epoch(generation) + int(payload.MAX_RUN_SECONDS) - 300 and
         epoch(mark['date_utc']) < int(schedule['USEC']) // 1000000, 'guest shutdown schedule')
    need(mark == verified_guard['mark'] and schedule == verified_guard['schedule'],
         'archived guard evidence differs from the host-verified guard')


def validate_received(output, manifest, worker_pin, expected_cases, generation, provenance, verified_guard):
    """Relit le recu du worker et ses fichiers bruts ; rend completed, partial ou gpu_mismatch."""
    need(type(generation) is str and generation and type(provenance) is dict and provenance and
         type(verified_guard) is dict and set(verified_guard) == {'mark', 'schedule'},
         'reception needs the session generation, provenance and host-verified guard')
    value = payload.strict_json((output / 'receipt.json').read_bytes())
    need(type(value) is dict and value.get('target') == payload.TARGET and value.get('generation') == generation and
         value.get('provenance') == provenance, 'worker receipt identity (target/generation/provenance)')
    _guard_evidence(output, generation, verified_guard)
    need(value.get('status') in RECEIVED_STATUSES and value.get('sources_stable') is True and
         value.get('compiled_dependencies_stable') is True and value.get('binary_stable') is True and
         value.get('worker_sha256') == worker_pin and value.get('base_worker_sha256') == manifest.get(payload.BASE_WORKER)
         and value.get('backend') == 'cuda_g4' and value.get('scope') == payload.SCOPE and
         value.get('GPU_attempted') is True and value.get('GPU_executed') is True and
         value.get('FULL_executed') is False and
         value.get('contract_certified') is False and value.get('public_status') == 'not_claimed' and
         value.get('CUDA_installation_attempted') is False and value.get('s1_threshold_ms') == payload.S1_THRESHOLD_MS,
         'worker completion/scope')
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
    tile_plan = any(case.get('tile_cache', False) for case in expected_cases)
    fixed_commands = set(FIXED_COMMANDS) | ({'preflight_tile', 'preflight_tile_mutant'} if tile_plan else set())
    need(fixed_commands <= set(rows), 'environment/build commands absent')
    for stem, row in rows.items():
        match = CASE_COMMAND.fullmatch(stem)
        need(stem in fixed_commands or (match is not None and int(match.group(2)) < len(expected_cases)),
             'unexpected worker command: ' + stem)
        if match is None or match.group(1) != 'probe':
            # The causal mutant of the preflight must exit 1 (mismatch found).
            need(row.get('exit_code') == (1 if stem in ('preflight_mutant', 'preflight_tile_mutant') else 0) and
                 row.get('group_closed') is True and
                 not row.get('residual_or_interrupted_group_killed'), 'worker command closure: ' + stem)
    need(payload.DEVICE_NAME in (output / 'gpu_inventory.stdout').read_text(), 'G4 GPU inventory')
    tools = dict(cmake=rows['cmake_version']['argv'][0], nvcc=rows['nvcc_version']['argv'][0],
                 **{'g++': rows['compiler']['argv'][0]})
    configure, build = rows['configure']['argv'], rows['build']['argv']
    need(len(configure) == 10 and configure[:3] == [tools['cmake'], '-S', configure[2]] and
         configure[2].endswith('/' + payload.SOURCE_ROOT) and configure[3] == '-B' and
         configure == payload.configure_command(tools, Path(configure[2][:-len('/' + payload.SOURCE_ROOT)]),
                                                Path(configure[4])) and
         build == [tools['cmake'], '--build', configure[4], '--target', payload.PROBE_TARGET, '--parallel',
                   payload.BUILD_PARALLEL] and tools['nvcc'] in payload.CUDA_PATHS,
         'strict CMake configure/build invocation (CUDA on, no -Wno-error)')
    cases = payload.validate_plan(dict(schema=value.get('plan_schema'), cases=value.get('cases')), manifest)
    need(cases == expected_cases, 'worker case plan differs from transported plan')
    pre_raw = (output / payload.PREFLIGHT_FILE).read_bytes()
    need(pre_raw == payload.base.preflight_cloud(), 'preflight cloud bytes')
    pre_case = payload.preflight_case(pre_raw)
    pre_argv = rows['preflight']['argv']
    need(pre_argv[:2] == [payload.TIME, '-v'] and pre_argv[2] == configure[4] + '/' + payload.PROBE_TARGET and
         pre_argv[3].endswith('/' + payload.PREFLIGHT_FILE) and pre_argv[4:] == payload.expected_probe_tail(pre_case),
         'exact preflight invocation')
    pre_value = payload.strict_json((output / 'preflight.stdout').read_bytes())
    need(payload.validate_probe(pre_value, pre_case, rows['preflight']['exit_code'],
                                inputs=payload.base.preflight_inputs(pre_raw)) == 'complete' and
         value.get('preflight') == dict(sites=pre_case['n'], pairs=pre_value['population']['pairs'],
                                        gpu_total_ms=pre_value['gpu']['total_ms']), 'preflight recomputation')
    payload.base.validate_gnu_time((output / 'preflight.stderr').read_text(errors='replace'), 0)
    mutant_argv = rows['preflight_mutant']['argv']
    need(mutant_argv[:4] == pre_argv[:4] and mutant_argv[4:] == payload.expected_probe_tail(pre_case, payload.INJECT),
         'exact causal mutant invocation')
    mutant_value = payload.strict_json((output / 'preflight_mutant.stdout').read_bytes())
    need(payload.validate_probe(mutant_value, pre_case, rows['preflight_mutant']['exit_code'],
                                inputs=payload.base.preflight_inputs(pre_raw), inject=payload.INJECT) == 'gpu_mismatch',
         'causal mutant recomputation')
    payload.base.validate_gnu_time((output / 'preflight_mutant.stderr').read_text(errors='replace'), 1)
    if tile_plan:
        tile_case = payload.preflight_case(pre_raw, tile_cache=True)
        for stem, inject, exit_code, outcome in (('preflight_tile', '', 0, 'complete'),
                                                ('preflight_tile_mutant', payload.INJECT, 1, 'gpu_mismatch')):
            need(rows[stem]['argv'][:4] == pre_argv[:4] and
                 rows[stem]['argv'][4:] == payload.expected_probe_tail(tile_case, inject),
                 'exact tile preflight invocation')
            probe = payload.strict_json((output / (stem + '.stdout')).read_bytes())
            need(payload.validate_probe(probe, tile_case, rows[stem]['exit_code'],
                                        inputs=payload.base.preflight_inputs(pre_raw), inject=inject) == outcome,
                 'tile preflight recomputation')
            payload.base.validate_gnu_time((output / (stem + '.stderr')).read_text(errors='replace'), exit_code)
            need(probe['population'] == pre_value['population'], 'tile preflight population differs')
            if not inject:
                need(value.get('preflight_tile') == dict(sites=tile_case['n'], pairs=probe['population']['pairs'],
                                                         gpu_total_ms=probe['gpu']['total_ms']),
                     'tile preflight summary recomputation')
    else:
        need('preflight_tile' not in value, 'unexpected tile preflight summary')
    outcomes = value.get('case_outcomes')
    need(type(outcomes) is list and len(outcomes) == len(cases), 'case outcome list')
    exhausted = False
    gated = len(outcomes) > 0 and type(outcomes[0]) is dict and outcomes[0].get('outcome') != 'complete'
    for index, (case, entry) in enumerate(zip(cases, outcomes)):
        need(type(entry) is dict and entry.get('index') == index and entry.get('outcome') in payload.OUTCOMES,
             'case outcome record')
        outcome, name = entry['outcome'], 'probe_' + str(index)
        need(outcome not in ('probe_failed', 'skipped_protocol_defect'), 'probe failure in an accepted receipt')
        # Same precedence as the worker: an exhausted budget, then the S1 gate
        # (after a non-complete gate case), then a normal or budget-skipped case.
        need((outcome == 'skipped_s1_gate') == (index > 0 and gated and not exhausted), 'S1 gate skip rule')
        if outcome == 'skipped_s1_gate':
            need(entry == dict(index=index, outcome=outcome) and name not in rows and
                 'uptime_before_' + str(index) not in rows, 'gated case has no command')
            continue
        need(not exhausted or outcome == 'skipped_budget', 'cases after budget exhaustion must be skipped')
        if outcome == 'skipped_budget':
            need(name not in rows and 'uptime_before_' + str(index) not in rows, 'skipped case has no command')
            exhausted = True
            continue
        need(name in rows and 'uptime_before_' + str(index) in rows, 'launched case commands')
        row = rows[name]
        argv = row['argv']
        need(argv[:2] == [payload.TIME, '-v'] and argv[2] == configure[4] + '/' + payload.PROBE_TARGET and
             argv[3].endswith('/' + case['file']) and argv[4:] == payload.expected_probe_tail(case),
             'exact GNU time / probe invocation')
        need(entry.get('exit_code') == row.get('exit_code'), 'case exit code record')
        if outcome in ('killed_case_cap', 'killed_budget'):
            need(row.get('session_deadline_reached') is True and row.get('residual_or_interrupted_group_killed') is True
                 and row.get('group_closed') is True, 'killed case record (group certified closed)')
            if outcome == 'killed_case_cap':
                need(row.get('elapsed_seconds', 0) >= payload.CASE_CAP_SECONDS - 1.0, 'case killed before its cap')
            else:
                exhausted = True
            need(set(entry) == {'index', 'outcome', 'exit_code', 'elapsed_seconds'} and
                 entry.get('elapsed_seconds') == row.get('elapsed_seconds'), 'killed case record fields')
        else:
            need(row.get('group_closed') is True and not row.get('residual_or_interrupted_group_killed') and
                 not row.get('session_deadline_reached'), 'probe closure')
            probe = payload.strict_json((output / (name + '.stdout')).read_bytes())
            need(payload.validate_probe(probe, case, row['exit_code']) == outcome, 'probe outcome recomputation')
            rss = payload.base.validate_gnu_time((output / (name + '.stderr')).read_text(errors='replace'),
                                                 row['exit_code'])
            need(entry == dict(index=index, exit_code=row['exit_code'], elapsed_seconds=row['elapsed_seconds'],
                               outcome=outcome, gnu_time_max_rss_kb=rss, pairs=probe['population']['pairs'],
                               gpu_total_ms=probe['gpu']['total_ms'],
                               cpu_filter_cache_ms=probe['cpu']['rect_ms'] + probe['cpu']['pair_cache_ms']),
                 'case summary differs from raw probe output / GNU time / command record')
        need((output / (name + '.summary.json')).is_file(), 'case summary file missing')
        summary = payload.strict_json((output / (name + '.summary.json')).read_bytes())
        need(summary == dict(case=case, input_file_sha256=manifest[case['file']], **entry), 'case summary file')
    completed = [i for i, entry in enumerate(outcomes) if entry['outcome'] == 'complete']
    need(value.get('completed_case_indices') == completed, 'completed case indices')
    status = payload.campaign_status(outcomes)
    need(status in RECEIVED_STATUSES, 'no GPU case measured')
    need(value['status'] == status, 'worker status recomputation')
    return status


def run_session(args):
    session = args.session_dir.absolute()
    key = args.ssh_key.absolute()
    need(session.is_dir() and not session.is_symlink() and session.stat().st_mode & 0o777 == 0o700, 'existing private session 0700')
    need(key.is_file() and not key.is_symlink() and key.stat().st_mode & 0o777 == 0o600 and
         Path(str(key) + '.pub').is_file() and not Path(str(key) + '.pub').is_symlink(), 'existing private/public key files')
    need(sha(__file__) == args.expected_controller_sha256, 'controller pin')
    need(args.worker_sha256 == sha(payload.__file__), 'worker must match this v9 GPU adapter')
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
    host = session / 'gpu_filter_v9_host'
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
                 backend='cuda_g4', scope=payload.SCOPE, GPU_executed=False, FULL_executed=False,
                 useful_budget_seconds=useful,
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
        prefix = '/tmp/ehgp-gpu-v9-' + nonce + '.'
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
        # The worker imports its pinned library (tower_worker_v9.py) from the
        # extracted, rehashed source tree; the worker checks both pins again.
        argv = ['env', 'PYTHONPATH=' + remote + '/source/gcp-migration', 'python3', remote + '/worker.py',
                '--source-root', remote + '/source',
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
                        state['GPU_executed'] = True
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
    return 0 if state['status'] in RECEIVED_STATUSES else 1


def require_committed_protocol(snapshot, snapshot_sha256):
    """Une vraie session n'emporte que le protocole GPU committe au commit du paquet."""
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
         'real session refused: GPU protocol files are not committed at the snapshot commit')
    import gpu_filter_snapshot_v9 as snapshot_builder
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
                              backend='cuda_g4', scope=payload.SCOPE, GCP_used=False,
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
