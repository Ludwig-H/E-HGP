#!/usr/bin/env python3
"""Scalar source model only: no imports/execution/allocation from the product."""
import json

rows = []
for bits in (18, 21, 24):
    record = 16 if bits <= 21 else 32
    digits = 3 + (3 * bits + 10) // 11
    histogram = digits * 2048 * 4
    for n, sites in ((50000, 50000), (50000, 25000)):
        sort = 2 * record * n + histogram
        fill = record * n + 4 * n + 24 * sites + 8
        result = 4 * n + 24 * sites + 8
        rows.append({"B": bits, "returns": n, "sites": sites, "R_contract_bytes": record,
                     "H_bytes": histogram, "sort_additional_bytes": sort,
                     "fill_additional_bytes": fill, "result_bytes": result,
                     "cloud_peak_additional_bytes": max(sort, fill),
                     "peak_with_four_live_input_buffers_bytes": 16 * n + max(sort, fill)})
if rows[0]["peak_with_four_live_input_buffers_bytes"] != 3000008:
    raise RuntimeError("B18 arithmetic")
if rows[4]["peak_with_four_live_input_buffers_bytes"] != 4081920:
    raise RuntimeError("B24 arithmetic")
# n<kNone protects every u32 count/prefix/multiplicity; maximal scalar formulas remain far below u64.
n = (1 << 32) - 2
maximum = 80 * n + 81920
if maximum >= 1 << 64:
    raise RuntimeError("maximum formula overflow")
print(json.dumps({"scope": "Analytic model from frozen source contracts; R layouts not measured here; no native execution or performance claim", "rows": rows, "n_max_scalar": n, "max_modeled_peak_with_inputs_bytes": maximum}, sort_keys=True))
