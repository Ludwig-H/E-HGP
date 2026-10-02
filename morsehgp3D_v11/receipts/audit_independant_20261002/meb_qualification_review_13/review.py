#!/usr/bin/env python3
"""Portable closed-meb1 evidence checks. No product/native/cloud execution."""
import contextlib
import hashlib
import importlib.util
import io
import json
from pathlib import Path, PurePosixPath
import re
import tarfile
import xml.etree.ElementTree as ET

HERE = Path(__file__).resolve().parent
FOLDER = HERE / 'raw/morsehgp3D_v11/receipts/meb_20261002/meb1'
PIN = 'ab04bc7b1ef61b9996eb7ec15db4fd4e5f2d511a'


def need(condition, label):
    if not condition:
        raise ValueError(label)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def load(path):
    return json.loads(path.read_text())


def main():
    receipt = load(FOLDER / 'receipt.json')
    original = HERE / 'raw/session_original/receipt.json'
    need(sha(original.read_bytes()) == receipt['original_receipt_sha256'], 'original_receipt')
    package = HERE / 'raw/package/package.tar.gz'
    need(sha(package.read_bytes()) == receipt['package_sha256'], 'package_sha256')
    expected = load(HERE / 'package_git_inventory.json')
    need(expected['pin'] == PIN == receipt['commit'] and expected['exact_git_blob_identity'] is True, 'source_pin')
    by_name = {r['path']: r for r in expected['files']}
    seen, expanded = set(), 0
    with tarfile.open(package, 'r:gz') as archive:
        for member in archive:
            name = PurePosixPath(member.name)
            need(not name.is_absolute() and '..' not in name.parts and (member.isfile() or member.isdir()), 'package_member')
            if member.isdir():
                continue
            need(member.name not in seen and member.name in by_name, 'package_inventory')
            seen.add(member.name)
            raw = archive.extractfile(member).read()
            row = by_name[member.name]
            oid = hashlib.sha1(b'blob ' + str(len(raw)).encode() + b'\0' + raw).hexdigest()
            need(len(raw) == row['bytes'] and sha(raw) == row['sha256'] and oid == row['git_blob'], 'package_blob')
            expanded += len(raw)
    need(seen == set(by_name) and len(seen) == expected['regular_files'] == 2578 and
         expanded == expected['expanded_bytes'] == 46067505, 'package_exact')
    spec = importlib.util.spec_from_file_location('captured_meb_reader', FOLDER.parent / 'check.py')
    reader = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(reader)
    # Sole portability adapter: redirect the declared original receipt to its
    # byte-identical archived copy. All reader transport checks stay active.
    old_path = reader.old.Path
    def mapped_path(value, *args):
        return old_path(original if str(value) == receipt['raw_receipt_local'] else value, *args)
    reader.old.Path = mapped_path
    captured_receipt, worker, data = reader.old.read_capture(FOLDER)
    entries = {}
    for line in data['results/MANIFEST.sha256'].decode().splitlines():
        digest, name = line.split('  ', 1)
        name = 'results/' + name.removeprefix('./')
        need(name not in entries, 'manifest_duplicate')
        entries[name] = digest
    need(set(entries) == set(data) - {'results/MANIFEST.sha256'} and len(entries) == 120, 'manifest_exhaustive')
    need(all(sha(data[name]) == digest for name, digest in entries.items()), 'manifest_hashes')
    need(len(data) == 121 and sum(len(raw) for raw in data.values()) == 3010625, 'archive_size')
    need(sha((HERE / 'raw/package/plan.json').read_bytes()) == receipt['plan_sha256'], 'plan_pin')
    data_manifest = HERE / 'raw/package/data/SHA256SUMS'
    need(sha(data_manifest.read_bytes()) == receipt['data_manifest_sha256'], 'data_manifest')
    counts, builds, code = reader.judge_matrix(data)
    need(code == 1 and sum(c[0] for c in counts.values()) == 1266 and
         sum(c[1] for c in counts.values()) == 1254, 'failed_main')
    failed = []
    tower_controls = {}
    for name in ('gcc_release', 'gcc_asan_ubsan', 'gcc_tsan', 'bits21', 'bits24', 'poison'):
        prefix = reader.BASE + name + '/'
        cases = list(ET.fromstring(data[prefix + 'junit.xml']).iter('testcase'))
        failures = [c.get('name') for c in cases if c.find('failure') is not None]
        need(set(failures) == {'mhgp11_tower_bench_io', 'mhgp11_tower_bench_io_opt'} and len(failures) == 2,
             'main_failure_identity')
        failed.extend((name, n) for n in failures)
        tower_controls[name] = reader.gates(load(FOLDER/'matrix.json')['configurations'][
            next(i for i,c in enumerate(load(FOLDER/'matrix.json')['configurations']) if c['name']==name)],
            data, reader.profiles.CONFIG_BITS[name])
        need(tower_controls[name] == 15, 'tower_partial_gate_count')
    mapped = {reader.BASE + p[len(reader.SUPPLEMENT):]: raw for p, raw in data.items()
              if p.startswith(reader.SUPPLEMENT)}
    supplement, _, supplement_code = reader.judge_supplement(mapped)
    need(supplement == (55,53,2,0) and supplement_code == 1, 'failed_supplement')
    last = data[reader.BASE+'mutants/LastTest.log'].decode()
    causes = re.findall(r'^\S+\s+TUE\s+(code|ligne|construction|signal|delai)\s*$', last, re.MULTILINE)
    kill_counts = {kind: causes.count(kind) for kind in ('code','ligne','construction','signal','delai')}
    need(len(causes)==141 and kill_counts==dict(code=136,ligne=3,construction=2,signal=0,delai=0), 'mutant_causes')
    need(reader.BENCH+'files/meb.json' not in data and not (FOLDER/'meb.json').exists(), 'benchmark_not_started')
    output = io.StringIO()
    with contextlib.redirect_stdout(output):
        failed_campaign = reader.check(FOLDER)
    need(failed_campaign is True and 'campagne=ECHEC' in output.getvalue(), 'campaign_must_fail')
    print(json.dumps(dict(scope='closed failed meb1 only', native=0, source=PIN,
          package_files=len(seen), package_expanded_bytes=expanded, archive_files=len(data), manifest_entries=len(entries),
          main_selected=1266, main_passed=1254, main_failed=12, supplement_selected=55, supplement_passed=53,
          supplement_failed=2, main_tower_gate_partial=tower_controls, mutant_causes=kill_counts,
          benchmark_attempted=0, benchmark_unplayed=18, portable_reader=output.getvalue()), sort_keys=True))


if __name__ == '__main__':
    main()
