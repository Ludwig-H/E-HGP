"""Exact retained birth lookup tables; geometric work and artifacts are unchanged."""
SCHEMA = 'ehgp.v11.full_dense_birth_lookup.v1'


def validate(full, sites, balls, need, unsigned):
    active = bool(full['optimizations'] & 512)
    need(type(full['dense_birth_lookup']) is bool and full['dense_birth_lookup'] == active,
         'dense birth lookup differs from request')
    need(type(sites) is int and 0 < sites < 2**32 and type(balls) is int and 0 < balls < 2**32,
         'dense birth lookup domain')
    unsigned(full, ('lookup_reserved_bytes',))
    total = 0
    for k, order in enumerate(full['orders'], 1):
        need(type(order['dense_birth_lookup']) is bool and order['dense_birth_lookup'] == active,
             'order birth lookup route')
        unsigned(order, ('lookup_reserved_bytes', 'births', 'k'))
        need(order['k'] == k and order['births'] > 0, 'birth lookup order identity')
        expected = 4 * (sites if k == 1 else balls) if active else 8 * order['births']
        need(order['lookup_reserved_bytes'] == expected, 'exact birth lookup reservation')
        total += expected
    need(total == full['lookup_reserved_bytes'] and total <= full['reserved_after_bytes'],
         'all birth lookup tables retained together')
