"""Validate optional owned catalogue diagnostics; linear work in the number of jobs, no n*J scan."""
from catalogue_semantic import need

SCHEMA = 'ehgp.v11.catalogue_diagnostics.v1'
RECORD_BYTES = 264  # fields/alignment of CatalogueTaskDiagnostic on the qualified x86_64 ABI, all three B
PLAN_FIELDS = {'adaptive', 'memory_fallback', 'plan_nodes', 'plan_leaves', 'empty_leaves', 'rounds',
               'priority_tests', 'replay_bytes'}
TASK_FIELDS = {'ordinal', 'path', 'path_known', 'inside_known', 'lo', 'hi', 'depth', 'count', 'capacity',
               'inside', 'count_ns', 'fill_ns', 'ledger'}
LEDGER_FIELDS = {'nodes', 'leaves', 'filter_tests', 'dominance_tests', 'prefixes', 'judged', 'census_tests',
                 'emitted', 'incidences', 'q4_candidates', 'q4_levels', 'region_pair_tests',
                 'region_pair_rejects', 'region_line_tests', 'region_line_rejects', 'region_line_evaluations',
                 'region_line_cache_hits', 'region_line_fallbacks', 'max_leaf', 'max_depth'}


def uint(value, bound=2**64):
    return type(value) is int and 0 <= value < bound


def check_request(row, event):
    mode, requested = row.get('optimizations', 0), row.get('diagnostics', False)
    need(type(mode) is int and 0 <= mode <= 15 and type(requested) is bool, 'invalid requested catalogue options')
    actual_mode = event.get('optimizations', 0)
    actual_requested = event.get('diagnostics_requested', False)
    need(type(actual_mode) is int and actual_mode == mode, 'native optimization mask differs')
    need(type(actual_requested) is bool and actual_requested == requested, 'native diagnostic request differs')
    need(('diagnostics' in event) == requested, 'missing or unsolicited diagnostics on success')
    return requested


def total_ledger(event):
    result = dict(event['logical'])
    result.update(event['work'])
    result.update(emitted=event['balls'], incidences=event['incidences'],
                  region_line_evaluations=event['cache_work']['evaluations'],
                  region_line_cache_hits=event['cache_work']['hits'],
                  region_line_fallbacks=event['cache_work']['fallbacks'])
    need(set(result) == LEDGER_FIELDS and all(uint(v) for v in result.values()), 'full logical ledger')
    return result


