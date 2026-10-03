"""Closed vertical sweep diagnostics: integer wall bounds, no geometric proof inferred from time."""
SCHEMA = 'ehgp.v11.full_vertical_parallel.v1'
COUNTS = {'vertical_batches', 'vertical_resolutions', 'max_vertical_batch'}
TIMES = {'vertical_dispatch_ns', 'vertical_task_sum_ns', 'vertical_task_max_ns', 'vertical_sweep_ns'}
FIELDS = COUNTS | TIMES


def validate(full, need, unsigned):
    active = bool(full['optimizations'] & 128)
    need(type(full['parallel_verticals']) is bool and full['parallel_verticals'] == active,
         'parallel vertical metadata differs from request')
    need(not active or full['optimizations'] & 8, 'parallel verticals require lanes')
    meta = full['parallel']
    for k, order in enumerate(full['orders'], 1):
        value = order['vertical_parallel']
        need(set(value) == FIELDS, 'parallel vertical fields')
        unsigned(value, FIELDS)
        if not active or k == 1:
            need(not any(value.values()), 'disabled or K1 parallel vertical work')
            continue
        unsigned(order, ('births',))
        births, capacity = order['births'], meta['regular_batch_capacity']
        need(capacity > 0 and births > 0 and value['vertical_resolutions'] == births and
             value['vertical_batches'] == (births + capacity - 1) // capacity and
             value['max_vertical_batch'] == min(births, capacity), 'complete bounded vertical birth batches')
        dispatch = value['vertical_dispatch_ns']
        # The maximum is one lane interval; dispatch sums all complete Pool windows.
        need(value['vertical_task_max_ns'] <= min(value['vertical_task_sum_ns'], dispatch),
             'maximum vertical lane duration')
        need(value['vertical_task_sum_ns'] <= min(full['workers'], meta['descent_lanes'], capacity) * dispatch,
             'vertical lane elapsed sum exceeds concurrent windows')
        need(dispatch + value['vertical_sweep_ns'] <= order['timings']['verticals_ns'],
             'parallel preparation and closed sweep exceed vertical wall')
