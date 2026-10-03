"""Temporary strict birth seeds: actual descents and closed images remain separate quantities."""
SCHEMA = 'ehgp.v11.full_regular_vertical_reuse.v1'


def validate(full, balls, need, unsigned):
    active = bool(full['optimizations'] & 1024)
    need(type(full['reuse_regular_verticals']) is bool and full['reuse_regular_verticals'] == active,
         'regular vertical reuse differs from request')
    need(type(balls) is int and 0 < balls < 2**32, 'regular vertical catalogue cardinality')
    unsigned(full, ('kmax', 'regular_vertical_reserved_bytes'))
    expected = 4 * balls if active and full['kmax'] > 1 else 0
    need(full['regular_vertical_reserved_bytes'] == expected, 'one temporary seed table indexed by all balls')
    need(full['reserved_after_bytes'] + full['memo_reserved_bytes'] +
         full['parallel']['lane_memo_reserved_bytes'] + full['census_workspace_reserved_bytes'] + expected <=
         full['peak_reserved_bytes'], 'seed table coexists with retained forests and all private workspaces')
    for k, order in enumerate(full['orders'], 1):
        unsigned(order, ('births',))
        work = order['work']
        unsigned(work, ('vertical_descents', 'vertical_reuses'))
        need(work['vertical_descents'] + work['vertical_reuses'] == (order['births'] if k > 1 else 0),
             'every upper birth is either resolved or reused exactly once')
        need(active or work['vertical_reuses'] == 0, 'disabled seed table has reuse work')
