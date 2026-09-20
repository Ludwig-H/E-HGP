#!/usr/bin/env python3
"""Recheck evidence and extract counts. Explicit checks also execute under -O."""
import gzip
import hashlib
import json
from pathlib import Path
import sys
from fixtures import BASE


def require(value, message):
    if not value:
        raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    folder = Path(sys.argv[1])
    manifest = json.loads((folder/'MANIFEST.json').read_text())
    completion = json.loads((folder/'COMPLETION.json').read_text())
    require(manifest['schema'] == 'audit_q4_center_blocks_capture_v1', 'Wrong schema')
    require(manifest['status'] == completion['status'] == 'completed', 'Incomplete capture')
    require(completion['manifest_sha256'] == sha(folder/'MANIFEST.json'), 'Manifest changed')
    require(completion['records'] == len(manifest['records']) == 78, 'Missing or unexpected experiment')
    for name, digest in manifest['sources'].items():
        require(sha(BASE/name) == digest, 'Source changed: '+name)
    rows, identities = [], set()
    for entry in manifest['records']:
        file = folder/entry['file']
        require(sha(file) == entry['sha256'], 'Record changed')
        record = json.loads(gzip.decompress(file.read_bytes()))
        require(record['returncode'] == 0 and not record['stderr'], 'Failed child')
        data = json.loads(record['stdout'])
        require(data['schema'] == 'mhgp8_audit_q4_center_blocks_v1' and data['status'] == 'passed', 'Child schema/status mismatch')
        require(data['fallback_executed'] is False and data['product_code_used'] is False, 'Unexpected execution scope')
        require(data['input_sha256'] == record['input_sha256'], 'Input digest mismatch')
        require(data['n'] == record['n'] and data['kmax'] == 10 and data['node_budget'] == 4096,
                'Input/configuration mismatch')
        require(data['depth'] == record['depth'] and data['domain'] == record['domain'], 'Option mismatch')
        require(record['command'][2:] == ['10',str(record['depth']),'4096',str(record['domain'])], 'Command mismatch')
        require(data['seeds'] == data['rejected']+data['survivors'], 'Partition mismatch')
        ids = data.pop('rejected_seed_ids')
        require(len(ids) == len(set(ids)) == data['rejected'] and all(2 <= i < data['n'] for i in ids),
                'Rejected IDs mismatch')
        require(data['forecast_scan_reads'] == data['cover_sites']*data['survivors'], 'Forecast mismatch')
        work, memory = data['work'], data['memory']
        require(work['cell_visits'] == memory['tree_nodes'] <= data['node_budget'], 'Node budget mismatch')
        require(work['cell_visits'] == sum(work[k] for k in ('out_disk','out_hull','deep','unknown','internal')), 'Cell partition mismatch')
        require(work['query_visits'] == work['query_line_tests'], 'Query test mismatch')
        require(sum(work[k] for k in ('query_no_intersection','query_out','query_deep','query_unknown')) <= work['query_visits'], 'Query terminal ledger mismatch')
        require(work['query_unknown'] == data['survivors'], 'Survivor ledger mismatch')
        identity = (record['case'],record['depth'],record['domain'])
        require(identity not in identities, 'Duplicate experiment')
        identities.add(identity)
        rows.append(dict(case=record['case'], provenance=record['provenance'], **data))
    print(json.dumps(dict(status='passed', records=len(rows),
                         scope='certificate counts; forecast is NOT executed family work', rows=rows), sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
