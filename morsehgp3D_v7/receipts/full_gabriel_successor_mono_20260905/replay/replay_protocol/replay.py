#!/usr/bin/env python3
"""Inert ONE-attempt replay of 8k/s8; separate ROOT GO and quiet-window declaration.

No build, five-job closure, publication or old timing ratio is performed.
The unchanged a2a92 controller supplies admissions, one probe, readers and seal.
The original campaign lock also excludes overlap with its remaining jobs.
An abrupt unhandled process loss requires a separate read-only recovery review;
it is not silently retried and no missing terminal is invented.
"""
from __future__ import annotations

import argparse
import fcntl
import hashlib
import json
from pathlib import Path
import re
import signal
import types

ROOT = Path('/workspaces/E-HGP')
BASE = ROOT / 'build/v7_successor_20260905_mono_controller'
CONTROLLER = BASE / 'capture.py'
CONTROLLER_SHA = 'a2a92ee2a84b7cec19f36654da3503e7e0f72f9c1125753e4567d5f3314748f6'
COMPARATOR_SHA = '15176b19ff7dd6b56c56470a887ecc9438fd673215cdefbd48f1a49953015718'
SOURCE_SHA = 'ea1b95177bf0a37feaa85ccfbaa79a770f9574adae4c734db90b587938980f97'
BINARY_SHA = '8ff0dd10bdc0b43d405abde53809242029ca5094be3c61b04120359af28b0780'
SCHEMA = 'mhgp7-successor-single-replay-v1'
SEQUENCE = [[8000, 8, 'lazy', 1000000]]
LABEL = 'n8000_s8_k10_lazy_c1000000'


