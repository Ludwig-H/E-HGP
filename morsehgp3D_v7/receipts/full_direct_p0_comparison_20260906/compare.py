#!/usr/bin/env python3
"""Read the two fixed P0/P∞ observations; no engine or completeness judgment."""
import argparse
import hashlib
import json
from pathlib import Path
import re

BINARY_SHA = '4938b94b3166e8c13d02b0fd9687168130d5c528702d7efcf7b7379b3adeb360'
BINARY = '/workspaces/E-HGP/build/v7_no_work_quotas_20260906_controller/build_r1/full_gabriel_lazy_probe'
PINS = {'0': '4b770c150c55bf5697e86a57925dbe2cbab900ae2421d8814fc35cf0e3247124',
        'unlimited': 'f7c23062b380523ecc6990dbf67c59240455d03e801d513ff028fd96a8613358'}
P_FIELDS = {'max_meb_proposal_supports_per_order', 'meb_proposal_budget_kind'}
MEB = {'meb_proposal_supports', 'meb_proposal_pivots', 'meb_proposal_certified',
       'meb_proposal_fallback', 'meb_reference_supports'}
ORDER_MEASURES = {'build_ms', 'digest_ms', 'expand_ms', 'read_ms', 'release_ms', 'rss_mib_sample', 'hwm_mib_sample'}
TERMINAL_MEASURES = {'stage_ms', 'generation_wspd_ms', 'generation_rects_ms', 'elapsed_before_terminal_ms',
    'provisional_output_ms', 'digest_ms', 'compute_read_release_ms_subtracted_diagnostic', 'rss_mib_sample', 'hwm_mib_sample'}


def require(ok, reason):
    if not ok:
        raise ValueError(reason)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def encoded(value):
    return json.dumps(value, sort_keys=True, allow_nan=False)


def same(left, right, reason):
    require(encoded(left) == encoded(right), reason)


def strip(row, fields):
    return {k: v for k, v in row.items() if k not in fields}


def compare(directory):
    arms, data = {}, {}
    for token, pin in PINS.items():
        d = directory / ('n8000_s8_p' + token)
        raw_run = (d / 'run.json').read_bytes()
        require(sha(raw_run) == pin, 'run hash')
        run = json.loads(raw_run)
        raw, stderr = (d / 'stdout.jsonl').read_bytes(), (d / 'stderr.txt').read_bytes()
        require(sha(raw) == run['stdout_sha256'] and sha(stderr) == run['stderr_sha256'], 'stream hashes')
        expected = ['taskset', '-c', '6', '/usr/bin/time', '-v', BINARY, '--n=8000', '--s=8', '--kmax=10',
            '--alias-policy=lazy', '--cache-entries=1000000', '--meb-proposal-supports=' + token]
        same(run['command'], expected, 'command')
        for key, value in dict(binary_sha256=BINARY_SHA, binary_sha256_after=BINARY_SHA,
                               status='completed', exit_code=0, public_status='not_claimed').items():
            same(run[key], value, 'run:' + key)
        rows = [json.loads(line) for line in raw.decode().splitlines()]
        same([r['type'] for r in rows], ['configuration'] + ['order'] * 10 + ['terminal'], 'row sequence')
        config, orders, terminal = rows[0], rows[1:-1], rows[-1]
        for row in rows:
            for key, value in dict(schema='mhgp7-full-gabriel-probe-v5',
                max_meb_proposal_supports_per_order=0 if token == '0' else (1 << 64) - 1,
                meb_proposal_budget_kind='disabled' if token == '0' else 'unlimited').items():
                same(row[key], value, 'profile:' + key)
        for row in (config, terminal):
            for key, value in dict(n=8000, s=8, kmax_requested=10, kmax_effective=10).items():
                same(row[key], value, 'domain:' + key)
        for index, row in enumerate(orders, 1):
            same(row['k'], index, 'order K')
            same(row['outcome'], 'complete_relative', 'order outcome')
        for key, value in dict(terminal_status='completed', outcome='complete_relative', exit_code=0,
            completed_orders_diagnostic=10, complete_requested_horizontal_orders=True,
            integrated_inter_k_tower=False, public_status='not_claimed').items():
            same(terminal[key], value, 'terminal:' + key)
        peak = re.findall(rb'Maximum resident set size \(kbytes\): (\d+)', stderr)
        require(len(peak) == 1, 'GNU RSS')
        counters = {key: sum(row[key] for row in orders)
                    for key in sorted(MEB | {'meb_calls', 'geometry_meb_calls', 'meb_supports'})}
        arms[token] = dict(run_sha256=pin, git_head_observed=run['git_head'],
            recorder_wall_seconds=run['elapsed_seconds'],
            elapsed_before_terminal_seconds=terminal['elapsed_before_terminal_ms'] / 1000,
            stage_seconds={k: v / 1000 for k, v in terminal['stage_ms'].items()},
            GNU_peak_RSS_KiB=int(peak[0]), work_summed_K1_to_K10=counters)
        data[token] = (config, orders, terminal)
    c0, o0, t0 = data['0']
    cp, op, tp = data['unlimited']
    same(strip(c0, P_FIELDS), strip(cp, P_FIELDS), 'same input/configuration except P')
    for left, right in zip(o0, op):
        same(strip(left, P_FIELDS | MEB | ORDER_MEASURES), strip(right, P_FIELDS | MEB | ORDER_MEASURES), 'order nonmeasures')
    left, right = strip(t0, P_FIELDS | TERMINAL_MEASURES), strip(tp, P_FIELDS | TERMINAL_MEASURES)
    left['last_order_work'] = strip(left['last_order_work'], MEB)
    right['last_order_work'] = strip(right['last_order_work'], MEB)
    same(left, right, 'terminal nonmeasures including generation')
    a, b = arms['0'], arms['unlimited']
    w0, wp = a['work_summed_K1_to_K10'], b['work_summed_K1_to_K10']
    ratio = lambda x, y: dict(P0_over_Punlimited=x / y, reduction_percent=100 * (1 - y / x))
    return dict(public_status='not_claimed', engine_invoked=False, arms=arms,
        same_binary_sha256=BINARY_SHA, same_input_digest=t0['input_digest'],
        equal_order_digests=[dict(k=row['k'], certificate_digest=row['certificate_digest']) for row in o0],
        equal_final_digest=t0['certificate_digest'], equal_nonmeasure_fields_except_P_and_MEB_diagnostics=True,
        exclusions=dict(P=sorted(P_FIELDS), MEB_diagnostics=sorted(MEB),
                        order_measures=sorted(ORDER_MEASURES), terminal_measures=sorted(TERMINAL_MEASURES)),
        observed_timing_ratios=dict(wall=ratio(a['recorder_wall_seconds'], b['recorder_wall_seconds']),
            FULL_phase=ratio(a['stage_seconds']['full'], b['stage_seconds']['full']),
            generation=ratio(a['stage_seconds']['generation'], b['stage_seconds']['generation'])),
        physical_F_support_attempts_P0_over_proposal_forms_Punlimited=
            w0['meb_reference_supports'] / wp['meb_proposal_supports'],
        counter_ratio_is_not_a_time_or_homogeneous_operation_ratio=True,
        standalone_MEB_time_measured=False, geometry_or_completeness_judged=False,
        integrated_inter_k_tower=False, slo_claim=False)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, default=Path(__file__).resolve().parent)
    print(json.dumps(compare(parser.parse_args().directory), indent=2, sort_keys=True))
