"""Physical census buffers: exact reservations, independent of memo lanes and geometric decoding."""
SCHEMA = 'ehgp.v11.full_census_workspace.v1'
FIELDS = {'census_workspaces', 'census_workspace_reserved_bytes'}


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
