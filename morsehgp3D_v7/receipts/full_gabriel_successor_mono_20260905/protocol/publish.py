#!/usr/bin/env python3
"""Create-only publication of CLOSED NEW-only FULLv3 captures; inert by default.

No engine, compiler, subprocess, Git, GCP or old mathematical projection is
invoked. Failed/censored captures retain their original bytes and status.
The qualification reference and optional comparison packet are not rerun or
promoted as fresh tests by publication. Separate ROOT GO and pins are required.
Copy architecture: singleton mono publisher c3acdb8e18baa8d2737f0de1c3a33769bf0aeff1a639818439006fffa4473ef2.
"""
from __future__ import annotations

import argparse
from datetime import datetime
import hashlib
from pathlib import Path
import re
import types

ROOT = Path('/workspaces/E-HGP')
BASE = ROOT / 'build/v7_successor_20260905_mono_controller'
QBASE = ROOT / 'build/v7_successor_20260905_controller'
PUBLIC_REL = 'morsehgp3D_v7/receipts/full_gabriel_successor_mono_20260905'
COMMON = ROOT / 'build/v7_full_lazy_20260905_controller/publish.py'
COMMON_SHA = '5c7f18a2577ee388a8f9652c3596ffe9ab9ade6bbc3101ae45f18fc91da6dfba'
CHECKER = ROOT / 'tools/check_v7_receipt_publication.py'
CHECKER_SHA = '32420385f487260e0706b3e649befca25cc95a9d45f17d22472c333870729580'
CONTROLLER_SHA = 'a2a92ee2a84b7cec19f36654da3503e7e0f72f9c1125753e4567d5f3314748f6'
COMPARISON_PINS = {
    'controller_sha256': '830dfe61ec083497f8b7d5c14d424f8b5820a0fac5d3cb41f495efc5f36b69dd',
    'comparator_sha256': '15176b19ff7dd6b56c56470a887ecc9438fd673215cdefbd48f1a49953015718',
    'receipt_judge_sha256': '5de5b8d20d6073d5521a4217b8d504a59d255fad514eb8f4ddc257553ee0f032',
    'new_producer_sha256': '85c27ab91d7f159520a8db3098629447b0a213a134c5c042a86c585416847fad',
}
REPLAY_SHA = '96fa9d1afb80af171e3195b513149416035caf3cc851986352d8e63e7d004b63'
REPLAY_SOURCE_SHA = 'ea1b95177bf0a37feaa85ccfbaa79a770f9574adae4c734db90b587938980f97'
REPLAY_BINARY_SHA = '8ff0dd10bdc0b43d405abde53809242029ca5094be3c61b04120359af28b0780'
REPLAY_LABEL = 'n8000_s8_k10_lazy_c1000000'
INCIDENT = ['2026-09-05T21:54:33.912911+00:00', '2026-09-05T21:54:34.770106+00:00']


def require(ok, reason):
    if not ok:
        raise ValueError(reason)


def pin(value):
    return type(value) is str and re.fullmatch('[0-9a-f]{64}', value) is not None


