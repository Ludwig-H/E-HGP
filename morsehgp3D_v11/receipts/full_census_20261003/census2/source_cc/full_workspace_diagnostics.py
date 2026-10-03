"""Physical census buffers: exact reservations, independent of memo lanes and geometric decoding."""
SCHEMA = 'ehgp.v11.full_census_workspace.v1'
FIELDS = {'census_workspaces', 'census_workspace_reserved_bytes'}
COMPARISON_SCHEMA = 'ehgp.v11.full_census_comparison.v1'
PAIRED_WORK_MASK = 15 | 128
POINT_TESTS = 'census_point_tests'


def validate(full, sites, need, unsigned):
    active = bool(full['optimizations'] & 256)
    need(type(full['reuse_census_workspace']) is bool and full['reuse_census_workspace'] == active,
         'census workspace route differs from request')
    need(type(sites) is int and 0 < sites < 2**32, 'census workspace whole site count')
    unsigned(full, FIELDS)
    meta = full['parallel']
    count = 0 if not active else 1 if not full['optimizations'] & 8 else min(
        full['workers'], meta['descent_lanes'], meta['regular_batch_capacity'])
    need(full['census_workspaces'] == count and full['census_workspace_reserved_bytes'] == 4 * sites * count,
         'exact census workspace count and bytes')
    need(full['reserved_after_bytes'] + full['memo_reserved_bytes'] + meta['lane_memo_reserved_bytes'] +
         full['census_workspace_reserved_bytes'] <= full['peak_reserved_bytes'],
         'census buffers and all memos coexist with retained FULL')


def comparisons(rows, work_fields, need):
    """Compare available successes; zero point tests never claim an exercised traversal.

    Both routes visit the same ordered sites for each miss, including saturation.
    The owned census walks twice. Normalize only this counter, never stored work.
    Caller validates whole-input identity, the requested inventory and each event.
    """
    need(POINT_TESTS in work_fields, 'census comparison work schema')
    other = sorted(set(work_fields) - {POINT_TESTS})
    groups = {}
    for row in rows:
        need(row['status'] == 'ok', 'census comparison requires successful attempts')
        mode, kmax = row['optimizations'], row['kmax']
        need(type(mode) is int and 0 <= mode <= 511 and (not mode & 128 or mode & 8),
             'census comparison mode')
        need(type(kmax) is int and 1 <= kmax <= 12, 'census comparison order')
        orders = row['events'][2]['orders']
        need(len(orders) == kmax, 'census comparison complete order inventory')
        for order in orders:
            work = order['work']
            need(set(work) == set(work_fields) and all(type(v) is int and 0 <= v < 2**64
                 for v in work.values()), 'census comparison unsigned work')
        reused = bool(mode & 256)
        points = tuple(o['work'][POINT_TESTS] * (2 if reused else 1) for o in orders)
        values = tuple(tuple(o['work'][key] for key in other) for o in orders)
        key = row['case'], kmax, mode & PAIRED_WORK_MASK
        groups.setdefault(key, []).append((reused, points, values))
    result = dict(other_work_equal=True, point_tests_equal=True, paired_groups=0,
                  run_pairs=0, order_pairs=0, positive_order_pairs=0, zero_order_pairs=0)
    for group in groups.values():
        result['other_work_equal'] &= len({value[2] for value in group}) <= 1
        result['point_tests_equal'] &= len({value[1] for value in group}) <= 1
        owned = [value[1] for value in group if not value[0]]
        borrowed = [value[1] for value in group if value[0]]
        result['paired_groups'] += int(bool(owned and borrowed))
        for left in owned:
            for right in borrowed:
                result['run_pairs'] += 1
                for a, b in zip(left, right):
                    result['order_pairs'] += 1
                    result['positive_order_pairs'] += int(a > 0 or b > 0)
                    result['zero_order_pairs'] += int(a == b == 0)
    return result
