"""Current FULL population shortcuts and overlapping order phases; explicit old producer adapter."""
import copy

BASELINE = '895680ff866fbe41c450c87b2498ebff2ac7408b'
FIELDS = {'population_lookup', 'population_lookup_entries', 'population_lookup_reserved_bytes',
          'concurrent_orders', 'phases'}
PHASES = {'classify_ns', 'births_ns', 'regular_ns', 'publish_ns', 'verticals_ns'}


def producer_event(event, producer, need):
    if producer == 'current':
        need(FIELDS <= set(event), 'current FULL acceleration metadata missing')
        return event
    need(producer == BASELINE and event['optimizations'] == 2047 and not FIELDS & set(event),
         'only the pinned mode2047 baseline uses the old producer contract')
    value = copy.deepcopy(event)
    value.update(population_lookup=False, population_lookup_entries=0, population_lookup_reserved_bytes=0,
                 concurrent_orders=False, phases=dict.fromkeys(PHASES, 0))
    for order in value['orders']:
        need('population_hits' not in order['work'], 'baseline work unexpectedly changed')
        order['work']['population_hits'] = 0
    return value


def validate(full, need, unsigned):
    mode, active, concurrent = full['optimizations'], bool(full['optimizations'] & 4096), bool(full['optimizations'] & 8192)
    need(type(full['population_lookup']) is bool and full['population_lookup'] == active and
         type(full['concurrent_orders']) is bool and full['concurrent_orders'] == concurrent,
         'population and concurrent routes differ from request')
    unsigned(full, ('population_lookup_entries', 'population_lookup_reserved_bytes'))
    entries, reserved = full['population_lookup_entries'], full['population_lookup_reserved_bytes']
    need(entries < 2**32, 'population table entry domain')
    capacity = 0 if entries == 0 else 1 << (2 * entries - 1).bit_length()
    # Ligne : boule, rang, naissance liee, puis K sites (src/tower/population_lookup.hpp).
    need(reserved == 8 * capacity + 4 * entries * (full['kmax'] + 3), 'exact population table reservations')
    need(active or entries == reserved == 0, 'disabled population table has reservations')
    need(full['reserved_after_bytes'] + full['memo_reserved_bytes'] +
         full['parallel']['lane_memo_reserved_bytes'] + full['census_workspace_reserved_bytes'] +
         full['regular_vertical_reserved_bytes'] + reserved <= full['peak_reserved_bytes'],
         'all simultaneous retained and temporary FULL reservations')
    phases = full['phases']
    need(set(phases) == PHASES, 'global FULL phase fields')
    unsigned(phases, PHASES)
    if not concurrent:
        need(not any(phases.values()), 'sequential orders have concurrent phase timings')
    else:
        need(sum(phases.values()) <= full['forest_ns'], 'disjoint global phases exceed forest wall')
        need(sum(o['timings']['classify_ns'] for o in full['orders']) <= phases['classify_ns'],
             'sequential classification orders exceed shared classification phase')
        for order in full['orders']:
            for field, phase in (('classify_ns', 'classify_ns'), ('births_ns', 'births_ns'),
                                 ('plateaus_ns', 'publish_ns'), ('verticals_ns', 'verticals_ns')):
                need(order['timings'][field] <= phases[phase], 'order interval exceeds its shared global phase')
    for order in full['orders']:
        hits = order['work']['population_hits']
        need(active or hits == 0, 'disabled population table has hit work')