def check(event, count, bits):
    from catalogue_parallel import check_timings
    check_timings(event)
    need(type(bits) is int and bits in (18, 21, 24) and uint(count, 2**32) and count > 0,
         'diagnostic input domain')
    value, mode, timing = event['diagnostics'], event['optimizations'], event['timings']
    need(type(mode) is int and 0 <= mode <= 15, 'diagnostic optimization mask')
    adaptive = bool(mode & 4)
    need(type(value) is dict and set(value) == {'schema', 'record_bytes', 'reserved_bytes', 'planning', 'tasks'} and
         value['schema'] == SCHEMA, 'diagnostic schema/fields')
    need(uint(value['record_bytes']) and value['record_bytes'] == RECORD_BYTES, 'diagnostic record ABI')
    tasks, p = value['tasks'], value['planning']
    need(type(tasks) is list and len(tasks) == timing['tasks'], 'diagnostic job inventory')
    need(uint(value['reserved_bytes']) and value['reserved_bytes'] == len(tasks)*RECORD_BYTES,
         'diagnostic Buffer reservation')
    need(all(uint(event[k]) for k in ('reserved_after_bytes', 'peak_reserved_bytes', 'wall_ns')),
         'diagnostic memory/wall unsigned values')
    need(value['reserved_bytes'] + 28*count+8 <= event['reserved_after_bytes'] <= event['peak_reserved_bytes'],
         'diagnostic/Cloud coexistence not included in memory')
    need(type(p) is dict and set(p) == PLAN_FIELDS and p['adaptive'] is adaptive and
         type(p['memory_fallback']) is bool, 'planning mode/fields')
    need(all(uint(p[k]) for k in PLAN_FIELDS-{'adaptive', 'memory_fallback'}), 'planning unsigned values')
    limit = 1024 if adaptive else 256
    need(1 <= p['plan_leaves'] <= limit and p['plan_nodes'] == 2*p['plan_leaves']-1 and
         len(tasks)+p['empty_leaves'] == p['plan_leaves'], 'bounded binary plan with empty leaves')
    need(p['rounds'] <= 3*bits and 8*count <= p['replay_bytes'] <= 4*count*2048, 'planning replay bound')
    if not adaptive:
        need(p['rounds'] == p['priority_tests'] == 0 and not p['memory_fallback'] and p['replay_bytes'] == 40*count,
             'fixed frontier acquired adaptive work')
    total = total_ledger(event)
    need(total['region_line_tests'] == total['region_line_evaluations'] + total['region_line_cache_hits'] and
         total['region_line_fallbacks'] <= total['region_line_evaluations'] and
         (mode & 1 or total['region_line_cache_hits'] == total['region_line_fallbacks'] == 0),
         'diagnostic cache work inventory')
    sums = dict.fromkeys(LEDGER_FIELDS, 0)
    paths, populations = [], 0
    clocks = {'count': [], 'fill': []}
    for i, t in enumerate(tasks):
        need(type(t) is dict and set(t) == TASK_FIELDS and uint(t['ordinal']) and t['ordinal'] == i,
             'task fields/ordinal')
        need(t['path_known'] is adaptive and t['inside_known'] is adaptive, 'known/unknown task geometry')
        need(all(uint(t[k]) for k in ('depth', 'count', 'capacity', 'inside', 'count_ns', 'fill_ns')), 'task integers')
        need(t['depth'] <= 3*bits and 0 < t['count'] <= t['capacity'] <= count and t['inside'] <= t['count'],
             'task capacities/depth')
        for name in ('lo', 'hi'):
            need(type(t[name]) is list and len(t[name]) == 3 and all(uint(v, 2**bits+1) for v in t[name]), 'T0 box')
        need(all(lo < hi for lo, hi in zip(t['lo'], t['hi'])), 'nonempty task box')
        need(type(t['path']) is list and len(t['path']) == 2 and all(uint(v) for v in t['path']), 'task path words')
        raw = (t['path'][0] << 64) | t['path'][1]
        need(raw % (1 << (128-t['depth'])) == 0, 'bits beyond path depth')
        if adaptive:
            paths.append(format(raw, '0128b')[:t['depth']]); populations += t['inside']
        else:
            need(raw == t['inside'] == 0, 'unknown geometry invented')
        ledger = t['ledger']
        need(type(ledger) is dict and set(ledger) == LEDGER_FIELDS and all(uint(v) for v in ledger.values()),
             'task ledger fields/values')
        for key, v in ledger.items():
            sums[key] = max(sums[key], v) if key.startswith('max_') else sums[key]+v
        for phase in clocks:
            clocks[phase].append(t[phase+'_ns'])
    if adaptive:
        need(paths == sorted(set(paths)) and all(not b.startswith(a) for a, b in zip(paths, paths[1:])),
             'task paths are not an ordered antichain')
        # For THIS G1 implementation, x in Q cannot be strictly dominated (choose c=x); envelope adjustment
        # preserves x, and each split partitions Q. A ghost therefore contains no global site. No n*J scan.
        need(populations == count, 'interior populations do not partition the whole Cloud')
    for phase, values in clocks.items():
        need(sum(values) == timing[phase+'_task_sum_ns'] and max(values, default=0) == timing[phase+'_task_max_ns'],
             'task intervals disagree with aggregate timings')
    for key, v in sums.items():
        if key == 'nodes':
            need(v+p['plan_nodes'] == total[key], 'prefix/suffix node accounting')
        elif key in ('filter_tests', 'max_depth'):
            need(v <= total[key], 'prefix work cannot subtract suffix work')
        else:
            need(v == total[key], 'suffix work mismatch: '+key)
    return dict(schema=SCHEMA, jobs=len(tasks), adaptive=adaptive, record_bytes=RECORD_BYTES,
                reserved_bytes=value['reserved_bytes'], plan_nodes=p['plan_nodes'],
                empty_leaves=p['empty_leaves'], priority_tests=p['priority_tests'])


def stable(value):
    """Comparison across W/profiles within one mode; measured clocks and ABI reservation are not geometry."""
    return dict(planning=value['planning'], tasks=[{k: v for k, v in t.items() if k not in ('count_ns', 'fill_ns')}
                                                 for t in value['tasks']])
