"""Actual catalogue work: pair masks remove only failed pair extensions, including mixed fallback."""
SCHEMA = 'ehgp.v11.full_catalogue_pair_graph.v1'
FIELDS = {'nodes','leaves','filter_tests','dominance_tests','prefixes','judged','census_tests','emitted','incidences',
          'q4_candidates','q4_levels','region_pair_tests','region_pair_rejects','region_line_tests','region_line_rejects',
          'region_line_evaluations','region_line_cache_hits','region_line_fallbacks','max_leaf','max_depth'}
VARIABLE = {'prefixes','region_pair_tests','region_pair_rejects'}


def validate(domain, full, need, unsigned):
    active = bool(full['optimizations'] & 2048)
    need(type(domain['pair_graph']) is bool and domain['pair_graph'] == active, 'catalogue pair graph request')
    work = domain['catalogue_work']
    need(set(work) == FIELDS, 'catalogue work fields')
    unsigned(work, FIELDS)
    need(work['emitted'] == domain['catalogue_balls'] and work['incidences'] == domain['catalogue_incidences'],
         'catalogue output and actual work')
    need(work['leaves'] <= work['nodes'] and 0 < work['max_leaf'] <= 256 and
         work['max_depth'] <= 3 * full['coord_bits'], 'catalogue traversal bounds')
    need(work['emitted'] <= work['judged'] <= work['prefixes'] and
         work['q4_levels'] <= min(work['q4_candidates'],work['emitted']), 'catalogue admission work')
    need(work['region_pair_rejects'] <= min(work['region_pair_tests'],work['prefixes']) and
         work['region_line_rejects'] <= work['region_line_tests'], 'catalogue rejection work')
    need(work['region_line_tests'] == work['region_line_evaluations'] + work['region_line_cache_hits'] and
         work['region_line_fallbacks'] <= work['region_line_evaluations'], 'catalogue line cache work')
    if active and work['max_leaf'] <= 32:
        need(work['region_pair_tests'] == work['region_pair_rejects'] == 0, 'small graph still tests pairs')


def comparisons(rows):
    result = dict(other_work_equal=True,same_route_equal=True,prefixes_equal=True,
                  pair_work_monotone=True,run_pairs=0,reduced_pairs=0,mixed_fallback_pairs=0)
    groups = {}
    for row in rows:
        # Cache-off/on changes line cache counts, while the pair option changes only VARIABLE.
        key = row['case'],row['kmax'],bool(row['optimizations'] & 1)
        work = row['events'][1]['catalogue_work']
        groups.setdefault(key,[]).append((bool(row['optimizations'] & 2048),work))
    for group in groups.values():
        result['other_work_equal'] &= len({tuple(w[key] for key in sorted(FIELDS-VARIABLE)) for _,w in group}) <= 1
        for active in (False,True):
            result['same_route_equal'] &= len({tuple(w[key] for key in sorted(FIELDS))
                                              for state,w in group if state == active}) <= 1
        for _,before in (value for value in group if not value[0]):
            for _,after in (value for value in group if value[0]):
                result['run_pairs'] += 1
                result['prefixes_equal'] &= after['prefixes'] == (
                    before['prefixes']-before['region_pair_rejects']+after['region_pair_rejects'])
                result['pair_work_monotone'] &= (after['region_pair_tests'] <= before['region_pair_tests'] and
                                                 after['region_pair_rejects'] <= before['region_pair_rejects'])
                result['reduced_pairs'] += int(after['prefixes'] < before['prefixes'])
                result['mixed_fallback_pairs'] += int(after['region_pair_rejects'] > 0)
    return result
