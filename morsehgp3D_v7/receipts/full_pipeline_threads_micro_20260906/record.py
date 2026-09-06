#!/usr/bin/env python3
"""Small, opt-in n8 capture; no compiler, benchmark or completeness oracle."""
import argparse
from datetime import datetime, timezone
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import resource
import signal
import subprocess
import sys
import time

ROOT = Path('/workspaces/E-HGP')
BUILD = ROOT / 'build/v7_full_pipeline_threads_20260906'
BINARY_SHA = '4f5ba475ae5075cabab6c84222742b2dcdae226a0b3879e460b7fb8200b76aff'
DEP_SHA = 'a198c74088e0128cb4ceba55f0ed4e1e95ae3382c2e54e8a1aa55ce481082c03'
PINS = {
    'morsehgp3D_v7/bench/full_gabriel_lazy_probe.cpp': '7a0867657fad131e12ce0035a33e46f860c7ac00fcb667ce738300045b81b30c',
    'morsehgp3D_v7/bench/run_full_probe.py': '4157c203d4490f00a6692b3379feb04f691bd49d6971c44c87ee4ccab14d0040',
    'morsehgp3D_v7/bench/full_gabriel_lazy_probe_audit.py': '040f738770ea2f141a2d0da80872c2a62118ad9d1e81dd6040066c10b8519a14',
}
PRIMARY = 'morsehgp3D_v7/bench/full_gabriel_lazy_probe_audit.py'
RUNNER = 'morsehgp3D_v7/bench/run_full_probe.py'
PARALLEL = {'parallel_profile', 'pipeline_threads', 'full_order_builder_threads', 'order_schedule'}
ORDER_MEASURES = {'expand_ms', 'build_ms', 'read_ms', 'digest_ms', 'release_ms', 'rss_mib_sample', 'hwm_mib_sample'}
TERMINAL_MEASURES = {'stage_ms', 'generation_wspd_ms', 'generation_rects_ms', 'elapsed_before_terminal_ms',
                     'provisional_output_ms', 'digest_ms', 'compute_read_release_ms_subtracted_diagnostic',
                     'rss_mib_sample', 'hwm_mib_sample'}
WORKERS = {'workers_wspd', 'workers_rects', 'workers_sort', 'workers_prefilter', 'workers_census', 'workers_expand'}
DIRECT_REJECTS = [ ['--threads=0'], ['--threads=2147483648'], ['--threads=18446744073709551616'],
                   ['--threads=1', '--threads=1'], ['--threads=1', '--threads=2'],
                   ['--threads='], ['--threads=-1'], ['--threads=1.5'], ['--threads=+1'] ]
RUNNER_REJECTS = [ ['--threads', '1', '--threads', '1'], ['--cpu-list', '0', '--cpu-list', '0'],
                   ['--cpu', '0', '--cpu-list', '0'], ['--threads', '2'],
                   ['--threads', '0'], ['--threads', '-1'], ['--cpu-list', 'bad'] ]


def need(ok, reason):
    if not ok:
        raise ValueError(reason)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def save(path, value):
    with Path(path).open('x') as stream:
        json.dump(value, stream, indent=2, sort_keys=True)
        stream.write('\n')


def copy(source, target):
    target.parent.mkdir(parents=True, exist_ok=True)
    raw = source.read_bytes()
    with target.open('xb') as stream:
        stream.write(raw)
    need(target.read_bytes() == raw, 'copy_changed')


def typed(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':'), allow_nan=False)


def project(row, removed):
    return {key: value for key, value in row.items() if key not in removed}


