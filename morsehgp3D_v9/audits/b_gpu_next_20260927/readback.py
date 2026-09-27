#!/usr/bin/env python3
"""Read-only closed-capture replay for the fixed six-case core/warm experiment."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
from statistics import median
import sys
import tarfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(ROOT / 'gcp-migration'))
import tower_session_v9 as session
import tower_worker_v9 as worker

need, sha = worker.need, worker.sha
read = lambda path: worker.strict_json(path.read_bytes())


def load_pinned(name, filename, pin):
    path = HERE.parent / 'b_q3_payload_g4_20260926' / filename
    need(sha(path) == pin, 'historical evidence helper changed')
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


evidence = load_pinned('mhgp9_core_evidence', 'readback.py',
                      '9a6a72f41221bb2b2a92320e7b6a6f466b46e58e7606a17dc73b7a8f1d5d77f4')
inventory, statistics = evidence.inventory, evidence.statistics


def compare_completed(cases, probes):
    """Exact objects across arms, direct same-core engine certificate twins."""
    need(len(cases) == 6 and set(probes) == set(range(6)), 'six completed cases required')
    reference = worker.logical_result(probes[0])
    for index, probe in probes.items():
        need(worker.logical_result(probe) == reference, 'core experiment object differs')
        engine = 4 if cases[index]['levers']['q34_dead_core'] else 5
        need(worker.certificate_work(probe) == worker.certificate_work(probes[engine]),
             'direct same-core GPU/engine certificate work differs')
    pairs = []
    for on, off in ((0, 1), (3, 2)):
        pairs.append(dict(on=on, off=off, repeat=cases[on]['repeat'],
            first_chain_off_minus_on_ms=probes[off]['frames']['chain_total_ms'][0] -
                                        probes[on]['frames']['chain_total_ms'][0],
            warm_chain_off_minus_on_ms=median(probes[off]['frames']['chain_total_ms'][1:]) -
                                       median(probes[on]['frames']['chain_total_ms'][1:]),
            first_certificate_kernel_off_minus_on_ms=probes[off]['q34_batch']['certificate_kernel_ms'] -
                                                      probes[on]['q34_batch']['certificate_kernel_ms']))
    return pairs


def analyze(cases, outcomes, probes, commands):
    need(cases == read(HERE / 'plan.json')['cases'], 'unexpected experiment plan')
    need(len(outcomes) == 6 and all(row['outcome'] == 'complete_relative' for row in outcomes),
         'incomplete experiment cannot become a completed result')
    rows = []
    for index, case in enumerate(cases):
        probe = probes[index]
        need(worker.validate_probe(probe, case, commands[index]['exit_code']) == 'complete_relative',
             'raw probe validation')
        frames = probe['frames']
        rows.append(dict(index=index, case=case, input=probe['input'],
            tower_digest=probe['tower_digest'], catalogue_digest=probe['catalogue_digest'],
            presentation_digest=probe['presentation_digest'], generator=probe['generator'],
            catalogue=probe['catalogue'], ledger=probe['ledger'], orders=probe['orders'],
            times_ms=probe['times_ms'], q34_batch=probe.get('q34_batch'),
            tower_phases_ms=probe.get('tower_phases_ms'),
            process_elapsed_seconds=commands[index]['elapsed_seconds'],
            frame_times_ms={key: frames[key] for key in worker.FRAME_LISTS},
            process_median_ms={key: median(frames[key]) for key in worker.FRAME_LISTS},
            warm_median_ms={key: median(frames[key][1:]) if len(frames[key]) > 1 else None
                            for key in worker.FRAME_LISTS}))
    pairs = compare_completed(cases, probes)
    by_arm = {}
    for arm, members in (('core_on', (0, 3)), ('core_off', (1, 2))):
        by_arm[arm] = dict(processes=2,
            first_frame_ms={key: statistics([probes[i]['frames'][key][0] for i in members])
                            for key in worker.FRAME_LISTS},
            warm_process_median_ms={key: statistics([median(probes[i]['frames'][key][1:]) for i in members])
                                    for key in worker.FRAME_LISTS})
    return rows, pairs, by_arm


def build(folder, snapshot, after_stop=None, verify_inventory=True):
    published = (folder / 'host/receipt.json').is_file()
    host = folder / 'host' if published else folder
    vm = folder / 'vm' if published else host / 'received/output'
    need(not any(path.is_symlink() for path in (folder, host, vm, snapshot)), 'symlink evidence root')
    if published and verify_inventory:
        evidence.check_inventory(folder)
    if published:
        evidence.published_redactions(host)
    receipt = read(host / 'receipt.json')
    evidence.closed_host(receipt)
    manifest_path = folder / 'source_manifest.json' if published else host / 'source_manifest.json'
    manifest = read(manifest_path)
    need(sha(snapshot) == receipt['snapshot_sha256'] and sha(manifest_path) == receipt['manifest_sha256'],
         'snapshot/manifest pin')
    session.validate_protocol_runtime(manifest)
    session.require_committed_protocol(snapshot, sha(snapshot))
    cases, provenance = session.validate_snapshot(snapshot, manifest)
    need(provenance['commit'] == 'ddf4776d754a8db59a1333e11d56b39d8cb6f51a',
         'this experiment judges the pinned ddf4776d7 engine')
    need(receipt.get('provenance') == provenance and receipt.get('target') == worker.TARGET and
         receipt.get('worker_sha256') == manifest['gcp-migration/tower_worker_v9.py'] and
         receipt.get('controller_sha256') == manifest['gcp-migration/tower_session_v9.py'],
         'host provenance/protocol pins')
    need(session.validate_received(vm, manifest, receipt['worker_sha256'], cases, receipt['generation'],
                                   provenance, receipt['verified_guard']) == 'completed', 'raw protocol replay')
    evidence.stop_evidence(host, receipt, published)
    cost = evidence.duration(read(after_stop or host / 'after_stop.json'), receipt['generation'])
    worker_receipt = read(vm / 'receipt.json')
    probes = {i: read(vm / ('probe_%d.stdout' % i)) for i in range(len(cases))}
    commands = {i: read(vm / ('probe_%d.command.json' % i)) for i in range(len(cases))}
    rows, pairs, timings = analyze(cases, worker_receipt['case_outcomes'], probes, commands)
    with tarfile.open(snapshot, 'r:*') as archive:
        plan_raw = archive.extractfile(worker.PLAN).read()
    if published:
        need((folder / 'plan.json').read_bytes() == plan_raw, 'published plan differs')
        package = read(folder / 'PACKAGE.json')
        need(package['snapshot_sha256'] == receipt['snapshot_sha256'] and
             package['manifest_sha256'] == receipt['manifest_sha256'] and
             package['worker_sha256'] == receipt['worker_sha256'] and
             package['commit'] == provenance['commit'] and package['cases'] == cases, 'published package pins')
    result = dict(schema='mhgp9_core_warm_g4_analysis_v1', public_status='not_claimed',
        capture_status='completed', contract_certified=False, global_subquadratic_claim=False,
        FULL_executed=True, GPU_executed=True, protocol_replayed=True, targeted_shutdown_certified=True,
        timing_scope='Prepared in-memory cloud to explicit FULL; read, segmentation and digests excluded',
        frame_scope='Identical input four times per GPU process; only two independent processes per arm',
        detailed_ledgers_scope='First frame only; do not attribute a warm-frame change to these first-frame ledgers',
        source_commit=provenance['commit'], snapshot_sha256=sha(snapshot), manifest_sha256=sha(manifest_path),
        plan_sha256=hashlib.sha256(plan_raw).hexdigest(), host_receipt_sha256=sha(host / 'receipt.json'),
        worker_receipt_sha256=sha(vm / 'receipt.json'), cost=cost, case_count=len(cases), rows=rows,
        paired_comparisons=pairs, timings=timings, direct_same_core_engine_comparisons=4)
    if published and verify_inventory:
        need(read(folder / 'SUMMARY.json') == result, 'stored summary differs')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('receipt', type=Path)
    parser.add_argument('--snapshot', type=Path, required=True)
    parser.add_argument('--after-stop', type=Path)
    args = parser.parse_args()
    print(json.dumps(build(args.receipt, args.snapshot, args.after_stop), sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
