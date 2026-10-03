#!/usr/bin/env python3
"""Whole-cloud exact FULL campaign; every invocation follows successful native qualification."""
import argparse
from pathlib import Path
import subprocess
import sys
import time

import catalogue_profiles as profiles
import full_semantic as semantic
import semantic_cache as reuse

base, need = profiles.base, semantic.need
SCHEMA = 'ehgp.v11.full_campaign.v5'
TIMEOUT = 60
BUDGET = 8 * 1024**3
WORK = {'cells', 'replayed_cells', 'plateaus', 'traces', 'unions', 'continuations', 'ancestor_hops',
        'descent_steps', 'vertical_descents', 'vertical_checks', 'part_meb_presentations',
        'trace_meb_presentations', 'census_point_tests',
        'classification_combinations', 'classification_examined', 'classification_meb_calls',
        'classification_meb_presentations', 'replay_trace_tests', 'replay_meb_calls',
        'replay_meb_presentations', 'ancestor_queries', 'ancestor_activations', 'ancestor_unions',
        'ancestor_find_steps', 'part_diameter_pairs', 'trace_diameter_pairs',
        'classification_diameter_pairs', 'replay_diameter_pairs', 'trace_meb_calls'}
MEMO = {'queries', 'lookups', 'hits', 'misses', 'collisions', 'insertions', 'evictions', 'suffix_hits'}
WORK |= {'memo_' + name for name in MEMO}
MEMO_CAPACITY = 65536
ORDER_TIMINGS = {'classify_ns', 'births_ns', 'plateaus_ns', 'verticals_ns'}


def unsigned(event, keys):
    need(all(type(event[key]) is int and 0 <= event[key] < 2**64 for key in keys), 'unsigned event values')


def optimization(value):
    need(type(value) is int and 0 <= value <= 7, 'optimization mode outside 0..7')
    return value



def check_order_diagnostics(full):
    active = bool(optimization(full['optimizations']) & 4)
    unsigned(full, ('memo_capacity', 'memo_slot_bytes', 'memo_reserved_bytes'))
    need(full['memo_capacity'] == (MEMO_CAPACITY if active else 0) and
         0 < full['memo_slot_bytes'] <= 2048 and
         full['memo_reserved_bytes'] == full['memo_capacity'] * full['memo_slot_bytes'], 'memo reservations')
    need(full['reserved_after_bytes'] + full['memo_reserved_bytes'] <= full['peak_reserved_bytes'],
         'memo table coexists with retained FULL buffers')
    total = 0
    for k, order in enumerate(full['orders'], 1):
        need(set(order['timings']) == ORDER_TIMINGS, 'order timing fields')
        unsigned(order['timings'], ORDER_TIMINGS)
        total += sum(order['timings'].values())
        work = order['work']
        need(set(work) == WORK, 'FULL work fields')
        unsigned(work, WORK)
        memo = {name: work['memo_' + name] for name in MEMO}
        if active:
            need(memo['queries'] == work['traces'] + work['vertical_descents'], 'memo query inventory')
            need(memo['lookups'] == memo['misses'] + memo['hits'] and
                 memo['misses'] == work['descent_steps'], 'memo lookups and actual steps')
            need(memo['suffix_hits'] <= memo['hits'] <= memo['queries'], 'memo hit inventory')
            need(memo['queries'] == memo['insertions'] + memo['hits'] - memo['suffix_hits'], 'memo publications')
            need(memo['collisions'] <= memo['misses'] and memo['evictions'] <= memo['insertions'], 'memo replacement work')
        else:
            need(not any(memo.values()), 'disabled memo has work')
        need(work['classification_meb_calls'] <= work['classification_examined'] <=
             work['classification_combinations'], 'classification work inclusion')
        need(work['replay_meb_calls'] <= work['replay_trace_tests'], 'replay work inclusion')
        diameter_bound = k * (k - 1) // 2
        for phase, calls in (('part', 'descent_steps'), ('trace', 'trace_meb_calls'),
                             ('classification', 'classification_meb_calls'), ('replay', 'replay_meb_calls')):
            need(work[phase + '_diameter_pairs'] <= diameter_bound * work[calls], 'diameter pair inventory')
            need(work[phase + '_meb_presentations'] >= work[calls], 'MEB candidate inventory')
        need(work['ancestor_hops'] == 0, 'old ancestor walks still used')
        need(work['ancestor_queries'] == work['vertical_descents'] + work['vertical_checks'], 'vertical query inventory')
        need(work['vertical_descents'] == (order['births'] if k > 1 else 0) and
             work['vertical_checks'] == (order['edges'] if k > 1 else 0), 'all vertical births and children checked')
        if k == 1:
            need(order['timings']['verticals_ns'] == 0 and all(work[name] == 0 for name in
                 ('ancestor_queries', 'ancestor_activations', 'ancestor_unions', 'ancestor_find_steps')), 'K1 vertical work')
        else:
            lower = full['orders'][k - 2]
            need(work['ancestor_activations'] <= lower['nodes'] - lower['births'] and
                 work['ancestor_unions'] <= lower['edges'], 'lower forest activation inventory')
    need(total <= full['forest_ns'], 'order stage sum exceeds forest wall')


