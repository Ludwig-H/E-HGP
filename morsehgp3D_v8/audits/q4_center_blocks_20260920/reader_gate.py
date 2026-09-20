#!/usr/bin/env python3
"""Semantic corruption checks; each damaged record is deliberately re-hashed."""
import copy
import gzip
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tempfile

BASE = Path(__file__).resolve().parent


def require(value, message):
    if not value:
        raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    origin = BASE/'capture'
    manifest0 = json.loads((origin/'MANIFEST.json').read_text())
    names = ['incomplete','missing_record','failed_child','wrong_input','wrong_command',
             'wrong_domain','claimed_fallback','duplicate_rejection','wrong_forecast',
             'wrong_nodes','wrong_survivor_ledger','changed_source']
    results = []
    parent = BASE/'.inputs'
    parent.mkdir(exist_ok=True)
    for name in names:
        with tempfile.TemporaryDirectory(prefix='reader_mutant_',dir=parent) as temporary:
            folder = Path(temporary)/'capture'
            shutil.copytree(origin,folder)
            manifest = copy.deepcopy(manifest0)
            entry = manifest['records'][0]
            file = folder/entry['file']
            record = json.loads(gzip.decompress(file.read_bytes()))
            data = json.loads(record['stdout'])
            if name == 'incomplete': manifest['status'] = 'started'
            elif name == 'missing_record': manifest['records'].pop()
            elif name == 'failed_child': record['returncode'] = 1
            elif name == 'wrong_input': record['input_sha256'] = '0'*64
            elif name == 'wrong_command': record['command'][-1] = '7'
            elif name == 'wrong_domain': data['domain'] = 9
            elif name == 'claimed_fallback': data['fallback_executed'] = True
            elif name == 'duplicate_rejection': data['rejected_seed_ids'].append(data['rejected_seed_ids'][0])
            elif name == 'wrong_forecast': data['forecast_scan_reads'] += 1
            elif name == 'wrong_nodes': data['memory']['tree_nodes'] += 1
            elif name == 'wrong_survivor_ledger': data['work']['query_unknown'] += 1
            elif name == 'changed_source': manifest['sources']['center_blocks.cpp'] = '0'*64
            record['stdout'] = json.dumps(data)
            file.write_bytes(gzip.compress(json.dumps(record).encode(),mtime=0))
            entry['sha256'] = sha(file)
            (folder/'MANIFEST.json').write_text(json.dumps(manifest))
            (folder/'COMPLETION.json').write_text(json.dumps(dict(status='completed',records=len(manifest['records']),
                    manifest_sha256=sha(folder/'MANIFEST.json'))))
            for optimized in (False,True):
                command = ['python3']+(['-O'] if optimized else [])+[str(BASE/'read.py'),str(folder)]
                child = subprocess.run(command,capture_output=True,text=True,timeout=30)
                require(child.returncode != 0 and 'RuntimeError:' in child.stderr, 'Reader mutant survived: '+name)
                results.append(dict(mutant=name, optimized=optimized, returncode=child.returncode,
                                    reason=child.stderr.strip().splitlines()[-1]))
    print(json.dumps(dict(status='passed',designs=len(names),rejections=len(results),results=results),sort_keys=True,indent=2))


if __name__ == '__main__':
    main()