def require(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def reference(argument: str) -> tuple[Path, str]:
    name, separator, pin = argument.rpartition('=')
    path = Path(name)
    require(separator and re.fullmatch('[0-9a-f]{64}', pin) is not None
            and path.is_absolute() and path.is_relative_to(BASE) and '..' not in path.parts,
            'explicit owned receipt PATH=SHA256 required')
    require(all(not p.is_symlink() for p in (path, *path.parents)), 'reference symlink')
    require(path.is_file() and path.stat().st_size <= 1 << 20 and digest(path) == pin,
            'reference pin/size differs')
    return path, pin


def compare_complete(M: object, left: tuple, right: tuple) -> dict:
    for config, record, _ in (left, right):
        M.coherent_model(config, record, 1)
        require(record['exit_code'] == 0, 'complete v3 replay comparison requires both successes')
    M.equal(left[0], right[0], 'replay configuration differs')
    M.equal(left[2], right[2], 'replay sources differ')
    require(len(left[1]['orders']) == len(right[1]['orders']) == 10, 'ten replay orders required')
    project = lambda row, excluded: {key: value for key, value in row.items() if key not in excluded}
    for a, b in zip(left[1]['orders'], right[1]['orders']):
        M.equal(project(a, M.ORDER_MEASURES), project(b, M.ORDER_MEASURES), 'replay order differs')
    M.equal(project(left[1]['terminal'], M.TERMINAL_MEASURES),
            project(right[1]['terminal'], M.TERMINAL_MEASURES), 'replay terminal differs')
    return dict(status='valid', scope='complete_v3_v3_nonmeasure_fields', orders_compared=10,
                order_measure_exclusions=sorted(M.ORDER_MEASURES),
                terminal_measure_exclusions=sorted(M.TERMINAL_MEASURES),
                successor_steps_unchanged=True, affine_accounting_transform_applied=False,
                timing_ratio_computed=False, global_FULL_successful=False)


def execute(args: argparse.Namespace) -> None:
    own = Path(__file__).resolve()
    require(re.fullmatch('[0-9a-f]{64}', args.expected_replay_sha256 or '') is not None
            and digest(own) == args.expected_replay_sha256, 'reviewed replay SHA required')
    require(args.expected_source_sha256 == SOURCE_SHA and args.go_reviewed_heavy
            and args.go_auditor_closed, 'same sources, explicit heavy GO and quiet-window declaration required')
    require(digest(CONTROLLER) == CONTROLLER_SHA, 'imported controller differs')
    source = CONTROLLER.read_bytes()
    require(hashlib.sha256(source).hexdigest() == CONTROLLER_SHA, 'controller changed during import')
    N = types.ModuleType('inert_single_successor_replay')
    N.__file__ = str(CONTROLLER)
    exec(compile(source, str(CONTROLLER), 'exec'), N.__dict__)
    protocols = {}
    def module(path: Path, pin: str) -> object:
        raw = path.read_bytes()
        require(hashlib.sha256(raw).hexdigest() == pin, 'pure comparison protocol pin')
        protocols[path] = (pin, raw)
        value = types.ModuleType('inert_replay_pure_' + pin[:8])
        value.__file__ = str(path)
        exec(compile(raw, str(path), 'exec'), value.__dict__)
        return value
    M = module(BASE / 'compare.py', COMPARATOR_SHA)
    J = module(ROOT / N.JUDGE, N.JUDGE_SHA)
    reader = M.Reader()
    def load_arm(path: Path, pin: str) -> tuple:
        plan, snapshot = path.parent / 'protocol.json', path.with_name(LABEL + '.sources_before.json')
        return M.load_arm(reader, str(path) + '=' + pin, str(plan) + '=' + digest(plan),
                          str(snapshot) + '=' + digest(snapshot), c.PINS[N.PRODUCER], None, J)
    excluded, excluded_pin = reference(args.excluded_attempt)
    require(excluded.name == LABEL + '.receipt.json', 'only the declared 8k/s8 attempt may be replayed')
    # Read-only advisory lock, shared with original attempts. Never touch their
    # captures or attempt to interrupt another job. Busy means refuse before work.
    lock_path = excluded.parent / 'session.lock'
    require(lock_path.is_file() and not lock_path.is_symlink(), 'original campaign lock missing')
    with lock_path.open('rb') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        ctl = N.Controller(CONTROLLER_SHA, SOURCE_SHA)
        c = ctl.c
        for sig in c.SIGNALS:
            signal.signal(sig, c.interrupted)
        _, micro = ctl.admission(args.micro, 'micro')
        _, build = ctl.admission(micro['build_argument'], 'build')
        require(micro['binary'] == build['binary'] and micro['binary_sha256'] == build['binary_sha256'] == BINARY_SHA,
                'replay must use the exact admitted binary')
        require(len(micro['attempts']) == 24 and micro['horizontal_orders'] == 156
                and micro['parser_rejects'] == 11 and micro['primary_mutants_per_mode'] == 35
                and micro['first_c_mutants_per_mode'] == 27, 'complete micro admission required')
        qualified = ctl.qualification(args.qualification)
        binary = ROOT / build['binary']
        old = ctl.read_attempt(excluded.parent, 0, binary, c.protocol(binary, 10, N.SEQUENCE))
        require(old['attempt_success'] is True and digest(excluded) == excluded_pin,
                'excluded time must retain its pinned successful functional capture')
        old_arm = load_arm(excluded, excluded_pin)
        M.coherent_model(old_arm[0], old_arm[1], 1)
        before = ctl.snapshot(binary)
        directory = c.new_directory('replay', args.id)
        result = dict(kind='replay', status='failed', started=c.now(), attempts=[],
                      planned_heavy_jobs=1, global_FULL_successful=False,
                      binary=build['binary'], binary_sha256=BINARY_SHA,
                      controller_sha256=CONTROLLER_SHA, replay_sha256=args.expected_replay_sha256,
                      comparator_sha256=COMPARATOR_SHA, receipt_judge_sha256=N.JUDGE_SHA,
                      functional_comparison={'status': 'not_run'},
                      micro_argument=args.micro, build_argument=micro['build_argument'],
                      qualification_argument=args.qualification, qualification=qualified,
                      excluded_attempt_argument=args.excluded_attempt,
                      excluded_timing_eligible=False, excluded_functional_result_retained=True,
                      incident_authority='ROOT_declared_auditor_CPU0_overlap',
                      incident_interval_utc=['2026-09-05T21:54:33.912911+00:00', '2026-09-05T21:54:34.770106+00:00'],
                      quiet_window_declared_by_ROOT=True, quiet_window_independently_certified=False,
                      timing_eligibility='requires_ROOT_review_after_capture',
                      original_campaign_modified=False, historical_timing_pair_claim=False,
                      integrated_inter_k_tower=False, public_status='not_claimed', gcp_used=False)
        try:
            plan = c.protocol(binary, 10, SEQUENCE)
            c.prepare_directory(directory, plan, binary)
            saved = directory / 'replay_protocol'
            saved.mkdir()
            with (saved / 'replay.py').open('xb') as stream:
                stream.write(own.read_bytes())
            require(digest(saved / 'replay.py') == args.expected_replay_sha256, 'copied replay pin differs')
            for path, (pin, raw) in protocols.items():
                with (saved / path.name).open('xb') as stream:
                    stream.write(raw)
                require(digest(saved / path.name) == pin, 'copied pure protocol differs')
            c.metadata(directory)
            c.save(directory / 'admission.json', dict(micro_argument=args.micro,
                   build_argument=micro['build_argument'], qualification_argument=args.qualification,
                   excluded_attempt_argument=args.excluded_attempt, planned_heavy_jobs=1,
                   replay_schema=SCHEMA, original_campaign_lock=str(lock_path)))
            N.same_typed(c.read_json(directory / 'protocol.json'), c.protocol(binary, 10, SEQUENCE),
                         'one-job protocol differs')
            N.same_typed(c.read_json(directory / 'sources_before.json'), ctl.snapshot(binary), 'replay source drift')
            require(digest(own) == args.expected_replay_sha256 and digest(excluded) == excluded_pin,
                    'pre-engine replay/reference drift')
            observed = ctl.probe(directory, 0)
            result['attempts'].append(observed)
            require(observed['status'] == 'completed', 'invalid/interrupted/censored replay')
            N.same_typed(ctl.read_attempt(directory, 0, binary, plan), observed, 'closed replay binding')
            if observed['attempt_success']:
                current = directory / (LABEL + '.receipt.json')
                result['functional_comparison'] = compare_complete(M, old_arm, load_arm(current, digest(current)))
                c.save(directory / 'functional_comparison.json', result['functional_comparison'])
            result.update(status='completed',
                          reason='one_closed_replay_not_five_jobs_or_a_performance_contract')
        except BaseException as error:
            result['reason'] = type(error).__name__ + ': ' + str(error)
        finally:
            try:
                require(digest(own) == args.expected_replay_sha256 and digest(CONTROLLER) == CONTROLLER_SHA
                        and digest(excluded) == excluded_pin, 'terminal replay/reference drift')
                require(all(digest(path) == pin for path, (pin, _) in protocols.items()), 'pure protocol drift')
                reader.stable()
            except BaseException as error:
                result.update(status='failed', reason=type(error).__name__ + ': ' + str(error))
            # Only the isolated module's seal schema changes, never capture.py
            # or its five-job functions. Exactly one replay has its own authority.
            c.SCHEMA = SCHEMA
            c.seal(directory, result, before, binary)
        print(json.dumps(result, sort_keys=True))
        require(result['status'] == 'completed', 'single replay capture failed')


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--execute', action='store_true')
    parser.add_argument('--go-reviewed-heavy', action='store_true')
    parser.add_argument('--go-auditor-closed', action='store_true')
    for name in ('id', 'expected-replay-sha256', 'expected-source-sha256', 'micro',
                 'qualification', 'excluded-attempt'):
        parser.add_argument('--' + name)
    args = parser.parse_args()
    if not args.execute:
        print(json.dumps(dict(status='prepared_not_executed', requires_ROOT_GO=True,
                              replay_sha256=digest(Path(__file__)), planned_heavy_jobs=1)))
        return
    require(all(value is not None for value in vars(args).values()), 'explicit replay arguments required')
    execute(args)


if __name__ == '__main__':
    try:
        main()
    except BaseException as error:
        if isinstance(error, SystemExit):
            raise
        print(json.dumps(dict(status='failed', reason=type(error).__name__ + ': ' + str(error))))
        raise SystemExit(1)