def collect(row, case, output, bits, semantic_cache=None):
    """A successful process still needs a complete stream and a structurally valid FULL artifact."""
    started = time.monotonic()
    ticket = None
    try:
        need(not row['stderr'], 'successful native process wrote stderr')
        events = row['events']
        need([e['phase'] for e in events] == ['cloud', 'domain', 'full', 'exit'], 'complete FULL phase stream')
        cloud, domain, full, end = events
        need(full['status'] == end['status'] == 'ok' and full['reason'] == end['reason'] == 'none', 'native verdict')
        unsigned(cloud, ('read_ns', 'cloud_ns', 'cloud_peak_bytes', 'sites', 'points'))
        unsigned(domain, ('index_ns', 'domain_ns', 'catalogue_balls', 'pool_ns', 'sort_ns', 'count_ns', 'fill_ns'))
        unsigned(full, ('coord_bits', 'kmax', 'workers', 'optimizations', 'wall_ns', 'index_ns', 'domain_ns', 'forest_ns',
                        'peak_reserved_bytes', 'reserved_after_bytes'))
        need(cloud['sites'] == cloud['points'] == case['count'], 'whole input cardinality')
        need(full['coord_bits'] == bits and full['kmax'] == row['kmax'] and full['workers'] == row['workers'] and
             full['optimizations'] == optimization(row['optimizations']),
             'requested native parameters')
        need(domain['index_ns'] == full['index_ns'] and domain['domain_ns'] == full['domain_ns'], 'duplicate durations')
        need(sum(domain[key] for key in ('sort_ns', 'count_ns', 'fill_ns')) <= domain['domain_ns'], 'domain stage walls')
        need(sum(full[key] for key in ('index_ns', 'domain_ns', 'forest_ns')) <= full['wall_ns'], 'FULL stage walls')
        cpu = full['cpu_seconds']
        need(type(cpu) in (int, float) and 0 <= cpu < float('inf'), 'finite CPU duration')
        need(cloud['cloud_peak_bytes'] <= BUDGET, 'input reservations')
        need(0 <= full['reserved_after_bytes'] <= full['peak_reserved_bytes'] <= BUDGET, 'native reservations')
        need(domain['catalogue_balls'] > 0 and len(full['orders']) == row['kmax'], 'nonempty whole tower')
        check_order_diagnostics(full)
        if semantic_cache is None:
            value = semantic.inspect(output, bits, row['kmax'], case['count'])
        else:
            context = reuse.Context('MHGP11FUL1', semantic.SCHEMA,
                reuse.decoder_digest([Path(semantic.__file__),Path(profiles.semantic.__file__)]),
                bits,row['kmax'],case['count'],case['sha256'],case['ids_sha256'])
            ticket = semantic_cache.inspect(output,context,list(identity(row)),
                lambda data: semantic.decode(data,bits,row['kmax'],case['count']))
            value = ticket.summary
            row['semantic_reuse'] = ticket.evidence
            need(value['raw_sha256'] == ticket.evidence['raw_sha256'] and
                 value['bytes'] == ticket.evidence['bytes'], 'FULL raw identity differs from cached inspection')
        for k, (order, decoded) in enumerate(zip(full['orders'], value['orders']), 1):
            unsigned(order, ('k', 'births', 'nodes', 'edges', 'verticals', 'node_capacity', 'edge_capacity'))
            need(order['k'] == decoded['order'] == k and all(order[key] == decoded[key] for key in
                 ('births', 'nodes', 'edges', 'verticals')), 'native/dump order counts')
            need(order['node_capacity'] == 2 * order['births'] - 1 and
                 order['edge_capacity'] == 2 * order['births'] - 2 and
                 order['nodes'] <= order['node_capacity'] and order['edges'] <= order['edge_capacity'], 'capacities')
            need(set(order['work']) == WORK, 'FULL work fields')
            unsigned(order['work'], WORK)
        row.update(status='ok', semantic=value, full_ms=full['wall_ns'] / 1e6,
                   whole_peak_reserved_bytes=max(cloud['cloud_peak_bytes'], full['peak_reserved_bytes']),
                   cloud_ms=cloud['cloud_ns'] / 1e6, read_ms=cloud['read_ns'] / 1e6, pool_ms=domain['pool_ns'] / 1e6,
                   stage_ms={key[:-3]: full[key] / 1e6 for key in ('index_ns', 'domain_ns', 'forest_ns')},
                   order_stage_ms=[{name[:-3]: order['timings'][name] / 1e6 for name in sorted(ORDER_TIMINGS)}
                                   for order in full['orders']],
                   full_within_200ms=full['wall_ns'] <= 200_000_000,
                   cloud_pool_full_ms=(cloud['cloud_ns'] + domain['pool_ns'] + full['wall_ns']) / 1e6)
    except (OSError, ValueError, KeyError, TypeError, OverflowError) as error:
        base.attempt_error(row, 'FULL_semantic', error, 'invalid_output')
    row['semantic_wall_seconds'] = time.monotonic() - started
    return ticket if row['status'] == 'ok' else None


