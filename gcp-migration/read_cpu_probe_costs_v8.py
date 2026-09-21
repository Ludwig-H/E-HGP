#!/usr/bin/env python3
"""CPU utilisation and logical worker distributions from a CLOSED v8 capture.

Companion to the frozen strict reader. No cloud call or capture mutation.
Logical work per slot is not a measurement of elapsed time per worker.
"""
import argparse
import json
from pathlib import Path
import unittest

import read_cpu_probe_v8 as verified

READER_SHA = '5d6f79e06a7a539d656cfa83c7fded4c631bae44efe48b2095eae1396ede554a'
need = verified.need


def elapsed_seconds(value):
    fields = value.split(':')
    need(1 <= len(fields) <= 3, 'GNUtime elapsed format')
    result = 0.0
    for field in fields:
        number = float(field)
        need(number >= 0, 'negative elapsed component')
        result = 60*result + number
    return result


def gnu_time(raw):
    fields = {}
    for line in raw.splitlines():
        if ': ' in line:
            name, value = line.strip().rsplit(': ', 1)
            need(name not in fields, 'duplicate GNUtime field')
            fields[name] = value
    user = float(fields['User time (seconds)'])
    system = float(fields['System time (seconds)'])
    wall = elapsed_seconds(fields['Elapsed (wall clock) time (h:mm:ss or m:ss)'])
    percent = fields['Percent of CPU this job got']
    need(percent.endswith('%') and min(user, system) >= 0 and wall > 0 and fields['Exit status'] == '0',
         'GNUtime completed command fields')
    return dict(user_seconds=user, system_seconds=system, wall_seconds=wall,
        reported_cpu_percent=float(percent[:-1]), average_busy_logical_CPUs=(user+system)/wall,
        maximum_resident_set_KiB=int(fields['Maximum resident set size (kbytes)']))


def distribution(workers, field):
    counts = [worker[field] for worker in workers]
    need(all(type(value) is int and value >= 0 for value in counts), 'worker counter type')
    total = sum(counts)
    rank = sorted(range(len(counts)), key=lambda index: (-counts[index], index))
    maximum = max(counts, default=0)
    return dict(total=total, slots=len(counts), active_slots=sum(value > 0 for value in counts),
        minimum=min(counts, default=0), maximum=maximum,
        largest_share=maximum/total if total else None,
        top4_share=sum(counts[i] for i in rank[:4])/total if total else None,
        ideal_count_balance=total/(len(counts)*maximum) if maximum else None,
        counts_by_slot=counts, descending_slots=rank)


def analyse(host, local_record=None):
    own_pin = verified.sha(__file__)
    need(verified.sha(verified.__file__) == READER_SHA, 'frozen posthoc reader pin differs')
    strict = verified.analyse(host, local_record)
    result = []
    for measurement in strict['measurements']:
        stem = measurement['command']
        output = host/'received/output'
        row = verified.read_json(output/(stem+'.stdout'))
        timing = gnu_time((output/(stem+'.stderr')).read_text())
        timing['average_requested_CPU_fraction'] = timing['average_busy_logical_CPUs']/row['workers']
        distributions = {field: distribution(row['workers_work'], field) for field in
            ('jobs', 'front_products', 'input_rectangles', 'expanded_pairs', 'q3_emitted', 'q4_emitted')}
        result.append(dict(command=stem, case=measurement['case'], input_hash=row['input_hash'],
            timing=timing, pipeline_ms=row['timings_ms']['pipeline_including_shared_preparation'],
            parallel=row['parallel'], distributions=distributions,
            geometry=dict(expanded_pairs=row['work']['expanded_pairs'], cover_sites=row['work']['cover_sites'],
                q3_seeds=row['work']['q3']['seeds'], q3_census_sites=row['work']['q3']['census_point_tests'],
                q4_active_sites=row['work']['local28']['sweep']['active_sites']), output=row['output']))
    need(verified.sha(__file__) == own_pin and all(verified.sha(path) == pin for path, pin in
         strict['read_evidence_sha256_after'].items()), 'closed evidence changed during cost extraction')
    return dict(schema='mhgp8_cpu_posthoc_costs_v1', status=strict['status'],
        host_receipt_sha256=strict['host_receipt_sha256'], reader_sha256=READER_SHA, analyser_sha256=own_pin,
        completed=strict['completed'], attempted_failures=strict['attempted_failures'], measurements=result,
        cross_worker=strict['cross_worker'], local_completed_comparison=strict['local_completed_comparison'],
        scope='CPU_aggregate_time_and_logical_work_distribution_not_per_worker_duration',
        full_contract_qualified=False, GPU_executed=False, universal_subquadratic_claim=False)


class PureTests(unittest.TestCase):
    def test_elapsed(self):
        self.assertEqual(elapsed_seconds('59.27'), 59.27)
        self.assertEqual(elapsed_seconds('0:59.27'), 59.27)
        self.assertEqual(elapsed_seconds('1:02:03.5'), 3723.5)

    def test_time(self):
        example = '\n'.join(('User time (seconds): 220.58', 'System time (seconds): 0.00',
            'Elapsed (wall clock) time (h:mm:ss or m:ss): 0:59.27', 'Percent of CPU this job got: 372%',
            'Maximum resident set size (kbytes): 4608', 'Exit status: 0'))
        value = gnu_time(example)
        self.assertAlmostEqual(value['average_busy_logical_CPUs'], 220.58/59.27)
        self.assertEqual(value['maximum_resident_set_KiB'], 4608)
        with self.assertRaises(ValueError):
            gnu_time(example.replace('Exit status: 0', 'Exit status: 1'))
        with self.assertRaises(ValueError):
            gnu_time(example+'\nExit status: 0')

    def test_distribution(self):
        value = distribution([{'jobs': 8}, {'jobs': 1}, {'jobs': 1}, {'jobs': 0}], 'jobs')
        self.assertEqual(value['active_slots'], 3)
        self.assertEqual(value['largest_share'], .8)
        self.assertEqual(value['descending_slots'], [0, 1, 2, 3])
        self.assertEqual(value['ideal_count_balance'], 10/32)
        self.assertIsNone(distribution([{'jobs': 0}], 'jobs')['largest_share'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--host', type=Path)
    parser.add_argument('--local-record', type=Path)
    parser.add_argument('--selftest', action='store_true')
    args = parser.parse_args()
    if args.selftest:
        return 0 if unittest.TextTestRunner().run(unittest.defaultTestLoader.loadTestsFromTestCase(PureTests)).wasSuccessful() else 1
    need(args.host is not None, '--host is required')
    print(json.dumps(analyse(args.host.resolve(), args.local_record), sort_keys=True, indent=2, allow_nan=False))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