def compare(outdir):
    """Read raw captures only; no attempt to feed v6 to the whole mono judge."""
    primary_path = outdir / 'sources' / PRIMARY
    need(sha(primary_path) == PINS[PRIMARY], 'primary_pin')
    spec = importlib.util.spec_from_file_location('mono_primary', primary_path)
    primary = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(primary)
    arms, diagnostics = {}, {}
    for threads in (None, 1, 2, 4, 8):
        name = 'default' if threads is None else 't' + str(threads)
        rows = [primary.loads(line) for line in (outdir / 'logs' / (name + '.stdout')).read_text().splitlines()]
        need([row['type'] for row in rows] == ['configuration'] + ['order'] * 8 + ['terminal'], 'ten_rows:' + name)
        primary.numeric(rows)
        config, terminal = rows[0], rows[-1]
        expected_schema = 'mhgp7-full-gabriel-probe-v' + ('5' if threads is None else '6')
        need(all(row['schema'] == expected_schema for row in rows), 'schema:' + name)
        need(config['threads'] == (threads or 1) and config['n'] == terminal['n'] == 8 and
             config['s'] == terminal['s'] == 8 and config['kmax_requested'] == terminal['kmax_requested'] == 10 and
             config['kmax_effective'] == terminal['kmax_effective'] == 8, 'configuration:' + name)
        need(terminal['terminal_status'] == 'completed' and terminal['exit_code'] == 0 and
             terminal['outcome'] == 'complete_relative' and terminal['complete_requested_horizontal_orders'] is True and
             terminal['completed_orders_diagnostic'] == 8 and terminal['certificate_retained'] is False, 'terminal:' + name)
        need([row['k'] for row in rows[1:-1]] == list(range(1, 9)) and
             all(row['outcome'] == 'complete_relative' and row['whole_tower_authority'] is False for row in rows[1:-1]), 'orders:' + name)
        need(all(row['cache_entries'] == 1000000 and row['alias_policy'] == 'lazy_first_c_strict_resolutions_v1' and
                 row['meb_proposal_budget_kind'] == 'unlimited' and row['max_meb_proposal_supports_per_order'] == (1 << 64)-1
                 for row in rows), 'policy:' + name)
        need(primary.is_digest(terminal['input_digest']) and
             all(primary.is_digest(row['certificate_digest']) for row in rows[1:-1]) and
             terminal['certificate_digest'] == primary.aggregate_digest(terminal['input_digest'],
                 [row['certificate_digest'] for row in rows[1:-1]]), 'digest_binding:' + name)
        need(WORKERS.issubset(terminal) and all(type(terminal[key]) is int and 0 <= terminal[key] <= (threads or 1)
                                              for key in WORKERS), 'workers:' + name)
        protocol = {key: config[key] for key in ('successor_accounting', 'meb_accounting', 'limits_profile')}
        protocol['probe_schema'] = expected_schema
        if threads is None:
            need(all(not PARALLEL.intersection(row) for row in rows), 'default_mono_wire')
            need(primary.accounting_binding(rows, protocol) == expected_schema, 'mono_schema_positive')
        else:
            need(all(row['parallel_profile'] == 'pipeline_workers_full_order_serial_v1' and
                     row['pipeline_threads'] == threads and row['full_order_builder_threads'] == 1 and
                     row['order_schedule'] == 'sequential_k1_to_kmax' for row in rows), 'parallel_declaration:' + name)
            try:
                primary.accounting_binding(rows, protocol)
            except ValueError as error:
                need(str(error) == 'schema', 'mono_reject_wrong_cause:' + name)
            else:
                raise ValueError('mono_reader_silently_accepts_v6:' + name)
        if arms:
            old = arms['default']
            need(typed(project(config, PARALLEL | {'schema', 'threads'})) ==
                 typed(project(old[0], PARALLEL | {'schema', 'threads'})), 'config_pair:' + name)
            need(typed([project(row, PARALLEL | {'schema'} | ORDER_MEASURES) for row in rows[1:-1]]) ==
                 typed([project(row, PARALLEL | {'schema'} | ORDER_MEASURES) for row in old[1:-1]]), 'order_pair:' + name)
            for key in ('input_digest', 'certificate_digest', 'last_order_work', 'raw_candidates', 'unique_candidates',
                        'census_balls', 'pair_mass_expected_per_lane', 'frontier_ledger_closed', 'rank_window_regular',
                        'ledger_q2_emitted', 'ledger_q3_emitted', 'ledger_q4_emitted',
                        'ledger_q2_killed', 'ledger_q3_killed', 'ledger_q4_killed'):
                need(typed(terminal[key]) == typed(old[-1][key]), 'terminal_pair:' + name + ':' + key)
            ignored = PARALLEL | {'schema'} | TERMINAL_MEASURES
            diagnostics[name] = {key: [old[-1].get(key), terminal.get(key)] for key in sorted(set(old[-1]) | set(terminal))
                                 if key not in ignored and typed(old[-1].get(key)) != typed(terminal.get(key))}
        arms[name] = rows
    return dict(status='passed', public_status='not_claimed', arms=5, orders_per_arm=8, paired_orders=32,
                input_and_all_order_and_final_digests_equal=True, full_order_nonmeasurement_fields_equal=True,
                mono_schema_positive=1, mono_schema_v6_rejections=4,
                observed_workers={name: {key: rows[-1][key] for key in sorted(WORKERS)} for name, rows in arms.items()},
                other_terminal_differences=diagnostics,
                scope='small_same_binary_functional_comparison_not_completeness_or_parallel_speedup')