def measure(exe, case, request, args, checkpoint, *, semantic_cache=None):
    need(semantic_cache is None or type(semantic_cache) is reuse.SummaryCache, 'FULL semantic cache option')
    bits, kmax, workers, repetition = (request[key] for key in ('coord_bits', 'kmax', 'workers', 'repetition'))
    mode = optimization(request['optimizations'])
    need(mode == optimization(args.optimizations), 'request optimization differs from campaign')
    output, argv = profiles.invocation(exe, case, bits, kmax, args, workers, repetition, mode)
    row = dict(request, argv=argv, timeout_seconds=TIMEOUT, whole_input=True, count=case['count'],
               exit_code=None, stdout='', stderr='', events=[], errors=[], status='exited')
    started = time.monotonic()
    try:
        result = subprocess.run(argv, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                timeout=TIMEOUT, check=False)
        row.update(exit_code=result.returncode, stdout=result.stdout.decode('utf-8', 'backslashreplace'),
                   stderr=result.stderr.decode('utf-8', 'backslashreplace'),
                   status='exited' if result.returncode == 0 else 'refused' if result.returncode == 2 else 'failed')
    except subprocess.TimeoutExpired as error:
        row.update(stdout=(error.stdout or b'').decode('utf-8', 'backslashreplace'),
                   stderr=(error.stderr or b'').decode('utf-8', 'backslashreplace'), status='timeout')
        base.attempt_error(row, 'process', error, 'timeout')
    except OSError as error:
        base.attempt_error(row, 'launch', error, 'launch_error')
    row['process_wall_seconds'] = time.monotonic() - started
    for line in row['stdout'].splitlines():
        if line.strip():
            try:
                row['events'].append(base.event_json(line))
            except (ValueError, TypeError) as error:
                base.attempt_error(row, 'events', error, 'invalid_output')
    if row['status'] == 'exited':
        row['status'] = 'pending_semantic'
    checkpoint(row)
    ticket = None
    if row['status'] == 'pending_semantic':
        row['status'] = 'exited'
        ticket = collect(row, case, output, bits, semantic_cache)
    try:
        output.unlink(missing_ok=True)
    except OSError as error:
        base.attempt_error(row, 'cleanup', error, 'artifact_error')
    if ticket is not None and row['status'] == 'ok':
        semantic_cache.publish(ticket)
    return row


