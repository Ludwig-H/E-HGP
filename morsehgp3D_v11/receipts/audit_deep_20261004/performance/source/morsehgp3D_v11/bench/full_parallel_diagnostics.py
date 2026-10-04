"""Validate the optional regular-cell lanes, independently of geometric dump decoding."""
META = {'regular_batch_capacity', 'descent_lanes', 'lane_memo_capacity', 'lane_memo_reserved_bytes'}
COUNTS = {'regular_batches', 'regular_cells', 'regular_traces', 'extended_cells', 'max_regular_batch'}
TIMES = {'regular_dispatch_ns', 'regular_task_sum_ns', 'regular_task_max_ns', 'regular_publish_ns', 'extended_ns'}
FIELDS = COUNTS | TIMES
BATCH, LANES, MEMO = 4096, 48, 4096


def validate(full, need, unsigned):
    active = bool(full['optimizations'] & 8)
    cached = bool(full['optimizations'] & 4)
    meta = full['parallel']
    need(set(meta) == META, 'parallel FULL metadata')
    unsigned(meta, META)
    expected = dict(regular_batch_capacity=BATCH if active else 0, descent_lanes=LANES if active else 0,
                    lane_memo_capacity=MEMO if active and cached else 0,
                    lane_memo_reserved_bytes=LANES * MEMO * full['memo_slot_bytes'] if active and cached else 0)
    need(meta == expected, 'parallel FULL requested configuration and reservations')
    need(full['reserved_after_bytes'] + full['memo_reserved_bytes'] + meta['lane_memo_reserved_bytes'] <=
         full['peak_reserved_bytes'], 'all memo tables coexist with retained FULL buffers')
    for order in full['orders']:
        value, work = order['parallel'], order['work']
        need(set(value) == FIELDS, 'parallel order fields')
        unsigned(value, FIELDS)
        if full['optimizations'] & 8192:
            # resolve_orders is one global distribution; this legacy per-order batch ledger is unused.
            need(not any(value.values()), 'concurrent route has legacy regular-batch diagnostics')
            continue
        if not active:
            need(not any(value.values()), 'disabled parallel path has diagnostics')
            continue
        cells, batches = value['regular_cells'], value['regular_batches']
        need(cells + value['extended_cells'] == work['replayed_cells'], 'all regular and extended cells visited')
        need(2 * cells <= value['regular_traces'] <= 4 * cells and
             value['regular_traces'] <= work['traces'], 'regular face inventory')
        need(work['traces'] >= value['regular_traces'] + value['extended_cells'] and
             (value['extended_cells'] != 0 or work['traces'] == value['regular_traces']),
             'all replayed traces belong to regular or extended cells')
        if cells:
            need(1 <= batches <= cells <= batches * BATCH and
                 1 <= value['max_regular_batch'] <= min(BATCH, cells) and
                 cells <= batches * value['max_regular_batch'], 'bounded complete regular batches')
        else:
            need(not any(value[key] for key in FIELDS - {'extended_cells', 'extended_ns'}), 'empty regular work')
        dispatch = value['regular_dispatch_ns']
        need(value['regular_task_max_ns'] <= min(value['regular_task_sum_ns'], dispatch), 'maximum lane duration')
        need(value['regular_task_sum_ns'] <= min(full['workers'], LANES, BATCH) * dispatch,
             'lane elapsed sum exceeds available concurrent windows')
        need(dispatch + value['regular_publish_ns'] + value['extended_ns'] <= order['timings']['plateaus_ns'],
             'parallel subintervals exceed plateau wall')
