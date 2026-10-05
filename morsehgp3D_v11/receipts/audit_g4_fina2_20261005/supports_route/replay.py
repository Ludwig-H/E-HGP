#!/usr/bin/env python3
"""Source-only replay: static six-profile LINE contract. No native invocation."""
from pathlib import Path
import hashlib
import json
import re
import struct
ROOT = Path(__file__).resolve().parent
M = json.loads((ROOT / 'source_manifest.json').read_text())
for path, meta in M['files'].items():
    data = (ROOT / 'sources' / path).read_bytes()
    if hashlib.sha256(data).hexdigest() != meta['sha256']:
        raise SystemExit('source SHA mismatch: ' + path)
def source(name):
    return (ROOT / 'sources' / 'morsehgp3D_v11' / name).read_text()
cmake = source('tests/api/tests.cmake')
body = cmake[cmake.index('foreach(case "scale8000;'):]
records = re.findall(r'"([^"\n]+;[^"\n]+)"', body.split('list(GET case 0 label)')[0])
rows = []
for record in records:
    label, n, request, counts, balls, cells = record.split(';')
    hashes = dict(re.findall(r'(fichier|manifeste)=([0-9a-f]{16})', counts))
    if set(hashes) != {'fichier', 'manifeste'}:
        raise SystemExit('missing static hashes')
    rows.append(dict(label=label, n=int(n), request=request, hashes=hashes))
if len(rows) != 6 or 'MHGP11_COORD_BITS' in body:
    raise SystemExit('six unconditional static records not found')
write = source('src/api/write_supports.cpp')
manifest = source('src/api/manifest.cpp')
probe = source('tests/api/supports_route.cpp')
judge = source('cmake/run_expect.cmake')
checks = {
    'SP_header_embeds_bits': 'const std::array<u64, 16> words = {1,          static_cast<u64>(kCoordBits)' in write,
    'manifest_embeds_bits': 'number(out, static_cast<u64>(kCoordBits));' in manifest and 'coord_bits' in manifest,
    'geometry_and_tree_signature_embed_bits': manifest.count('out.u64le(static_cast<u64>(kCoordBits));') == 2,
    'probe_reads_raw_file': 'out.file = read_file(directory + "/" + std::string(api::kSupportsFileName));' in probe,
    'probe_reads_raw_manifest': 'out.manifest = read_file(directory + "/manifeste.json");' in probe,
    'probe_emits_raw_hash_prefixes': 'sha_hex(p.file).substr(0, 16)' in probe and 'sha_hex(p.manifest).substr(0, 16)' in probe,
    'identity_between_routes_retained': 'a.file == b.file && a.manifest == b.manifest && a.diagnostics.log == b.diagnostics.log' in probe,
    'identity_between_workers_retained': 'if (first && !same(*first, b))' in probe,
    'public_compute_identity_retained': 'c.file != b.file || c.manifest != b.manifest' in probe,
    'judge_requires_exact_LINE': 'string(FIND "\\n${run_expect_normalized}\\n" "\\n${EXPECT_LINE}\\n" run_expect_pos)' in judge,
}
if not all(checks.values()):
    raise SystemExit('source pattern mismatch: ' + repr(checks))
logs = json.loads((ROOT / 'log_facts.json').read_text())
result = dict(pin=M['pin'], static_hash_records=rows, checks=checks,
    header_prefix_24bytes={str(bits): struct.pack('<8sQQ', b'MHGP11SP', 1, bits).hex() for bits in (18, 21, 24)},
    profiles={name: dict(failed_route_gates=len(info['route_failures']), LastTest_bytes=info['LastTest_bytes'],
        runtime_ECART_lines=info['route_runtime_ECART_lines'], runtime_verdict_lines=info['route_runtime_verdict_lines'])
        for name, info in logs['profiles'].items()},
    verdict='static_gate_contract_invalid_outside_its_u21_hash_baseline',
    native_runtime_cause='not_recorded_in_archived_test_output', engine_defect_established=False)
print(json.dumps(result, sort_keys=True, indent=2))