def schedule(optimizations=0):
    optimization(optimizations)
    lidar = sorted(name for name in profiles.COUNTS if name.startswith('lidar'))
    result = []
    for kmax, repetitions in ((5, (0, 1, 2)), (10, (0,))):
        for repetition in repetitions:
            for name in lidar:
                for bits in (21, 24):
                    result.append(dict(case=name, coord_bits=bits, kmax=kmax, workers=48, repetition=repetition,
                                       optimizations=optimizations))
    return result


def identity(row):
    return tuple(row[key] for key in ('case', 'coord_bits', 'kmax', 'workers', 'repetition', 'optimizations'))


def comparisons(rows, requested):
    result = []
    for name, kmax, mode in sorted({(r['case'], r['kmax'], optimization(r['optimizations'])) for r in requested}):
        expected = [r for r in requested if (r['case'], r['kmax'], r['optimizations']) == (name, kmax, mode)]
        found = [r for r in rows if (r['case'], r['kmax'], r['optimizations']) == (name, kmax, mode) and r['status'] == 'ok']
        semantic_equal = len({r['semantic']['sha256'] for r in found}) <= 1
        raw_equal = all(len({r['semantic']['raw_sha256'] for r in found if r['coord_bits'] == bits}) <= 1
                        for bits in (21, 24))
        work_equal = len({tuple(tuple(sorted(o['work'].items())) for o in r['events'][2]['orders']) for r in found}) <= 1
        equal = semantic_equal and raw_equal and work_equal
        result.append(dict(case=name, kmax=kmax, optimizations=mode, requested=len(expected), successful=[identity(r) for r in found],
                           semantic_equal=semantic_equal, same_profile_bytes_equal=raw_equal, work_equal=work_equal,
                           status='different' if not equal else 'equal' if len(found) == len(expected) else 'incomplete'))
    return result