def load(path, expected):
    require(path.is_file() and not path.is_symlink() and path.stat().st_size <= 16 << 20, 'protocol file bound')
    raw = path.read_bytes()
    require(pin(expected) and hashlib.sha256(raw).hexdigest() == expected, 'reviewed protocol pin')
    module = types.ModuleType('inert_successor_mono_publication_' + expected[:8])
    module.__file__ = str(path)
    exec(compile(raw, str(path), 'exec'), module.__dict__)
    return module, raw


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--execute', action='store_true')
    parser.add_argument('--expected-publisher-sha256')
    parser.add_argument('--expected-controller-sha256')
    for name in ('build', 'micro', 'heavy', 'qualification', 'comparisons', 'replay'):
        parser.add_argument('--' + name, metavar='PATH=SHA256')
    args = parser.parse_args()
    if not args.execute:
        print('prepared_not_executed: no capture read or publication; separate ROOT GO required')
        return 0
    require(pin(args.expected_publisher_sha256) and args.expected_controller_sha256 == CONTROLLER_SHA,
            'reviewed publisher/controller pins required')
    H, common_raw = load(COMMON, COMMON_SHA)
    # A bundle may contain micro + five heavy jobs + a separate qualification.
    # Keep the pinned Reader byte bounds (16 MiB/file, 128 MiB total); only the
    # explicit aggregate file-count bound is raised from 1024 to 4096.
    H.MAX_FILES = 4096
    reader, payload, watches, omissions, seen = H.Reader(), {}, [], [], set()
    captured = []
    N, controller_raw = load(BASE / 'capture.py', CONTROLLER_SHA)
    V, checker_raw = load(CHECKER, CHECKER_SHA)
    public = ROOT / PUBLIC_REL
    require(not public.exists() and not public.is_symlink() and not public.parent.is_symlink(), 'create-only target exists or symlink')
    for name, path, expected, raw in (
        ('publication_common.py', COMMON, COMMON_SHA, common_raw),
        ('capture.py', BASE / 'capture.py', CONTROLLER_SHA, controller_raw),
        ('check_v7_receipt_publication.py', CHECKER, CHECKER_SHA, checker_raw),
    ):
        require(reader.pinned(path, expected) == raw, 'protocol changed during import')
        payload['protocol/' + name] = raw
    payload['protocol/publish.py'] = reader.pinned(Path(__file__).absolute(), args.expected_publisher_sha256)

    def value(path):
        return H.json_value(reader.get(path))

    def equal(a, b, reason):
        require(H.encode(a) == H.encode(b), reason)

    def collect(argument, kind, prefix, scope=BASE, copy_all=True):
        written, separator, expected = (argument or '').rpartition('=')
        path = Path(written)
        require(separator and pin(expected) and path.is_absolute() and path.name == 'receipt.json'
                and '..' not in path.parts and path.is_relative_to(scope) and path not in seen, 'explicit unique closed receipt required')
        for parent in (path.parent, *path.parent.parents):
            if parent == scope.parent:
                break
            require(not parent.is_symlink(), 'capture ancestor symlink')
        seen.add(path)
        record = H.json_value(reader.pinned(path, expected))
        schema = {'build': N.SCHEMA, 'micro': N.SCHEMA, 'heavy': N.SCHEMA,
                  'qualification': 'mhgp7-successor-qualification-v1',
                  'comparison': 'mhgp7-successor-comparison-capture-v1',
                  'replay': 'mhgp7-successor-single-replay-v1'}[kind]
        require(record.get('kind') == kind and record.get('status') in ('completed', 'failed')
                and record.get('schema') == schema, 'closed capture kind/schema/status')
        started, ended = (record.get(key) for key in (('started_utc', 'ended_utc') if kind == 'qualification' else ('started', 'ended')))
        require(type(started) is str and type(ended) is str and datetime.fromisoformat(started) <= datetime.fromisoformat(ended), 'closed capture chronology')
        names = H.files_in(path.parent)
        require(type(record.get('artifacts')) is dict and names == set(record['artifacts']) | {'receipt.json'}, 'exact sealed artifact inventory')
        payload[prefix + '/receipt.json'] = reader.get(path)
        for name, expected_artifact in record['artifacts'].items():
            require(H.safe_name(name) and pin(expected_artifact), 'unsafe artifact/hash')
            target = path.parent / name
            raw = reader.pinned(target, expected_artifact)
            binary_target = path.parent / 'full_gabriel_lazy_probe'
            if raw.startswith(b'\x7fELF'):
                require(kind == 'build' and target == binary_target and len(omissions) == 0, 'unexpected ELF forbidden')
                if record['status'] == 'completed':
                    require(ROOT / record['binary'] == target and record['binary_sha256'] == H.sha(raw), 'exact build ELF binding')
                omissions.append({'path': str(target), 'sha256': H.sha(raw), 'bytes': len(raw),
                                  'captured_build_status': record['status'], 'reason': 'only_exact_probe_ELF_omitted'})
            elif copy_all:
                payload[prefix + '/' + name] = raw
        watches.append((path.parent, names))
        captured.append((path.parent, record))
        return path, record

    def commands(directory, record):
        for name in record['artifacts']:
            if not name.endswith('.command.json'):
                continue
            path, command = directory / name, value(directory / name)
            require(command['status'] in ('completed', 'failed', 'censored') and command.get('ended'), 'command not closed')
            intent = value(path.with_name(path.name.removesuffix('.command.json') + '.intent.json'))
            equal({key: command[key] for key in intent}, intent, 'command intent mirror')
            if command['status'] == 'completed':
                require(command['error'] is None and type(command['exit_code']) is int
                        and all(type(code) is int for code in command['expected_rc'])
                        and command['exit_code'] in command['expected_rc'], 'completed command exit binding')
            for stream, stamp in command['streams'].items():
                require(H.safe_name(stream) and '/' not in stream and type(stamp['bytes']) is int, 'stream inventory/type')
                raw = reader.get(path.parent / stream)
                require(stamp['bytes'] == len(raw) and stamp['sha256'] == H.sha(raw), 'raw stream byte/hash binding')

    def attempts(directory, record):
        found, qualified = {}, []
        for name in record['artifacts']:
            if not name.endswith('.extended_verdict.json'):
                continue
            ext, at = value(directory / name), (directory / name).parent
            label = ext['attempt_id']
            require(label not in found and ext['status'] in ('completed', 'failed') and ext.get('ended'), 'attempt verdict identity/closure')
            found[label] = ext
            if ext['status'] != 'completed':
                continue
            base, rec = value(at / (label + '.verdict.json')), value(at / (label + '.receipt.json'))
            cmd = value(at / (label + '.command.json'))
            equal({key: rec[key] for key in cmd}, cmd, 'engine receipt/command mirror')
            raw = reader.get(at / (label + '.raw.txt'))
            rows = [H.json_value(line.encode()) for line in raw.decode().splitlines() if line.startswith('{')]
            N.terminal_binding(ext, base, rec, rows)
            require(all(row['schema'] == N.PROBE_SCHEMA and row.get('successor_accounting') == N.ACCOUNTING for row in rows), 'v3 accounting on every raw row')
            for mode in ('normal', 'optimized'):
                for suffix, key in (('judge_', 'audit_status'), ('first_c_', 'supplement_status')):
                    label_j = label + '.' + suffix + mode
                    out, call = value(at / (label_j + '.stdout')), value(at / (label_j + '.command.json'))
                    require(call['status'] == 'completed' and call['exit_code'] == 0 and out[key] == 'valid'
                            and out['attempt_success'] is ext['attempt_success'] and out['probe_schema'] == N.PROBE_SCHEMA
                            and out['successor_accounting'] == N.ACCOUNTING, 'linked versioned judge result')
                    if suffix == 'first_c_':
                        require(out['requires_frozen_judge_sha256'] == N.JUDGE_SHA, 'first-C primary pin')
            qualified.append(ext)
        for reported in record.get('attempts', []):
            equal(reported, found[reported['attempt_id']], 'top-level attempt mirror')
        return found, qualified

    build_path, build = collect(args.build, 'build', 'build')
    commands(build_path.parent, build)
    before = value(build_path.parent / 'sources_before.json')
    require(before['files'][str((BASE / 'capture.py').relative_to(ROOT))] == CONTROLLER_SHA
            and before['probe_schema'] == N.PROBE_SCHEMA and before['successor_accounting'] == N.ACCOUNTING
            and H.sha(H.encode(before['files'])) == before['source_sha256'], 'build source/protocol pin')
    producer = before['files'][N.PRODUCER]
    micro = heavy = qualification = comparisons = replay = replay_summary = timing_exclusion = None
    heavy_valid = []
    require(not args.heavy or args.micro and args.qualification, 'heavy needs explicit micro and qualification')
    if args.micro:
        require(build['status'] == 'completed' and build['sources_stable'] is True and len(omissions) == 1, 'micro requires admitted build')
        micro_path, micro = collect(args.micro, 'micro', 'micro')
        equal(micro['build_argument'], args.build, 'micro/build receipt binding')
        commands(micro_path.parent, micro)
        found, valid = attempts(micro_path.parent, micro)
        if micro['status'] == 'completed':
            require(micro['sources_stable'] is True and len(found) == len(valid) == len(micro['attempts']) == 24
                    and all(row['attempt_success'] is True for row in valid) and micro['horizontal_orders'] == 156
                    and micro['parser_rejects'] == 11 and micro['primary_mutants_per_mode'] == 35
                    and micro['first_c_mutants_per_mode'] == 27, 'completed micro nonvacuum')
    if args.qualification:
        qpath, q = collect(args.qualification, 'qualification', 'qualification_reference', QBASE, False)
        require(q['status'] == 'completed' and q['development'] is False and q['errors'] == []
                and q['controller_sha256'] == N.QUALIFICATION_CONTROLLER_SHA and q['producer_sha256'] == producer
                and q['historical_results_reused'] is False and q['public_status'] == 'not_claimed', 'separate completed qualification reference')
        require(len(q['tests']) == 20 and len(q['targets']) == 8 and [p['mode'] for p in q['phases']] == ['release', 'san']
                and all(p['status'] == 'completed' and p['errors'] == [] and p['inventory']['tests'] == 20
                        and p['test_outputs']['fresh'] is True for p in q['phases']), 'separate 20-test phase reference')
        qualification = {'argument': args.qualification, 'producer_sha256': producer, 'tests_per_mode_reported': 20,
                         'all_sealed_artifacts_verified': True, 'tests_rerun_here': False, 'reference_only': True}
    if args.heavy:
        require(micro['status'] == 'completed', 'heavy requires successful micro admission')
        hpath, heavy = collect(args.heavy, 'heavy', 'heavy')
        admission, plan = value(hpath.parent / 'admission.json'), value(hpath.parent / 'protocol.json')
        require(admission['micro_argument'] == args.micro and admission['qualification_argument'] == args.qualification,
                'heavy admission receipt links')
        equal(plan['planned_sequence'], N.SEQUENCE, 'five-job heavy plan')
        require(plan['probe_schema'] == N.PROBE_SCHEMA and plan['successor_accounting'] == N.ACCOUNTING
                and plan['producer_sha256'] == producer and plan['binary_sha256'] == build['binary_sha256'], 'heavy protocol binding')
        commands(hpath.parent, heavy)
        found, heavy_valid = attempts(hpath.parent, heavy)
        expected = [f'n{n}_s{s}_k10_{policy}_c{cache}' for n, s, policy, cache in N.SEQUENCE]
        require(set(found) <= set(expected) and [r['attempt_id'] for r in heavy['attempts']] == expected[:len(heavy['attempts'])], 'captured heavy prefix identity')
        if heavy['status'] == 'completed':
            require(len(heavy['attempts']) == len(heavy_valid) == 5, 'completed heavy capture has five closed valid terminals')
            require(heavy['all_successful'] is all(r['attempt_success'] is True for r in heavy_valid), 'heavy success mirror')
        original_path = hpath.parent / (REPLAY_LABEL + '.receipt.json')
        if original_path.name in heavy['artifacts']:
            original = value(original_path)
            require(datetime.fromisoformat(original['started']) <= datetime.fromisoformat(INCIDENT[1])
                    and datetime.fromisoformat(INCIDENT[0]) <= datetime.fromisoformat(original['ended']), 'declared original incident overlap')
            original_valid = any(row['attempt_id'] == REPLAY_LABEL and row['attempt_success'] is True for row in heavy_valid)
            timing_exclusion = {'attempt_argument': str(original_path) + '=' + heavy['artifacts'][original_path.name],
                'incident_authority': 'ROOT_declared_auditor_CPU0_overlap', 'incident_interval_utc': INCIDENT,
                'original_interval_utc': [original['started'], original['ended']], 'timing_eligible': False,
                'functional_capture_valid_and_successful': original_valid, 'original_bytes_unchanged': True}
    if args.comparisons:
        require(args.heavy is not None, 'comparison packet requires explicit heavy binding')
        cpath, comparisons = collect(args.comparisons, 'comparison', 'comparisons')
        require(comparisons['build_argument'] == args.build and comparisons['micro_argument'] == args.micro
                and comparisons['heavy_argument'] == args.heavy and comparisons['historical_timing_pair_claim'] is False,
                'optional comparison packet binding/scope')
        require(all(comparisons[key] == expected for key, expected in COMPARISON_PINS.items())
                and comparisons['new_producer_sha256'] == producer and comparisons['engine_invoked'] is False
                and comparisons['global_FULL_successful'] is False and comparisons['public_status'] == 'not_claimed',
                'comparison pins and non-promotion scope')
        for filename, key in (('compare_capture.py', 'controller_sha256'), ('compare.py', 'comparator_sha256'), ('primary_judge.py', 'receipt_judge_sha256')):
            require(H.sha(reader.get(cpath.parent / 'protocol' / filename)) == COMPARISON_PINS[key], 'comparison protocol bytes')
        commands(cpath.parent, comparisons)
        if comparisons['status'] == 'completed':
            require(comparisons['inputs_stable'] is True, 'completed comparison needs stable inputs')
            equal(value(cpath.parent / 'inputs_observed.json'), value(cpath.parent / 'sources_after.json'), 'completed comparison input closure')
    if args.replay:
        require(args.heavy and timing_exclusion and timing_exclusion['functional_capture_valid_and_successful'] is True,
                'replay needs the exact excluded successful original in the explicit heavy packet')
        rpath, replay = collect(args.replay, 'replay', 'replay')
        for key, expected_value in {'build_argument': args.build, 'micro_argument': args.micro,
                'qualification_argument': args.qualification, 'excluded_attempt_argument': timing_exclusion['attempt_argument'],
                'controller_sha256': CONTROLLER_SHA, 'replay_sha256': REPLAY_SHA,
                'comparator_sha256': COMPARISON_PINS['comparator_sha256'], 'receipt_judge_sha256': N.JUDGE_SHA,
                'binary': build['binary'], 'binary_sha256': REPLAY_BINARY_SHA, 'planned_heavy_jobs': 1,
                'incident_authority': timing_exclusion['incident_authority'], 'incident_interval_utc': INCIDENT,
                'excluded_timing_eligible': False, 'excluded_functional_result_retained': True,
                'quiet_window_declared_by_ROOT': True, 'quiet_window_independently_certified': False,
                'timing_eligibility': 'requires_ROOT_review_after_capture', 'original_campaign_modified': False,
                'historical_timing_pair_claim': False, 'global_FULL_successful': False,
                'integrated_inter_k_tower': False, 'public_status': 'not_claimed', 'gcp_used': False}.items():
            equal(replay[key], expected_value, 'replay exact field: ' + key)
        require(before['source_sha256'] == REPLAY_SOURCE_SHA and build['binary_sha256'] == REPLAY_BINARY_SHA,
                'replay frozen source and binary')
        equal(replay['qualification'], {'path': str(qpath), 'sha256': args.qualification.rpartition('=')[2],
            'source_sha256': q['source_sha256'], 'producer_sha256': producer, 'tests_per_mode': 20}, 'replay qualification reference mirror')
        def replay_artifact(name):
            if name in replay['artifacts']:
                return rpath.parent / name
            require(replay['status'] == 'failed', 'completed replay missing artifact: ' + name)
            return None
        for filename, expected_pin in (('replay.py', REPLAY_SHA), ('compare.py', COMPARISON_PINS['comparator_sha256']),
                                       ('full_gabriel_lazy_probe_audit.py', N.JUDGE_SHA)):
            target = replay_artifact('replay_protocol/' + filename)
            if target is not None:
                require(H.sha(reader.get(target)) == expected_pin, 'replay copied protocol bytes')
        admission_path, protocol_path = replay_artifact('admission.json'), replay_artifact('protocol.json')
        if admission_path is not None:
            equal(value(admission_path), {'build_argument': args.build, 'micro_argument': args.micro,
                'qualification_argument': args.qualification, 'excluded_attempt_argument': timing_exclusion['attempt_argument'],
                'planned_heavy_jobs': 1, 'replay_schema': 'mhgp7-successor-single-replay-v1',
                'original_campaign_lock': str(hpath.parent / 'session.lock')}, 'replay exact admission')
        if protocol_path is not None:
            equal(value(protocol_path), dict(plan, planned_sequence=[[8000, 8, 'lazy', 1000000]]), 'one-job replay preserves all other protocol fields')
        commands(rpath.parent, replay)
        rfound, rvalid = attempts(rpath.parent, replay)
        require(set(rfound) <= {REPLAY_LABEL} and type(replay['attempts']) is list and len(replay['attempts']) <= 1,
                'only one declared replay attempt')
        functional = replay['functional_comparison']
        if functional.get('status') == 'valid':
            expected_functional = {'status': 'valid', 'scope': 'complete_v3_v3_nonmeasure_fields', 'orders_compared': 10,
                'order_measure_exclusions': sorted('build_ms digest_ms expand_ms read_ms release_ms rss_mib_sample hwm_mib_sample'.split()),
                'terminal_measure_exclusions': sorted(('compute_read_release_ms_subtracted_diagnostic digest_ms elapsed_before_terminal_ms '
                    'generation_rects_ms generation_wspd_ms provisional_output_ms rss_mib_sample hwm_mib_sample stage_ms').split()),
                'successor_steps_unchanged': True, 'affine_accounting_transform_applied': False,
                'timing_ratio_computed': False, 'global_FULL_successful': False}
            equal(functional, expected_functional, 'complete v3-v3 comparison scope')
            comparison_path = replay_artifact('functional_comparison.json')
            if comparison_path is not None:
                equal(value(comparison_path), functional, 'replay functional result mirror')
        else:
            equal(functional, {'status': 'not_run'}, 'no replay comparison promotion')
            require('functional_comparison.json' not in replay['artifacts'], 'unexpected replay comparison artifact')
        if replay['status'] == 'completed':
            require(replay['sources_stable'] is True and len(rfound) == len(rvalid) == len(replay['attempts']) == 1,
                    'completed replay needs exactly one valid terminal')
            require({'sources_before.json', 'sources_after.json'} <= set(replay['artifacts']), 'completed replay snapshot inventory')
            require((functional.get('status') == 'valid') is rvalid[0]['attempt_success'], 'replay functional result and terminal outcome')
        replay_success = bool(replay['status'] == 'completed' and replay.get('sources_stable') is True and len(rvalid) == 1
                              and rvalid[0]['attempt_success'] is True and functional.get('status') == 'valid')
        replay_summary = {'argument': args.replay, 'status': replay['status'], 'planned_jobs': 1,
            'same_binary_functional_replay_successful': replay_success, 'comparison': functional,
            'functional_comparison_requalified_here': False, 'timing_eligibility': replay['timing_eligibility'],
            'quiet_window_independently_certified': False, 'historical_comparison_is_separate': True,
            'replaces_original_five_job_capture': False, 'global_FULL_successful': False}
    for directory, names in watches:
        if 'sources_before.json' in names and 'sources_after.json' in names:
            rec = value(directory / 'receipt.json')
            if rec.get('sources_stable') is True:
                equal(value(directory / 'sources_before.json'), value(directory / 'sources_after.json'), 'declared source stability')
        for name in names:
            if name.endswith('protocol_sources/capture.py'):
                require(H.sha(reader.get(directory / name)) == CONTROLLER_SHA, 'captured controller pin')
    for directory, record in captured:
        if record['kind'] not in ('build', 'micro', 'heavy', 'replay') or record['status'] != 'completed':
            continue
        require(record['sources_stable'] is True, 'completed capture needs stable sources')
        for name in record['artifacts']:
            if name.endswith(('sources_before.json', 'sources_after.json')):
                snap = value(directory / name)
                equal(snap['files'], before['files'], 'completed captured source map differs from build')
                require(snap['source_sha256'] == before['source_sha256'] and snap['probe_schema'] == N.PROBE_SCHEMA
                        and snap['successor_accounting'] == N.ACCOUNTING, 'completed snapshot identity')
                if 'binary' in snap:
                    require(snap['binary'] == build['binary'] and snap['binary_sha256'] == build['binary_sha256'], 'completed snapshot binary binding')
    all_heavy = bool(heavy and heavy['status'] == 'completed' and heavy.get('sources_stable') is True
                     and len(heavy_valid) == 5 and all(row['attempt_success'] is True for row in heavy_valid))
    payload['publication.json'] = H.encode({'schema': 'mhgp7-successor-mono-publication-v1',
        'status': 'captured_bytes_verified_only', 'build_argument': args.build, 'micro_argument': args.micro,
        'heavy_argument': args.heavy, 'comparisons_argument': args.comparisons, 'producer_sha256': producer,
        'excluded_original_timing': timing_exclusion, 'single_replay': replay_summary,
        'captured_statuses': {key: row['status'] if row else None for key, row in (('build', build), ('micro', micro), ('heavy', heavy))},
        'planned_heavy_jobs': 5, 'closed_valid_heavy_terminals': len(heavy_valid),
        'all_planned_heavy_attempts_successful': all_heavy, 'prefix_promoted_to_global_success': False,
        'qualification_reference': qualification, 'comparisons_requalified_here': False, 'omissions': omissions,
        'engine_or_judge_rerun': False, 'historical_results_reused_as_fresh': False, 'public_status': 'not_claimed',
        'historical_timing_pair_claim': False, 'integrated_inter_k_tower': False, 'gcp_used': False})
    payload['README.md'] = ('# Captures mono FULLv3 — normalisation des successeurs\n\n'
        'Publication de captures fermées vérifiées par leurs octets, pas une requalification globale. '
        'Les refus, censures et campagnes partielles restent tels quels ; un préfixe ne devient jamais un succès global.\n\n'
        'Build, micro et heavy conservent les commandes, sorties brutes, protocoles, jugements liés et sceaux originaux. '
        'Seul le binaire ELF exact de la sonde est omis sous hash. La qualification ciblée 20 tests est une référence séparée ; '
        'ses artefacts sont vérifiés, mais seul son reçu est recopié ici. Les comparaisons facultatives ne sont pas rejugées.\n\n'
        'Aucun ancien résultat n’est présenté comme frais ; aucun ratio chronométrique historique, contrat 50k/1 s/100 ms, '
        'tour inter-K intégrée ou résultat massif G4 n’en découle. public_status=not_claimed. GCP non utilisé.\n\n'
        '[Statuts exacts](publication.json), [inventaire](manifest.json), [sommes](SHA256SUMS).\n').encode()
    if timing_exclusion:
        payload['README.md'] += ('\nLe temps original 8k/s=8 est exclu pour chevauchement CPU déclaré ; ses octets et son résultat fonctionnel restent conservés. '
            'Le rejeu facultatif reste une capture distincte d’un seul passage. Son égalité fonctionnelle v3-v3, tous compteurs conservés, '
            'est séparée des comparaisons historiques v1-v2. La publication ne certifie pas indépendamment sa fenêtre chronométrique.\n').encode()
    payload['manifest.json'] = H.encode({name: {'bytes': len(raw), 'sha256': H.sha(raw)} for name, raw in sorted(payload.items())})
    payload['SHA256SUMS'] = ''.join(H.sha(raw) + '  ' + name + '\n' for name, raw in sorted(payload.items())).encode()
    def fetch(path):
        require(path.startswith(PUBLIC_REL + '/'), 'manifest escaped publication')
        return payload[path[len(PUBLIC_REL) + 1:]]
    V.verify_manifest(PUBLIC_REL + '/SHA256SUMS', payload['SHA256SUMS'], fetch)
    reader.recheck()
    require(all(H.files_in(path) == names for path, names in watches), 'closed inventory drift')
    public.mkdir()
    for name, raw in sorted(payload.items()):
        target = public / name
        target.parent.mkdir(parents=True, exist_ok=True)
        with target.open('xb') as stream:
            stream.write(raw)
        require(target.read_bytes() == raw, 'published bytes differ')
    reader.recheck()
    require(H.files_in(public) == set(payload), 'published inventory differs')
    print('published', public, 'files', len(payload), 'SHA256SUMS', H.sha(payload['SHA256SUMS']))
    return 0


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, ValueError, KeyError, TypeError) as error:
        print('Successor mono publication refused:', error)
        raise SystemExit(1)