def command(outdir, label, argv, expected, rows):
    started = time.monotonic()
    row = dict(label=label, argv=argv, cwd=str(ROOT), expected_exit=expected,
               started_utc=datetime.now(timezone.utc).isoformat(), wall_limit_seconds=60,
               cpu_limit_seconds=45, file_limit_bytes=64 << 20, pid=None, exit_code=None, group_closed=False)
    save(outdir / 'logs' / (label + '.intent.json'), row)
    process = None
    def limits():
        resource.setrlimit(resource.RLIMIT_CPU, (45, 45))
        resource.setrlimit(resource.RLIMIT_FSIZE, (64 << 20, 64 << 20))
    try:
        with (outdir / 'logs' / (label + '.stdout')).open('xb') as out, (outdir / 'logs' / (label + '.stderr')).open('xb') as err:
            process = subprocess.Popen(argv, cwd=ROOT, stdin=subprocess.DEVNULL, stdout=out, stderr=err,
                                       start_new_session=True, preexec_fn=limits)
            row['pid'] = process.pid
            row['exit_code'] = process.wait(timeout=60)
    finally:
        if process is not None:
            try:
                os.killpg(process.pid, 0)
                row['forced_group_kill'] = True
                os.killpg(process.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            row['exit_code'] = process.wait()
            try:
                os.killpg(process.pid, 0)
            except ProcessLookupError:
                row['group_closed'] = True
        row.update(ended_utc=datetime.now(timezone.utc).isoformat(), elapsed_seconds=time.monotonic()-started)
        row['streams'] = {suffix: sha(outdir / 'logs' / (label + '.' + suffix)) for suffix in ('stdout', 'stderr')}
        save(outdir / 'logs' / (label + '.command.json'), row)
        rows.append(row)
    need(row['exit_code'] == expected and row['group_closed'] and not row.get('forced_group_kill'), 'command:' + label)
    print(label, row['exit_code'], flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--execute', action='store_true')
    parser.add_argument('--check', type=Path)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--cpu-list')
    args = parser.parse_args()
    if args.check is not None:
        need(not args.execute, 'check_is_read_only')
        print(json.dumps(compare(args.check.resolve()), sort_keys=True))
        return 0
    if not args.execute:
        print('INERT: after ROOT GO only, --execute --output NEW_DIRECTORY --cpu-list AVAILABLE_CPUS')
        return 0
    need(args.output is not None and args.cpu_list, 'explicit_new_output_and_affinity_required')
    outdir = args.output.resolve()
    need(not outdir.exists(), 'fresh_output_required')
    binary, dep = BUILD / 'full_probe', BUILD / 'full_probe.d'
    need(sha(binary) == BINARY_SHA and sha(dep) == DEP_SHA, 'build_pin')
    dependencies = sorted({str((ROOT / token).resolve().relative_to(ROOT))
                           for token in dep.read_text().replace('\\\n', ' ').split(':', 1)[1].split()})
    need(len(dependencies) == 42, 'actual_project_dependency_count')
    files = sorted(set(dependencies) | set(PINS))
    before = {name: sha(ROOT / name) for name in files}
    need(all(before[name] == value for name, value in PINS.items()), 'source_pin')
    outdir.mkdir(parents=True)
    (outdir / 'logs').mkdir()
    rows = []
    receipt = dict(status='failed', public_status='not_claimed', binary_sha256=BINARY_SHA, source_hashes=before,
                   source_timing='post_compilation_before_micro_only_not_a_prebuild_snapshot',
                   dependency_count=42, cpu_list_requested=args.cpu_list,
                   build_observation='ROOT reports strict O3 compile exit0 without stderr; no build run by this recorder')
    try:
        copy(dep, outdir / 'full_probe.d')
        copy(Path(__file__), outdir / 'record.py')
        for name in files:
            copy(ROOT / name, outdir / 'sources' / name)
        save(outdir / 'sources_before.json', before)
        base = [str(binary), '--n=8', '--s=8', '--kmax=10', '--alias-policy=lazy',
                '--cache-entries=1000000', '--meb-proposal-supports=unlimited']
        for threads in (None, 1, 2, 4, 8):
            name = 'default' if threads is None else 't' + str(threads)
            command(outdir, name, ['/usr/bin/taskset', '-c', args.cpu_list, *base,
                                  *([] if threads is None else ['--threads=' + str(threads)])], 0, rows)
        for index, extra in enumerate(DIRECT_REJECTS):
            label = 'probe_reject_' + str(index)
            command(outdir, label, ['/usr/bin/taskset', '-c', args.cpu_list, *base, *extra], 2, rows)
            raw = [json.loads(line) for line in (outdir / 'logs' / (label + '.stdout')).read_text().splitlines()]
            need(len(raw) == 1 and raw[0]['schema'] == 'mhgp7-full-gabriel-probe-v6' and raw[0]['type'] == 'terminal' and
                 raw[0]['terminal_status'] == 'failed' and raw[0]['outcome'] == 'invalid_input' and raw[0]['exit_code'] == 2 and
                 raw[0]['complete_requested_horizontal_orders'] is False and raw[0]['certificate_digest'] == '', 'probe_reject:' + label)
        for index, extra in enumerate(RUNNER_REJECTS):
            label = 'runner_reject_' + str(index)
            forbidden_output = outdir / (label + '_must_not_exist')
            command(outdir, label, [sys.executable, '-B', str(ROOT / RUNNER), '--binary', str(binary),
                    '--output', str(forbidden_output), '--n', '8', '--s', '8', *extra], 2, rows)
            need(not forbidden_output.exists() and (outdir / 'logs' / (label + '.stderr')).stat().st_size > 0,
                 'runner_reject_before_output:' + label)
        for optimized in (False, True):
            command(outdir, 'compare_O' if optimized else 'compare_normal',
                    [sys.executable, '-B', *(['-O'] if optimized else []), str(outdir / 'record.py'), '--check', str(outdir)], 0, rows)
        need((outdir / 'logs/compare_O.stdout').read_bytes() == (outdir / 'logs/compare_normal.stdout').read_bytes(), 'normal_O_equal')
        receipt.update(comparison=json.loads((outdir / 'logs/compare_normal.stdout').read_text()),
                       direct_cli_rejects=9, runner_cli_rejects=7, status='completed')
    except BaseException as error:
        receipt['error'] = type(error).__name__ + ': ' + str(error)
    finally:
        after = {name: sha(ROOT / name) for name in files}
        save(outdir / 'sources_after.json', after)
        receipt.update(sources_stable=before == after, binary_sha256_after=sha(binary), commands=rows,
                       all_groups_closed=all(row['group_closed'] for row in rows))
        if before != after or sha(binary) != BINARY_SHA or not receipt['all_groups_closed']:
            receipt['status'] = 'failed'
        save(outdir / 'receipt.json', receipt)
        print(json.dumps(dict(status=receipt['status'], commands=len(rows), output=str(outdir)), sort_keys=True))
    return 0 if receipt['status'] == 'completed' else 1


if __name__ == '__main__':
    raise SystemExit(main())