def run(args):
    mode = optimization(args.optimizations)
    reuse_enabled = getattr(args,'reuse_semantic',False)
    need(type(reuse_enabled) is bool,'FULL semantic reuse option')
    summary_cache = reuse.SummaryCache() if reuse_enabled else None
    args.out.mkdir(parents=True, exist_ok=True)
    args.work.mkdir(parents=True, exist_ok=False)
    started = time.monotonic()
    builds = profiles.checked_builds(args, executable='mhgp11_full_bench')
    supplement = profiles.checked_supplement(args.supplement)
    manifest, manifest_hash = profiles.inputs(args.data)
    cases = {row['name']: row for row in manifest['cases']}
    requested = schedule(mode)
    report = dict(schema=SCHEMA, complete=False, conforming=False, manifest=manifest, manifest_sha256=manifest_hash,
                  qualification_sha256=base.digest(args.qualification), supplement_sha256=supplement,
                  builds=list(builds.values()), requested=requested, requested_runs=len(requested),
                  timeout_seconds=TIMEOUT, budget_seconds=args.budget_seconds, leaf_size=16, max_leaf=256,
                  optimizations=mode, work_schema='ehgp.v11.full_work.v4',
                  order_timing_scope='disjoint non-exhaustive per-order classify/births/plateaus/verticals walls',
                  scope='CPU FULL K1..K exact merge forests and closed verticals; unit weights; whole nonground frames',
                  timing_scope='FULL wall: index + catalogue/lookup + forests/verticals; Cloud/Pool/IO separate',
                  excluded='ground segmentation; preparation of staged integer inputs; point projection; GPU',
                  memory_scope='native Buffer reservations including Cloud; not RSS nor Python decoder',
                  precision='same 1mm integer input in u21/u24; wider arithmetic, not finer input precision',
                  semantic_scope='structure and exact rational identity; geometry independently gated on small fixtures',
                  semantic_reuse_enabled=reuse_enabled,
                  semantic_reuse_scope='current complete payload rehashed per attempt; summary reuse under SHA256 identity assumption',
                  repetitions='three fresh processes per LiDAR K5/profile; K10 one process',
                  runs=[], launch_intents=[], not_run=[], comparisons=[], full_schedule_completed=False)
    path = args.out / 'full.json'

    def save():
        report['comparisons'] = comparisons(report['runs'], requested)
        base.save(path, report)

    save()
    failed_baselines = set()
    for request in requested:
        key = request['case'], request['coord_bits']
        reason = ('same_profile_K5_first_attempt_failed' if key in failed_baselines else
                  'campaign_budget_before_launch' if time.monotonic() - started + TIMEOUT + 20 >= args.budget_seconds else None)
        if reason:
            report['not_run'].append(dict(request, reason=reason)); save(); continue
        ordinal = len(report['runs'])
        report['launch_intents'].append(profiles.launch_intent(Path(builds[request['coord_bits']]['path']),
            cases[request['case']], request['coord_bits'], request['kmax'], args, request['workers'],
            request['repetition'], TIMEOUT, mode))
        save()

        def checkpoint(row):
            need(len(report['runs']) == ordinal and identity(row) == identity(request), 'attempt checkpoint')
            report['runs'].append(row); save()

        cache_option = {'semantic_cache':summary_cache} if summary_cache is not None else {}
        row = measure(Path(builds[request['coord_bits']]['path']), cases[request['case']], request, args, checkpoint, **cache_option)
        need(len(report['runs']) == ordinal + 1, 'missing checkpoint')
        report['runs'][ordinal] = row
        if row['status'] != 'ok' and request['kmax'] == 5 and request['repetition'] == 0:
            failed_baselines.add(key)
        save()
        print('%s B%d K%d W%d r%d o%d %s' % (*identity(request), row['status']), flush=True)
    report['complete'] = True
    report['full_schedule_completed'] = not report['not_run'] and len(report['runs']) == len(requested)
    report['conforming'] = report['full_schedule_completed'] and all(r['status'] == 'ok' for r in report['runs']) and all(
        r['status'] == 'equal' for r in report['comparisons'])
    report['campaign_wall_seconds'] = time.monotonic() - started
    save()
    return 0 if report['conforming'] else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for option in ('builds', 'data', 'out', 'work', 'qualification', 'supplement'):
        parser.add_argument('--' + option, type=Path, required=True)
    parser.add_argument('--budget-seconds', type=int, default=900)
    parser.add_argument('--reuse-semantic',action='store_true')
    parser.add_argument('--optimizations', type=int, choices=range(8), default=0)
    args = parser.parse_args()
    if not 90 <= args.budget_seconds <= 1800:
        parser.error('budget outside 90..1800 seconds')
    try:
        return run(args)
    except (OSError, ValueError, KeyError, TypeError, StopIteration) as error:
        print('full_campaign_refused: ' + type(error).__name__, flush=True)
        return 2


if __name__ == '__main__':
    sys.exit(main())
