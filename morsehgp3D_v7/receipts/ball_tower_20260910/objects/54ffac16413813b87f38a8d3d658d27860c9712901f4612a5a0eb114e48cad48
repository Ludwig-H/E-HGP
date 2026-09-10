#!/usr/bin/env python3
"""Pure worker predicates only: no subprocess, compilation or cloud access."""
import copy
import importlib.util
import json
from pathlib import Path

path = Path(__file__).with_name('full_probe_worker_v7.py')
spec = importlib.util.spec_from_file_location('full_worker', path)
worker = importlib.util.module_from_spec(spec)
spec.loader.exec_module(worker)


def main():
    target = dict(project='p', zone='z', instance='i')
    generation = '2026-09-06T00:00:00Z'
    begin = worker.epoch(generation)
    mark = dict(schema='e-hgp.guard-mark.v1', mark='double_guard_verified', **target, generation=generation,
                max_run_seconds='3600', guest_shutdown_minutes='45', date_utc='2026-09-06T00:02:00Z')
    schedule = dict(MODE='poweroff', USEC=str(int((begin+2820)*1000000)))
    positive = worker.guard_values(mark, schedule, target, generation, int(begin+2800), 300, begin+200)
    worker.need(positive['work_deadline_epoch'] == begin+2500, 'session-only deadline')
    mutants = [('mark', 'mark', 'guest_guard_pending'), ('mark', 'instance', 'another'),
               ('mark', 'generation', '2026-09-05T00:00:00Z'), ('mark', 'max_run_seconds', '28801'),
               ('schedule', 'MODE', 'reboot'), ('schedule', 'USEC', str(int((begin+100)*1000000))),
               ('schedule', 'USEC', str(int((begin+3400)*1000000)))]
    rejected = 0
    for owner, key, value in mutants:
        m, s = copy.deepcopy(mark), copy.deepcopy(schedule)
        (m if owner == 'mark' else s)[key] = value
        try:
            worker.guard_values(m, s, target, generation, int(begin+2800), 300, begin+200)
        except ValueError:
            rejected += 1
        else:
            raise ValueError('guard mutant survived: ' + owner + ':' + key)
    positive_summaries, wire_rejections = 0, 0
    # Synthetic vectors computed independently with struct.pack('<Q', ...).
    final_by_count = {5: '3f16d9889f96f03689e3b618ff2515d0e7532ecccff052a05781f8a44c1eb753',
                      8: 'b3d507860986e2ee2a1f47327c7fa3c9fcfd6ed8a1d76db99bffa74ccf06efce',
                      10: '052c64222c6110b55b005384fa6344e4f1225661418c12c236392aa3e74349b3'}
    for n, kmax in ((8, 10), (50000, 10), (50000, 5)):
        count = min(n, kmax)
        common = dict(schema=worker.SCHEMA, census_payload_accounting=worker.CENSUS_ACCOUNTING,
            pipeline_threads=48, full_order_builder_threads=1,
            order_schedule='sequential_k1_to_kmax', meb_proposal_budget_kind='unlimited',
            max_meb_proposal_supports_per_order=(1 << 64)-1, alias_policy='lazy_first_c_strict_resolutions_v1',
            cache_entries=1000000)
        domain = dict(n=n, s=8, kmax_requested=kmax, kmax_effective=count)
        rows = [dict(common, **domain, type='configuration')]
        rows += [dict(common, type='order', k=k, outcome='complete_relative', certificate_digest='a'*64)
                 for k in range(1, count+1)]
        rows += [dict(common, **domain, type='terminal', terminal_status='completed', exit_code=0,
                      complete_requested_horizontal_orders=True, completed_orders_diagnostic=count,
                      outcome='complete_relative', public_status='not_claimed', input_digest='b'*64,
                      certificate_digest=final_by_count[count])]
        raw = '\n'.join(json.dumps(row) for row in rows)
        worker.need(worker.probe_summary(raw, 0, n, kmax)['reported_complete'], 'valid summary')
        positive_summaries += 1
        worker.need(not worker.probe_summary(raw, 3, n, kmax)['reported_complete'], 'nonzero cannot promote')
        worker.need(not worker.probe_summary('\n'.join(raw.splitlines()[:-1]), 0, n, kmax)['reported_complete'], 'partial cannot promote')
        worker.need(not worker.probe_summary(raw, 0, n, 5 if kmax == 10 else 10)['reported_complete'], 'K10 is not K5')
        bad_wires = [raw.replace('{', '{"schema":"duplicate",', 1),
                     raw.replace('{', '{"nested":{"value":0,"value":1},', 1)]
        bad_wires += [raw.replace('{', '{"number":' + value + ',', 1)
                      for value in ('NaN', 'Infinity', '-Infinity', '1e999')]
        for row_index, key, value in ((1, 'certificate_digest', 'd'*64), (-1, 'certificate_digest', 'd'*64),
                                     (-1, 'input_digest', 'd'*64), (-1, 'census_payload_accounting', 'old_accounting')):
            altered = copy.deepcopy(rows)
            altered[row_index][key] = value
            bad_wires.append('\n'.join(json.dumps(row) for row in altered))
        altered = copy.deepcopy(rows)
        del altered[1]['census_payload_accounting']
        bad_wires.append('\n'.join(json.dumps(row) for row in altered))
        for bad in bad_wires:
            worker.need(not worker.probe_summary(bad, 0, n, kmax)['reported_complete'], 'invalid wire accepted')
            wire_rejections += 1
    for as_root in (False, True):
        commands = worker.bootstrap_commands(['g++', 'time'], as_root)
        worker.need(len(commands) == 2 and commands[0][1][-2:] == ['apt-get', 'update'] and
                    commands[1][1][-6:] == ['apt-get', 'install', '-y', '--no-install-recommends', 'g++', 'time'] and
                    all('NEEDRESTART_MODE=l' in argv and 'DEBIAN_FRONTEND=noninteractive' in argv and
                        not {'upgrade', 'reboot', 'shutdown', 'systemctl'}.intersection(argv)
                        for _, argv in commands), 'bootstrap must not request restart or upgrade')
    print(json.dumps(dict(status='passed', pure_guard_positive=1, rejected_guard_mutants=rejected,
                          full_summaries=positive_summaries, failed_partial_wrongK_rejected=9,
                          invalid_wire_rejected=wire_rejections, list_only_bootstrap_plans=2,
                          subprocess_invoked=False, GCP_used=False), sort_keys=True))


if __name__ == '__main__':
    main()
