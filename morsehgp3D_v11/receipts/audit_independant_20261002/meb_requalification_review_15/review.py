#!/usr/bin/env python3
"""Portable frozen MEB3 evidence checks; no product/native/build/cloud execution."""
import contextlib
import hashlib
import importlib.util
import io
import json
from pathlib import Path, PurePosixPath
import re
import tarfile

HERE = Path(__file__).resolve().parent
BASE = HERE / 'raw/morsehgp3D_v11/receipts/meb_20261002'
PIN = '25792084eb4e672c5222d62f5b2ae87bd2ee4948'


def need(value, message):
    if not value:
        raise ValueError(message)


def load(path):
    return json.loads(path.read_text())


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def main():
    receipt = load(BASE / 'meb3/receipt.json')
    need(receipt['commit'] == PIN, 'source')
    package = HERE / 'raw/package/package.tar.gz'
    need(sha(package.read_bytes()) == receipt['package_sha256'], 'package_hash')
    expected = load(HERE / 'package_git_inventory.json')
    need(expected['pin'] == PIN and expected['exact_git_blob_identity'] is True, 'git_pin')
    records = {r['path']: r for r in expected['files']}
    seen, size = set(), 0
    with tarfile.open(package, 'r:gz') as archive:
        for member in archive:
            name = PurePosixPath(member.name)
            need(not name.is_absolute() and '..' not in name.parts and (member.isfile() or member.isdir()), 'package_member')
            if member.isdir():
                continue
            need(member.name not in seen and member.name in records, 'package_inventory')
            seen.add(member.name)
            raw = archive.extractfile(member).read()
            row = records[member.name]
            oid = hashlib.sha1(b'blob ' + str(len(raw)).encode() + b'\0' + raw).hexdigest()
            need(len(raw) == row['bytes'] and sha(raw) == row['sha256'] and oid == row['git_blob'], 'package_blob')
            size += len(raw)
    need(seen == set(records) and len(seen) == 2578 and size == 46069010, 'exact_package')
    spec = importlib.util.spec_from_file_location('frozen_meb_reader', BASE / 'check.py')
    reader = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(reader)
    # Redirect only absolute metadata paths to their byte-identical copies.
    # The stockout directory keeps the original name and DONE marker.
    mapping = {load(BASE / (name+'/receipt.json'))['raw_receipt_local']:
               HERE / ('raw/session_original/'+name+'_receipt.json') for name in ('meb1','meb3')}
    stock = load(BASE / 'meb2/receipt.json')
    stockdir = HERE / 'raw/session_original/v11.20261002.meb2'
    mapping[stock['raw_receipt_local']] = stockdir/'receipt.json'
    mapping[stock['external_closure_local']] = stockdir/'external_closure.json'
    old_path = reader.old.Path
    def mapped(value, *args):
        return old_path(mapping.get(str(value), value), *args)
    reader.old.Path = mapped
    reader.stockout.Path = mapped
    captured_receipt, worker, data = reader.old.read_capture(BASE/'meb3')
    entries = {}
    for line in data['results/MANIFEST.sha256'].decode().splitlines():
        digest, name = line.split('  ', 1)
        name = 'results/' + name.removeprefix('./')
        need(name not in entries, 'duplicate_manifest')
        entries[name] = digest
    need(set(entries) == set(data)-{'results/MANIFEST.sha256'} and len(entries) == 121, 'manifest_inventory')
    need(all(sha(data[name]) == digest for name,digest in entries.items()), 'manifest_hashes')
    need(len(data)==122 and sum(map(len,data.values()))==4595381, 'archive_size')
    need(sha((HERE/'raw/package/plan.json').read_bytes())==receipt['plan_sha256'], 'plan')
    need(sha((HERE/'raw/package/data/SHA256SUMS').read_bytes())==receipt['data_manifest_sha256'], 'data_manifest')
    counts, builds, code = reader.judge_matrix(data)
    need(code==0 and sum(c[0] for c in counts.values())==sum(c[1] for c in counts.values())==1266, 'main')
    supplement_data={reader.BASE+p[len(reader.SUPPLEMENT):]:raw for p,raw in data.items() if p.startswith(reader.SUPPLEMENT)}
    supplement, _, supplement_code=reader.judge_supplement(supplement_data)
    need(supplement==(55,55,0,0) and supplement_code==0, 'supplement')
    causes=re.findall(r'^\S+\s+TUE\s+(code|ligne|construction|signal|delai)\s*$',
                      data[reader.BASE+'mutants/LastTest.log'].decode(),re.MULTILINE)
    kill_counts={k:causes.count(k) for k in ('code','ligne','construction','signal','delai')}
    need(len(causes)==141 and kill_counts==dict(code=136,ligne=3,construction=2,signal=0,delai=0), 'mutants')
    report=load(BASE/'meb3/meb.json')
    need(len(report['runs'])==18 and not report['not_run'] and report['conforming'] is True, 'complete_benchmark')
    complete=saturated=presentations=0
    timings=[]
    for row in report['runs']:
        need(row['status']=='ok', 'attempt')
        events=row['events'];total=events[-2]
        complete+=total['complete'];saturated+=total['saturated']
        base=events[1]['reserved_after_bytes']
        for event in events[2:-2]:
            need(event['wrapper_peak_bytes']-base==2*(event['census_peak_bytes']-base), 'two_populations')
            need(event['census_logical']['passes']==2, 'two_census_passes')
            presentations+=event['meb_logical']['presentations']
        timings.append(dict(case=row['case'],bits=row['coord_bits'],process_ms=1000*row['process_wall_seconds'],
            meb_ms=total['meb_ns']/1e6,census_ms=total['census_ns']/1e6,wrapper_ms=total['wrapper_ns']/1e6,
            reference_ms=total['reference_ns']/1e6,python_decode_ms=1000*row['semantic_wall_seconds']))
    need(complete==216 and saturated==648 and presentations==170352, 'query_totals')
    output=io.StringIO()
    with contextlib.redirect_stdout(output):
        results=[reader.check(BASE/name) for name in ('meb1','meb2','meb3')]
    need(results==[True,True,False], 'historical_failures_preserved')
    print(json.dumps(dict(scope='MEB3 bounded primitive only; MEB1 and MEB2 failures separate',native=0,
        source=PIN,package_files=len(seen),package_expanded_bytes=size,archive_files=len(data),manifest_entries=len(entries),
        main=1266,supplement=55,mutant_causes=kill_counts,attempts=18,queries=864,complete=complete,saturated=saturated,
        first_meb_presentations=presentations,including_wrapper_presentations=2*presentations,
        timings=timings,portable_reader=output.getvalue()),sort_keys=True))


if __name__=='__main__':
    main()
