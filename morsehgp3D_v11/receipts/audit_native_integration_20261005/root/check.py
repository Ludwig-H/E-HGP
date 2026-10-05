#!/usr/bin/env python3
"""Bounded mask-contract and portable PH block reader review; no HGP executable."""
import ast
import hashlib
import json
from pathlib import Path
import re
import struct

HERE = Path(__file__).resolve().parent
SRC = HERE / 'sources/morsehgp3D_v11'
checks = 0

def need(value, label):
    global checks
    checks += 1
    if not value:
        raise RuntimeError(label)

for row in json.loads((HERE / 'SOURCES.json').read_text()):
    raw = (HERE / 'sources' / row['copy']).read_bytes()
    need(len(raw) == row['bytes'], 'source size ' + row['copy'])
    need(hashlib.sha256(raw).hexdigest() == row['sha256'], 'source sha ' + row['copy'])

probe = (SRC / 'bench/full_probe.cpp').read_text()
fields = ('parallel_verticals', 'reuse_regular_verticals', 'concurrent_orders')
bits = {}
for field in fields:
    pattern = r'full_params\.' + field + r' = \(optimizations & (\d+)\) != 0;'
    matches = re.findall(pattern, probe)
    need(len(matches) == 1, 'unique mask assignment ' + field)
    bits[field] = int(matches[0])
need(bits == {'parallel_verticals': 128, 'reuse_regular_verticals': 1024, 'concurrent_orders': 8192},
     'three unavailable flags')
full_mask = 16379
order_mask = full_mask & ~sum(bits.values())
need(order_mask == 7035, 'single order equivalent mask')
for bit in (1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192):
    need(bool(order_mask & bit) == (bool(full_mask & bit) and bit not in bits.values()),
         'preserve all other flags ' + str(bit))
order = (SRC / 'src/tower/order_tree.cpp').read_text()
need('params.concurrent_orders || params.parallel_verticals || params.reuse_regular_verticals' in order,
     'build_order refuses those options')
judge = (SRC / 'tests/tower/attach_judge.cpp').read_text()
need('p.parallel_verticals = p.reuse_regular_verticals = p.concurrent_orders = !order;' in judge,
     'identity judge already makes correct distinction')
need('build_full(std::move(domain.value()), budget, nullptr, full_params(false), &pool)' in judge,
     'identity retains qualified FULL reference')

# Execute exact existing Python reader on synthetic blocks only, never run its main or any binary.
path = SRC / 'tests/tower/attach_export_gate.py'
tree = ast.parse(path.read_text(), feature_version=(3, 10))
need(not any(isinstance(n, ast.Assert) for n in ast.walk(tree)), 'gate does not depend on assert')
selected = [n for n in tree.body if isinstance(n, (ast.FunctionDef, ast.ClassDef))
            and n.name in ('Refusal', 'words', 'order_blocks')]
need(len(selected) == 3, 'reader declarations extracted')
ns = {'struct': struct, 'MAGIC': b'MHGP11PH'}
exec(compile(ast.Module(body=selected, type_ignores=[]), str(path), 'exec'), ns)
pack = lambda words: struct.pack('<' + 'Q' * len(words), *words)
reader_cases = 0
for version, width, bits_profile in ((1, 3, 18), (1, 3, 21), (2, 4, 24)):
    for k in (1, 2, 5, 10, 12):
        n, levels, nodes, incidences = 2, 2, 3, 4
        prelude = b'MHGP11PH' + pack([version, bits_profile, k, n, levels, 1, k])
        prelude += pack([0] * (4 * n + 2 * width * levels))
        forest = pack([k, nodes, 2, 2, 2, 0, 2, 0, 0xFFFFFFFF, 1])
        ignored_owners = pack([0] * (2 * n))
        incidence = pack([incidences, 0, 2, 4, 0, 1, 1, 2])
        data = prelude + forest + ignored_owners + incidence
        result = ns['order_blocks'](data, k)
        need(result == (forest, incidence, nodes, incidences), 'extract blocks')
        reader_cases += 1
        for altered in (data[:-1], data + b'x', b'X' + data[1:], data[:8] + pack([9]) + data[16:]):
            try:
                ns['order_blocks'](altered, k)
            except ns['Refusal'] as refusal:
                need(refusal.code == 1, 'format refusal code')
            else:
                need(False, 'malformed blocks accepted')
print(json.dumps({'status': 'conforme', 'checks': checks, 'source_files': len(json.loads((HERE / 'SOURCES.json').read_text())),
                  'full_mask': full_mask, 'single_order_mask': order_mask, 'removed_bits': bits,
                  'reader_cases': reader_cases, 'native_hgp_executed': False,
                  'limits': 'mask/source model and synthetic block reader; no native identity, timing or qualification'}, sort_keys=True))
