"""Autonomous read-only judge: external manifest before JSON; independent insertion enumeration."""
import datetime
import hashlib
import json
import pathlib
import re
import sys

FILES = {'README.txt', 'input.json', 'enumerate.py', 'read.py', 'normal.json', 'optimized.json', 'run_receipt.json'}


def need(ok, why):
    if not ok:
        raise ValueError(why)


def unique(pairs):
    out = {}
    for k, v in pairs:
        need(k not in out, 'duplicate JSON key')
        out[k] = v
    return out


def blocks(values):
    return frozenset(frozenset(b) for b in values)


def retain(partition, mcs):
    return frozenset(b for b in partition if len(b) >= mcs)


def main():
    need(len(sys.argv) == 3, 'root external_manifest_sha')
    root, pin = pathlib.Path(sys.argv[1]), sys.argv[2]
    digest = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
    need(re.fullmatch('[a-f0-9]{64}', pin) is not None and digest(root/'MANIFEST.sha256') == pin, 'external manifest')
    need((root/'MANIFEST_SHA256').read_text() == pin+'\n', 'manifest anchor')
    actual = set()
    for p in root.rglob('*'):
        need(not p.is_symlink(), 'symlink')
        if p.is_file():
            actual.add(p.relative_to(root).as_posix())
        else:
            need(p.is_dir() and p == root, 'unexpected directory or special file')
    need(actual == FILES | {'MANIFEST.sha256', 'MANIFEST_SHA256'}, 'closed inventory')
    listed = {}
    manifest = (root/'MANIFEST.sha256').read_bytes()
    for line in manifest.decode().splitlines():
        m = re.fullmatch('([a-f0-9]{64})  (.+)', line)
        need(m is not None and m[2] in FILES and m[2] not in listed, 'manifest line')
        listed[m[2]] = m[1]
    need(set(listed) == FILES and all(digest(root/n) == h for n, h in listed.items()), 'manifest hashes')
    # Hash-first: neither input nor captures have been interpreted above.
    fixture = json.loads((root/'input.json').read_text(), object_pairs_hook=unique)
    normal = (root/'normal.json').read_bytes()
    need(normal == (root/'optimized.json').read_bytes(), 'normal/-O bytes')
    result = json.loads(normal, object_pairs_hook=unique)
    run = json.loads((root/'run_receipt.json').read_text(), object_pairs_hook=unique)
    need(run['schema'] == 1 and run['normal_optimized_byte_equal'] is True and
         run['native_engine_launched'] is False and run['scope'] == 'conditional_fixed_cut_retention_only', 'run scope')
    need(len(run['commands']) == 2, 'command inventory')
    origin = str(pathlib.Path(run['commands'][0]['argv'][-1]).parent)
    for i, c in enumerate(run['commands']):
        expected = (['timeout', '10s', 'python3', '-B'] + (['-O'] if i else []) +
                    [origin+'/enumerate.py', origin+'/input.json'])
        need(c['argv'] == expected and type(c['exit']) is int and c['exit'] == 0 and
             c['combined_output_file'] == ('optimized.json' if i else 'normal.json') and
             0 <= c['wall_time_seconds'] < 10, 'command/exit/time')
    fmt = '%Y-%m-%d %H:%M:%S UTC'
    need(datetime.datetime.strptime(run['ended']['current_time'], fmt) >=
         datetime.datetime.strptime(run['started']['current_time'], fmt), 'UTC ordering')
    ids = tuple('ABCDEF')
    need(fixture['ids'] == list(ids), 'IDs')
    t2, t3 = blocks(fixture['target_mcs2']), blocks(fixture['target_mcs3'])
    need(t2 == blocks(['AB', 'CD', 'EF']) and t3 == blocks(['ABC', 'DEF']), 'conditional targets')
    # Alternative complete generation: insert the next ID in every old block,
    # or give it a new singleton. No RGS recursion and no source-code import.
    partitions = {frozenset()}
    for p in ids:
        nxt = set()
        for part in partitions:
            nxt.add(part | {frozenset([p])})
            for old in part:
                nxt.add((part-{old}) | {old | {p}})
        partitions = nxt
    need(len(partitions) == 203, 'Bell6 exhaustive oracle')
    def code(part):
        ordered = sorted(part, key=lambda b: min(b))
        return ''.join(str(next(i for i, b in enumerate(ordered) if p in b)) for p in ids)
    need(result['all_rgs'] == sorted(code(p) for p in partitions), 'all203 inventory')
    hit2 = sorted(code(p) for p in partitions if retain(p, 2) == t2)
    hit3 = sorted(code(p) for p in partitions if retain(p, 3) == t3)
    both = sorted(code(p) for p in partitions if retain(p, 2) == t2 and retain(p, 3) == t3)
    pc = lc = mc = 0
    for part in partitions:
        for low in range(1, 7):
            for high in range(low, 8):
                need(retain(part, high).issubset(retain(part, low)), 'retention property')
                mc += 1
        for renamed in fixture['point_renamings']:
            need(len(renamed) == 6 and len(set(renamed)) == 6, 'point bijection')
            mapping = dict(zip(ids, renamed))
            transform = lambda p: blocks([[mapping[x] for x in b] for b in p])
            for mcs in (2, 3):
                need(retain(transform(part), mcs) == transform(retain(part, mcs)), 'point labels')
                pc += 1
        ordered = sorted(part, key=lambda b: min(b))
        for labels in fixture['cluster_labels']:
            need(len(labels) == 6 and len(set(labels)) == 6, 'opaque labels')
            groups = {}
            for i, b in enumerate(ordered):
                groups.setdefault(labels[i], set()).update(b)
            need(blocks(groups.values()) == part, 'cluster labels')
            lc += 1
    compatibility = []
    for c in fixture['compatible_cases']:
        part = blocks(c['partition'])
        need(sum(map(len, part)) == 6 and frozenset().union(*part) == frozenset(ids), 'compatible partition')
        need(retain(part, 2) == blocks(c['mcs2']) and retain(part, 3) == blocks(c['mcs3']), 'compatible retention')
        compatibility.append(c['id'])
    need(compatibility == ['three_plus_two_plus_single', 'four_plus_two', 'two_triangles_unchanged',
                           'three_pairs_removed'], 'compatible inventory')
    expected = dict(schema=1, scope='conditional_fixed_cut_retention_only_not_Pi2_or_user_choice',
                    partitions=203, all_rgs=result['all_rgs'], witnesses_mcs2=hit2, witnesses_mcs3=hit3,
                    witnesses_both=both, point_renaming_checks=pc, opaque_label_checks=lc,
                    retention_monotonicity_checks=mc, compatible_cases=compatibility,
                    external_oracle=False, engine_used=False)
    need(result == expected and hit2 == ['001122'] and hit3 == ['000111'] and both == [], 'causal results')
    need(all(digest(root/n) == h for n, h in listed.items()) and
         (root/'MANIFEST.sha256').read_bytes() == manifest, 'changed during read')
    print('CUT203_ARCHIVE_OK partitions=203 target2=1 target3=1 joint=0 controls=4 native_replayed=0')
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (ValueError, KeyError, OSError, TypeError, IndexError) as e:
        print('REFUS '+str(e))
        sys.exit(2)
