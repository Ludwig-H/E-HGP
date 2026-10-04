"""Current FULL population shortcuts and overlapping order phases; explicit old producer adapter."""
import copy

BASELINE = '895680ff866fbe41c450c87b2498ebff2ac7408b'
FIELDS = {'population_lookup', 'population_lookup_entries', 'population_lookup_reserved_bytes',
          'concurrent_orders', 'phases'}
PHASES = {'classify_ns', 'births_ns', 'regular_ns', 'publish_ns', 'verticals_ns'}
# Diagnostic T0 du pipeline (optionnel : absent des producteurs anterieurs a son ajout).
PIPELINE_LANES = {'lanes_last_start_ns', 'lanes_first_finish_ns', 'lanes_last_finish_ns', 'lanes_cpu_ns'}
PIPELINE_ORDER = {'publish_start_ns', 'publish_end_ns', 'publish_cpu_ns', 'publish_wait_ns', 'vertical_start_ns',
                  'vertical_end_ns', 'vertical_cpu_ns', 'vertical_wait_ns'}
VERTICAL_TASK = ('vertical_start_ns', 'vertical_end_ns', 'vertical_cpu_ns', 'vertical_wait_ns')


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
    tasks = full.get('pipeline_tasks')
    if tasks is not None:
        need(type(tasks) is dict and set(tasks) == PIPELINE_LANES | {'orders'}, 'pipeline task diagnostic fields')
        unsigned(tasks, PIPELINE_LANES)
        need(type(tasks['orders']) is list and len(tasks['orders']) == full['kmax'], 'pipeline task orders')
        for index, row in enumerate(tasks['orders']):
            need(type(row) is dict and set(row) == PIPELINE_ORDER | {'k'} and row['k'] == index + 1,
                 'pipeline task order row')
            unsigned(row, PIPELINE_ORDER)
            need(concurrent or not any(row[key] for key in PIPELINE_ORDER), 'sequential orders have task timings')
            need(index != 0 or not any(row[key] for key in VERTICAL_TASK), 'order one has no vertical sweep')
            # Debut et fin d'une meme tache, pris par le meme fil a la meme horloge, sous le mur des forets.
            need(row['publish_start_ns'] <= row['publish_end_ns'] <= full['forest_ns'] and
                 row['vertical_start_ns'] <= row['vertical_end_ns'] <= full['forest_ns'],
                 'task start precedes its end, within the forest wall')
        if concurrent:
            # Sans barriere de depart, une voie peut finir avant qu'une autre demarre : deux bornes par le mur,
            # aucune relation entre le dernier depart et la premiere fin (audit du 4 octobre, pin 66372e621).
            need(tasks['lanes_last_start_ns'] <= full['forest_ns'] and
                 tasks['lanes_first_finish_ns'] <= tasks['lanes_last_finish_ns'] <= full['forest_ns'],
                 'lane starts and finishes within the forest wall')
        else:
            need(not any(tasks[key] for key in PIPELINE_LANES), 'sequential orders have lane timings')
    for order in full['orders']:
        hits = order['work']['population_hits']
        need(active or hits == 0, 'disabled population table has hit work')
